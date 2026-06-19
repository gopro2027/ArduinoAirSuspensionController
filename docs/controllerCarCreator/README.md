# OAS-Man Car Editor

Web tool for aligning a custom car photo over the controller preset images and uploading
custom car/wheel graphics to the Wireless Controller over USB serial, or exporting
ready-to-compile LVGL C sources for developers. Replaces the Python Tkinter tool at
`Wireless_Controller/tools/imageCreator/car_creator.py`.

The shipped page, `index.html`, is **generated** — don't edit it by hand. Edit the
source files and rebuild.

## Building `index.html`

Requires [Node.js](https://nodejs.org/) (v18+).

```bash
# from docs/controllerCarCreator/
npm install      # one-time: installs esbuild
npm run build    # compiles app.tsx + template.html + preset PNGs -> index.html
```

That's it. `index.html` is a standalone file — open it in Chrome or Edge (no local
server needed). Preset reference images are embedded at build time from
`Wireless_Controller/device_libs/<device>/images/presets/`.

### Rebuild automatically on change

```bash
npm run watch    # rebuilds index.html whenever app.tsx or template.html changes
```

## How the build works

```
app.tsx ────────────────┐
                        ├──►  build.mjs (esbuild + preset embed)  ──►  index.html
template.html ──────────┤
device_libs presets ────┘
```

- **`app.tsx`** — the React app and workflow logic. **Source of truth for behavior.**
- **`template.html`** — the HTML shell: `<head>`, CSS, CDN `<script>` tags (React,
  ReactDOM), preset placeholder, and `<div id="root">`.
- **`build.mjs`** — transpiles/minifies `app.tsx`, scans `device_libs` for preset PNGs,
  base64-embeds them as `window.PRESET_IMAGES`, and writes `index.html`.

## Upload to controller (recommended)

Requires firmware with USB car upload support (current `dev` controller builds).

1. Complete the align-car and align-wheels steps in the editor.
2. On the controller: **Settings → Upload custom car (USB)** → confirm reboot.
3. Plug the controller into your computer with USB.
4. In the editor (Chrome or Edge, `https://` or localhost): **Connect serial** → **Upload to device**.
5. The controller reboots with your custom car on the presets screen.

Custom images are stored on the controller's **LittleFS** flash partition and survive OTA firmware updates.

To remove USB-uploaded images: **Settings → Clear custom car images**.

## Developer fallback (compile-time)

1. Download `img_car_custom.c` and `img_wheels_custom.c` from the editor.
2. Copy them into `Wireless_Controller/src/`.
3. Uncomment `-D CUSTOM_CAR_IMAGE` in `Wireless_Controller/platformio.ini`.
4. Build and flash your controller env.

The editor converts images to **LVGL RGB565A8** in the browser (same format as
[lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter)).

## Files

| File            | Committed? | Notes                            |
| --------------- | ---------- | -------------------------------- |
| `app.tsx`       | yes        | React source                     |
| `template.html` | yes        | HTML shell + CSS                 |
| `build.mjs`     | yes        | Build script + preset bundler    |
| `index.html`    | yes        | **Generated** — do not hand-edit |
| `HOW_IT_WORKS.md` | yes        | In-depth architecture doc        |
| `node_modules/` | no         | gitignored                       |
