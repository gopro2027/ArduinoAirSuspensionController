// OAS-Man car image editor — web port of car_creator.py.
// Compiled to index.html by build.mjs. React / ReactDOM are UMD globals.

declare const React: {
	createElement: any;
	Fragment: any;
	useState<T>(initial: T | (() => T)): [T, (next: T | ((prev: T) => T)) => void];
	useEffect(effect: () => void | (() => void), deps?: any[]): void;
	useRef<T>(initial: T): { current: T };
	useCallback<T extends (...args: any[]) => any>(fn: T, deps: any[]): T;
};
declare const ReactDOM: { createRoot(el: Element): { render(node: any): void } };

interface PresetDevice {
	name: string;
	car: string;
	carW: number;
	carH: number;
	wheels: string;
	wheelsW: number;
	wheelsH: number;
}

declare global {
	interface Window {
		PRESET_IMAGES: Record<string, PresetDevice>;
	}
}

const { useState, useEffect, useRef, useCallback } = React;

const OVERLAY_SCALE_MIN = 0.05;
const OVERLAY_SCALE_MAX = 1.5;
const OVERLAY_ALPHA = 0.88;
const CAR_ON_WHEELS_ALPHA = 0.25;
const HANDLE_R = 10;

type WorkflowStep =
	| null
	| "outline_wheel1"
	| "outline_wheel2"
	| "outline_car"
	| "align_car"
	| "align_wheels";

type Point = [number, number];

const STEP_LABELS: { key: WorkflowStep; label: string }[] = [
	{ key: null, label: "Upload" },
	{ key: "outline_wheel1", label: "Wheel 1" },
	{ key: "outline_wheel2", label: "Wheel 2" },
	{ key: "outline_car", label: "Car outline" },
	{ key: "align_car", label: "Align car" },
	{ key: "align_wheels", label: "Wheels & upload" },
];

function stepIndex(step: WorkflowStep): number {
	if (step === null) return 0;
	const idx = STEP_LABELS.findIndex((s) => s.key === step);
	return idx >= 0 ? idx : 0;
}

function loadImageCanvas(src: string): Promise<HTMLCanvasElement> {
	return new Promise((resolve, reject) => {
		const img = new Image();
		img.onload = () => {
			const c = document.createElement("canvas");
			c.width = img.width;
			c.height = img.height;
			c.getContext("2d")!.drawImage(img, 0, 0);
			resolve(c);
		};
		img.onerror = () => reject(new Error("Failed to load image"));
		img.src = src;
	});
}

function loadFileCanvas(file: File): Promise<HTMLCanvasElement> {
	return new Promise((resolve, reject) => {
		const reader = new FileReader();
		reader.onload = () => {
			loadImageCanvas(reader.result as string).then(resolve).catch(reject);
		};
		reader.onerror = () => reject(new Error("Failed to read file"));
		reader.readAsDataURL(file);
	});
}

function ellipsePolygonPoints(
	center: Point,
	axis1: Point,
	axis2: Point,
	numPoints = 64,
): Point[] {
	const [cx, cy] = center;
	const ux = axis1[0] - cx;
	const uy = axis1[1] - cy;
	const vx = axis2[0] - cx;
	const vy = axis2[1] - cy;
	const pts: Point[] = [];
	for (let i = 0; i < numPoints; i++) {
		const t = (2 * Math.PI * i) / numPoints;
		pts.push([cx + Math.cos(t) * ux + Math.sin(t) * vx, cy + Math.cos(t) * uy + Math.sin(t) * vy]);
	}
	return pts;
}

function extractPolygonCrop(source: HTMLCanvasElement, points: Point[]): HTMLCanvasElement | null {
	if (points.length < 3) return null;
	const xs = points.map((p) => p[0]);
	const ys = points.map((p) => p[1]);
	const x0 = Math.max(0, Math.floor(Math.min(...xs)));
	const x1 = Math.min(source.width, Math.floor(Math.max(...xs)) + 1);
	const y0 = Math.max(0, Math.floor(Math.min(...ys)));
	const y1 = Math.min(source.height, Math.floor(Math.max(...ys)) + 1);
	if (x1 <= x0 || y1 <= y0) return null;

	const cropW = x1 - x0;
	const cropH = y1 - y0;
	const crop = document.createElement("canvas");
	crop.width = cropW;
	crop.height = cropH;
	const ctx = crop.getContext("2d")!;
	ctx.beginPath();
	const rel = points.map((p) => [p[0] - x0, p[1] - y0] as Point);
	ctx.moveTo(rel[0][0], rel[0][1]);
	for (let i = 1; i < rel.length; i++) ctx.lineTo(rel[i][0], rel[i][1]);
	ctx.closePath();
	ctx.clip();
	ctx.drawImage(source, -x0, -y0);
	return crop;
}

function extractEllipseCrop(
	source: HTMLCanvasElement,
	center: Point,
	axis1: Point,
	axis2: Point,
): HTMLCanvasElement | null {
	const ellipsePts = ellipsePolygonPoints(center, axis1, axis2);
	return extractPolygonCrop(source, ellipsePts);
}

