# How the OAS-Man Car Editor Works

This document explains the web-based car image editor in `docs/controllerCarCreator/`: what problem it solves, how it is built, how the editing workflow runs in the browser, and how exported LVGL C files plug into the Wireless Controller firmware.

For quick build/run instructions, see [README.md](./README.md).

---

## Purpose

The OAS-Man Wireless Controller shows a stylized car and wheels on the presets screen. Builders can replace those graphics with a photo of their own vehicle so the UI feels personalized.

The original workflow used a Python Tkinter desktop tool:

`Wireless_Controller/tools/imageCreator/car_creator.py`

That tool let you outline wheels and body from a photo, align them over device-specific reference images, and export PNGs. You then had to run those PNGs through [lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter) to get `.c` files for the firmware.

The web editor replaces that two-step export with a **single browser workflow** that downloads ready-to-compile LVGL sources:

- `img_car_custom.c`
- `img_wheels_custom.c`

Those files match the format of the preset assets under `Wireless_Controller/device_libs/<device>/images/presets/` (for example `img_car.c`, `img_wheels.c`).

---

## High-level architecture

The app follows the same pattern as `docs/flash/`: source files are compiled into one standalone `index.html` that runs without a web server.

```
┌─────────────────┐
│    app.tsx      │  React UI + canvas image logic + LVGL C export
└────────┬────────┘
         │
┌────────┴────────┐     ┌──────────────────────────────────────────┐
│  template.html  │     │  Wireless_Controller/device_libs/        │
│  (HTML + CSS)   │     │    <device>/images/presets/img_car.png   │
└────────┬────────┘     │    <device>/images/presets/img_wheels.png│
         │              └──────────────────┬───────────────────────┘
         │                                 │
         └──────────────┬──────────────────┘
                        │
                 ┌──────▼──────┐
                 │  build.mjs  │  esbuild transform + preset embed
                 └──────┬──────┘
                        │
                 ┌──────▼──────┐
                 │ index.html  │  Single file, open in Chrome/Edge
                 └─────────────┘
```

At runtime:

1. React and ReactDOM load from CDN `<script>` tags (UMD globals).
2. `window.PRESET_IMAGES` holds base64 data URLs and dimensions for each supported controller.
3. The inlined app mounts on `#root` and never talks to a backend.

---

## Build pipeline (`build.mjs`)

### Step 1 — Transpile `app.tsx`

`esbuild.transform()` compiles TypeScript/JSX to minified ES2019 JavaScript. React is **not** bundled; `app.tsx` declares `React` and `ReactDOM` as globals, matching `template.html`.

### Step 2 — Embed device presets

`loadPresets()` scans `Wireless_Controller/device_libs/` for subdirectories. For each device it tries to read:

- `images/presets/img_car.png`
- `images/presets/img_wheels.png`

If both exist, they are:

1. Base64-encoded into `data:image/png;base64,...` strings.
2. Measured with a lightweight PNG header parse (`pngDimensions()` reads width/height from bytes 16–23).

Devices without both PNGs are skipped (for example `ws1p8knob`, which has no circular-display car presets).

The result is injected at the `/* __PRESETS__ */` placeholder:

```javascript
window.PRESET_IMAGES = {
  ws2p8: {
    name: '2.8" Waveshare',
    car: 'data:image/png;base64,...',
    carW: 240,
    carH: 79,
    wheels: 'data:image/png;base64,...',
    wheelsW: 240,
    wheelsH: 53
  },
  // ws2p8b, ws3p5, ws3p5b, ...
};
```

Human-readable names come from a fixed map in `build.mjs`; unknown folder names fall back to the folder id.

### Step 3 — Write `index.html`

`template.html` placeholders are replaced:

| Placeholder        | Replaced with                          |
| ------------------ | -------------------------------------- |
| `/* __PRESETS__ */` | `window.PRESET_IMAGES = {...}`        |
| `/* __APP_JS__ */`  | Minified JavaScript from `app.tsx`    |

The output file is self-contained except for the React CDN scripts.

**Important:** Preset PNGs live on disk under `device_libs` but are often gitignored locally. You must have those files present when running `npm run build`, or the build fails with “No preset PNGs found”.

---

## Runtime stack (`template.html` + `app.tsx`)

### UI shell

`template.html` provides:

- Dark-theme CSS (layout, stepper, sliders, drop zone, canvas containers).
- React 18 and ReactDOM from unpkg CDN.
- `<div id="root">` mount point.

### State model

The main `App` component holds workflow state:

| State | Role |
| ----- | ---- |
| `device` | Selected controller id (`ws2p8`, etc.) |
| `step` | Current workflow step (see below) |
| `deviceLocked` | Device dropdown disabled after upload starts |
| `sourceCanvas` | User's uploaded photo |
| `wheelCrops` | Two ellipse-cropped wheel images |
| `newCanvas` | Polygon-cropped car body (wheels excluded) |
| `carExportCanvas` | Final car image at preset resolution (after align step) |
| `overlayX/Y/Scale` | Car alignment on reference |
| `w1x/w1y`, `w2x/w2y` | Wheel alignment (scale locked to 1.0) |

