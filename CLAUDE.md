# CLAUDE.md — OAS-Man

Guidance for Claude Code when working in this repository.

## What this project is

**OAS-Man (Open Air Suspension Management)** — an open-source DIY ESP32 air suspension controller, a budget alternative to AirLift/AccuAir-class systems. The firmware **controls a real, possibly moving, occupied vehicle's suspension** and is **updated OTA in the field**. Treat changes accordingly: conservative by default, safety first, reversible steps.

## Repository map

- `OASMan_ESP32/` — **manifold board** firmware. ESP32-WROOM, `espressif32@6.10.0` + Bluepad32 fork. Default env `manifold_v4_dev`. Drives 8 solenoids, 4 pressure sensors (ADS1115), compressor, 128×64 SSD1306 OLED.
- `Wireless_Controller/` — **touchscreen controller** firmware. ESP32-S3 Waveshare LCDs, `pioarduino 55.03.37`, LVGL 9.4, NimBLE. Envs: `controller_ws2p8/ws2p8b/ws3p5/ws3p5b/ws1p8knob_dev`.
- `ESP32_SHARED_LIBS/` — code shared by both firmwares: `user_defines.h` (config), `BTOas.*` (the BLE wire protocol), OTA (`directdownload.*`, workers).
- `MobileApp/oasman_mobile/` — Flutter mobile app (BLE client, `flutter_blue_plus`).
- `PCB/` — EasyEDA project, Gerbers, JLCPCB order packages (revisions V2.x→V3.x).
- `3d Prints/` — enclosures. `tutorial/`, `docs/` — build instructions. `LCDImageCreatorTool/` — controller image assets.

## Build commands (always name the env)

```bash
# Manifold
pio run -e manifold_v4_dev
pio run -e manifold_v4_dev -t upload

# Controller (pick the board)
pio run -e controller_ws2p8_dev
pio run -e controller_ws2p8_dev -t upload
```

Monitors run at 115200 with `esp32_exception_decoder` — always decode a full backtrace before guessing at a crash.

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