function buildCompositePreview(
	origCanvas: HTMLCanvasElement,
	origW: number,
	origH: number,
	newCanvas: HTMLCanvasElement,
	overlayX: number,
	overlayY: number,
	overlayScale: number,
	previewMax: [number, number],
): HTMLCanvasElement {
	const scaleDisplay = Math.min(previewMax[0] / origW, previewMax[1] / origH);
	const pw = Math.max(1, Math.floor(origW * scaleDisplay));
	const ph = Math.max(1, Math.floor(origH * scaleDisplay));

	const out = document.createElement("canvas");
	out.width = pw;
	out.height = ph;
	const ctx = out.getContext("2d")!;
	ctx.fillStyle = "#f0f0f0";
	ctx.fillRect(0, 0, pw, ph);
	ctx.drawImage(origCanvas, 0, 0, pw, ph);

	const nw = newCanvas.width;
	const nh = newCanvas.height;
	const newWPreview = Math.max(1, Math.floor(Math.floor(nw * overlayScale) * scaleDisplay));
	const newHPreview = Math.max(1, Math.floor(Math.floor(nh * overlayScale) * scaleDisplay));
	const xPreview = Math.floor(overlayX * scaleDisplay);
	const yPreview = Math.floor(overlayY * scaleDisplay);

	ctx.globalAlpha = OVERLAY_ALPHA;
	ctx.drawImage(newCanvas, xPreview, yPreview, newWPreview, newHPreview);
	ctx.globalAlpha = 1;
	return out;
}

function buildExportImage(
	origW: number,
	origH: number,
	newCanvas: HTMLCanvasElement,
	overlayX: number,
	overlayY: number,
	overlayScale: number,
): HTMLCanvasElement {
	const out = document.createElement("canvas");
	out.width = origW;
	out.height = origH;
	const ctx = out.getContext("2d")!;
	const newW = Math.max(1, Math.floor(newCanvas.width * overlayScale));
	const newH = Math.max(1, Math.floor(newCanvas.height * overlayScale));
	ctx.drawImage(newCanvas, overlayX, overlayY, newW, newH);
	return out;
}

function buildWheelsCompositePreview(
	wheelsCanvas: HTMLCanvasElement,
	wheelsW: number,
	wheelsH: number,
	wheelCrops: (HTMLCanvasElement | null)[],
	w1x: number,
	w1y: number,
	w1s: number,
	w2x: number,
	w2y: number,
	w2s: number,
	previewMax: [number, number],
	carCanvas: HTMLCanvasElement | null,
): HTMLCanvasElement {
	const scaleDisplay = Math.min(previewMax[0] / wheelsW, previewMax[1] / wheelsH);
	const pw = Math.max(1, Math.floor(wheelsW * scaleDisplay));
	const ph = Math.max(1, Math.floor(wheelsH * scaleDisplay));

	const out = document.createElement("canvas");
	out.width = pw;
	out.height = ph;
	const ctx = out.getContext("2d")!;
	ctx.fillStyle = "#f0f0f0";
	ctx.fillRect(0, 0, pw, ph);
	ctx.drawImage(wheelsCanvas, 0, 0, pw, ph);

	if (carCanvas && carCanvas.width > 0 && carCanvas.height > 0) {
		const carDW = Math.max(1, Math.floor(carCanvas.width * scaleDisplay));
		const carDH = Math.max(1, Math.floor(carCanvas.height * scaleDisplay));
		const carX = Math.floor(pw / 2 - carDW / 2);
		const carY = Math.floor(ph / 2 - carDH);
		ctx.globalAlpha = CAR_ON_WHEELS_ALPHA;
		ctx.drawImage(carCanvas, carX, carY, carDW, carDH);
		ctx.globalAlpha = 1;
	}

	const wheels = [
		{ wc: wheelCrops[0], x: w1x, y: w1y, s: w1s },
		{ wc: wheelCrops[1], x: w2x, y: w2y, s: w2s },
	];
	for (const { wc, x, y, s } of wheels) {
		if (!wc) continue;
		const newWPreview = Math.max(1, Math.floor(Math.floor(wc.width * s) * scaleDisplay));
		const newHPreview = Math.max(1, Math.floor(Math.floor(wc.height * s) * scaleDisplay));
		const xPreview = Math.floor(x * scaleDisplay);
		const yPreview = Math.floor(y * scaleDisplay);
		ctx.globalAlpha = OVERLAY_ALPHA;
		ctx.drawImage(wc, xPreview, yPreview, newWPreview, newHPreview);
		ctx.globalAlpha = 1;
	}
	return out;
}

function buildWheelsExportImage(
	wheelsW: number,
	wheelsH: number,
	wheelCrops: (HTMLCanvasElement | null)[],
	w1x: number,
	w1y: number,
	w1s: number,
	w2x: number,
	w2y: number,
	w2s: number,
): HTMLCanvasElement {
	const out = document.createElement("canvas");
	out.width = wheelsW;
	out.height = wheelsH;
	const ctx = out.getContext("2d")!;
	const wheels = [
		{ wc: wheelCrops[0], x: w1x, y: w1y, s: w1s },
		{ wc: wheelCrops[1], x: w2x, y: w2y, s: w2s },
	];
	for (const { wc, x, y, s } of wheels) {
		if (!wc) continue;
		const newW = Math.max(1, Math.floor(wc.width * s));
		const newH = Math.max(1, Math.floor(wc.height * s));
		ctx.drawImage(wc, Math.floor(x), Math.floor(y), newW, newH);
	}
	return out;
}