Preset reference canvases (`origCanvas` for car, `wheelsCanvas` for wheels) reload when the device changes.

---

## User workflow

The stepper mirrors the Python tool's sequence:

```
Upload → Wheel 1 → Wheel 2 → Car outline → Align car → Align wheels → Download .c files
```

### 1. Device selection + upload

User picks a controller variant. That choice sets:

- Reference car size (`carW` × `carH`)
- Reference wheels size (`wheelsW` × `wheelsH`)
- Background images used in alignment previews

User uploads PNG or JPG via file picker or drag-and-drop. The device is locked for the rest of the session so export dimensions stay consistent.

### 2. Wheel outlining (×2)

Each wheel uses a **three-point ellipse**:

1. Click center of the wheel.
2. Click a point on the rim (defines first axis).
3. Click another rim point (defines second axis).

The ellipse is approximated as a 64-point polygon (`ellipsePolygonPoints()`), then cropped with `extractEllipseCrop()`. Handles can be dragged to refine the shape.

Wheel 1 crop is stored, then the user repeats for wheel 2 on the same source photo.

### 3. Car body outlining

User clicks polygon vertices around the car **excluding** the wheels. `extractPolygonCrop()`:

1. Computes bounding box of all points.
2. Creates a transparent canvas of that size.
3. Builds a clip path from the polygon.
4. Draws the source image offset so only the interior remains.

### 4. Align car + export `img_car_custom.c`

The cropped car is composited over the device's `img_car.png` reference:

- **Preview:** reference at full opacity + new car at 88% opacity (`OVERLAY_ALPHA = 0.88`).
- **Sliders:** X, Y, and scale (0.05–1.5, same range as the Python tool).

Export uses `buildExportImage()`: a canvas exactly `carW` × `carH` with **only** the aligned car (transparent background). No reference layer is baked in.

Clicking download runs `downloadLvglC(exportCanvas, "img_car_custom", "img_car_custom.c")`.

The export canvas is also saved in `carExportCanvas` for the wheels step.

### 5. Align wheels + export `img_wheels_custom.c`

The cropped wheels are composited over the device's `img_wheels.png` reference. The previously exported car image is drawn faintly (`CAR_ON_WHEELS_ALPHA = 0.25`) as a positioning guide.

