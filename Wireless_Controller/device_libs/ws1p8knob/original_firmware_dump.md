# `original_firmware_dump.bin` — stock firmware snapshot & provenance

Full flash snapshot of the 1.8" knob controller as it shipped, taken before reflashing it
for OAS-Man. Captured and analysed 2026-09-06.

| | |
|---|---|
| **File** | `original_firmware_dump.bin` |
| **Size** | 16,777,216 bytes (16 MB — the entire flash) |
| **SHA-256** | `142cd6d775ffc3d9c1fc05a9a1498ebb252933f50a5511775c54635055319bcc` |
| **MD5** | `4c580af006ad0cdf5f7f2a0160e8b682` |
| **SHA-256 of first 4 MB** | `cfe6cc576124dff0e5a6c5332918fcd51e8fecff176839e561c6b48e291fbc2c` |

The last row is the hash of the earlier partial 4 MB dump, which this file supersedes. The
prefix was verified byte-identical before the old file was replaced, so nothing was lost.

## Chip this came off

```
Chip type:          ESP32-S3 (QFN56) revision v0.2
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240 MHz, Embedded PSRAM 8 MB (AP_3v3)
Crystal:            40 MHz
USB mode:           USB-Serial/JTAG (native USB, no UART bridge chip)
MAC:                28:84:85:4b:d9:4c
Flash:              16 MB, DIO, 80 MHz  (from image header byte 3 = 0x4F)
```

The MAC matches the `cal_mac` blob stored in this dump's NVS, confirming image and chip
are the same unit.

**Flash encryption and secure boot are off** — the app partition is plaintext, so this dump
is directly readable and directly restorable.

---

## 1. Taking the snapshot

esptool here is **v5.1.2**, which renamed subcommands to hyphenated form (`read-flash`,
`write-flash`). The underscore spellings still work but are deprecated. Note also that inside
`pkg exec` only the `esptool.py` entry point exists — plain `esptool` is not on PATH, despite
the deprecation warning telling you to use it.

```bash
C:\Users\user\.platformio\penv\Scripts\platformio.exe pkg exec -p "tool-esptoolpy" -- esptool.py --chip esp32s3 --port COM23 --baud 921600 read-flash 0x0 0x1000000 original_firmware_dump.bin
```

`0x1000000` is the full 16 MB. Reading the whole chip took **121 s at 921600** (~1.1 Mbit/s).
Baud barely matters on this board — it enumerates as native USB-Serial/JTAG, so the figure is
USB-bound, not UART-bound.

Reading is non-destructive; it does not disturb the running firmware.

## 2. What is actually on the flash

Only **1.98 MB of the 16 MB is written**. Everything else is erased (`0xFF`):

| Region | Size | Contents |
|---|---|---|
| `0x000000`–`0x005000` | 20 K | bootloader (ESP32-S3, magic `0xE9`) |
| `0x005000`–`0x008000` | 12 K | blank |
| `0x008000`–`0x00B000` | 12 K | partition table + used NVS pages |
| `0x00B000`–`0x00E000` | 12 K | blank (unused NVS) |
| `0x00E000`–`0x00F000` | 4 K | `otadata` (one page) |
| `0x00F000`–`0x010000` | 4 K | blank |
| `0x010000`–`0x201000` | 1.94 M | **`app0` — the firmware image** |
| `0x201000`–`0x1000000` | 14.0 M | blank |

Declared partition table:

| Offset | Partition | Size | State |
|---|---|---|---|
| `0x009000` | `nvs` | 20 K | in use |
| `0x00E000` | `otadata` | 8 K | `ota_seq = 1` → boots `ota_0` |
| `0x010000` | `app0` (`ota_0`) | 3 M | 1.94 M image |
| `0x310000` | `app1` (`ota_1`) | 3 M | **erased — no OTA ever taken** |
| `0x610000` | `spiffs` | 9.8 M | **erased — never used** |
| `0xFF0000` | `coredump` | 64 K | erased — never crashed |

