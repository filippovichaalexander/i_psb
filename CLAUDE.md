# Project Specification: Ultra-Compact Retro E-Ink Hi-Fi Player (v2.2 — Final Unified Spec)

## 1. Role & Agent Workflow Rules
Ты — Senior Embedded C++, Full-Stack Web & CAD Engineer. Твоя задача — разработать прошивку (PlatformIO / Arduino Framework), встроенный SPA веб-интерфейс (LittleFS) и параметрическую 3D-модель корпуса (`case.scad`) для ультракомпактного автономного Hi-Fi аудиоплеера на базе **ESP32-S3-N16R8 MINI**.

### Обязательные правила разработки (Guardrails):
1. **Modular Architecture:** Строго разделяй код на независимые модули: `AudioEngine`, `InputManager`, `UiRenderer`, `StorageManager`, `WifiPortal`, `BleCompanion`. Никакой бизнес-логики в `loop()`.
2. **Pin Safety:** Строго запрещено использовать `GPIO33–GPIO37` (заняты Octal PSRAM), `GPIO19–GPIO20` (USB D-/D+) и strapping-пины `GPIO0, GPIO3, GPIO45, GPIO46`.
3. **Cyrillic & UTF-8 Support:** Все шрифты интерфейса (`U8g2_for_Adafruit_GFX`), парсеры ID3/Vorbis тегов и веб-загрузчик обязаны корректно работать с кириллицей (UTF-8).
4. **Hearing Protection:** Стартовая громкость при каждом включении плеера программно ограничена на **20%** (`4` из `21`), чтобы защитить слух при использовании чувствительных внутриканальных наушников (IEM) с усилителем MAX97220.
5. **Step-by-Step Build Order:**
   - Шаг 1: `platformio.ini` с поддержкой `qio_opi` PSRAM и актуальными библиотеками (`GxEPD2`, `U8g2_for_Adafruit_GFX`, `ESP32-audioI2S`, `NimBLE-Arduino`, `ESPAsyncWebServer`, `ArduinoJson`).
   - Шаг 2: Драйверы периферии (`AudioEngine`, `InputManager`, `UiRenderer`).
   - Шаг 3: Сетевые и беспроводные модули (`WifiPortal` + фронтенд для `LittleFS`, `BleCompanion`).
   - Шаг 4: Связывание задач FreeRTOS в `main.cpp` и генерация 3D-корпуса `case.scad`.

---

## 2. Hardware-to-Software Architecture & Bus Isolation

Прошивка обязана распределять задачи между двумя ядрами ESP32-S3 и изолировать высокоскоростные шины во избежание заиканий звука и электрических наводок на аудиотракт:

```text
+-----------------------------------------------------------------------------------+

|                                ESP32-S3-N16R8 MINI                                |
|                                                                                   |
|   [ CORE 0: Real-Time Audio Task ]             [ CORE 1: System, UI & Wireless ]  |
|   - Priority: High (configMAX_PRIORITIES-2)    - Priority: Normal (1..2)          |
|   - Ring Buffer: 512KB in Octal PSRAM          - Display Framebuffer (122x250)    |
|   - Decoders: FLAC (24/96), WAV, MP3, AAC      - Hybrid Wheel & Button FSM        |
|   - AudioEngine (ESP32-audioI2S)               - NimBLE GATT + AsyncWebServer     |
|                 |                                        |        |               |
|                 | FreeRTOS xQueue (Commands / Telemetry) |        |               |
|                 +-------------------><-------------------+        |               |
+-----------------|-----------------------|-------------------------|---------------+

                  |                       |                         |
        [ I2S DMA Bus ]         [ SPI Bus #2 (HSPI) ]     [ SPI Bus #1 (FSPI) ]
        GPIO16, 17, 18          GPIO6, 7, 8, 9            GPIO10..15

                  |                       |                         |
                  v                       v                         v
        +-------------------+   +-------------------+     +-------------------+

        | TZT PCM5102 DAC   |   | JETTING MicroSD   |     | WeAct 2.13" E-Ink |
        | -> MAX97220 Amp   |   | 64GB FAT32        |     | (SSD1680, 1-bit)  |
        | -> PJ320 SOP 3.5  |   | (/Music, /config) |     | Partial Refresh   |
        +-------------------+   +-------------------+     +-------------------+
                                                                    ^
        +-----------------------------------------------------------+
        | [ Control & Telemetry Lines (Core 1) ]
        | - TTP229 (2-Wire Bitbang/Timer 10ms): SCL=GPIO4, SDO=GPIO5 -> 8 Sectors
        | - 5 Tactile Switches (INPUT_PULLUP): GPIO1, GPIO2, GPIO38, GPIO39, GPIO40
        | - Vibro Haptic Driver (MOSFET 2N7002): GPIO41 (15ms pulse per wheel tick)
        | - Battery ADC (100K/100K Divider): GPIO42 (Multisampling + EMA filter)
```

