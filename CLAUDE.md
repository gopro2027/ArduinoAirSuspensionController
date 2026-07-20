# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

**OAS-Man (Open Air Suspension Management)** — an open-source DIY ESP32 air suspension controller, a budget (<$500) alternative to AirLift/AccuAir-class systems. The firmware **controls a real, possibly moving, occupied vehicle's suspension** and is **updated OTA in the field**. Treat changes accordingly: conservative by default, safety first, reversible steps.

Branches: **`main`** = current stable ESP32 firmware · **`dev`** = bleeding edge (active development) · **`nano-v1`** = legacy Arduino Nano (unmaintained).

## Architecture (the big picture)

Three programs, one wire protocol:

- **Manifold (`OASMan_ESP32/`)** is the **BLE peripheral/server** bolted to the air manifold. It owns all physical actuation and safety enforcement — 8 solenoids (IN+OUT per corner), 4 pressure sensors via ADS1115, compressor, 128×64 SSD1306 OLED. ESP32-WROOM, `espressif32@6.10.0` + a **forked Bluepad32 framework** (PS3/PS4/Xbox gamepad input).
- **Wireless Controller (`Wireless_Controller/`)** and **Mobile App (`MobileApp/oasman_mobile/`)** are **BLE clients** of the manifold. The controller is an ESP32-S3 Waveshare touch LCD with an LVGL 9.x UI (`pioarduino 55.03.37`, NimBLE); the app is Flutter (`flutter_blue_plus`), feature-behind the controller and Android-only.
- **The contract between them** is `BTOasPacket` (a fixed 104-byte command struct) exchanged over three GATT characteristics (STATUS / REST / VALVECONTROL). It is defined once in `ESP32_SHARED_LIBS/src/BTOas.h` and mirrored byte-for-byte in the Dart app. Changing it touches all three programs at once (see Hard rules).
- **`ESP32_SHARED_LIBS/`** is compiled into *both* firmwares via `lib_extra_dirs = ../ESP32_SHARED_LIBS/`. It holds `user_defines.h` (config, pin maps, safety limits — board-version-conditional), the `BTOas.*` protocol, and the OTA client + Cloudflare worker sources. A change here rebuilds both firmwares.
- **OTA** (`directdownload.*` + workers in `ESP32_SHARED_LIBS/src/`): the device GETs `oasman-ota.gopro2027.workers.dev` with its `FIRMWARE_RELEASE_NAME` + installed tag; the worker compares against the latest GitHub release and returns `204` (up-to-date) or `200` + firmware bytes streamed through `Update.writeStream()`. Flow: `ESP32_SHARED_LIBS/src/oasman-ota_flowchart.md`.

