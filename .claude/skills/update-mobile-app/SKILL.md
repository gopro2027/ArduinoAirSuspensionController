---
name: update-mobile-app
description: Update the Flutter mobile app (MobileApp/oasman_mobile) so it stays in feature and BLE parity with the LVGL Wireless_Controller client. Use when the user asks to sync, update, port, or add Wireless_Controller / firmware / protocol features into the Android Flutter app, or to close the app's feature gap vs the touchscreen controller.
disable-model-invocation: true
---

# Update Flutter App from Wireless_Controller

The Flutter app (`MobileApp/oasman_mobile`) is a **BLE client of the manifold**, peer to the touchscreen Wireless_Controller. It should expose the same BLE capabilities and user-facing features as the controller, with **mobile-appropriate UI** (not a pixel-perfect LVGL clone). This skill ports new controller/protocol features into that app.

Sibling skills (website repo `gopro2027.github.io`): `update-web-controller` (live Web Bluetooth `/controller`), `update-controller-demo` (non-functional LVGL UI mock). Prefer this skill for Flutter work.

## Source of truth and target

| Role | Path |
|------|------|
| Protocol contract (structs, enums, packet types) | `ESP32_SHARED_LIBS/src/BTOas.h` (+ `BTOas.cpp`) |
| Constants (passkey, names, wheel/solenoid indices, safety) | `ESP32_SHARED_LIBS/src/user_defines.h` |
| Human-readable API reference | `OASMan_ESP32/BLE_API_DOCUMENTATION.md` |
| Feature / UX reference (what the app should support) | `Wireless_Controller/src/ui/screens/` (`ui_scrHome.cpp`, `ui_scrPresets.cpp`, `ui_scrSettings.cpp`) |
| Firmware client reference (scan, auth, valve writes, REST) | `Wireless_Controller/src/bt/ble.cpp`, `src/utils/util.cpp` |
| Flutter app (edit these) | `MobileApp/oasman_mobile/` |

Always read the current Wireless_Controller + `BTOas.h` sources for the area being changed; do not rely on memory. A protocol change that touches packets/GATT is a **breaking change** — update Dart in the same change set as firmware when possible (Hard rule: protocol never moves on one side alone).

## Flutter file map

| File | Responsibility |
|------|----------------|
| `lib/ble_manager.dart` | Service/characteristic UUIDs, scan/connect/auth/MTU, REST/STATUS/VALVE writes, 104-byte packet builders/parsers, config flags, command helpers. **Wire protocol lives here.** |
| `lib/main.dart` | App shell, Provider wiring, startup auto-connect via the **same** `BLEManager` instance (never construct a second one). |
| `lib/pages/buttons.dart` | Home / Controller tab: valve hold controls, presets Save/Load, live pressures. |
| `lib/pages/setup.dart` | Settings tab: passkey, config toggles/fields, action commands (compressor, aux, OTA, reboot, etc.). |
| `lib/pages/popup/bluetooth.dart` | Device list, refresh, disconnect / switch device. |
| `lib/pages/header.dart` | BLE status icon / open device popup. |
| `lib/pages/popup/nobt.dart` | Disconnected / no-BT UX. |
| `lib/provider/settings_provider.dart`, `lib/models/appSettings.dart` | Local prefs (passkey, `pairedManifoldId`, units). |
| `lib/provider/unit_provider.dart` | PSI/Bar display. |

Legacy / unused (do not extend; remove or leave unmarked): `lib/bluetooth.dart`, `lib/pages/setbar.dart`.

## Workflow

```
- [ ] 1. Find what changed in Wireless_Controller / protocol
- [ ] 2. Map each change to the Flutter file (table below)
- [ ] 3. Apply edits to ble_manager.dart first (if wire format), then UI pages
- [ ] 4. Verify with flutter analyze (and device run when possible)
```

### Step 1: Find what changed

Ask the user what changed if unclear. Otherwise inspect:

```bash
# from repo root
git log --oneline -20 -- Wireless_Controller/src/ui Wireless_Controller/src/bt ESP32_SHARED_LIBS/src/BTOas.h ESP32_SHARED_LIBS/src/BTOas.cpp
git diff <since>..HEAD -- Wireless_Controller/src ESP32_SHARED_LIBS/src/BTOas.h ESP32_SHARED_LIBS/src/BTOas.cpp
```

Do **not** trust `git log` alone. Inventory the **current** controller UI vs the app:

```bash
# every settings control (labels + types)
rg -n "new Option\\(|new RadioOption\\(" Wireless_Controller/src/ui/screens/ui_scrSettings.cpp
# REST commands the controller actually sends
rg -n "sendRestPacket|BTOasIdentifier::" Wireless_Controller/src/bt/ble.cpp Wireless_Controller/src/ui
```

For each `Option` / `RadioOption` label, find the matching control in `setup.dart` / `buttons.dart` (or mark out-of-scope per the skip list below). **Parsed-in-Dart but no UI is still a gap.** **Commented-out Flutter controls that the controller still shows are still a gap.**

Decide which bucket each change falls into:

1. **Wire format** — new/changed `BTOasIdentifier`, args layout, STATUS/GETCONFIGVALUES fields, valve bitmask, GATT behavior → must update `ble_manager.dart` (and usually Settings/Home UI).
2. **Controller UX only** — new setting row, preset confirm, label, gated control that already has a command on the wire → update Flutter UI to match behavior/naming; no protocol edit if bytes already exist.
3. **Firmware-internal only** — manifold/AI/safety internals with no client-visible command or config field → Flutter usually needs no edit.

### Step 2: Wireless_Controller / protocol → Flutter mapping

