# Журнал архитектурных решений (ADR)

| ADR | Решение | Статус | Дата |
|---|---|---|---|
| [0001](adr/0001-audio-stack.md) | Arduino + ESP32-audioI2S для v0; абстракция под hi-res | Accepted | 2026-11-24 |
| [0002](adr/0002-display-and-refresh.md) | E-Ink UI страничный, partial по области, full каждые 20–30 | Accepted (экран — open) | 2026-11-24 |
| [0003](adr/0003-battery-adc-pin.md) | `BAT_ADC` = GPIO10, `EPD_CS` = GPIO21 | Accepted | 2026-11-24 |
| [0004](adr/0004-wheel-input.md) | TTP229 + декодер по переходам; MPR121 — план B | Accepted | 2026-11-24 |
| [0005](adr/0005-build-and-branches.md) | Docker/PlatformIO, main/develop, CI | Accepted | 2026-11-24 |
| [0006](adr/0006-security-model.md) | Уникальный AP-пароль, токен, BLE bonding, OTA-подпись | Accepted (частично) | 2026-11-24 |
| [0007](adr/0007-usb-data-path.md) | Вывести USB data (вариант 1/3) | Proposed | 2026-11-24 |

## Формат ADR

```markdown
# ADR-NNNN: Название

- Статус: Proposed | Accepted | Superseded | Rejected
- Дата:
- Контекст:
- Решение:
- Последствия:
- Альтернативы:
```