> ### The `spiffs` partition is empty — assets live on the TF card
>
> This firmware keeps nothing in SPIFFS. Fonts, images, videos and music all load from the
> microSD card (`/fonts/%s_%u.bin`, `[pic]`, `[mjpeg]`, `[music]`, `[aida64]`, `[txt]`).
> The vendor's own release notes say firmware and TF-card assets must be version-matched.
>
> **This flash dump therefore does not constitute a complete device backup.** Restoring it
> gives you a working firmware that will boot to "No Font File" / "Please insert TF card"
> unless the original TF-card contents are also preserved. **Back up the microSD card
> separately** — copy it off wholesale before reusing it.

## 3. Restoring the snapshot

**Tested end-to-end on 2026-09-06** — restored over an OAS-Man install and verified bit-exact
against this snapshot across all 16 MB.

> **Do not write the 16 MB file directly.** That was tried first and it **fails**: esptool
> gets partway through and dies with `A fatal error occurred: The chip stopped responding.`,
> leaving the flash partially written. The device is recoverable (the ROM USB-Serial/JTAG
> loader works regardless of flash contents), but the write does not complete.
>
> Write only the **used region** instead. `erase-flash` already produces `0xFF` everywhere,
> which is exactly what the remaining 14 MB contains, so the result is identical.

### Step 1 — trim the image to the used region

`0x201000` (2,101,248 bytes) is where the written data ends; see §2.

```bash
head -c $((0x201000)) original_firmware_dump.bin > restore_2mb.bin
```

PowerShell equivalent, if you are not in Git Bash:

```bash
$b=[IO.File]::ReadAllBytes("original_firmware_dump.bin"); [IO.File]::WriteAllBytes("restore_2mb.bin",$b[0..2101247])
```

Expected SHA-256 of `restore_2mb.bin`:
`eee95b2a2ec8a4906dd92473693f187619218032c8bde622a891dd1ff7a7212b`

### Step 2 — erase

```bash
C:\Users\user\.platformio\penv\Scripts\platformio.exe pkg exec -p "tool-esptoolpy" -- esptool.py --chip esp32s3 --port COM23 --baud 921600 erase-flash
```

### Step 3 — write

```bash
C:\Users\user\.platformio\penv\Scripts\platformio.exe pkg exec -p "tool-esptoolpy" -- esptool.py --chip esp32s3 --port COM23 --baud 921600 write-flash --flash-size 16MB --flash-mode dio --flash-freq 80m 0x0 restore_2mb.bin
```

Takes ~15 s (2,101,248 bytes → 1,300,280 compressed, ~1.1 Mbit/s) and ends with
`Wrote ... Hash of data verified.`

### Step 4 — verify

```bash
C:\Users\user\.platformio\penv\Scripts\platformio.exe pkg exec -p "tool-esptoolpy" -- esptool.py --chip esp32s3 --port COM23 --baud 921600 verify-flash 0x0 restore_2mb.bin
```

Expect `Verification successful (digest matched).` For a full-chip check, re-read all 16 MB
with the §1 command and confirm the SHA-256 equals
`142cd6d775ffc3d9c1fc05a9a1498ebb252933f50a5511775c54635055319bcc` — that is what was
confirmed on the tested run.

### Notes

- The image starts at `0x0` because the ESP32-S3 bootloader lives at offset 0 (unlike the
  ESP32-WROOM manifold, where it sits at `0x1000`). Do not use `0x1000` here.
- `erase-flash` first is what reproduces the *exact* captured state, including the erased
  `spiffs` / `app1` / `coredump` regions. Without it, a prior OAS-Man install could leave
  stale bytes in regions the trimmed image does not cover.
- esptool prints `SHA digest in image updated.` on write. It recomputes the bootloader's
  appended SHA-256 — for this image it recomputes the *same* value already present, which is
  why `verify-flash` still matches the file exactly. Not a cause for concern.
- **This restores NVS too**, including the stock Wi-Fi AP config (`My-Ap` / `12345678`) and
  the RF calibration blobs. Writing it to a *different* ESP32-S3 also writes this unit's
  `cal_mac` into that chip's NVS — harmless, since the real MAC lives in eFuse and calibration
  is re-derived, but it is not a clean provisioning path for a second device.
- **eFuses are not captured by a flash dump** and are not restorable from this file. Nothing
  on this unit appears to depend on them.
- Restoring flash does **not** restore the TF card. If the card has been reused since, the
  stock firmware will boot but complain about missing fonts/assets — see the §2 callout.

### Step 5 — confirm it boots

