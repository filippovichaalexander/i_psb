# Распиновка и электрическая карта (исправленная)

> **Статус:** M0. Карта соответствует коду в `firmware/include/config.h`.
> **Перед пайкой** сверьте её с реальной платой ESP32-S3 — у продавцов «N16R8 MINI»
> разные наборы выведенных GPIO. См. раздел «Что проверить на плате».

## 1. Главная таблица

| Узел | Сигнал | GPIO | Шина / режим | Примечание |
|---|---|---|---|---|
| **E-Ink WeAct 2.13"** | `SDA` (MOSI) | **11** | FSPI | `SPI.begin(12, -1, 11, 21)` |
| | `SCL` (SCK) | **12** | FSPI | |
| | `CS` | **21** | FSPI, software CS | **Перенесён с GPIO10** |
| | `D/C` | **13** | GPIO out | |
| | `RES` | **14** | GPIO out | |
| | `BUSY` | **15** | GPIO in | |
| **microSD JETTING** | `SI` (MOSI) | **7** | HSPI | Отдельный контроллер SPI |
| | `SO` (MISO) | **8** | HSPI | |
| | `CLK` (SCK) | **6** | HSPI | |
| | `CS` | **9** | HSPI | |
| **PCM5102A** | `DIN` | **17** | I2S | |
| | `BCK` | **18** | I2S | |
| | `LCK` (LRCK) | **16** | I2S | |
| | `SCK` (MCLK) | **-1** / 47/48 | I2S MCLK | По умолчанию `SCK` на GND (PLL); опция — MCLK с GPIO47/48 |
| **TTP229** | `SCL` | **4** | 2-wire bitbang | Выход MCU, ~10–50 кГц |
| | `SDO` | **5** | 2-wire input | `INPUT_PULLUP`; active-low |
| **Кнопки** | `MENU` | **1** | `INPUT_PULLUP` | 12 ч |
| | `PLAY` | **2** | `INPUT_PULLUP` | 6 ч |
| | `PREV` | **38** | `INPUT_PULLUP` | 9 ч |
| | `NEXT` | **39** | `INPUT_PULLUP` | 3 ч |
| | `SELECT` | **40** | `INPUT_PULLUP` | центр |
| **Вибромотор** | Gate 2N7002 | **41** | GPIO out | 15 мс импульс, 10K на GND |
| **Батарея** | ADC делителя | **10** | ADC1_CH9 | **Перенесён с GPIO42**; 100K/100K + 100 нФ |

## 2. Почему изменения

### `BAT_ADC`: GPIO42 → GPIO10

ESP32-S3: ADC1 = GPIO1–GPIO10, ADC2 = GPIO11–GPIO20. **GPIO42 не имеет ADC.**
ADC2 нельзя использовать при активном Wi-Fi, поэтому батарея должна быть на ADC1.
GPIO10 — ADC1_CH9, ранее был `EPD_CS`; CS экрана легко переносится на любой GPIO.

### `EPD_CS`: GPIO10 → GPIO21

GPIO21 свободен, не strapping, не USB, не PSRAM, не ADC-конфликт. GxEPD2 управляет
CS программно, поэтому аппаратный FSPICS0 не нужен.

### Шрифты

`u8g2_font_helvB*_t_cyrillic` не существует. Используем `unifont_t_cyrillic` и
`cu12_t_cyrillic`; кастомный шрифт — M2.

## 3. Занятые и свободные GPIO

| Диапазон | Статус |
|---|---|
| GPIO0 | Strapping (BOOT). Не использовать; вывести кнопку BOOT на корпус. |
| GPIO1–2 | Кнопки MENU/PLAY (ADC1, но заняты). |
| GPIO3 | Strapping (JTAG sel). Не использовать. |
| GPIO4–5 | TTP229. |
| GPIO6–9 | microSD (HSPI). |
| GPIO10 | **BAT_ADC** (ADC1_CH9). |
| GPIO11–15 | E-Ink (FSPI + control). |
| GPIO16–18 | I2S. |
| GPIO19–20 | USB D−/D+. **Должны быть выведены наружу** (см. B2). |
| GPIO21 | E-Ink CS. |
| GPIO26–32 | Flash (не трогать). |
| GPIO33–37 | Octal PSRAM (не трогать). |
| GPIO38–40 | Кнопки PREV/NEXT/SELECT. Использование отключает аппаратный JTAG на 39–42; USB-Serial-JTAG на 19/20 остаётся. |
| GPIO41 | Вибромотор. |
| GPIO42 | Свободен как GPIO; **ADC нет**. Можно повесить `XSMT`, `CHRG` (через делитель) или LED. |
| GPIO43–44 | UART0 TX/RX. Оставить для логов/аварийного доступа. |
| GPIO45–46 | Strapping. Не использовать. |
| GPIO47–48 | Свободны (если выведены на плате). Кандидаты: MCLK, XSMT, I2C для MPR121/MAX17048. |

