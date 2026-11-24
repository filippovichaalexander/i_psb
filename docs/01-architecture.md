# Архитектура

## 1. Цели и не-цели

**Цели**

- Локальное воспроизведение с microSD без заиканий: MP3, AAC, WAV, FLAC (16/44.1–48 в M0).
- Предсказуемый UI на E-Ink: страницы, а не анимация; отклик на wheel ≤200 мс.
- Изоляция audio и UI по ядрам/задачам; никакой бизнес-логики в `loop()`.
- Ремонтопригодность: модули, логи, coredump, recovery, OTA.
- Повторяемая сборка: Docker + CI + pinned versions.

**Не-цели M0**

- Hi-res 24/96 (решение открыто), Bluetooth-аудио, видео, обложки в цвете.
- Полноценный filesystem-менеджер в вебе (M3), BLE (M4), радио (M5).

## 2. Блок-схема

```text
┌──────────────────────────── ESP32-S3-N16R8 ────────────────────────────┐
│ Core 0                                        Core 1                   │
│ ┌──────────────────────────┐                 ┌───────────────────────┐ │
│ │ AudioTask (prio max-2)   │                 │ UiTask (prio 2)       │ │
│ │  AudioEngine             │  telemetry Q    │  App FSM              │ │
│ │  ESP32-audioI2S          │ ───────────────►│  UiRenderer           │ │
│ │  I2S DMA                 │                 │  BatteryMonitor       │ │
│ └───────────┬──────────────┘                 └──────────▲────────────┘ │
│             │ command Q                                 │ input Q      │
│             ▼                                           │              │
│ ┌──────────────────────────┐                 ┌──────────┴────────────┐ │
│ │ StorageManager (SD/HSPI) │                 │ InputTask (prio 3)    │ │
│ └──────────────────────────┘                 │  TTP229 + buttons     │ │
│                                              │  haptics              │ │
│                                              └───────────────────────┘ │
└───────────┬──────────────────────┬──────────────────────┬──────────────┘
            │ I2S (16/17/18)       │ FSPI (10–15, CS 21)  │ HSPI (6–9)
            ▼                      ▼                      ▼
      PCM5102A → MAX97220      E-Ink 2.13"           microSD 64GB
            → 3.5mm jack
```

## 3. Задачи и очереди

| Объект | Ядро | Приоритет | Стек | Ответственность |
|---|---|---:|---:|---|
| `AudioTask` | 0 | `configMAX_PRIORITIES-2` | 8 КБ | `audio.loop()`, команды, телеметрия |
| `InputTask` | 1 | 3 | 4 КБ | TTP229, кнопки, вибро |
| `UiTask` | 1 | 2 | 8 КБ | App FSM, E-Ink, батарея, логи |
| `loop()` | 1 | 1 | — | `vTaskDelay(1000)`; ничего больше |

| Очередь | Направление | Размер | Элемент |
|---|---|---:|---|
| `inputQueue` | Input → UI | 16 | `InputEvent{type,id,value}` |
| `audioCmdQueue` | UI → Audio | 8 | `AudioCommand{type,path[160],value}` |
| `audioTelemetryQueue` | Audio → UI | 8 | `AudioTelemetry{type,a,b,c,text[96]}` |

**Правила:**

- Audio task никогда не ждёт UI и не делает `delay()` >1 мс.
- UI task не трогает `Audio` напрямую; только очередь.
- Все структуры очередей — POD, без `String`/`std::string` в hot path.
- Телеметрия шлётся не чаще 2 Гц; ID3 — по событию.

## 4. Модули

| Модуль | Файлы | Ответственность | Зависимости |
|---|---|---|---|
| `core` | `lib/ipsb_core` | Батарея, плейлист, wheel, UTF-8, ID3, формат | STL, без Arduino |
| `AudioEngine` | `src/audio` | Обёртка audioI2S, команды, телеметрия | core, Audio.h |
| `StorageManager` | `src/storage` | SD mount, список треков, конфиги, логи | SD, HSPI |
| `InputManager` | `src/input` | TTP229, кнопки, debounce, haptics | core wheel |
| `UiRenderer` | `src/ui` | E-Ink, шрифты, экраны, refresh policy | GxEPD2, U8g2 |
| `BatteryMonitor` | `src/power` | ADC, EMA, кривая | core battery |
| `Logger` | `src/system` | Serial, ring buffer, boot info | Arduino |
| `WifiPortal` | `src/net` | STA/AP, HTTP API, upload | **M3** |
| `BleCompanion` | `src/net` | GATT, команды, обложки | **M4** |
| `App` | `src/app` | FSM, связывание, обработка событий | все |

## 5. Boot sequence

1. `Logger::begin()` → Serial 115200, boot reason, chip/flash/PSRAM/heap.
2. `StorageManager::begin()` → HSPI, `SD.begin`, mount; если fail — статус, но boot продолжается.
3. `BatteryMonitor::begin()` → ADC attenuation, первый замер.
4. `UiRenderer::begin()` → FSPI, EPD init, splash.
5. `AudioEngine::begin()` → I2S pins, volume 4/21, callback, task core 0.
6. `InputManager::begin()` → pins, TTP idle, buttons pull-up.
7. Создать очереди и задачи; `UiTask` строит плейлист, показывает меню.
8. `loop()` уходит в `vTaskDelay`.

