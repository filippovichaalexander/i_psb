# ADR-0005: Сборка, Docker, ветки, CI

- **Статус:** Accepted
- **Дата:** 2026-11-24
- **Контекст:** Нужна воспроизводимая сборка на разных машинах; хост не должен
  зависеть от Python/PlatformIO. Публичный репозиторий.
- **Решение:**
  - `main` — стабильно; `develop` — интеграция; `feat/*` → PR.
  - Docker: `python:3.12-slim` + `platformio==6.2.0` (Docker Hub
    `platformio/platformio-core` недоступен). Версии библиотек пинуются.
  - CI: build, native tests, clang-format, web syntax, size, release по тегу.
  - Docker — build/dev, не runtime устройства.
- **Последствия:** одинаковые сборки локально/CI; кэш `.pio-core`; образ ~1 ГБ
  после установки toolchain.
- **Альтернативы:** pioarduino (лишняя зависимость); ESP-IDF (больше работы).
