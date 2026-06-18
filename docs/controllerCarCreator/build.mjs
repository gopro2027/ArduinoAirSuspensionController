// Compiles app.tsx -> JS (esbuild), embeds device preset PNGs, and inlines both
// into template.html, producing a standalone index.html.
//
//   npm run build      # one-shot
//   npm run watch      # rebuild on change
//
// React / ReactDOM are loaded from CDN <script> tags in template.html; app.tsx
// references them as globals, so esbuild only transforms JSX/TS (no bundling).

import { readFile, writeFile, readdir } from "node:fs/promises";
import { watch } from "node:fs";
import * as esbuild from "esbuild";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const root = dirname(fileURLToPath(import.meta.url));
const SRC = join(root, "app.tsx");
const TEMPLATE = join(root, "template.html");
const OUT = join(root, "index.html");
const APP_PLACEHOLDER = "/* __APP_JS__ */";
const PRESETS_PLACEHOLDER = "/* __PRESETS__ */";
const DEVICE_LIBS = join(root, "..", "..", "Wireless_Controller", "device_libs");

const DEVICE_NAMES = {
	ws2p8: '2.8" Waveshare',
	ws2p8b: '2.8" B Model Waveshare',
	ws3p5: '3.5" Waveshare',
	ws3p5b: '3.5" B Model Waveshare',
};

function pngDimensions(buffer) {
	if (buffer[0] !== 0x89 || buffer[1] !== 0x50) {
		throw new Error("Not a PNG file");
	}
	return { width: buffer.readUInt32BE(16), height: buffer.readUInt32BE(20) };
}

async function loadPresets() {
	const entries = await readdir(DEVICE_LIBS, { withFileTypes: true });
	const presets = {};

	for (const entry of entries) {
		if (!entry.isDirectory()) continue;
		const device = entry.name;
		const carPath = join(DEVICE_LIBS, device, "images", "presets", "img_car.png");
		const wheelsPath = join(DEVICE_LIBS, device, "images", "presets", "img_wheels.png");

		try {
			const carBuf = await readFile(carPath);
			const wheelsBuf = await readFile(wheelsPath);
			const carDim = pngDimensions(carBuf);
			const wheelsDim = pngDimensions(wheelsBuf);

			presets[device] = {
				name: DEVICE_NAMES[device] || device,
				car: `data:image/png;base64,${carBuf.toString("base64")}`,
				carW: carDim.width,
				carH: carDim.height,
				wheels: `data:image/png;base64,${wheelsBuf.toString("base64")}`,
				wheelsW: wheelsDim.width,
				wheelsH: wheelsDim.height,
			};
		} catch {
			// skip devices without preset PNGs (e.g. ws1p8knob)
		}
	}

	return presets;
}

async function build() {
	const source = await readFile(SRC, "utf8");
	const presets = await loadPresets();
	const deviceCount = Object.keys(presets).length;
	if (deviceCount === 0) {
		throw new Error(`No preset PNGs found under ${DEVICE_LIBS}`);
	}

	const { code } = await esbuild.transform(source, {
		loader: "tsx",
		jsx: "transform",
		jsxFactory: "React.createElement",
		jsxFragment: "React.Fragment",
		target: "es2019",
		minify: true,
	});

	const template = await readFile(TEMPLATE, "utf8");
	if (!template.includes(APP_PLACEHOLDER)) {
		throw new Error(`template.html is missing the ${APP_PLACEHOLDER} placeholder`);
	}
	if (!template.includes(PRESETS_PLACEHOLDER)) {
		throw new Error(`template.html is missing the ${PRESETS_PLACEHOLDER} placeholder`);
	}

	const presetsBanner =
		"// AUTO-GENERATED preset images from device_libs by build.mjs — do not edit directly.\n";
	const presetsJs =
		presetsBanner + `window.PRESET_IMAGES = ${JSON.stringify(presets)};`;

	const appBanner = "// AUTO-GENERATED from app.tsx by build.mjs — do not edit directly.\n";
	let html = template.replace(PRESETS_PLACEHOLDER, () => presetsJs);
	html = html.replace(APP_PLACEHOLDER, () => appBanner + code.trim());

	await writeFile(OUT, html, "utf8");
	console.log(
		`[build] wrote ${OUT} (${(html.length / 1024).toFixed(1)} kB, ${deviceCount} devices)`,
	);
}

const isWatch = process.argv.includes("--watch");

await build().catch((err) => {
	console.error(err);
	process.exit(1);
});

if (isWatch) {
	console.log("[watch] watching app.tsx and template.html…");
	let timer = null;
	const rebuild = () => {
		clearTimeout(timer);
		timer = setTimeout(() => build().catch((e) => console.error(e)), 100);
	};
	watch(SRC, rebuild);
	watch(TEMPLATE, rebuild);
}
