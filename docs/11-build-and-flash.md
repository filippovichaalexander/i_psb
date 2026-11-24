# Сборка, прошивка, отладка

## 1. Что нужно

- Docker + Docker Compose.
- USB-кабель **с данными** (не charge-only) для платы ESP32.
- Пользователь в группе `dialout` (Linux) для доступа к `/dev/ttyACM0`.

На хосте **не нужны** Python/PlatformIO/esptool — всё в контейнере.

## 2. Команды

```bash
make build        # firmware: pio run
make test         # native unit tests
make fs           # web/ → firmware/data/www + littlefs.bin
make web-dev      # mock API + SPA на http://localhost:8080
make lint         # clang-format --dry-run
make cad          # OpenSCAD render (если установлен docker image)
make clean        # удалить .pio/build
```

Прямые команды:

```bash
docker compose run --rm firmware pio run
docker compose run --rm native-tests pio test -e native
docker compose run --rm firmware pio run -t size
docker compose run --rm firmware pio run -t upload   # нужен --device
```

## 3. Docker-окружение

`docker/Dockerfile.pio`:

```dockerfile
FROM python:3.12-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    git curl ca-certificates && rm -rf /var/lib/apt/lists/*
RUN pip install --no-cache-dir platformio==6.2.0
WORKDIR /workspace
```

- `PLATFORMIO_CORE_DIR=/workspace/.pio-core` — кэш платформы в репозитории (игнорируется git).
- Первый `pio run` скачает espressif32 + toolchain (~1 ГБ), это нормально.
- Образ не содержит прошивку; это build-среда.

## 4. PlatformIO env

| Env | Назначение |
|---|---|
| `esp32s3` | Прошивка ESP32-S3, 16 МБ flash, qio_opi PSRAM |
| `native` | Unit-тесты pure C++ (без Arduino) |

`platform = espressif32@8.0.0`, `board = esp32-s3-devkitc-1`,
`board_build.arduino.memory_type = qio_opi`, `board_upload.flash_size = 16MB`.

## 5. Partition table (16 МБ)

| Раздел | Offset | Size | Назначение |
|---|---|---|---|
| nvs | 0x9000 | 0x5000 | Wi-Fi/BLE/settings |
| otadata | 0xE000 | 0x2000 | OTA state |
| app0 | 0x10000 | 4 МБ | прошивка |
| app1 | 0x410000 | 4 МБ | OTA-слот |
| littlefs | 0x810000 | 6 МБ | web, шрифты, config, logs |
| coredump | 0xE10000 | 64 КБ | crash dump |
| nvs_keys | 0xE20000 | 4 КБ | резерв |

## 6. Прошивка

### Через Docker (Linux)

```bash
# найти порт
ls /dev/ttyACM* /dev/ttyUSB*

# прошить (пример)
docker compose run --rm \
  --device=/dev/ttyACM0 \
  --group-add dialout \
  firmware pio run -t upload --upload-port /dev/ttyACM0
```

Если `--device` не работает в вашей версии Compose — используйте `docker run`:

```bash
docker run --rm -it \
  --device=/dev/ttyACM0 --group-add dialout \
  -v "$PWD":/workspace -w /workspace/firmware \
  -e PLATFORMIO_CORE_DIR=/workspace/.pio-core \
  ipsb-pio pio run -t upload --upload-port /dev/ttyACM0
```

### Через esptool (аварийно)

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
  write_flash 0x0 build/bootloader.bin \
  0x8000 build/partitions.bin \
  0x10000 build/firmware.bin
```

Точные offset'ы смотрите в `firmware/partitions_16MB.csv`.

### LittleFS

```bash
make fs
docker compose run --rm --device=/dev/ttyACM0 --group-add dialout \
  firmware pio run -t uploadfs --upload-port /dev/ttyACM0
```

## 7. Serial monitor

```bash
docker compose run --rm --device=/dev/ttyACM0 --group-add dialout \
  firmware pio device monitor -b 115200
```

Ожидаемый вывод:

```text
[i] i_psb 0.1.0 boot
[i] reset_reason=...
[i] flash=16777216 psram=8388608 heap=...
[i] SD: mounted, size=...
[i] EPD: init OK
[i] audio: I2S ready, volume=4/21
```

## 8. OTA (план M2/M3)

- ArduinoOTA при активном Wi-Fi + пароль из NVS.
- HTTP `/api/ota` с токеном и подписью (M3).
- Rollback: `otadata` + `app1`.
- После OTA — проверка версии и health-check; иначе откат.

## 9. Troubleshooting

| Проблема | Что проверить |
|---|---|
| `pio: command not found` | Вы не в контейнере; `make build`. |
| `Permission denied /dev/ttyACM0` | `sudo usermod -aG dialout $USER`, перелогиниться. |
| Порт не появляется | Кабель data, драйвер, BOOT+RST. |
| `Failed to connect` | Зажать BOOT, нажать RST, отпустить BOOT, повторить. |
| Boot loop | 3.3 В, brownout, PSRAM mode, USB ток. |
| PSRAM 0 | `qio_opi`, 16 МБ flash, правильная плата. |
| SD mount fail | FAT32, CS, 3.3 В, другая карта. |
| EPD мусор | BUSY/RST, `GxEPD2_213_B74`, 3.3 В. |
| Нет звука | XSMT, SCK-GND, I2S пины, MCLK. |
| CI падает на lib | Проверить pinned version, `pio pkg update`. |

## 10. CI

См. `.github/workflows/ci.yml`:

- `firmware`: `pio run` в контейнере, кэш `.pio-core`/`.pio`, size report.
- `native`: `pio test -e native`.
- `format`: `clang-format --dry-run --Werror`.
- `web`: `node --check`, проверка JSON.
- `cad`: OpenSCAD render (если доступен образ).
- `release`: по тегу `v*` — bin/fs/merged/STL в GitHub Release.

## 11. Воспроизводимость

- Версии библиотек — точные (`@1.6.9`, `#4.0.1`), не `^`.
- PlatformIO — `6.2.0`.
- Платформа — `espressif32@8.0.0`.
- Обновление версий — отдельный PR + прогон CI + запись в devlog.