/** LVGL v9 RGB565A8: RGB565 plane (LE uint16 per pixel) then alpha plane. */
function canvasToRgb565A8(canvas: HTMLCanvasElement): Uint8Array {
	const w = canvas.width;
	const h = canvas.height;
	const { data } = canvas.getContext("2d")!.getImageData(0, 0, w, h);
	const pixelCount = w * h;
	const out = new Uint8Array(pixelCount * 3);

	for (let y = 0; y < h; y++) {
		for (let x = 0; x < w; x++) {
			const src = (y * w + x) * 4;
			const r = data[src];
			const g = data[src + 1];
			const b = data[src + 2];
			const color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
			const dst = (y * w + x) * 2;
			out[dst] = color & 0xff;
			out[dst + 1] = (color >> 8) & 0xff;
		}
	}

	const alphaOffset = pixelCount * 2;
	for (let y = 0; y < h; y++) {
		for (let x = 0; x < w; x++) {
			out[alphaOffset + y * w + x] = data[(y * w + x) * 4 + 3];
		}
	}
	return out;
}

const SERIAL_SUPPORTED = typeof navigator !== "undefined" && "serial" in navigator;
const MAX_SERIAL_LOG = 60000;

function escapeRegExp(text: string) {
	return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

/** Match a full protocol line (not a substring inside ESP log or binary noise). */
function containsWholeLine(text: string, line: string) {
	const re = new RegExp(`(?:^|\\n)${escapeRegExp(line)}(?:\\r?\\n|$)`);
	return re.test(text);
}

function findErrLine(text: string) {
	return text.split("\n").find((l) => l.startsWith("ERR "));
}

/** Wait for okLine in serial log after the most recent sent `> beginMarker` line. */
async function waitForResponseAfter(beginMarker: string, okLine: string, timeoutMs: number, logRef: { current: string }) {
	const start = Date.now();
	while (Date.now() - start < timeoutMs) {
		const log = logRef.current;
		const beginIdx = log.lastIndexOf(beginMarker);
		if (beginIdx >= 0) {
			const tail = log.slice(beginIdx);
			if (containsWholeLine(tail, okLine)) return;
			const errLine = findErrLine(tail);
			if (errLine) throw new Error(errLine);
		}
		await new Promise((r) => setTimeout(r, 100));
	}
	throw new Error(`Timed out waiting for ${okLine}`);
}

function appendPrintableSerialChunk(decoder: TextDecoder, chunk: Uint8Array, appendLog: (text: string) => void) {
	const text = decoder.decode(chunk);
	let out = "";
	for (let i = 0; i < text.length; i++) {
		const code = text.charCodeAt(i);
		// Drop raw binary runs; keep newlines and printable ASCII (protocol + ESP logs).
		if (code === 10 || code === 13 || (code >= 32 && code <= 126)) {
			out += text[i];
		}
	}
	if (out) appendLog(out);
}

function SerialUploadPanel({
	carCanvas,
	wheelsW,
	wheelsH,
	wheelCrops,
	w1x,
	w1y,
	w2x,
	w2y,
	wheelScale,
	carW,
	carH,
}: {
	carCanvas: HTMLCanvasElement;
	wheelsW: number;
	wheelsH: number;
	wheelCrops: (HTMLCanvasElement | null)[];
	w1x: number;
	w1y: number;
	w2x: number;
	w2y: number;
	wheelScale: number;
	carW: number;
	carH: number;
}) {
	const [connected, setConnected] = useState(false);
	const [busy, setBusy] = useState(false);
	const [log, setLog] = useState("");
	const [status, setStatus] = useState<string | null>(null);
	const portRef = useRef<any>(null);
	const readerRef = useRef<any>(null);
	const keepReadingRef = useRef(false);
	const logRef = useRef("");
	const logEndRef = useRef<HTMLDivElement>(null);
	const uploadInProgressRef = useRef(false);

	const appendLog = (text: string) => {
		logRef.current += text;
		if (logRef.current.length > MAX_SERIAL_LOG) {
			logRef.current = logRef.current.slice(logRef.current.length - MAX_SERIAL_LOG);
		}
		setLog(logRef.current);
	};

	useEffect(() => {
		if (logEndRef.current) logEndRef.current.scrollTop = logEndRef.current.scrollHeight;
	}, [log]);

	async function readLoop() {
		const port = portRef.current;
		const decoder = new TextDecoder();
		while (port && port.readable && keepReadingRef.current) {
			const reader = port.readable.getReader();
			readerRef.current = reader;
			try {
				while (true) {
					const { value, done } = await reader.read();
					if (done) break;
					if (value) appendPrintableSerialChunk(decoder, value, appendLog);
				}
			} catch (e: any) {
				appendLog(`\n[read error: ${e?.message || e}]\n`);
			} finally {
				reader.releaseLock();
				readerRef.current = null;
			}
		}
	}

	async function writeBytes(data: Uint8Array) {
		const port = portRef.current;
		if (!port?.writable) throw new Error("Serial port not open");
		const writer = port.writable.getWriter();
		try {
			await writer.write(data);
		} finally {
			writer.releaseLock();
		}
	}

	async function writeLine(line: string) {
		appendLog(`> ${line}\n`);
		await writeBytes(new TextEncoder().encode(line + "\n"));
	}

	async function writeBinary(data: Uint8Array) {
		const chunkSize = 4096;
		for (let i = 0; i < data.length; i += chunkSize) {
			await writeBytes(data.subarray(i, i + chunkSize));
		}
	}

	async function waitForReady(timeoutMs: number) {
		const start = Date.now();
		while (Date.now() - start < timeoutMs) {
			if (containsWholeLine(logRef.current, "OASMAN_IMG_READY")) return;
			const errLine = findErrLine(logRef.current);
			if (errLine) throw new Error(errLine);
			await new Promise((r) => setTimeout(r, 100));
		}
		throw new Error("Timed out waiting for OASMAN_IMG_READY");
	}

	async function connect() {
		setBusy(true);
		setStatus(null);
		try {
			const port = await (navigator as any).serial.requestPort();
			await port.open({ baudRate: 115200 });
			portRef.current = port;
			keepReadingRef.current = true;
			readLoop();
			setConnected(true);
			setStatus("Connected @ 115200 baud — waiting for OASMAN_IMG_READY…");
			await waitForReady(60000);
			setStatus("Device ready — uploading automatically…");
			await uploadToDevice();
		} catch (e: any) {
			if (e?.name !== "NotFoundError") {
				setStatus(e?.message || String(e));
			}
		} finally {
			setBusy(false);
		}
	}

	async function disconnect() {
		setBusy(true);
		keepReadingRef.current = false;
		try {
			if (readerRef.current) await readerRef.current.cancel().catch(() => {});
			if (portRef.current) await portRef.current.close().catch(() => {});
		} finally {
			portRef.current = null;
			setConnected(false);
			setBusy(false);
		}
	}

	async function uploadImage(
		beginPrefix: "CAR_BEGIN" | "WHEELS_BEGIN",
		okLine: string,
		canvas: HTMLCanvasElement,
		expectedW: number,
		expectedH: number,
	) {
		if (canvas.width !== expectedW || canvas.height !== expectedH) {
			throw new Error(`Export size ${canvas.width}×${canvas.height} does not match device ${expectedW}×${expectedH}`);
		}
		const bytes = canvasToRgb565A8(canvas);
		const beginMarker = `> ${beginPrefix}`;
		await writeLine(`${beginPrefix} ${expectedW} ${expectedH} ${bytes.length}`);
		await writeBinary(bytes);
		await waitForResponseAfter(beginMarker, okLine, 180000, logRef);
	}

	async function uploadToDevice() {
		if (!portRef.current || uploadInProgressRef.current) return;
		uploadInProgressRef.current = true;
		setBusy(true);
		setStatus("Uploading car image…");
		try {
			const wheelsCanvas = buildWheelsExportImage(
				wheelsW,
				wheelsH,
				wheelCrops,
				w1x,
				w1y,
				wheelScale,
				w2x,
				w2y,
				wheelScale,
			);
			await uploadImage("CAR_BEGIN", "CAR_OK", carCanvas, carW, carH);
			setStatus("Uploading wheels image…");
			await uploadImage("WHEELS_BEGIN", "WHEELS_OK", wheelsCanvas, wheelsW, wheelsH);
			await writeLine("DONE");
			await waitForResponseAfter("> DONE", "DONE", 30000, logRef);
			setStatus("Upload complete — the controller is rebooting with your custom car.");
		} catch (e: any) {
			setStatus(e?.message || String(e));
		} finally {
			uploadInProgressRef.current = false;
			setBusy(false);
		}
	}

	if (!SERIAL_SUPPORTED) {
		return (
			<div className="serial-panel">
				<p className="instruction">
					Web Serial is not available in this browser. Use Chrome or Edge on desktop over HTTPS or
					localhost.
				</p>
			</div>
		);
	}

	return (
		<div className="serial-panel">
			<p className="instruction">
				1. Plug in your controller and click 'Connect serial'
				<br />
				2. On the controller: Screen Settings → Upload custom car (USB) → Click 'Start'
				<br /><br />
				Upload starts automatically when the device is ready (use 'Upload to device' to retry if failed).
			</p>
			<div className="btn-row">
				{!connected ? (
					<button type="button" className="btn btn-secondary" disabled={busy} onClick={connect}>
						Connect serial
					</button>
				) : (
					<>
						<button type="button" className="btn btn-primary" disabled={busy} onClick={uploadToDevice}>
							Upload to device
						</button>
						<button type="button" className="btn btn-secondary" disabled={busy} onClick={disconnect}>
							Disconnect
						</button>
					</>
				)}
			</div>
			{status && <p className="serial-status">{status}</p>}
			{log && (
				<div className="serial-log" ref={logEndRef}>
					<pre>{log}</pre>
				</div>
			)}
		</div>
	);
}

function Stepper({ current }: { current: WorkflowStep }) {
	const curIdx = stepIndex(current);
	return (
		<div className="stepper">
			{STEP_LABELS.map((s, i) => {
				let cls = "step-badge";
				if (i === curIdx) cls += " active";
				else if (i < curIdx) cls += " done";
				return (
					<span key={s.label} className={cls}>
						{s.label}
					</span>
				);
			})}
		</div>
	);
}

function SliderControl({
	label,
	min,
	max,
	value,
	onChange,
}: {
	label: string;
	min: number;
	max: number;
	value: number;
	onChange: (v: number) => void;
}) {
	return (
		<div className="slider-row">
			<span className="slider-label">{label}</span>
			<input
				type="range"
				min={min}
				max={max}
				value={value}
				onChange={(e) => onChange(Number(e.target.value))}
			/>
			<span className="slider-val">{value}</span>
		</div>
	);
}

function OutlineView({
	step,
	sourceCanvas,
	instruction,
	onDone,
}: {
	step: "outline_wheel1" | "outline_wheel2" | "outline_car";
	sourceCanvas: HTMLCanvasElement;
	instruction: string;
	onDone: (crop: HTMLCanvasElement) => void;
}) {
	const canvasRef = useRef<HTMLCanvasElement>(null);
	const wrapRef = useRef<HTMLDivElement>(null);
	const [points, setPoints] = useState<Point[]>([]);
	const dragIndex = useRef<number | null>(null);
	const imgW = sourceCanvas.width;
	const imgH = sourceCanvas.height;
	const isWheel = step === "outline_wheel1" || step === "outline_wheel2";

	const canvasToImage = useCallback(
		(cx: number, cy: number, pw: number, ph: number): Point => {
			return [Math.floor((cx * imgW) / pw), Math.floor((cy * imgH) / ph)];
		},
		[imgW, imgH],
	);

	const imageToCanvas = useCallback(
		(ix: number, iy: number, pw: number, ph: number): Point => {
			return [Math.floor((ix * pw) / imgW), Math.floor((iy * ph) / imgH)];
		},
		[imgW, imgH],
	);

	const redraw = useCallback(() => {
		const canvas = canvasRef.current;
		const wrap = wrapRef.current;
		if (!canvas || !wrap) return;
		const pw = wrap.clientWidth;
		const ph = Math.max(300, Math.floor((pw * imgH) / imgW));
		canvas.width = pw;
		canvas.height = ph;
		const ctx = canvas.getContext("2d")!;
		ctx.drawImage(sourceCanvas, 0, 0, pw, ph);

		const drawHandle = (pt: Point) => {
			const [cx, cy] = imageToCanvas(pt[0], pt[1], pw, ph);
			ctx.fillStyle = "#000";
			ctx.strokeStyle = "#000";
			ctx.lineWidth = 2;
			ctx.beginPath();
			ctx.arc(cx, cy, 5, 0, Math.PI * 2);
			ctx.fill();
			ctx.strokeStyle = "#0f0";
			ctx.fillStyle = "#0f0";
			ctx.beginPath();
			ctx.arc(cx, cy, 4, 0, Math.PI * 2);
			ctx.fill();
		};

		if (isWheel && points.length === 3) {
			const ellipsePts = ellipsePolygonPoints(points[0], points[1], points[2]);
			const canvasEllipse = ellipsePts.map((p) => imageToCanvas(p[0], p[1], pw, ph));
			ctx.strokeStyle = "#000";
			ctx.lineWidth = 4;
			ctx.beginPath();
			ctx.moveTo(canvasEllipse[0][0], canvasEllipse[0][1]);
			for (let i = 1; i < canvasEllipse.length; i++) ctx.lineTo(canvasEllipse[i][0], canvasEllipse[i][1]);
			ctx.closePath();
			ctx.stroke();
			ctx.strokeStyle = "#0f0";
			ctx.lineWidth = 2;
			ctx.stroke();
			points.forEach(drawHandle);
		} else if (points.length >= 2) {
			ctx.strokeStyle = "#000";
			ctx.lineWidth = 4;
			ctx.beginPath();
			const [x0, y0] = imageToCanvas(points[0][0], points[0][1], pw, ph);
			ctx.moveTo(x0, y0);
			for (let i = 1; i < points.length; i++) {
				const [xi, yi] = imageToCanvas(points[i][0], points[i][1], pw, ph);
				ctx.lineTo(xi, yi);
			}
			ctx.stroke();
			ctx.strokeStyle = "#0f0";
			ctx.lineWidth = 2;
			ctx.stroke();
			points.forEach(drawHandle);
		}
	}, [sourceCanvas, points, isWheel, imageToCanvas]);

	useEffect(() => {
		redraw();
		const onResize = () => redraw();
		window.addEventListener("resize", onResize);
		return () => window.removeEventListener("resize", onResize);
	}, [redraw]);

	useEffect(() => {
		setPoints([]);
	}, [step]);

	const handlePointerDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
		const canvas = canvasRef.current;
		const wrap = wrapRef.current;
		if (!canvas || !wrap) return;
		const rect = canvas.getBoundingClientRect();
		const cx = e.clientX - rect.left;
		const cy = e.clientY - rect.top;
		const pw = canvas.width;
		const ph = canvas.height;

		if (isWheel) {
			if (points.length < 3) {
				setPoints((prev) => [...prev, canvasToImage(cx, cy, pw, ph)]);
				return;
			}
			for (let i = 0; i < 3; i++) {
				const [hx, hy] = imageToCanvas(points[i][0], points[i][1], pw, ph);
				if (Math.abs(cx - hx) <= HANDLE_R && Math.abs(cy - hy) <= HANDLE_R) {
					dragIndex.current = i;
					return;
				}
			}
		} else {
			setPoints((prev) => [...prev, canvasToImage(cx, cy, pw, ph)]);
		}
	};

	const handlePointerMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
		if (dragIndex.current === null) return;
		const canvas = canvasRef.current;
		if (!canvas) return;
		const rect = canvas.getBoundingClientRect();
		const cx = e.clientX - rect.left;
		const cy = e.clientY - rect.top;
		const pw = canvas.width;
		const ph = canvas.height;
		const idx = dragIndex.current;
		setPoints((prev) => {
			const next = [...prev];
			next[idx] = canvasToImage(cx, cy, pw, ph);
			return next;
		});
	};

	const handlePointerUp = () => {
		dragIndex.current = null;
	};

	const handleDone = () => {
		let crop: HTMLCanvasElement | null = null;
		if (isWheel) {
			if (points.length !== 3) {
				alert("Place exactly 3 points: center, then two on the rim.");
				return;
			}
			crop = extractEllipseCrop(sourceCanvas, points[0], points[1], points[2]);
		} else {
			if (points.length < 3) {
				alert("Add at least 3 points, then click Done.");
				return;
			}
			crop = extractPolygonCrop(sourceCanvas, points);
		}
		if (!crop) {
			alert("Could not extract crop.");
			return;
		}
		onDone(crop);
	};

	return (
		<div>
			<p className="instruction">{instruction}</p>
			<div className="canvas-wrap" ref={wrapRef}>
				<canvas
					ref={canvasRef}
					onMouseDown={handlePointerDown}
					onMouseMove={handlePointerMove}
					onMouseUp={handlePointerUp}
					onMouseLeave={handlePointerUp}
				/>
			</div>
			<div className="btn-row">
				<button type="button" className="btn btn-primary" onClick={handleDone}>
					Done
				</button>
			</div>
		</div>
	);
}