| Controller / protocol change | Flutter edit |
|------------------------------|--------------|
| New `BTOasIdentifier` command | Add cmd id + send helper in `ble_manager.dart`; wire a control in `setup.dart` or `buttons.dart`. |
| New / changed GETCONFIGVALUES field or `ConfigFlagsBit` | Extend parse + `saveConfigToManifold` / `_buildConfigFlagsBits` byte offsets in `ble_manager.dart`; add/update row in `setup.dart`. |
| New STATUSREPORT bit/field | Parse **every `StatusPacketBittset` the controller UI uses** (including `ADJUSTMENT_IN_PROGRESS`); surface on Home header and/or Settings Status. |
| New settings section/option in `ui_scrSettings.cpp` | Mirror in `setup.dart` with mobile widgets (Switch/TextField/Dropdown/Button) — match **behavior and label**, not LVGL layout. |
| Home valve / axle pill behavior (`ui_scrHome.cpp`) | Update hold/bitmask logic in `buttons.dart`; valve write stays 4-byte LE u32 via `writeValveValue`. |
| Preset Save/Load / confirmations (`ui_scrPresets.cpp`) | Update `buttons.dart` (AIRUPQUICK load, SAVECURRENTPRESSURESTOPROFILE save, preset-1 air-out confirm). |
| Scan / connect / auth / init sequence (`ble.cpp`) | Update `ble_manager.dart` + `main.dart` auto-connect + `bluetooth.dart` popup. |
| Broadcast name / passkey / pairing | Settings BLE section + auth packet path in `ble_manager.dart`. |
| Feature removed from controller / not on wire | **Hide or remove** from `setup.dart` / `buttons.dart` — do not leave dead no-op controls. |

### Parity rules (always apply)

- **Service-filtered scan:** OASMan service UUID `679425c8-d3b4-4491-9eb2-3e3d15b625f0`. Prefer `FlutterBluePlus.startScan(withServices: [...])`; if needed, fallback-filter `advertisementData.serviceUuids`.
- **Remember + auto-connect:** Persist `pairedManifoldId` (`SharedPreferences` key `_pairedManifoldId`). On launch, use the Provider's single `BLEManager` — never a second instance. Connect path must `discoverServices` before REST/status/valve use.
- **Valve writes:** 4-byte little-endian `uint32` bitmask to VALVECONTROL (not a 104-byte `BTOasPacket`), matching Wireless_Controller.
- **Disconnected UX:** When `!isConnected()`, disable/hide manifold controls on Controller; Settings shows only BLE/connect affordances. Header BLE icon still opens the device popup.
- **Naming:** Prefer controller wording — tab "Controller" / "Settings", preset "Save" / "Load".
- **Safety:** Never bypass `MAX_PRESSURE_SAFETY` or add an all-corner-dump default. Keep preset-1 (typical air-out) confirmation. Do not invent commands the manifold does not implement.
- **Visibility rules:** If the controller hides a control in height-sensor mode (or the reverse), hide it in the app the same way (bag-stretch fields, Height levelling, height-calibrate buttons).
- **Labels:** Copy Wireless_Controller option strings (e.g. `"Height levelling"`, `"Sample Learn Progress:"`, `"Bag Stretch Below PSI"`).
- **UI scope:** Mobile widgets are fine; do not clone LVGL pixels or controller-only hardware (rotary encoder, brightness/theme/screen rotation, swipe nav, show-battery, USB custom-car upload). **Do port** the Wifi/Update **manifold OTA** flow (`STARTWEB` SSID/password + update-status). A typed SSID field is acceptable on the phone; a scan dropdown is optional and must not add a new plugin unless asked.

### Protocol invariants (do not break)

- `BTOasPacket` is **104 bytes**, little-endian: `cmd` u16 @0, `sender` @2, `recipient` @3, `args[100]` @4.
- Typed views: `args8[n]` → offset `4+n`, `args16[n]` → `4+2n`, `args32[n]` → `4+4n`. Match `BTOas.h` / Wireless_Controller packing exactly.
- Characteristics: STATUS `66fda100-8972-4ec7-971c-3fd30b3072ac`, REST `f573f13f-b38e-415e-b8f0-59a6a19a4e02`, VALVECONTROL `e225a15a-e816-4e9d-99b7-c384f91f273b`.
- Auth within ~5s of connect; request MTU 517 when negotiating.
- Corner order **FP, RP, FD, RD**. Profiles on the wire are **0-based**; UI shows **1–5**.
- Config save: only write GETCONFIGVALUES with `setValues` when manifold config has been loaded; do not blast defaults on connect.

### Step 3: Apply edits

1. If wire format changed → update `ble_manager.dart` builders/parsers/cmd enums first.
2. Then wire UI in `buttons.dart` / `setup.dart` / popups.
3. Keep labels aligned with Wireless_Controller where users will recognize them.
4. Gate new controls on `isConnected()` (and any feature flags the controller uses, e.g. height-sensor-only actions).
5. After protocol parse/save, confirm there is a Settings/Home control — echoing a byte in `ble_manager.dart` without UI is incomplete.
6. Do **not** change Wireless_Controller, manifold firmware, or `BTOas.h` from this skill unless the user explicitly asked for a protocol change (and then update all clients together).

### Step 4: Verify

```bash
# from MobileApp/oasman_mobile
flutter analyze
# optional: flutter test
# on-device (BLE does not work on emulators):
flutter run
```

`flutter analyze` must be clean for edited files. Full BLE behavior only counts on a real Android device against a manifold.

## Notes

- App is **Android-shipping**; keep iOS from regressing if touched, but do not block on iOS BLE bring-up unless asked.
- Cross-check byte offsets against Wireless_Controller `ble.cpp` / `util.cpp` and, if present, the website `protocol.ts` — all three clients must agree with `BTOas.h`.
- After porting a protocol feature here, the website `update-web-controller` skill may need a matching pass (separate repo).