### Ключевые правила коммуникации Хард ↔ Софт:
1. **Изоляция двух аппаратных шин SPI:** Экран (`FSPI`) и карта MicroSD (`HSPI`) работают на разных контроллерах SPI. Чтение тяжёлых FLAC-потоков на `Core 0` никогда не блокируется и не прерывается при перерисовке E-Ink экрана на `Core 1`.
2. **Непрерывный DMA-поток I2S:** Кольцевой аудиобуфер (`512 КБ`) выделяется строго во внешней памяти Octal PSRAM (`ps_malloc`), чтобы исключить просадки при обращении к SD-карте.
3. **Межъядерный обмен (Thread Safety):** `Core 1` отправляет команды управления (`PLAY`, `PAUSE`, `SEEK`, `SET_VOL`, `LOAD_TRACK`, `PLAY_STREAM`) в `Core 0` только через потокобезопасную очередь `xQueueSend()`, а обратно получает телеметрию (позицию в секундах, длительность, битрейт, ID3/ICY-теги).
4. **Измерение заряда батареи (`GPIO42`):** Из-за аппаратного делителя `100 кОм / 100 кОм` от `OUT+` к `GND` напряжение на пине `GPIO42` составляет `0.5 * VBAT` (`1.50 В – 2.10 В`). Прошивка выполняет мультисэмплинг (32 замера АЦП раз в 5 секунд с экспоненциальным сглаживанием EMA) и переводит напряжение в проценты по нелинейной кривой разряда Li-Po (`3.25 В = 0%`, `4.18 В = 100%`).

---

## 3. Логика работы беспроводных интерфейсов (Radio Coexistence)

На кристалле ESP32-S3 антенна 2.4 ГГц разделяется между Wi-Fi и Bluetooth, поэтому их работа строго разграничена по сценариям:

1. **Режим локального Hi-Fi плеера (режим по умолчанию):**
   - **Wi-Fi принудительно отключен** (`WiFi.mode(WIFI_OFF)`): это полностью исключает импульсные ВЧ-наводки на аналоговый тракт `PCM5102 -> MAX97220` и экономит заряд компактного аккумулятора `600 мА·ч`.
   - **BLE (`NimBLE-Arduino`) активен в фоне:** работает в энергосберегающем режиме рекламы/соединения, позволяя смартфону-компаньону мгновенно подключаться для поиска треков, управления очередью и отправки обложек альбомов без влияния на качество звука.
2. **Режим `Wi-Fi Sync` (беспроводной веб-загрузчик):**
   - Запускается только по выбору пользователя в меню (во время синхронизации локальное воспроизведение аудио ставится на паузу для максимальной скорости записи на SD-карту).
   - Плеер пытается подключиться к домашней сети из `/config/wifi.json` (`STA`, имя `ipod.local`); при неудаче поднимает собственную точку доступа `E-Ink-iPod-Sync` (пароль `12345678`, IP `192.168.4.1`) и рисует QR-код подключения прямо на E-Ink экране.
3. **Режим `Internet Radio / Podcasts` (сетевой стриминг):**
   - Поднимает Wi-Fi в режиме `STA`, подключается к потоку из `/config/stations.json` и передаёт буфер в `AudioEngine` на `Core 0`. При выходе из режима радио в главное меню Wi-Fi автоматически отключается.

---

## 4. Exact Hardware Pinout & Component Dimensions

### Аппаратная распиновка (ESP32-S3-N16R8 MINI)
- **Display — WeAct Studio 2.13" E-Paper (`250×122 px`, 1-bit Black-White, `SSD1680` / `GxEPD2_213_B74`), SPI Bus #1 (FSPI):**
  - `EPD_MOSI (SDA) = GPIO11`
  - `EPD_SCK (SCL)  = GPIO12`
  - `EPD_CS         = GPIO10`
  - `EPD_DC         = GPIO13`
  - `EPD_RST (RES)  = GPIO14`
  - `EPD_BUSY       = GPIO15`