function AlignCarView({
	origCanvas,
	origW,
	origH,
	newCanvas,
	overlayX,
	overlayY,
	overlayScale,
	onOverlayX,
	onOverlayY,
	onOverlayScale,
	onContinue,
}: {
	origCanvas: HTMLCanvasElement;
	origW: number;
	origH: number;
	newCanvas: HTMLCanvasElement;
	overlayX: number;
	overlayY: number;
	overlayScale: number;
	onOverlayX: (v: number) => void;
	onOverlayY: (v: number) => void;
	onOverlayScale: (v: number) => void;
	onContinue: () => void;
}) {
	const canvasRef = useRef<HTMLCanvasElement>(null);
	const wrapRef = useRef<HTMLDivElement>(null);

	const redraw = useCallback(() => {
		const canvas = canvasRef.current;
		const wrap = wrapRef.current;
		if (!canvas || !wrap) return;
		const maxW = wrap.clientWidth;
		const maxH = Math.max(540, wrap.clientHeight || 540);
		const preview = buildCompositePreview(
			origCanvas,
			origW,
			origH,
			newCanvas,
			overlayX,
			overlayY,
			overlayScale,
			[maxW, maxH],
		);
		canvas.width = preview.width;
		canvas.height = preview.height;
		canvas.getContext("2d")!.drawImage(preview, 0, 0);
	}, [origCanvas, origW, origH, newCanvas, overlayX, overlayY, overlayScale]);

	useEffect(() => {
		redraw();
		const onResize = () => redraw();
		window.addEventListener("resize", onResize);
		return () => window.removeEventListener("resize", onResize);
	}, [redraw]);

	return (
		<div>
			<p className="instruction">
				Position and scale your car to fit within the reference image. Match wheel wells
				as closely as possible.
			</p>
			<div className="canvas-wrap align-mode" ref={wrapRef}>
				<canvas ref={canvasRef} style={{ cursor: "default" }} />
			</div>
			<div className="controls-grid">
				<div className="control-group">
					<div className="control-group-title">Align new car</div>
					<SliderControl label="X:" min={-origW} max={origW} value={overlayX} onChange={onOverlayX} />
					<SliderControl label="Y:" min={-origH} max={origH} value={overlayY} onChange={onOverlayY} />
					<SliderControl
						label="Scale:"
						min={Math.round(OVERLAY_SCALE_MIN * 100)}
						max={Math.round(OVERLAY_SCALE_MAX * 100)}
						value={Math.round(overlayScale * 100)}
						onChange={(v) =>
							onOverlayScale(Math.max(OVERLAY_SCALE_MIN, Math.min(OVERLAY_SCALE_MAX, v / 100)))
						}
					/>
				</div>
			</div>
			<div className="btn-row">
				<button type="button" className="btn btn-primary" onClick={onContinue}>
					Continue to wheel alignment
				</button>
			</div>
		</div>
	);
}