Non-code dirs: `PCB/` (EasyEDA project + JLCPCB order packages, revs V2.x→V3.x), `3d Prints/` (enclosures), `tutorial/` + [oasman.dev](https://oasman.dev) (build docs), `LCDImageCreatorTool/` (manifold OLED image assets), `PS3_Controller_Tool/` (PS3 MAC pairing), `builds/` (committed app APKs).

## Build & run

PlatformIO (`pio` CLI or the VS Code extension). **Always name the env** — manifold and controller are different chips on different platform stacks, so a bare "it builds" is not evidence. Run `pio` from inside the relevant project dir (`OASMan_ESP32/` or `Wireless_Controller/`), each of which has its own `platformio.ini`.

```bash
# Manifold (ESP32-WROOM).   Dev envs: manifold_v2_dev / v3_dev / v4_dev
pio run -e manifold_v4_dev                 # build
pio run -e manifold_v4_dev -t upload       # build + flash
pio device monitor                         # serial @ 115200 (esp32_exception_decoder is on)

# Controller (ESP32-S3).    Dev envs: controller_{ws2p8,ws2p8b,ws3p5,ws3p5b,ws1p8knob}_dev
pio run -e controller_ws2p8_dev
pio run -e controller_ws2p8_dev -t upload
```

Always decode a full backtrace before guessing at a crash.

**Mobile app** (from `MobileApp/oasman_mobile/`):

```bash
flutter pub get
flutter run            # needs a real Android device — BLE doesn't work on emulators
flutter analyze        # lint (analysis_options.yaml)
flutter build apk      # release APK
```

## Releases

Official firmware is cut by the GitHub Actions workflow `.github/workflows/main.yml` — manual `workflow_dispatch` taking a `version_num` input. It builds every shipping variant's `*_release` env and publishes the `.bin`s as a GitHub release.

- The release **tag is `build-<run_number>`** and is **load-bearing**: the OTA worker compares the device's installed tag against it to decide "already up to date." Do not repurpose the tag scheme.
- `*_release` envs require the `version_num` and `release_tag_name` environment variables and set `OFFICIAL_RELEASE` (disables debug/mock paths; `ENABLE_AIR_OUT_ON_SHUTOFF` is always false in official releases). Reproduce a release build locally with, e.g.:
  ```bash
  version_num=test release_tag_name=build-local pio run -e manifold_v4_release
  ```
- Every in-field variant needs its asset in every release (manifold v2/v3/v4 + controller ws2p8/ws2p8b/ws3p5/ws3p5b/ws1p8knob). A missing asset strands those users on a fail-file-request.

## Tests & verification

There is **no firmware unit-test suite** — the `test/` dirs in both PlatformIO projects are empty placeholders. Firmware is verified on-device/bench: build the named env, flash, watch the serial log, observe the valves. The Flutter app has `flutter analyze` and scaffold tests under `test/` (`flutter test`) but no meaningful coverage. Treat a clean build of the affected env(s) + observed on-device behavior as the bar — not a green test run.

## The agent team

This repo uses a team of specialized agents defined in `.claude/agents/` (subagents) with a full roster in `docs/AGENTS.md`. Delegate to them by name or let Claude Code route automatically:

- **esp32-expert** · **lvgl-expert** · **air-suspension-expert** · **ble-expert** · **ota-expert** · **mobile-app-expert** · **hardware-pcb-expert** (domain owners)
- **librarian** (docs/constants) · **devils-advocate** (challenge) · **qa-engineer** (the gate) · **project-manager** (sequencing/priority)

Start non-trivial sessions with **project-manager** (it runs a 5-question startup checklist). Route the final go/no-go to **qa-engineer**. Get **devils-advocate** before locking in any new approach.

## Hard rules (do not violate)

1. **Name the build env** in any build/flash instruction — manifold and controller are different chips on different platform stacks; "it builds" without an env is not evidence.
2. **The BLE protocol never moves on one side alone.** A change to `BTOas.h` packets or the GATT contract must update firmware + controller + the Flutter app together. It is a breaking change by default.
3. **`ESP32_SHARED_LIBS` changes rebuild both firmwares.**
4. **Safety limits are sacred:** never exceed or disable `MAX_PRESSURE_SAFETY`; no all-corner-dump default; define the safe state on BLE/power/sensor loss; the AI fill model bounds but never overrides hard limits; preserve corner order FP/RP/FD/RD.
5. **A failed OTA must never brick a device** — every fail path stays bootable; bump `BINARY_CACHE_VERSION` on any OTA response-format change and test on real ESP32 `HTTPClient`.
6. **Board rev ↔ firmware define must match** (e.g. `BOARD_VERSION_4_VALVE_PINOUT`) or every valve is mis-wired.
7. **LVGL is single-threaded here** (`LV_OS_NONE`) — never call `lv_*` from a BLE/notification callback; hand data to the UI task.
8. **Read before write** — preserve prior values as a `; was:` comment.

## Load-bearing constants

These are quoted exactly from code; never paraphrase. (Verify against source via **librarian** if in doubt.)

- BLE service UUID: `679425c8-d3b4-4491-9eb2-3e3d15b625f0`
- Characteristics: STATUS `66fda100-8972-4ec7-971c-3fd30b3072ac`, REST `f573f13f-b38e-415e-b8f0-59a6a19a4e02`, VALVECONTROL `e225a15a-e816-4e9d-99b7-c384f91f273b`
- `BTOasPacket` = 104 bytes (`uint16_t cmd`, `uint8_t sender`, `uint8_t recipient`, `uint8_t args[100]`); MTU 517; auth within 5 s of connect
- Corner order: **FP, RP, FD, RD** · `MAX_PRESSURE_SAFETY 200`
- Pairing uses the configured 6-digit `BLE_PASSKEY` in `user_defines.h` (builders should change the default)
