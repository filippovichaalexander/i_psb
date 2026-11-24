# Дневник разработки

Формат записи: **дата → контекст → сделано → найдено/ошибки → решение → вывод → дальше**.
Записи не удаляются; ошибочные решения остаются с пометкой.

---

## 2026-11-24 — M0: аудит, репозиторий, каркас

### Контекст

Получено ТЗ v2.2 (`CLAUDE.md`, `HARDWARE_AND_WIRING.md`) на ультракомпактный
E-Ink Hi-Fi плеер. Задача: проанализировать, создать публичный репозиторий
`i_psb`, настроить Docker/CI, вести документацию, дать конструктивную критику.

### Сделано

- Проведён аудит железа и софта; найдены блокеры (ADC, USB, FLAC, шрифты, TTP229).
- Создан публичный репозиторий, ветки `main` и `develop`.
- Исправлена распиновка: `BAT_ADC` 42→10, `EPD_CS` 10→21.
- Собран каркас: PlatformIO, модули, native-тесты, web SPA, Docker, CI, CAD.
- Задокументированы риски, тест-план, bring-up, BOM, ADR.

### Найдено / ошибки

| # | Симптом | Причина | Решение |
|---|---|---|---|
| 1 | `BAT_ADC=GPIO42` не измеряет | У S3 ADC только GPIO1–20 | Перенос на GPIO10 (ADC1_CH9) |
| 2 | `u8g2_font_helvB*_t_cyrillic` не компилируется | Helvetica в U8g2 без кириллицы | Unifont/10x20 Cyrillic |
| 3 | `gh repo create --push` упал | обёртка `gh` не нашла `git-remote-https` | Прямой `git push` сработал |
| 4 | Docker Hub `platformio/platformio-core` недоступен | образ не существует/переименован | `python:3.12-slim` + `pip install platformio==6.2.0` |
| 5 | 24/96 FLAC в ТЗ | audioI2S документирует 16–48 кГц | v0 — 48 кГц; hi-res тракт — отдельное решение |
| 6 | TTP229 «залипает» при удержании | autocal + 32 мс | Декодер по переходам; MPR121 как план B |
| 7 | `kVolumeStartupStep` не найден | `types.h` не включал `ipsb/volume.h` | Добавлен include |
| 8 | `U8G2_FOR_ADAFRUIT_GFX` без `setFontPosTop` | В 1.8.0 позиционирование по baseline, нет `u8g2_uint_t` | `y + getFontAscent()`, `int16_t` |
| 9 | `Network.h` / `FS.h` not found | `lib_ldf_mode = deep+` ломает резолв встроенных библиотек ядра | Убран deep+, default LDF |
| 10 | Лишняя зависимость U8g2 | FAF сам содержит `u8g2_fonts.c` (1689 шрифтов) | U8g2 удалён из `lib_deps` |
| 11 | Риск голодания I2S | audioI2S создаёт свою DMA-задачу priority 2 | feeder priority 1, не выше DMA |
| 12 | CI: `Failed to save cache: tar exit 2` | контейнер пишет `.pio-core`/`.pio` от root, `actions/cache` не может прочитать | `sudo chown -R $(id -u):$(id -g)` перед cache post-step |
| 13 | CI: Node.js 20 deprecation warnings | `actions/*@v4` ещё на Node 20 | не блокирует; обновить actions отдельным PR |

### Верификация M0 (2026-11-24)

| Проверка | Команда | Результат |
|---|---|---|
| Native unit | `docker compose run --rm native-tests` | **29/29 PASS**, 0.7 s |
| ESP32 build | `docker compose run --rm firmware pio run` | **SUCCESS**, 45 s first / 7 s incremental |
| RAM | size report | 46 480 B / 327 680 B (**14.2 %**) |
| Flash | size report | 1 566 864 B / 4 194 304 B (**37.4 %**) |
| LittleFS | `make fs` | SUCCESS, образ 6 291 456 B (раздел 6 МБ) |
| Web JS | `node --check web/app.js` | PASS |
| Mock API | status/files/upload/delete | PASS (curl) |
| Format | `./tools/lint.sh` | PASS |
| CI (push) | GitHub Actions run 45449902939 | **PASS** (firmware, native, format, web, CAD) |
| CI (PR) | GitHub Actions run 45450030167 | **PASS** |
| HIL | — | **BLOCKED**: нет подтверждённой платы/железа |

### Решения

- **AD-001:** v0 — Arduino + ESP32-audioI2S (16–48 кГц), абстракция под будущий hi-res.
- **AD-002:** E-Ink UI — страничный, partial по области, full каждые 20–30 partial.
- **AD-003:** Docker — только reproducible build/dev; runtime — на ESP32.
- **AD-004:** TTP229 остаётся в M0; MPR121/IPS рассматриваются после bring-up.
- **AD-005:** Публичный репозиторий, MIT, приватный email через GitHub noreply.

### Выводы

- ТЗ красивое, но содержит три «несовместимости с реальностью» (ADC, USB, FLAC) и
  одну ошибку компиляции (шрифт). Их лучше было найти до пайки — что и сделано.
- E-Ink + iPod — это осознанный компромисс: получится «широкий» плеер, не классический
  portrait. Нужно решение владельца.
- Без подтверждённой платы ESP32 нельзя фиксировать pinout. Это первый вход от владельца.

### Дальше

1. Подтвердить плату/GPIO/USB.
2. Собрать макет: ESP32 + EPD + SD + кнопки + батарея.
3. Проверить TTP229 и ADC на железе.
4. Решить экран и 24/96.
5. M1: bring-up, замеры, обновление risk register.

---

## Шаблон

```markdown
## YYYY-MM-DD — <тема>

### Контекст
### Сделано
### Найдено / ошибки
| # | Симптом | Причина | Решение |
### Решения
### Выводы
### Дальше
```