function AlignWheelsView({
	wheelsCanvas,
	wheelsW,
	wheelsH,
	wheelCrops,
	carExportCanvas,
	w1x,
	w1y,
	w2x,
	w2y,
	wheelScale,
	onW1x,
	onW1y,
	onW2x,
	onW2y,
}: {
	wheelsCanvas: HTMLCanvasElement;
	wheelsW: number;
	wheelsH: number;
	wheelCrops: (HTMLCanvasElement | null)[];
	carExportCanvas: HTMLCanvasElement | null;
	w1x: number;
	w1y: number;
	w2x: number;
	w2y: number;
	wheelScale: number;
	onW1x: (v: number) => void;
	onW1y: (v: number) => void;
	onW2x: (v: number) => void;
	onW2y: (v: number) => void;
}) {
	const canvasRef = useRef<HTMLCanvasElement>(null);
	const wrapRef = useRef<HTMLDivElement>(null);

	const redraw = useCallback(() => {
		const canvas = canvasRef.current;
		const wrap = wrapRef.current;
		if (!canvas || !wrap) return;
		const maxW = wrap.clientWidth;
		const maxH = Math.max(540, wrap.clientHeight || 540);
		const preview = buildWheelsCompositePreview(
			wheelsCanvas,
			wheelsW,
			wheelsH,
			wheelCrops,
			w1x,
			w1y,
			wheelScale,
			w2x,
			w2y,
			wheelScale,
			[maxW, maxH],
			carExportCanvas,
		);
		canvas.width = preview.width;
		canvas.height = preview.height;
		canvas.getContext("2d")!.drawImage(preview, 0, 0);
	}, [
		wheelsCanvas,
		wheelsW,
		wheelsH,
		wheelCrops,
		w1x,
		w1y,
		w2x,
		w2y,
		wheelScale,
		carExportCanvas,
	]);

	useEffect(() => {
		redraw();
		const onResize = () => redraw();
		window.addEventListener("resize", onResize);
		return () => window.removeEventListener("resize", onResize);
	}, [redraw]);

	return (
		<div>
			<p className="instruction">
				Align wheels on the reference. A faint car outline helps with wheel-well placement.
				Try X adjustments first, then Y if needed.
			</p>
			<div className="canvas-wrap align-mode" ref={wrapRef}>
				<canvas ref={canvasRef} style={{ cursor: "default" }} />
			</div>
			<div className="controls-grid">
				<div className="control-group">
					<div className="control-group-title">Wheel 1</div>
					<SliderControl label="X:" min={-wheelsW} max={wheelsW} value={w1x} onChange={onW1x} />
					<SliderControl label="Y:" min={-wheelsH} max={wheelsH} value={w1y} onChange={onW1y} />
				</div>
				<div className="control-group">
					<div className="control-group-title">Wheel 2</div>
					<SliderControl label="X:" min={-wheelsW} max={wheelsW} value={w2x} onChange={onW2x} />
					<SliderControl label="Y:" min={-wheelsH} max={wheelsH} value={w2y} onChange={onW2y} />
				</div>
			</div>
		</div>
	);
}

