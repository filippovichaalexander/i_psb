# Contributing

## Ветки

- `main` — только стабильные, проверенные состояния. Прямые пуши запрещены.
- `develop` — интеграционная ветка; сюда идут изменения.
- `feat/*`, `fix/*`, `docs/*` — короткоживущие ветки от `develop`.

Поток: `feat/x` → PR → `develop` → PR → `main` (release).

## Коммиты

Используется Conventional Commits:

```text
feat(ui): add partial refresh policy
fix(power): move battery ADC to GPIO10
docs(audit): record TTP229 auto-calibration finding
ci: cache PlatformIO core directory
```

Один коммит — одна причина. Не смешивайте переформатирование и логику.
Сообщения коммитов и код — на английском; документация — на русском (английские
термины и подписи к таблицам допустимы).

## Перед PR

```bash
make test        # native unit tests
make build       # firmware must compile
make lint        # clang-format check
```

Обязательно обновите:

- `docs/06-devlog.md` — что сделано, какие ошибки встретились, выводы;
- `docs/05-test-plan.md` — новые/изменённые кейсы;
- `docs/10-decisions.md` — если принято архитектурное решение (ADR).

## Стиль кода

- C++17, без исключений и RTTI на устройстве (`-fno-exceptions` уже в Arduino).
- Никакой бизнес-логики в `loop()`; только задачи FreeRTOS и очереди.
- Не блокировать audio task дольше 1 мс; не делать `delay()` в audio task.
- Любое новое железо — сначала документ, потом код.
- Не использовать `String` в audio task и в hot path.

## Железо

Перед пайкой прочитайте [docs/07-hardware-bringup.md](docs/07-hardware-bringup.md).
Любое изменение распиновки — через [docs/02-pinout.md](docs/02-pinout.md) и ADR.
