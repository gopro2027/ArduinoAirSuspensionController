# OAS-Man Web Installer

The web firmware flasher served at [oasman.dev/flash](https://oasman.dev/flash). It's a
React multi-step UI (Type → Device → Version → Install) that uses
[ESP Web Tools](https://esphome.github.io/esp-web-tools/) to flash OAS-Man firmware
straight from the browser.

The shipped page, `index.html`, is **generated** — don't edit it by hand. Edit the
source files and rebuild.

## Building `index.html`

Requires [Node.js](https://nodejs.org/) (v18+).

```bash
# from docs/flash/
npm install      # one-time: installs esbuild
npm run build    # compiles app.tsx + template.html -> index.html
```

That's it. `index.html` is a standalone file — just open it (no local server needed;
it only needs internet for the CDN scripts). To preview, open `index.html` in Chrome or
Edge. Note: the actual flashing step requires the page be served over `https://` or
`localhost` (a browser Web Serial rule), but the UI itself renders fine from `file://`.

### Rebuild automatically on change

```bash
npm run watch    # rebuilds index.html whenever app.tsx or template.html changes
```

## How the build works

```
app.tsx ───────┐
               ├──►  build.mjs (esbuild)  ──►  index.html  (generated, shippable)
template.html ─┘
```

- **`app.tsx`** — the React app and the multi-step flow logic. **Source of truth for behavior.**
- **`template.html`** — the HTML shell: `<head>`, all CSS, the CDN `<script>` tags
  (React, ReactDOM, esp-web-tools, `manifestgen.js`), and the `<div id="root">` mount
  point. Contains a single `/* __APP_JS__ */` placeholder.
- **`build.mjs`** — transpiles/minifies `app.tsx` with esbuild and inlines it into the
  template at the `/* __APP_JS__ */` marker, writing `index.html`.
- **`manifestgen.js`** — builds the ESP Web Tools manifest; loaded as a global script and
  called by the app.

### What to edit

| To change…                                   | Edit…           |
| -------------------------------------------- | --------------- |
| App behavior / steps / device list           | `app.tsx`       |
| Styling, meta tags, favicon, which scripts load | `template.html` |
| Manifest / flash offsets                      | `manifestgen.js`|

After editing, run `npm run build` to regenerate `index.html`.

## Files

| File                       | Committed? | Notes                                  |
| -------------------------- | ---------- | -------------------------------------- |
| `app.tsx`                  | yes        | React source                           |
| `template.html`            | yes        | HTML shell + CSS                       |
| `build.mjs`                | yes        | Build script                           |
| `manifestgen.js`           | yes        | Manifest generator                     |
| `index.html`               | yes        | **Generated** — do not hand-edit       |
| `boot_app0.bin`            | yes        | Referenced by the manifest             |
| `index_legacy_raw_html.html` | yes      | Previous plain-HTML installer (archive)|
| `node_modules/`            | no         | gitignored                             |