**Отказоустойчивость:** если SD/EPD/audio не поднялись, UI показывает экран ошибки
с кодом и продолжает работать; логи пишутся в Serial и ring buffer. Не входить в
бесконечный `while(1)`.

## 6. Состояния App

```text
Boot ──► Menu ──► Browse ──► NowPlaying
          │  ▲       │            │
          │  └───────┴────────────┘
          ├──► Settings
          ├──► About
          └──► Error (любой экран)

Кнопки:
  MENU    — назад/в корень
  PLAY    — play/pause
  PREV    — предыдущий трек (long: seek −10 с)
  NEXT    — следующий трек (long: seek +10 с)
  SELECT  — войти/выбрать
  Wheel   — навигация; на NowPlaying — громкость
```

## 7. Память

| Область | Что | Примечание |
|---|---|---|
| Internal RAM | I2S DMA buffers, очереди, EPD framebuffer (3,8 КБ), стеки задач | Не выносить DMA в PSRAM |
| PSRAM (8 МБ) | Декодированный аудиобуфер библиотеки, крупные временные | `ps_malloc`, проверять `nullptr` |
| Flash 16 МБ | app0/app1 по 4 МБ, LittleFS 6 МБ, coredump 64 КБ | См. `partitions_16MB.csv` |
| SD | `/Music`, `/config`, `/logs`, `/system` | FAT32, 32 КБ кластер |

**Правило:** любой `malloc` в audio task — только на старте; в hot path — ноль аллокаций.

## 8. E-Ink refresh policy

| Событие | Тип | Окно |
|---|---|---|
| Смена экрана, boot, error | Full | Весь экран |
| Выбор в меню/списке | Partial | Область списка (0,16,248,106) |
| Громкость/батарея | Partial | Статус-бар/низ |
| Прогресс | Partial раз в 10–15 с | Полоса прогресса |
| Каждые 20–30 partial | Full | Весь экран (сброс ghosting) |

- Coalesce wheel: накопить тики 150–200 мс → один render.
- Минимальный интервал между render — 250 мс.
- Не обновлять экран, если ничего не изменилось.
- В M0 реализованы full/partial и счётчик; точная настройка — после измерений.

## 9. Аудио-пайплайн

**M0 (принято):**

```text
SD (HSPI) → ESP32-audioI2S decoder → I2S DMA → PCM5102A → MAX97220 → jack
                 │
                 └── callback: id3/eof/log → telemetry queue
```

- Форматы: MP3, AAC, WAV, FLAC (16/44.1–48).
- Громкость: шаг 0–21, старт 4, максимум 18 (firmware cap).
- MCLK: по умолчанию `SCK=GND` (PLL), опция — GPIO47/48.
- Underrun: измерять `audio.isRunning()`, счётчик ошибок, лог.

**M1+ (если 24/96 обязательно):** отдельный `FlacBackend` (`dr_flac`) + свой I2S
ring; общий интерфейс `IAudioBackend`. Не смешивать с audioI2S в одном тракте.

## 10. Ошибки, логи, диагностика

- `Logger`: уровни I/W/E, millis, тег модуля; ring buffer 32×160 для веба.
- Boot log: reset reason, heap, PSRAM, SD, EPD, audio init.
- Ошибки SD/EPD/audio → код + короткий текст на экране; подробности в лог.
- Task WDT 30 с; UI/audio периодически `esp_task_wdt_reset()`.
- Coredump-раздел 64 КБ; разбор `esp-coredump` (документируется в M2).
- Лог-файл `/logs/boot.log` с ротацией 64 КБ — M1.

## 11. Данные и файлы

```text
/Music/...              аудио
/config/wifi.json       SSID/пароль (не коммитить)
/config/stations.json   радио
/config/settings.json   громкость, repeat, theme
/system/library.db      индекс треков (M1)
/logs/boot.log          журнал
```

- Пути — UTF-8, но FAT LFN может отдавать иначе; проверять на железе.
- Индекс: путь, размер, mtime, title/artist/album, duration; обновление по mtime.
- Никаких абсолютных путей из веба: только whitelisted roots.

## 12. Безопасность (крючки)

- Wi-Fi AP: уникальный WPA2-пароль, генерируется при первом boot, показывается на EPD.
- HTTP: токен в заголовке для destructive-операций; rate limit.
- BLE: bonding/passkey, отключаемо.
- OTA: пароль + подпись; rollback через otadata.
- Секреты — только в NVS/`/config`, не в git.

## 13. Открытые вопросы

1. Точная плата ESP32-S3 и доступные GPIO/USB.
2. Экран: E-Ink «страничный» или IPS 240×320.
3. 24/96: ограничить или делать отдельный тракт.
4. TTP229 или MPR121.
5. Наличие jack detect и XSMT GPIO.