Bit-exact flash is not proof of a working device. Capture the boot log over the same USB
serial port (115200) after a reset. A healthy restore looks like this:

```
rst:0x15 (USB_UART_CHIP_RESET),boot:0x2b (SPI_FAST_FLASH_BOOT)
[I] [MAIN] System start...
[I] [OTA] CURRENT FIRMWARE: VALID
[I] [MAIN] Used psram: 7870208
[I] [FS] TF Card : SDSC 480MB
[I] [MAIN] Boot finished.
[I] [APP] Setup UI...
[I] [IO] Play: /night7/boot.mjpeg
[I] [WIFI] WebServer started!
```

`CURRENT FIRMWARE: VALID` is the OTA state machine accepting the image. `Play: /night7/boot.mjpeg`
means it found its assets and is running the boot animation — i.e. the TF card is intact as well
as the flash.

### Confirming which firmware is on the device

Cheap check without a full read — the `esp_app_desc` at `0x10020` carries version, build time
and IDF version:

```bash
C:\Users\user\.platformio\penv\Scripts\platformio.exe pkg exec -p "tool-esptoolpy" -- esptool.py --chip esp32s3 --port COM23 --baud 921600 read-flash 0x10020 0x100 appdesc.bin
```

| Firmware | version field | IDF | build date |
|---|---|---|---|
| Stock TAIJI | `769c168` | `v5.1.4-972-g632e0c2a9f-dirty` | Jan 26 2026 |
| OAS-Man controller | `8cabf2c` | `v5.5.2-729-g87912cd291` | Feb 11 2026 |

### Reflashing OAS-Man afterwards

Nothing special is required to go back — the OAS-Man build overwrites the same offsets:

```bash
pio run -e controller_ws1p8knob_dev -t upload
```

Run that from `Wireless_Controller/`. See `CLAUDE.md` for the full env list.

---

## 4. Device identity

- **Model:** `TAIJI KNOB 32` (the settings screen renders this; "KBOB" is a misread of KNOB)
- **Version:** `H05F1.3.0`, from the format string `H%02XF%d.%d.%d`
  → hardware revision `0x05`, firmware `1.3.0`
- **NVS namespace:** `GODVISION` — carried over from the author's earlier 神之眼 ("Vision") product
- **SoftAP defaults in NVS:** ssid `My-Ap`, passphrase `12345678`

## 5. Build fingerprint

| | |
|---|---|
| Compile date/time | **Jan 26 2026 17:28:28** |
| Arduino-ESP32 core | **3.0.7** |
| ESP-IDF | `v5.1.4-972-g632e0c2a9f-dirty` |
| Build host | `C:/Users/Fei/.platformio/...` — Windows, PlatformIO, user "Fei" |
| Framework package | `framework-arduinoespressif32@src-8805c63d0df9a0be3b3c0911dc695685` (a *source* pin — custom/forked platform, not a stock PlatformIO release) |
| Vendored library | `lib/ESP32_Display_Panel-0.2.2` |

The `esp_app_desc` project name reads `arduino-lib-builder` with date `Oct 22 2024` — that is the
arduino-esp32 **library** build stamp, not this firmware's. The real build date is the
`Jan 26 2026` compile stamp above.

Author's own source tree, from assert paths left in the binary:

```
src/mydriver/screen_init/scr_st77916.h
src/mydriver/idfknob/iot_knob.c
src/mydriver/uart1.cpp
```

## 6. Hardware implied by the image

ST77916 LCD · CST816S touch · rotary encoder (`iot_knob`) · DRV2605 haptic driver ·
SD/TF card · I2S DAC + microphone · IMU/accelerometer · RGB LED · battery sense ·
USB MSC + UAC · PSRAM (firmware aborts with `NO PSRAM` without it).

This is consistent with the Waveshare ESP32-S3-Knob-Touch-LCD-1.8 class of board that the
`controller_ws1p8knob_*` envs target — same SoC, same display and touch controllers, same
16 MB flash, and the 8 MB embedded PSRAM is confirmed by esptool. Consistent with, not proof
of, an identical board; the vendor sells it as its own product and the PCB has not been compared.

