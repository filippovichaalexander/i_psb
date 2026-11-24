# План тестирования

## 1. Уровни

| Уровень | Где | Что проверяет | Частота |
|---|---|---|---|
| Unit | `firmware/test` (native) | Чистая логика: батарея, плейлист, wheel, UTF-8/ID3, формат | каждый PR |
| Build | Docker + CI | Компиляция ESP32, размер, зависимости | каждый PR |
| Web | Node syntax + mock API | Синтаксис, API-контракт, отсутствие внешних зависимостей | каждый PR |
| HIL | Макет с платой | Дисплей, SD, кнопки, TTP, DAC/AMP, батарея, RF | каждый milestone |
| Soak | Собранное устройство | 1–4 ч воспроизведения, заряд/разряд, температура | перед релизом |

## 2. Unit-кейсы (native)

| ID | Модуль | Вход | Ожидание |
|---|---|---|---|
| U-BAT-01 | battery | 3250 мВ | 0% |
| U-BAT-02 | battery | 4180 мВ | 100% |
| U-BAT-03 | battery | 3700 мВ | 35–55% (интерполяция) |
| U-BAT-04 | battery | 3000 мВ | 0% (clamp) |
| U-BAT-05 | battery | 4200 мВ | 100% (clamp) |
| U-BAT-06 | EMA | 0, затем 100 | плавный рост, не скачок |
| U-BAT-07 | divider | 2100 мВ ADC | 4200 мВ батареи |
| U-VOL-01 | volume | старт | 4/21 = 19% |
| U-VOL-02 | volume | clamp 25 | 18 (max) |
| U-PL-01 | playlist | repeat off, конец | next() = false |
| U-PL-02 | playlist | repeat all, конец | next() = 0 |
| U-PL-03 | playlist | repeat one | next() = тот же |
| U-PL-04 | playlist | prev на 0, repeat off | false |
| U-PL-05 | playlist | shuffle, seed | не тот же индекс подряд |
| U-WH-01 | wheel | 0→1→2 | +1, +1 |
| U-WH-02 | wheel | 2→1→0 | −1, −1 |
| U-WH-03 | wheel | 7→0 | +1 (wrap) |
| U-WH-04 | wheel | 0→4 | 0 (слишком большой прыжок) |
| U-WH-05 | wheel | 0+1 (multi) → 1+2 | +1 (центроид) |
| U-WH-06 | wheel | пропадание 200 мс | сброс, следующий переход не скачет |
| U-UTF-01 | utf8 | «Привет» | длина/границы корректны |
| U-UTF-02 | utf8 | обрезка в середине 2-байтного | без битого символа |
| U-UTF-03 | utf8 | CP1251 «Привет» | UTF-8 «Привет» |
| U-UTF-04 | utf8 | ellipsize | «…» и валидный UTF-8 |
| U-ID3-01 | id3 | UTF-16LE BOM «Тест» | «Тест» |
| U-ID3-02 | id3 | UTF-8 frame | корректно |
| U-ID3-03 | id3 | мусор/короткий буфер | пусто, без падения |
| U-ID3-04 | id3 | TIT2/TPE1/TALB | три поля |
| U-FMT-01 | format | 0, 59, 60, 3661 | 0:00, 0:59, 1:00, 1:01:01 |
| U-FMT-02 | format | bytes | «1.0 MB» |

**Запуск:**

```bash
make test
# или
docker compose run --rm native-tests
```

## 3. Build/CI-кейсы

| ID | Проверка | Ожидание |
|---|---|---|
| B-01 | `pio run -e esp32s3` | SUCCESS |
| B-02 | размер flash | < 80% app-раздела, warning при >90% |
| B-03 | предупреждения компилятора | без новых critical |
| B-04 | `clang-format --dry-run --Werror` | без диффа |
| B-05 | `node --check web/app.js` | OK |
| B-06 | OpenSCAD render | STL создаётся |
| B-07 | release artifacts | bin + littlefs + merged + STL |

## 4. HIL-кейсы