Each wheel has independent X/Y sliders; scale is fixed at 1.0 (matching the Python tool's wheels step).

Export uses `buildWheelsExportImage()` at `wheelsW` × `wheelsH`.

---

## Image processing (Canvas 2D)

The Python tool uses Pillow (`Image`, `ImageDraw`, alpha compositing). The web port uses the same math with HTML5 Canvas:

| Python (Pillow) | Web (Canvas) |
| --------------- | ------------ |
| `Image.open()` | `loadImageCanvas()` / `loadFileCanvas()` |
| Polygon mask + crop | `ctx.clip()` + `drawImage()` |
| `Image.alpha_composite` | `ctx.globalAlpha` + layered `drawImage()` |
| Resize for preview | Scale factor from `previewMax / origW` |
| Export at native size | Dedicated export canvases at preset dimensions |

All intermediate results are `HTMLCanvasElement` instances in memory. Nothing is uploaded to a server.

### Key constants (shared with Python)

```typescript
OVERLAY_SCALE_MIN = 0.05
OVERLAY_SCALE_MAX = 1.5
OVERLAY_ALPHA = 0.88
CAR_ON_WHEELS_ALPHA = 0.25
```

---

## LVGL C export (RGB565A8)

Previously, users downloaded PNGs and converted them externally. The web app now generates `.c` files in the same format as LVGL v9's online converter and the project's device_lib presets.

### Color format: `LV_COLOR_FORMAT_RGB565A8`

Each pixel occupies **3 bytes** in the array, but memory layout is **two planes**, not interleaved RGBA:

```
[ RGB565 byte 0 ][ RGB565 byte 1 ]  × (width × height)   ← color plane
[ alpha byte ]                      × (width × height)   ← alpha plane
```

Total array size = `width × height × 3`.

### Conversion: `canvasToRgb565A8()`

1. Read RGBA pixels with `getImageData()`.
2. For each pixel, quantize to RGB565:
   - R: 5 bits (`r >> 3`)
   - G: 6 bits (`g >> 2`)
   - B: 5 bits (`b >> 3`)
   - Packed: `(r5 << 11) | (g6 << 5) | b5`
3. Store as **little-endian** uint16 (low byte first, then high byte).
4. Append full 8-bit alpha values in row-major order after the color plane.

### C file generation: `generateLvglCFile()`

Output structure matches `Wireless_Controller/device_libs/ws2p8/images/presets/img_car.c`:

```c
#include "lvgl.h"   // or "lvgl/lvgl.h" depending on include style

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST
      LV_ATTRIBUTE_IMAGE_IMG_CAR_CUSTOM uint8_t img_car_custom_map[] = {
  0x00, 0x00, ...   // 16 bytes per line
};

const lv_image_dsc_t img_car_custom = {
  .header.cf = LV_COLOR_FORMAT_RGB565A8,
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.w = <width>,
  .header.h = <height>,
  .data_size = <width * height> * 3,
  .data = img_car_custom_map,
};
```

`formatCArrayBytes()` writes hex literals (`0xHH`) with 16 bytes per line for readability.

### Download

`downloadLvglC()` builds the string in memory and triggers a browser download via `Blob` + temporary `<a download>` — no server round trip.

---

## Firmware integration

### Where custom images are used

When `-D CUSTOM_CAR_IMAGE` is enabled in `Wireless_Controller/platformio.ini`, the presets screen aliases the custom symbols:

```cpp
#ifdef CUSTOM_CAR_IMAGE
LV_IMG_DECLARE(img_car_custom);
LV_IMG_DECLARE(img_wheels_custom);
const lv_image_dsc_t img_car = img_car_custom;
const lv_image_dsc_t img_wheels = img_wheels_custom;
#else
LV_IMG_DECLARE(img_car);
LV_IMG_DECLARE(img_wheels);
#endif
```

(`ui_scrPresets.cpp`)

Layout code reads `img_car.header.w/h` and `img_wheels.header.w/h` directly, so exported dimensions **must** match the selected device's preset sizes. That is why the editor locks the device after upload and exports at exact `carW/carH` and `wheelsW/wheelsH`.

### Builder steps after export

1. Copy `img_car_custom.c` and `img_wheels_custom.c` into `Wireless_Controller/src/`.
2. Uncomment `-D CUSTOM_CAR_IMAGE` in `platformio.ini` for your controller env.
3. Build and flash, for example:
   ```bash
   pio run -e controller_ws2p8_dev -t upload
   ```
4. If wheels sit slightly off on the display, tweak alignment in the editor and re-export, or adjust layout offsets in `ui_scrPresets.cpp`.

---

## Supported devices

At build time, any `device_libs` folder with both preset PNGs is included. Known mappings:

| Device id | Display name |
| --------- | ------------ |
| `ws2p8`   | 2.8" Waveshare |
| `ws2p8b`  | 2.8" B Model Waveshare |
| `ws3p5`   | 3.5" Waveshare |
| `ws3p5b`  | 3.5" B Model Waveshare |

Example preset dimensions (ws2p8):

| Asset | Size |
| ----- | ---- |
| Car | 240 × 79 |
| Wheels | 240 × 53 |

B-model variants use higher-resolution assets (for example ws2p8b car at 480 × 158).

---

## Development workflow

```bash
cd docs/controllerCarCreator
npm install          # once
npm run build        # writes index.html
npm run watch        # rebuild on app.tsx / template.html changes
```

Edit **`app.tsx`** for behavior, **`template.html`** for shell/CSS, **`build.mjs`** for preset bundling logic.

Do **not** hand-edit `index.html`; it is regenerated and marked with `AUTO-GENERATED` banners.

### Testing changes

1. Rebuild `index.html`.
2. Open it locally in Chrome or Edge (double-click or `file://` URL).
3. Walk through the full workflow with a test photo.
4. Inspect downloaded `.c` files: check `header.w/h`, `data_size`, and that symbols are `img_car_custom` / `img_wheels_custom`.
5. Drop the `.c` files into `Wireless_Controller/src/`, enable `CUSTOM_CAR_IMAGE`, build the matching controller env, and verify on hardware.

---

## Design decisions

| Decision | Rationale |
| -------- | --------- |
| Standalone `index.html` | Same deploy model as `docs/flash/`; easy to host on oasman.dev |
| Embed presets at build time | Preset PNGs are not in git; bundling avoids CORS/`file://` fetch issues |
| CDN React | Keeps generated HTML smaller; acceptable for a dev/builder tool |
| Direct LVGL C export | Removes manual lvgl.io step; guarantees format match with firmware |
| Canvas instead of WebGL | Sufficient for static image ops; mirrors Pillow pipeline closely |
| Device lock after upload | Prevents accidental dimension mismatch on export |

---

## Limitations

- **Browser only:** Requires a modern browser with Canvas 2D; no mobile-specific UI.
- **No project save:** Closing the tab loses in-progress work; re-upload to start over.
- **Local preset files required to build:** CI or fresh clones need `device_libs/.../presets/*.png` on disk.
- **ws1p8knob:** No car/wheels presets in the usual layout; not offered in the editor.
- **Color accuracy:** RGB565 quantization loses subtle gradients compared to full PNG; same limitation as the official LVGL converter.
- **File size:** `index.html` grows with embedded preset PNGs (multiple devices × two images each).

---

## File reference

| File | Role |
| ---- | ---- |
| `app.tsx` | Application source (workflow, canvas, LVGL export) |
| `template.html` | HTML shell, CSS, CDN scripts, placeholders |
| `build.mjs` | Build script: esbuild + preset scanner |
| `package.json` | npm scripts and esbuild dependency |
| `index.html` | **Generated** output — do not edit |
| `README.md` | Short build/usage guide |
| `HOW_IT_WORKS.md` | This document |

Legacy Python tool (still in repo for reference): `Wireless_Controller/tools/imageCreator/car_creator.py`
