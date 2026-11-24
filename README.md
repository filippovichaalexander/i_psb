# i_psb — ultra-compact retro Hi-Fi player on ESP32-S3

**Status: M0 — blueprint + buildable skeleton.** The firmware boots, drives the E-Paper display,
reads the wheel/buttons, monitors the battery and mounts the SD card. Audio playback uses
`ESP32-audioI2S` (16–48 kHz); the hi-res FLAC path is an open design decision (see the audit).

[English abstract](#english) · [Документация](#документация) · [Быстрый старт](#быстрый-старт-docker)

---

## Что это

Портативный музыкальный плеер в духе iPod на **ESP32-S3-N16R8**:

- **Звук:** PCM5102A → MAX97220 → 3.5 мм, I2S DMA, локальные файлы с microSD.
- **Интерфейс:** E-Ink 2.13" 250×122, ч/б, без подсветки; Click Wheel на TTP229 + 5 тактильных кнопок.
- **Питание:** Li-Po 602040 600 мА·ч, TP4056 Type-C, buck-boost 3.3 В, ADC-контроль заряда.
- **Связь:** BLE-компаньон (план), Wi-Fi-загрузчик через встроенный SPA (план).
- **Корпус:** параметрический OpenSCAD (каркас, не финал).

Проект сознательно **не** гонится за «максимумом функций». Цель — быстрый, предсказуемый,
ремонтопригодный плеер с характером iPod и честными инженерными компромиссами.

## Что изменено против исходного ТЗ (важно)

Исходные документы лежат в корне: [CLAUDE.md](CLAUDE.md), [HARDWARE_AND_WIRING.md](HARDWARE_AND_WIRING.md); аудит — в [docs/03-critique.md](docs/03-critique.md).
Три изменения уже внесены в код и документацию:

| Было | Стало | Почему |
|---|---|---|
| `BAT_ADC = GPIO42` | `BAT_ADC = GPIO10` | На ESP32-S3 ADC есть только на GPIO1–20; GPIO42 — обычный GPIO. |
| `EPD_CS = GPIO10` | `EPD_CS = GPIO21` | Освободили ADC1_CH9 (GPIO10) под делитель батареи. |
| `u8g2_font_helvB*_t_cyrillic` | `u8g2_font_unifont_t_cyrillic` / `cu12_t_cyrillic` | Helvetica-шрифтов с кириллицей в U8g2 нет — код бы не собрался. |

Ещё два пункта требуют решения до закупки/пайки: **USB data path** (TP4056 Type-C — только зарядка)
и **24/96 FLAC** (ESP32-audioI2S официально ограничен 16–48 кГц). Подробности — в аудите.

## Архитектура (кратко)

```text
Core 0  Audio task (high prio)   SD → decoder → I2S DMA → PCM5102A → MAX97220 → jack
Core 1  UI task  (prio 2)        E-Ink renderer, app FSM, battery, logs
        Input task (prio 3)       TTP229 wheel + 5 buttons + haptics
Queues  input→UI, UI→audio commands, audio→UI telemetry
Buses   EPD on FSPI (GPIO10–15*), SD on HSPI (GPIO6–9), I2S (GPIO16–18)
        * EPD_CS moved to GPIO21, see corrected pinout
```

Подробно: [docs/01-architecture.md](docs/01-architecture.md), [docs/02-pinout.md](docs/02-pinout.md).

## Быстрый старт (Docker)

Требуется Docker + Compose. PlatformIO ставится внутрь контейнера — на хост ничего не нужно.

```bash
# 1. Собрать прошивку
make build                 # docker compose run --rm firmware

# 2. Прогнать нативные unit-тесты (без железа)
make test                  # docker compose run --rm native-tests

# 3. Собрать LittleFS-образ с веб-интерфейсом
make fs                    # копирует web/ → firmware/data/www и собирает littlefs.bin

# 4. Открыть веб-интерфейс с mock-API (без железа)
make web-dev               # http://localhost:8080
```

Прошивка на устройство (когда плата подключена):

```bash
make flash PORT=/dev/ttyACM0
```

Инструкции, оговорки и troubleshooting: [docs/11-build-and-flash.md](docs/11-build-and-flash.md).

> **Docker здесь — это reproducible build/dev environment, а не runtime плеера.**
> Само приложение живёт на ESP32; веб-интерфейс раздаётся из LittleFS на устройстве.
> Контейнеры нужны, чтобы сборка не зависела от версии Python/PlatformIO на хосте и
> чтобы CI и разработчик собирали одно и то же.

## Структура репозитория

```text
firmware/          PlatformIO: ESP32-S3, модули, partition table, native-тесты
  lib/ipsb_core/   чистый C++ без Arduino: батарея, плейлист, wheel, UTF-8, ID3
web/               SPA для LittleFS (vanilla JS, без сборщика) + mock API
cad/               параметрический OpenSCAD-каркас корпуса
docs/              аудит, архитектура, pinout, тесты, devlog, bring-up, ADR
docker/            Dockerfile'ы (PlatformIO, dev server)
tools/             сборка/прошивка/упаковка, mock API
.github/workflows/ CI, release, security
```

## Документация

| Документ | Зачем |
|---|---|
| [docs/03-critique.md](docs/03-critique.md) | **Аудит железа и софта: блокеры, риски, альтернативы** |
| [docs/02-pinout.md](docs/02-pinout.md) | Исправленная распиновка и обоснование |
| [docs/01-architecture.md](docs/01-architecture.md) | Задачи, очереди, память, состояния, ошибки |
| [docs/04-risk-register.md](docs/04-risk-register.md) | Реестр рисков с мерами |
| [docs/05-test-plan.md](docs/05-test-plan.md) | Unit / integration / HIL-кейсы |
| [docs/06-devlog.md](docs/06-devlog.md) | История разработки, ошибки, выводы |
| [docs/07-hardware-bringup.md](docs/07-hardware-bringup.md) | Пошаговый ввод железа + список инструментов |
| [docs/08-roadmap.md](docs/08-roadmap.md) | M0–M7 и критерии выхода |
| [docs/09-bom.md](docs/09-bom.md) | BOM, альтернативы, инструменты |
| [docs/10-decisions.md](docs/10-decisions.md) | ADR-журнал решений |
| [docs/11-build-and-flash.md](docs/11-build-and-flash.md) | Docker, PlatformIO, прошивка, OTA |
| [docs/12-web-api.md](docs/12-web-api.md) | HTTP API веб-загрузчика |
| [docs/13-security.md](docs/13-security.md) | Модель угроз и дефолты |
| [docs/14-mechanical.md](docs/14-mechanical.md) | Корпус, антенна, колесо, сборка |

## Безопасность

- Плеер — устройство с Li-Po в замкнутом корпусе. Не заряжайте его без присмотра,
  не оставляйте на солнце, не используйте повреждённую батарею.
- Дефолтный пароль AP `12345678` из исходного ТЗ **не будет использоваться**:
  в M3 он заменяется на уникальный пароль, отображаемый на экране.
- Никогда не коммитьте `secrets.h`, `.env`, приватные ключи. См. [SECURITY.md](SECURITY.md).

## English

`i_psb` is a compact, open-source, retro-styled Hi-Fi player built around an ESP32-S3.
It targets local playback from microSD through a PCM5102A DAC and MAX97220 headphone amp,
an e-ink UI with a TTP229-based click wheel, and a lightweight LittleFS web uploader.
This repository contains firmware, web companion, CAD scaffold, Docker build environment,
CI, and an engineering audit that corrects several blocking issues in the original spec
(notably the battery ADC pin and the unsupported 24/96 FLAC claim).

## Лицензия

MIT — см. [LICENSE](LICENSE). Документация и CAD — та же лицензия, если не указано иное.