- **Storage — JETTING MicroSD SPI Adapter (`3.3V`, FAT32 64GB), SPI Bus #2 (HSPI):**
  - `SD_MOSI (SI)   = GPIO7`
  - `SD_MISO (SO)   = GPIO8`
  - `SD_SCK (CLK)   = GPIO6`
  - `SD_CS          = GPIO9`
- **Hi-Fi Audio — TZT PCM5102 DAC -> MAX97220 Amp -> PJ320 SOP (3.5mm), I2S DMA:**
  - `I2S_DIN        = GPIO17`
  - `I2S_BCLK (BCK) = GPIO18`
  - `I2S_LRCK (LCK) = GPIO16`
  - `I2S_MCLK       = -1` *(отключен; пин `SCK` на плате `TZT PCM5102` замкнут на `GND`)*
- **Hybrid Click Wheel & Haptics:**
  - **Сенсор TTP229** (режим Multi-Key `TP2+TP3`; 8 медных секторов по кругу `TP0..TP7`): `TTP_SCL = GPIO4`, `TTP_SDO = GPIO5`.
  - **5 тактовых кнопок `6.0 × 6.0 × 4.3 мм`** (`INPUT_PULLUP`, активный `LOW`):
    - `BTN_MENU = GPIO1` (верх, 12ч)
    - `BTN_PLAY = GPIO2` (низ, 6ч)
    - `BTN_PREV = GPIO38` (лево, 9ч)
    - `BTN_NEXT = GPIO39` (право, 3ч)
    - `BTN_SELECT = GPIO40` (центр кольца)
  - **Вибромотор `Ø 10.0 × 2.7 мм`** (через MOSFET `2N7002` + диод `1N5819`): `VIBRO_PIN = GPIO41`.
- **Battery Telemetry:**
  - **Делитель напряжения `100K / 100K` от `OUT+` к `GND`:** `BAT_ADC = GPIO42`.

### Точные габариты компонентов для 3D-компоновки (Д × Ш × В):
1. **Контроллер `ESP32-S3-N16R8 MINI`:** `23.5 × 18.0 × 3.5 мм`
2. **Дисплей `WeAct Studio 2.13" E-Paper`:** плата `72.0 × 30.0 × 6.0 мм` (видимая область экрана `48.55 × 23.70 мм`, крепёжные отверстия `66.4 × 24.4 мм`)
3. **Слот `JETTING MicroSD 3.3V`:** `25.0 × 22.0 × 3.5 мм`
4. **ЦАП `TZT PCM5102` (без штатного джека):** `32.0 × 17.0 × 3.2 мм`
5. **Усилитель `MAX97220`:** `20.0 × 23.0 × 3.0 мм`
6. **Аудиоразъём `PJ320 SOP` (3.5 мм):** `14.5 × 6.5 × 5.0 мм` (втулка `Ø 5.0 мм`)
7. **Сенсор `TTP229` (обрезанный HW-136):** `35.0 × 15.0 × 3.0 мм`
8. **Тактовые SMD-кнопки (5 шт.):** `6.0 × 6.0 × 4.3 мм`
9. **Вибромотор-таблетка 3В:** `Ø 10.0 × 2.7 мм`
10. **Аккумулятор `Li-Po 602040` (3.7В, 600 мА·ч):** `40.0 × 20.0 × 6.0 мм`
11. **Плата зарядки `RIXITE TP4056 Type-C`:** `26.0 × 17.0 × 3.8 мм`
12. **Стабилизатор `TZT 1.8–5V -> 3.3V`:** `15.0 × 11.0 × 2.5 мм`
13. **Переключатель питания `SS12D00 (3 мм)`:** корпус `8.5 × 3.5 × 4.0 мм`, высота ползунка `3.0 мм`
14. **Крепёжные винты корпуса:** нейлоновые винты **M2.5 × 6 мм** (4 шт. по углам).

---

## 5. Functional Modules Specification

### 1. `AudioEngine` (`Core 0`)
- Декодирование форматов **FLAC (16/24-bit), WAV, MP3, AAC, OGG** с карты MicroSD и потоков HTTP/HTTPS.
- Извлечение метаданных (`Title`, `Artist`, `Album`, `SampleRate`, `BitDepth`) и `ICY-Title` для интернет-радио.
- Режимы воспроизведения: `Sequential`, `Repeat One`, `Repeat All`, `Shuffle`.

