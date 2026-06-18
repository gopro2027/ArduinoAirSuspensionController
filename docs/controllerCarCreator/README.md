# OAS-Man Car Editor

Web tool for aligning a custom car photo over the controller preset images and exporting
`img_car_custom.png` and `img_wheels_custom.png` for use with the `-D CUSTOM_CAR_IMAGE`
build flag. Replaces the Python Tkinter tool at
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

## After exporting

1. Download both PNGs from the editor.
2. Convert them at [lvgl.io/tools/imageconverter](https://lvgl.io/tools/imageconverter)
   as **RGB565A8**.
3. Copy the resulting `.c` files into `Wireless_Controller/src/`.
4. Uncomment `-D CUSTOM_CAR_IMAGE` in `Wireless_Controller/platformio.ini`.
5. Build and flash your controller env to test.

## Files

| File            | Committed? | Notes                            |
| --------------- | ---------- | -------------------------------- |
| `app.tsx`       | yes        | React source                     |
| `template.html` | yes        | HTML shell + CSS                 |
| `build.mjs`     | yes        | Build script + preset bundler    |
| `index.html`    | yes        | **Generated** — do not hand-edit |
| `node_modules/` | no         | gitignored                       |