function App() {
	const presets = window.PRESET_IMAGES || {};
	const deviceIds = Object.keys(presets).sort();
	const [device, setDevice] = useState(deviceIds[0] || "");
	const [step, setStep] = useState<WorkflowStep>(null);
	const [deviceLocked, setDeviceLocked] = useState(false);
	const [origCanvas, setOrigCanvas] = useState<HTMLCanvasElement | null>(null);
	const [wheelsCanvas, setWheelsCanvas] = useState<HTMLCanvasElement | null>(null);
	const [sourceCanvas, setSourceCanvas] = useState<HTMLCanvasElement | null>(null);
	const [wheelCrops, setWheelCrops] = useState<(HTMLCanvasElement | null)[]>([]);
	const [newCanvas, setNewCanvas] = useState<HTMLCanvasElement | null>(null);
	const [carExportCanvas, setCarExportCanvas] = useState<HTMLCanvasElement | null>(null);
	const [overlayX, setOverlayX] = useState(0);
	const [overlayY, setOverlayY] = useState(0);
	const [overlayScale, setOverlayScale] = useState(1);
	const [w1x, setW1x] = useState(0);
	const [w1y, setW1y] = useState(0);
	const [w2x, setW2x] = useState(0);
	const [w2y, setW2y] = useState(0);
	const [toast, setToast] = useState<string | null>(null);
	const [dragOver, setDragOver] = useState(false);
	const fileInputRef = useRef<HTMLInputElement>(null);

	const preset = presets[device];
	const origW = preset?.carW ?? 0;
	const origH = preset?.carH ?? 0;
	const wheelsW = preset?.wheelsW ?? 0;
	const wheelsH = preset?.wheelsH ?? 0;

	useEffect(() => {
		if (!preset) return;
		loadImageCanvas(preset.car).then(setOrigCanvas).catch(console.error);
	}, [device, preset?.car]);

	const showToast = (msg: string) => {
		setToast(msg);
		setTimeout(() => setToast(null), 3500);
	};

	const resetWorkflow = () => {
		setStep(null);
		setDeviceLocked(false);
		setSourceCanvas(null);
		setWheelCrops([]);
		setNewCanvas(null);
		setCarExportCanvas(null);
	};

	const loadNewImage = async (file: File) => {
		if (!origCanvas) {
			alert("Select a device with a valid reference image first.");
			return;
		}
		if (!file.type.match(/^image\/(png|jpeg|jpg)$/i) && !file.name.match(/\.(png|jpe?g)$/i)) {
			alert("Please choose a PNG or JPG file.");
			return;
		}
		try {
			const canvas = await loadFileCanvas(file);
			setSourceCanvas(canvas);
			setWheelCrops([]);
			setNewCanvas(null);
			setStep("outline_wheel1");
			setDeviceLocked(true);
		} catch {
			alert("Could not open image.");
		}
	};

	const handleFileInput = (e: React.ChangeEvent<HTMLInputElement>) => {
		const file = e.target.files?.[0];
		if (file) loadNewImage(file);
		e.target.value = "";
	};

	const handleDrop = (e: React.DragEvent) => {
		e.preventDefault();
		setDragOver(false);
		const file = e.dataTransfer.files?.[0];
		if (file) loadNewImage(file);
	};

	const handleOutlineDone = (crop: HTMLCanvasElement) => {
		if (step === "outline_wheel1") {
			setWheelCrops((prev) => [...prev, crop]);
			setStep("outline_wheel2");
		} else if (step === "outline_wheel2") {
			setWheelCrops((prev) => [...prev, crop]);
			setStep("outline_car");
		} else if (step === "outline_car") {
			setNewCanvas(crop);
			const nw = crop.width;
			const nh = crop.height;
			let scale = Math.min(origW / nw, origH / nh, 1);
			scale = Math.max(0.1, Math.min(2, scale));
			const newW = Math.floor(nw * scale);
			const newH = Math.floor(nh * scale);
			setOverlayX(Math.floor((origW - newW) / 2));
			setOverlayY(Math.floor((origH - newH) / 2));
			setOverlayScale(scale);
			setStep("align_car");
		}
	};

	const handleContinueFromCarAlign = async () => {
		if (!newCanvas || !preset) return;
		const exp = buildExportImage(origW, origH, newCanvas, overlayX, overlayY, overlayScale);
		setCarExportCanvas(exp);

		try {
			const wc = await loadImageCanvas(preset.wheels);
			setWheelsCanvas(wc);
			const ww = wheelsW;
			const wh = wheelsH;
			const s = Math.max(OVERLAY_SCALE_MIN, Math.min(OVERLAY_SCALE_MAX, overlayScale));
			const crops = wheelCrops;
			if (crops[0]) {
				const newH = Math.floor(crops[0].height * s);
				setW1x(0);
				setW1y(Math.floor((wh - newH) / 2));
			}
			if (crops[1]) {
				const newW = Math.floor(crops[1].width * s);
				const newH = Math.floor(crops[1].height * s);
				setW2x(ww - newW);
				setW2y(Math.floor((wh - newH) / 2));
			}
			setStep("align_wheels");
			showToast("Align the wheels, then upload to your controller below.");
		} catch {
			alert("img_wheels.png reference could not be loaded.");
		}
	};

	const outlineInstruction =
		step === "outline_wheel1"
			? "Step 1: Outline the first wheel. Click center, then two points on the rim. Drag points to adjust. Done when ready."
			: step === "outline_wheel2"
				? "Step 2: Outline the second wheel. Click center, then two points on the rim. Drag points to adjust. Done when ready."
				: "Step 3: Outline the car. Click to add points around the car (excluding wheels). Click Done when finished.";

	if (!deviceIds.length) {
		return (
			<div className="editor">
				<p>No device presets found. Run <code>npm run build</code> with preset PNGs present.</p>
			</div>
		);
	}

	return (
		<div className="editor">
			<header className="app-header">
				<div className="brand">
					<img
						className="brand-logo"
						src="https://oasman.dev/docs/assets/images/oasman_logo.jpg"
						alt="OAS-Man"
					/>
					<div>
						<div className="brand-name">Car Creator</div>
						<div className="brand-sub">Align &amp; upload custom car images to your controller</div>
					</div>
				</div>
				<Stepper current={step} />
			</header>

			<div className="panel">
				<div className="panel-title">Device</div>
				<div className="field-row">
					<span className="field-label">Device lib:</span>
					<select
						value={device}
						disabled={deviceLocked}
						onChange={(e) => {
							setDevice(e.target.value);
							resetWorkflow();
						}}
					>
						{deviceIds.map((id) => (
							<option key={id} value={id}>
								{presets[id].name} ({id})
							</option>
						))}
					</select>
				</div>
			</div>

			{step === null && (
				<div className="panel">
					<div className="panel-title">img_car.png — drop PNG/JPG or click to browse</div>
					<input
						ref={fileInputRef}
						type="file"
						className="hidden-input"
						accept="image/png,image/jpeg,.png,.jpg,.jpeg"
						onChange={handleFileInput}
					/>
					<div
						className={"drop-zone" + (dragOver ? " drag-over" : "")}
						onClick={() => fileInputRef.current?.click()}
						onDragOver={(e) => {
							e.preventDefault();
							setDragOver(true);
						}}
						onDragLeave={() => setDragOver(false)}
						onDrop={handleDrop}
					>
						<p>Select a device, then drop a PNG or JPG</p>
						<p>or click to load a new car image</p>
						{origCanvas && (
							<p style={{ marginTop: 16, fontSize: 12 }}>
								Reference: {origW}×{origH}px
							</p>
						)}
					</div>
				</div>
			)}

			{(step === "outline_wheel1" || step === "outline_wheel2" || step === "outline_car") &&
				sourceCanvas && (
					<div className="panel">
						<div className="panel-title">Outline</div>
						<OutlineView
							step={step}
							sourceCanvas={sourceCanvas}
							instruction={outlineInstruction}
							onDone={handleOutlineDone}
						/>
					</div>
				)}

			{step === "align_car" && origCanvas && newCanvas && (
				<div className="panel">
					<div className="panel-title">img_car.png — align new car</div>
					<AlignCarView
						origCanvas={origCanvas}
						origW={origW}
						origH={origH}
						newCanvas={newCanvas}
						overlayX={overlayX}
						overlayY={overlayY}
						overlayScale={overlayScale}
						onOverlayX={setOverlayX}
						onOverlayY={setOverlayY}
						onOverlayScale={setOverlayScale}
						onContinue={handleContinueFromCarAlign}
					/>
				</div>
			)}

			{step === "align_wheels" && wheelsCanvas && wheelCrops.length >= 2 && (
				<div className="panel">
					<div className="panel-title">img_wheels.png — align wheels</div>
					<AlignWheelsView
						wheelsCanvas={wheelsCanvas}
						wheelsW={wheelsW}
						wheelsH={wheelsH}
						wheelCrops={wheelCrops}
						carExportCanvas={carExportCanvas}
						w1x={w1x}
						w1y={w1y}
						w2x={w2x}
						w2y={w2y}
						wheelScale={overlayScale}
						onW1x={setW1x}
						onW1y={setW1y}
						onW2x={setW2x}
						onW2y={setW2y}
					/>
				</div>
			)}

			{step === "align_wheels" && wheelsCanvas && wheelCrops.length >= 2 && carExportCanvas && (
				<div className="panel">
					<div className="panel-title">Upload to controller (USB)</div>
					<SerialUploadPanel
						carCanvas={carExportCanvas}
						carW={origW}
						carH={origH}
						wheelsW={wheelsW}
						wheelsH={wheelsH}
						wheelCrops={wheelCrops}
						w1x={w1x}
						w1y={w1y}
						w2x={w2x}
						w2y={w2y}
						wheelScale={overlayScale}
					/>
				</div>
			)}

			{toast && <div className="toast">{toast}</div>}
		</div>
	);
}

const rootEl = document.getElementById("root");
if (rootEl) {
	ReactDOM.createRoot(rootEl).render(React.createElement(App));
}
