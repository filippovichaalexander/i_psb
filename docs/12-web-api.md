# HTTP API веб-загрузчика (спецификация M3)

> В M0 веб-интерфейс работает с **mock API** (`tools/devserver.py`).
> Прошивка поднимет этот API в M3. Контракт фиксируем сейчас, чтобы фронтенд
> и прошивка не разъехались.

## 1. База

- AP: `http://192.168.4.1` (fallback), STA: `http://ipod.local` (mDNS).
- Формат: JSON UTF-8, кроме upload (raw stream) и OTA (multipart).
- Аутентификация: заголовок `X-IPSB-Token: <token>` для всех `/api/*`, кроме
  `GET /api/status` (read-only) и `GET /api/ping`.
- Токен — 16–32 символа, генерируется при первом boot, показывается на EPD.
- CORS: только same-origin; в dev — `http://localhost:8080`.

## 2. Эндпоинты

### `GET /api/status`

```json
{
  "fw": "0.1.0",
  "uptime_s": 1234,
  "battery": {"mv": 3920, "pct": 78, "charging": false},
  "storage": {"total": 63800000000, "used": 12000000000, "free": 51800000000},
  "playback": {"state": "playing", "title": "Song", "artist": "Artist", "pos": 42, "dur": 210},
  "wifi": {"mode": "sta", "ssid": "home", "rssi": -55},
  "sd": true,
  "errors": []
}
```

### `GET /api/files?path=/Music&offset=0&limit=100`

```json
{
  "path": "/Music",
  "total": 1234,
  "items": [
    {"name": "track.flac", "is_dir": false, "size": 34567890, "mtime": 1760000000},
    {"name": "Album", "is_dir": true, "size": 0, "mtime": 1760000000}
  ]
}
```

- Только внутри `/Music`, `/config`, `/logs`, `/system`.
- Сортировка: dirs first, затем name (byte order; UI может сортировать локально).
- `offset`/`limit` обязательны для больших каталогов; max limit 200.

### `POST /api/upload?path=/Music/Album&name=track.flac`

- Body: raw bytes (`Content-Type: application/octet-stream`) или multipart.
- Прошивка пишет **потоком** на SD, не в RAM.
- Проверки: свободное место, расширение, длина имени, path traversal, лимит 4 ГБ.
- Ответ:

```json
{"ok": true, "path": "/Music/Album/track.flac", "bytes": 34567890}
```

- Ошибки: `409` file exists (если `overwrite=false`), `507` no space, `400` bad path.

### `POST /api/mkdir`

```json
{"path": "/Music/New Album"}
```

### `DELETE /api/file?path=/Music/Album/track.flac`

- Только файлы/пустые папки; рекурсивное удаление — отдельный `?recursive=true`
  с подтверждением в UI.

### `POST /api/playback`

```json
{"action": "play", "path": "/Music/track.flac"}
```

`action`: `play`, `pause`, `resume`, `next`, `prev`, `stop`, `seek`, `volume`.
Для `seek` — `{"action":"seek","value":42}`; для `volume` — `0..21`.

### `GET /api/logs?lines=100`

```json
{"lines": ["[i] ...", "[e] ..."]}
```

- Ring buffer 32×160; не отдавать больше 200 строк.

### `GET /api/wifi` / `POST /api/wifi`

```json
{"ssid": "home", "password": "***", "mode": "sta"}
```

- `GET` не возвращает пароль, только `ssid`/`mode`/`rssi`.
- `POST` сохраняет в `/config/wifi.json` (права 0600, не логировать пароль).
- Перезагрузка Wi-Fi — explicit `{"apply": true}`.

### `POST /api/ota`

- multipart: `firmware.bin` + `sha256` + `signature` (M3+).
- Только с токеном; проверка размера/подписи; OTA в `app1`, rollback при fail.

## 3. Коды ошибок

| HTTP | `error` | Когда |
|---|---|---|
| 400 | `bad_request` | неверный JSON/путь |
| 401 | `unauthorized` | нет/неверный токен |
| 403 | `forbidden` | путь вне whitelist |
| 404 | `not_found` | файл/каталог |
| 409 | `exists` | файл уже есть |
| 413 | `too_large` | > лимита |
| 507 | `no_space` | нет места |
| 500 | `internal` | сбой SD/FS |

## 4. Примеры

```bash
TOKEN=...  # с экрана
curl -s http://192.168.4.1/api/status | jq
curl -s -H "X-IPSB-Token: $TOKEN" \
  "http://192.168.4.1/api/files?path=/Music&limit=20" | jq
curl -s -X POST -H "X-IPSB-Token: $TOKEN" \
  --data-binary @track.flac \
  "http://192.168.4.1/api/upload?path=/Music&name=track.flac"
curl -s -X POST -H "X-IPSB-Token: $TOKEN" -H "Content-Type: application/json" \
  -d '{"action":"pause"}' http://192.168.4.1/api/playback
```

## 5. Mock API

`tools/devserver.py` реализует те же эндпоинты с in-memory файлами и фейковым
статусом. Запуск:

```bash
make web-dev   # http://localhost:8080
```

Токен в mock — `dev-token`; в UI поле токена заполняется автоматически в dev-режиме.

## 6. Безопасность

- Path traversal: canonicalize, запрет `..`, только whitelisted roots.
- Имена: запрет `/`, `\0`, управляющих символов; лимит 255 байт UTF-8.
- Upload: не доверять `Content-Length`; считать фактически записанное.
- Не логировать пароли/токены.
- Rate limit: 10 req/s на IP для API (M3).
