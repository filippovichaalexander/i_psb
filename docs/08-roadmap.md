# Дорожная карта

## Определение готовности (DoD) для milestone

- Код собирается в CI, тесты зелёные, размер в бюджете.
- Документация обновлена (devlog, test plan, risk register).
- Есть демонстрация на реальном железе или mock, и она записана.
- Нет открытых блокеров, влияющих на следующий milestone.

---

## M0 — Blueprint + buildable skeleton ✅ (2026-11-24)

**Deliverables**

- Аудит и исправленная распиновка.
- Репозиторий, ветки, CI, Docker.
- Каркас прошивки: EPD, input, SD, battery, audio backend.
- Native-тесты, web SPA + mock, CAD-каркас.
- Документация: риски, тесты, bring-up, BOM, ADR.

**Exit criteria:** `pio run` и `pio test -e native` зелёные; документация полная;
решения по блокерам зафиксированы (часть — открыта до ответа владельца).

---

## M1 — Hardware bring-up (1–2 недели)

**Цель:** доказать, что железо работает на макете, без корпуса.

**Deliverables**

- Подтверждённая плата ESP32, фото/схема, финальный pinout.
- Питание: 3.3 В, ripple, ток; TP4056 с Rprog 3–4K, температура.
- EPD full/partial, SD 64 ГБ, кнопки, TTP229, вибро.
- ADC ±0.05 В, кривая разряда.
- DAC/AMP: синус, без клиппинга/щелчков, старт 4/21.
- RF: RSSI с батареей/медью.
- Заполненный `06-devlog.md` и `05-test-plan.md` (HIL).

**Exit criteria:** все HIL-кейсы M1 PASS или ACCEPTED с объяснением.
**Блокеры:** нет USB data path; плата не подтверждена; экран не тот.

---

## M2 — Local player (2–3 недели)

**Цель:** слушать музыку с SD, управлять колесом, видеть нормальный UI.

**Deliverables**

- Плейлист, repeat/shuffle, next/prev, seek.
- ID3/Vorbis, кириллица, fallback на имя файла.
- Индекс библиотеки, быстрый boot.
- Now Playing: прогресс (10–15 с), громкость, батарея.
- Настройки: repeat, max volume, logs.
- OTA + BOOT/RST + WDT + coredump.
- Кастомный шрифт (Cyrillic) или обоснованный отказ.

**Exit criteria:** 2 ч FLAC 44.1 без underrun; 500 смен треков без утечек;
UI откликается ≤200 мс; HIL-кейсы M2 PASS.

---

## M3 — Wi-Fi sync + web uploader (2 недели)

**Deliverables**

- STA из `/config/wifi.json`, fallback AP с уникальным паролем.
- HTTP API: status, files, upload (stream), delete, mkdir, logs.
- LittleFS SPA, gzip, токен для destructive.
- QR-код на EPD.
- Upload 1 ГБ без corruption; пауза воспроизведения при синхронизации.
- OTA через веб (пароль/подпись).

**Exit criteria:** S-05 PASS; AP-пароль уникален; path traversal закрыт.

---

## M4 — BLE companion (1–2 недели)

**Deliverables**

- NimBLE GATT: control, queue, metadata, album art (1-bit).
- Pairing/passkey; отключаемо.
- Команды: play/pause/next/prev/seek/volume.
- Тест RSSI/стабильность с аудио.

**Exit criteria:** 2 ч MP3 + BLE без underrun; pairing работает; нет открытого GATT.

---

## M5 — Radio/podcasts (1–2 недели)

**Deliverables**

- `/config/stations.json`, HTTP/HTTPS, ICY.
- Реконнект, буфер, таймауты.
- Wi-Fi включается только в режиме радио, выключается при выходе.
- UI: станция, bitrate, title.

**Exit criteria:** 30 мин потока без зависаний; Wi-Fi off в локальном режиме.

---

## M6 — Case + assembly (2–3 недели)

**Deliverables**

- Финальный `case.scad`: окно экрана, колесо, USB, jack, switch, BOOT/RST.
- Антенное окно, вентиляция, латунные вставки.
- Прототип PETG, примерка, доработка.
- Сборка, strain relief, каптон, баланс.
- Фото/видео сборки.

**Exit criteria:** корпус закрывается без усилия; RSSI в норме; TP4056 <60 °C;
устройство переживает 3 цикла заряд/разряд.

---

## M7 — Polish + release (1–2 недели)

**Deliverables**

- Финальные UI-экраны, иконки, шрифт.
- Батарея: калибровка, low-power, сон экрана.
- Документация для сборки, BOM, прошивка, OTA.
- Релиз `v1.0.0`, артефакты, changelog.
- 4-часовой soak.

**Exit criteria:** S-01..S-04 PASS; релиз воспроизводим из тега; известные ограничения
описаны честно.

---

## Зависимости и риски по срокам

```text
M0 ──► M1 ──► M2 ──► M3 ──► M4
        │       │
        │       └──► M6 (корпус) ──► M7
        └──► M5 (радио)
```

- M1 блокируется ответом по плате и USB.
- M2 блокируется решением по экрану/24-96 (можно частично).
- M6 нельзя начинать до стабилизации габаритов и антенны.
