# Contributing

Thanks for your interest in Llavero Amigo! Issues and pull requests are welcome.

## Ground rules

- **Never commit secrets.** `src/secrets.h` and `platformio_local.ini` are git-ignored; keep it that way. Use the `*.example` files for templates and double-check `git diff` before pushing.
- Keep it building for the plain `esp32c3` environment (`pio run -e esp32c3`) before opening a PR.
- Keep the code simple and readable; match the style of the surrounding code.
- If you change wiring or pins, update `src/config.h`, `docs/WIRING.md` and the diagram together.

## Building

```bash
pip install platformio     # or use the PlatformIO VS Code extension
pio run -e esp32c3         # build
pio run -e esp32c3 -t upload   # flash over USB
```

## Ideas that would be great to have

- Real phone notifications (BLE: ANCS on iOS, a small companion app on Android) feeding the existing `/api/notify` path.
- Charger / power-path support documented for common ESP32-C3 boards.
- More characters and expressions (see "Adding a character" in the README).
- Translations of the web UI and on-screen text (currently Spanish).
- Display brightness control and power saving (backlight PWM, CPU frequency, screen-off timeout).
- Support for other ESP32 variants and displays.

## Adding a character

Characters are data plus a few drawing routines — see the README section *Adding a character*. A screenshot of the new face from `GET /api/frame?c=<id>` in the PR description is very helpful.

## Reporting bugs

Please include: board model, display module, firmware commit, what you expected, what happened, and the serial log if there is one.
