// Compiles app.tsx -> JS (esbuild) and inlines it into template.html,
// producing a standalone index.html with no in-browser transpilation.
//
//   npm run build      # one-shot
//   npm run watch      # rebuild on change
//
// React / ReactDOM / esp-web-tools are still loaded from CDN <script> tags in
// template.html; app.tsx references them as the globals `React` / `ReactDOM`,
// so esbuild only needs to transform JSX/TS (no bundling).

import { readFile, writeFile } from "node:fs/promises";
import { watch } from "node:fs";
import * as esbuild from "esbuild";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const root = dirname(fileURLToPath(import.meta.url));
const SRC = join(root, "app.tsx");
const TEMPLATE = join(root, "template.html");
const OUT = join(root, "index.html");
const PLACEHOLDER = "/* __APP_JS__ */";

async function build() {
	const source = await readFile(SRC, "utf8");

	const { code } = await esbuild.transform(source, {
		loader: "tsx",
		jsx: "transform",
		jsxFactory: "React.createElement",
		jsxFragment: "React.Fragment",
		target: "es2019",
		minify: true,
	});

	const template = await readFile(TEMPLATE, "utf8");
	if (!template.includes(PLACEHOLDER)) {
		throw new Error(`template.html is missing the ${PLACEHOLDER} placeholder`);
	}

	const banner = "// AUTO-GENERATED from app.tsx by build.mjs — do not edit directly.\n";
	const html = template.replace(PLACEHOLDER, () => banner + code.trim());

	await writeFile(OUT, html, "utf8");
	console.log(`[build] wrote ${OUT} (${(html.length / 1024).toFixed(1)} kB)`);
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
