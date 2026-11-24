# Changelog

Формат: [Keep a Changelog](https://keepachangelog.com/ru/1.1.0/), версии — [SemVer](https://semver.org/lang/ru/).

## [Unreleased]

### Added

- M0 blueprint: аудит железа/софта, исправленная распиновка, risk register.
- Docker-окружение сборки (PlatformIO) и mock-сервер веб-интерфейса.
- Buildable firmware skeleton: E-Ink UI, TTP229 + кнопки, SD, батарея, audio backend.
- Native unit tests для батареи, плейлиста, wheel-декодера, UTF-8/ID3, форматирования.
- Lightweight SPA для LittleFS (vanilla JS) + mock API.
- CI: firmware build, native tests, size report, release workflow, clang-format.
- Параметрический каркас корпуса `cad/case.scad`.

### Verified

- 29/29 native unit tests PASS (Docker, 2026-11-24).
- ESP32-S3 firmware builds: flash 1 566 864 B (37.4 %), RAM 46 480 B (14.2 %).
- LittleFS image builds with the SPA (6 MB partition).
- Mock API endpoints (status/files/upload/delete) pass a curl smoke test.
- clang-format passes on 46 C++ files.

### Changed

- `BAT_ADC`: GPIO42 → GPIO10 (на ESP32-S3 GPIO42 не имеет ADC).
- `EPD_CS`: GPIO10 → GPIO21 (освобождение ADC1_CH9).
- Шрифты: Helvetica Cyrillic (не существует в U8g2) → Unifont/10x20 Cyrillic.

### Security

- Зафиксировано: дефолтный пароль AP `12345678` недопустим; план — уникальный пароль на устройство.
