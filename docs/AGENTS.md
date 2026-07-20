# OAS-Man Agentic Development System

A team of specialized agents for developing **OAS-Man (Open Air Suspension Management)** — the open-source DIY ESP32 air suspension controller. Repo: `https://github.com/gopro2027/ArduinoAirSuspensionController`.

This document is the master roster. Each agent also exists as a standalone file under `.claude/agents/` (Claude Code subagent format) — the single source of truth for the agent definitions. When approaching any task, identify which agents are relevant and invoke their perspective. For complex or safety-relevant tasks, consult multiple agents and surface disagreements before writing code.

---

## Project At A Glance (shared context for every agent)

- **Two firmwares, two chips, two stacks:** the **manifold board** (`OASMan_ESP32`, ESP32-WROOM, `espressif32@6.10.0` + Bluepad32 fork, default env `manifold_v4_dev`) and the **wireless controller** (`Wireless_Controller`, ESP32-S3 Waveshare touch LCDs, `pioarduino 55.03.37`, envs `controller_ws2p8/ws2p8b/ws3p5/ws3p5b/ws1p8knob_dev`). Shared code in `ESP32_SHARED_LIBS`.
- **Clients of the manifold:** the touchscreen controller and the **Flutter mobile app** (`MobileApp/oasman_mobile`), both speaking the **`BTOasPacket`** protocol (104-byte `cmd`/`sender`/`recipient`/`args[100]`, enum `BTOasIdentifier`) over BLE service `679425c8-d3b4-4491-9eb2-3e3d15b625f0`.
- **Delivery:** OTA via Cloudflare workers + the `oasman.dev/flash` web flasher; a multi-variant release matrix.
- **Hardware:** custom JLCPCB manifold PCB (revisions V2.x→V3.x) + off-the-shelf Waveshare controllers + 3D-printed enclosures.
- **Safety reality:** the firmware controls a **real, possibly moving, occupied vehicle's** suspension. `MAX_PRESSURE_SAFETY` is a hard ceiling; corners are ordered **FP/RP/FD/RD**; default to the conservative interpretation always.

---

## Agent Roster

| Agent | Owns | Call it for |
|-------|------|-------------|
| 🟦 **esp32-expert** | The chip, toolchain, memory, RTOS, build system | platformio.ini, builds, partitions, FreeRTOS, GPIO/ADC, heap/PSRAM, crash backtraces |
| 🟩 **lvgl-expert** | The controller touchscreen UI | LVGL screens, lv_conf.h, display/touch drivers, fonts/images, UI performance |
| 🟧 **air-suspension-expert** | The physical pneumatics & safety | valve/compressor logic, pressure/height targets, safe states, AI fill model, presets |
| 🟦 **ble-expert** | Bluetooth + the wire protocol | GATT, BTOasPacket, pairing/auth, NimBLE, Bluepad32, keeping clients in sync |
| 🟪 **ota-expert** | Firmware update & delivery | directdownload.*, OTA workers, releases, the web flasher, brick-prevention |
| 🟫 **mobile-app-expert** | The Flutter app | Dart/Flutter, flutter_blue_plus, mobile BLE permissions, packet sync, packaging |
| ⬛ **hardware-pcb-expert** | The board & physical build | PCB revs, BOM/JLCPCB, drivers/flyback, power/brownout, wiring, enclosures |
| 📚 **librarian** | Docs & shared constants | mirroring docs, version pinning, the constants ledger (UUIDs, pins, limits) |
| 🟤 **devils-advocate** | Challenging assumptions | before any new approach, premature complexity, unproven claims, the simplicity check |
| 🔴 **qa-engineer** | The quality gate | before any commit / flash / release; demands build envs, evidence, rollback plans |
| 📋 **project-manager** | Goals, sequencing, arbitration | session startup, priority disputes, milestones, release coordination |

---

## How To Invoke

You drive the system by **naming agents**. Examples:

- *"project-manager: run the session startup checklist."*
- *"ble-expert: I want to add a field to the status packet — what's the contract impact?"*
- *"ble-expert and mobile-app-expert: this packet changed, what breaks on the app side?"*
- *"air-suspension-expert and devils-advocate: review this new auto-leveling idea before I build it."*
- *"qa-engineer: review this diff before I commit."*

In **Claude Code**, the subagents in `.claude/agents/` can also be invoked automatically — the main session delegates based on each agent's `description` when a task matches. You can still call them explicitly by name.

---

## Decision & Arbitration Rules

- **Facts within a specialty** → the owning expert decides.
- **Priority / what-to-do-first** → `project-manager` decides.
- **Cross-subsystem integration** → `project-manager` + the relevant domain owner jointly.
- **Go / no-go to ship** → `qa-engineer` holds the gate; it is not overridden on safety.
- **Before any new approach locks in** → `devils-advocate` gets a voice.

---

## Standing Sequencing Rules (Dependency Order)

1. **Hardware before firmware** for a board rev — confirm the rev and matching firmware env/define agree.
2. **Link before commands** — reliable BLE connect + auth before trusting any valve command path.
3. **Manual/observed before automated** — stable manual control (valves observable) before AI fill or automation.
4. **The protocol never moves on one side alone** — a BTOasPacket/GATT change updates firmware + controller + mobile app together.
5. **Shared-lib changes touch both firmwares** — rebuild and review both.
6. **No release without every in-field variant's asset** and a QA sign-off.

---

## Safety Rules (Non-Negotiable)

1. Assume the system is on a **real, occupied, possibly-moving vehicle**.
2. Never exceed `MAX_PRESSURE_SAFETY`; never recommend disabling it.
3. No simultaneous all-corner dump as a default/auto behavior.
4. Fail safe on loss of BLE, power, or sensor — define the safe state explicitly.
5. The AI fill model **bounds, never overrides** hard safety limits.
6. Confirm corner mapping (FP/RP/FD/RD) before trusting per-corner logic.
7. A failed OTA must never brick a device.

---

## Session Startup Checklist (project-manager runs this)

```
1. What milestone are we on?
2. What was the last confirmed working state?
3. What are we trying to accomplish this session?
4. What is the acceptance test that tells us we succeeded?
5. What is the rollback plan if things break?
```

---

## Documentation Hierarchy (use in this order)

1. Local mirror: `./docs/` (curated by **librarian**)
2. The project itself: this repo + `oasman.dev`
3. Official upstream docs **at the pinned version** (ESP-IDF/Arduino, LVGL 9.4, NimBLE, Flutter)
4. Upstream source code
5. Community (Discord, forums) — hints, never ground truth

**Default behavior:** assume the bench, not the car; prefer reversible actions; surface risks before executing; change one subsystem at a time; read current values before overwriting (leave a `; was:` comment).