| ID | Что | Метод | Критерий |
|---|---|---|---|
| H-PWR-01 | 3.3 В | Мультиметр | 3.30 ±0.05 В, ripple <30 мВ |
| H-PWR-02 | Потребление idle | USB-метр/PSU | <45 мА (BLE off) |
| H-PWR-03 | Потребление mp3 | PSU | 70–110 мА, без пиков >200 мА |
| H-EPD-01 | Инициализация | Прошивка | Boot screen, без мусора |
| H-EPD-02 | Full refresh | Смена экрана | <3 с, без артефактов |
| H-EPD-03 | Partial | Выбор в списке | <0.5 с, ghosting допустим |
| H-EPD-04 | 30 partial | Серия | Full refresh сбрасывает ghost |
| H-SD-01 | Mount | Прошивка | 64 ГБ FAT32, лог OK |
| H-SD-02 | Список | 100+ файлов | <2 с, кириллица корректна |
| H-SD-03 | Play FLAC 16/44.1 | Наушники | без заиканий 10 мин |
| H-SD-04 | Play MP3 320 | Наушники | без заиканий 10 мин |
| H-BTN-01 | 5 кнопок | Нажатие | событие, debounce |
| H-WH-01 | 8 секторов | Медленное вращение | +/−1 на сектор |
| H-WH-02 | Удержание | 5 с | нет «дребезга», autocal не ломает |
| H-WH-03 | Быстрое вращение | 2 оборота | не теряет больше 1 тика |
| H-AUD-01 | Тишина | Пауза | нет щелчков/шума |
| H-AUD-02 | Старт трека | Play | нет громкого «попа» |
| H-AUD-03 | Громкость 4/21 | IEM | комфортно, не громко |
| H-AUD-04 | Max 18/21 | 32Ω dummy | без клиппинга |
| H-BAT-01 | ADC vs мультиметр | 3 точки | ±0.05 В |
| H-BAT-02 | Кривая | Разряд | проценты монотонны |
| H-RF-01 | BLE RSSI 1 м | nRF Connect | > −60 дБм |
| H-RF-02 | BLE RSSI 5 м | nRF Connect | > −75 дБм |
| H-RF-03 | Wi-Fi AP | Телефон | подключение, IP |
| H-THERM-01 | Заряд 30 мин | Термопара | TP4056 < 60 °C |
| H-THERM-02 | Play 30 мин | Термопара | корпус < 45 °C |

## 5. Soak / регрессия

| ID | Сценарий | Длительность | Критерий |
|---|---|---|---|
| S-01 | FLAC 44.1 loop | 2 ч | 0 underrun, heap стабилен |
| S-02 | MP3 + BLE advertising | 2 ч | 0 underrun, RSSI стабилен |
| S-03 | Смена треков 500× | 1 ч | нет утечек, UI жив |
| S-04 | Заряд/разряд 3 цикла | — | ёмкость, температура, % |
| S-05 | Wi-Fi upload 1 ГБ | 30 мин | без corruption, скорость |

## 6. Журнал прогонов

| Дата | Версия | Уровень | Результат | Комментарий |
|---|---|---|---|---|
| 2026-11-24 | M0 | Unit (native) | **PASS 29/29** | 0.7 s, Docker |
| 2026-11-24 | M0 | Build esp32s3 | **PASS** | flash 1 566 864 B (37.4 %), RAM 46 480 B (14.2 %) |
| 2026-11-24 | M0 | LittleFS | **PASS** | `make fs`, образ 6 291 456 B |
| 2026-11-24 | M0 | Web syntax | **PASS** | `node --check`, manifest JSON |
| 2026-11-24 | M0 | Web mock API | **PASS** | status, files, upload, delete (curl) |
| 2026-11-24 | M0 | clang-format | **PASS** | 46 files |
| 2026-11-24 | M0 | HIL | **BLOCKED** | нет подтверждённой платы; см. 07-bringup |
| — | M0 | CI (GitHub Actions) | ожидает push | см. Actions после develop |

> Обновляйте таблицу после каждого прогона. FAIL без причины — не результат.
