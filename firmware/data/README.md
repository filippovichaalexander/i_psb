# LittleFS data

`tools/package_web.sh` copies the static SPA from `web/` into `data/www/`.
The generated directory is ignored by git; only this file is tracked.

Optional runtime assets (fonts, default configs) can live here. Do not commit
audio files or secrets.