> **Проверка:** если на вашей плате не выведены 38–42 или 47–48, распиновку нужно
> пересобрать. Это первое, что надо подтвердить фотографией/ссылкой на товар.

## 4. Дерево питания

```text
Li-Po 602040 3.7V ──► SS12D00 (на стороне нагрузки!) ──► TP4056 B+/B-
                                                          │
USB-C 5V ─────────────────────────────────────────────────┘
                                                          │
                                             OUT+ (3.0–4.2V)
                    ┌──────────────────────┬──────────────┴───────────────┐
                    ▼                      ▼                              ▼
          TZT 3.3V buck-boost      PCM5102A VIN                 MAX97220 VCC
                    │                      │                              │
              3.3V_DIG                     │ I2S                          │ analog
        ┌───────────┼───────────┐          ▼                              ▼
        ▼           ▼           ▼      PCM5102A L/R ──► MAX97220 INL/INR ──► PJ320
     ESP32-S3    EPD 2.13"   microSD
        │           │           │
        └───────────┴───────────┴──► TTP229 VCC (3.3V)

BAT_ADC ← делитель 100K/100K от OUT+ к GND, средняя точка → GPIO10 + 100 нФ
Вибромотор + ← OUT+; − ← Drain 2N7002; Source → GND; Gate ← GPIO41 + 10K↓
```

**Правила:**

- `OUT−` и все GND — «звездой» в одной точке у TP4056.
- Аналоговый GND PCM5102A/MAX97220 — отдельной дорожкой к звезде, не через цифровой шум.
- 22 мкФ 0805 — по питанию MAX97220; 100 нФ — везде рядом с пинами.
- 3.3V_DIG — только ESP32/EPD/SD/TTP. **Не** питать PCM5102A/MAX97220 от 3.3V_DIG:
  им нужен `OUT+` (у PCM5102A свой LDO).
- Не паять `Vo` стабилизатора на пин `5V` ESP32.

## 5. Разъёмы и механика

| Разъём | Назначение | Примечание |
|---|---|---|
| USB-C #1 | Зарядка (TP4056) | Только питание |
| USB-C #2 / pogo | Данные ESP32 (GPIO19/20) | **Обязательно** для прошивки/логов |
| 3.5 мм PJ320 | Наушники | Tip=L, Ring=R, Sleeve=GND; желательно со switch для detect |
| SS12D00 | Питание | На стороне нагрузки, не на B+ |
| BOOT/RST | Recovery | Отверстия под скрепку |
| Pogo 4-pin | `3V3/GND/TX/RX` или `D+/D−` | Внутри корпуса, для сервиса |

## 6. Что измерить на макете (до корпуса)

| Точка | Ожидание | Допуск |
|---|---|---|
| `OUT+` от батареи | 4.2 В заряжено, 3.0–3.7 В разряжено | ±0.05 В |
| `3.3V_DIG` | 3.30 В | ±0.05 В, ripple < 30 мВpp |
| `BAT_ADC` (GPIO10) | `OUT+ / 2` | ±0.02 В |
| I2S BCK | 2.8224 МГц @44.1k (32×fs) или 5.6448 МГц (64×fs) | по формату |
| I2S LRCK | 44.1 кГц | ±1% |
| EPD CS/DC | 3.3 В логика | — |
| SD CLK | ≤10 МГц (старт) | — |
| Ток в простое (EPD static, BLE off) | 25–45 мА | измерить |
| Ток при mp3 44.1 | 70–110 мА | измерить |

## 7. Ссылки

- [ESP32-S3 ADC](https://docs.espressif.com/projects/esp-idf/en/v6.1.1/esp32s3/api-reference/peripherals/adc.html)
- [ESP32-S3 GPIO](https://docs.espressif.com/projects/esp-idf/en/v6.1.1/esp32s3/api-reference/peripherals/gpio.html)
- [GxEPD2](https://github.com/ZinggJM/GxEPD2)
- [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S)