Firmware features (from the app list): AIDA64 PC sub-screen, Bluetooth A2DP audio, audio
spectrum, MJPEG video, photo album, weather clock, text reader, music player, pomodoro timer,
G-meter, USB MSC/UAC. Assets are Genshin Impact / Honkai Star Rail themed
(`genshin200mn.mjpeg`, `cyrene_chr_%d.bin`, `Oronys_ring_mask_360.bin`).

## 7. Author / where to ask for source

The firmware credits its author on the About screen. The literal at app offset `0x2474` is:

```
设计: #d0c0c0 萨纳兰的黄昏# #00AEEC \uE3D9#
Design: #d0c0c0 萨纳兰的黄昏# #00AEEC \uE3D9#
```

`#RRGGBB text#` are LVGL recolor tags. `U+E3D9` is a private-use glyph tinted `#00AEEC` —
bilibili's brand blue — i.e. a bilibili logo. The credit reads **"Design: 萨纳兰的黄昏"**
("Dusk of Thanalan"), a solo hardware developer in Sichuan, China.

| Channel | |
|---|---|
| bilibili | <https://space.bilibili.com/18598545> — bio states he takes custom electronics commissions |
| Website | <http://pressf5.run/> — "码的第七章 / Code Chapter Seven", 蜀ICP备2023035069号 |
| Firmware + changelog | <https://pressf5.run/?p=186> (downloads via Baidu Pan; needs matching TF-card assets) |
| General manual | <https://pressf5.run/?p=119> |
| QQ group | **636426429** ("ESP32DIY群") — support group for his ESP32 products; most direct line |
| GitHub | <https://github.com/planevina> |
| OSHWHub | <https://oshwhub.com/planevina> |
| Retail | 闲鱼 (Xianyu) / bilibili 工房, search 萨纳兰的黄昏 |

Product line on the site: TAIJI_KNOB_32, TAIJI_VIEWE_PI, TAIJI_KNOB_HUB, 神之眼 S3, iCRT,
HALO TOUCH, FAKE POD NANO.

### Source availability

**Not published.** No TAIJI repository exists on his GitHub or OSHWHub, and neither the site nor
the manual makes any open-source claim for this product. His pattern is selective: iCRT's *PCB*
is on OSHWHub, 神之眼's *code* is on GitHub (`planevina/genshin_godeye_esp32` — the origin of the
`GODVISION` namespace still present here), and his `platform-espressif32` fork is public. Asking is
reasonable; expecting is not.

Two points that support a polite request:

1. He already open-sources selectively, and the build environment is effectively half-published
   already via the public platform fork.
2. Arduino-ESP32 core 3.0.7 is **LGPL-2.1-or-later** and is statically linked here. That obligates
   providing the means to relink (object files or a linkable image) — it does **not** obligate
   release of his own `src/mydriver/*` and UI code. A narrow hook, better raised as context than as
   a demand.

If the goal is this hardware running our code rather than his source, his bio explicitly advertises
custom electronics work — a commission or a board-support subset is likelier to land.

## 8. Negative findings

No vendor server domain is hardcoded anywhere in the app. Checked plaintext across the full
app image and single-byte-XOR obfuscation for `http` / `.com` / `.cn` / `update` / `firmware` —
nothing. The only domains present are `ntp.aliyun.com`, `time.windows.com`, `cn.pool.ntp.org`,
`lbs.amap.com` and `openweathermap.org`.

"Check update" and "OTA mode" operate over the LAN or a pushed connection; releases are hand-
distributed via Baidu Pan. **There is no update endpoint to enumerate official images from.**

## 9. Reproducing the analysis

```bash
dd if=original_firmware_dump.bin of=app0.bin bs=1 skip=$((0x10000)) count=$((0x1F1000))
xxd -s 0x8000 -l 256 original_firmware_dump.bin   # partition table
xxd -s 0x10020 -l 256 original_firmware_dump.bin  # esp_app_desc
strings -n 5 app0.bin | less                      # UI strings, source paths, build host
```

Chinese UI strings are UTF-8 and start around app offset `0x1980`; the About/credit block is at
`0x2440`–`0x24B0` (Chinese) and `0x0CB0`–`0x0D00` (English).

## 10. Repo hygiene

This `.bin` is 16 MB and is **not** currently tracked by git and **not** covered by any
`.gitignore` rule — it is one `git add .` away from entering the repo's history permanently.
Decide deliberately: either ignore it, or commit it via Git LFS.
