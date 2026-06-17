// OAS-Man web installer — multi-step React flow.
// Compiled to index.html by build.mjs (esbuild). React / ReactDOM are provided
// as UMD globals (loaded via <script> in template.html) rather than ES imports.

declare const React: {
	createElement: any;
	Fragment: any;
	useState<T>(initial: T | (() => T)): [T, (next: T | ((prev: T) => T)) => void];
	useEffect(effect: () => void | (() => void), deps?: any[]): void;
	useRef<T>(initial: T): { current: T };
	useMemo<T>(factory: () => T, deps?: any[]): T;
};
declare const ReactDOM: { createRoot(el: Element): { render(node: any): void } };

const { useState, useEffect, useRef, useMemo } = React;

// ---------------------------------------------------------------------------
// Static device catalog. The `id` MUST match the firmware asset prefix used in
// the GitHub releases (e.g. `${id}_firmware.bin`).
// ---------------------------------------------------------------------------
type Category = "Manifold" | "Controller";
type DeviceGroup = "Manifold" | "Controller" | "Legacy";

interface Device {
	id: string;
	name: string;
	group: DeviceGroup;
	desc?: string;
	link?: string;
}

// Note: the CYD boards are legacy *controllers*, so they live under the
// Controller category (group "Legacy") alongside the current controllers.

const DEVICES: Device[] = [
	{ id: "manifold_v4", name: "Board V4 Manifold", group: "Manifold", desc: "Latest manifold board" },
	{ id: "manifold_v3", name: "Board V3 Manifold", group: "Manifold" },
	{
		id: "manifold_v2",
		name: "Board V2 Manifold",
		group: "Manifold",
		desc: "No acc wire functionality, will work for some V3 boards also",
	},
	{
		id: "controller_ws2p8",
		name: '2.8" Waveshare Controller',
		group: "Controller",
		link: "https://www.waveshare.com/esp32-s3-touch-lcd-2.8.htm?sku=27690",
	},
	{
		id: "controller_ws2p8b",
		name: '2.8" B Model Waveshare Controller',
		group: "Controller",
		link: "https://www.waveshare.com/esp32-s3-touch-lcd-2.8b.htm?sku=30103",
	},
	{
		id: "controller_ws3p5",
		name: '3.5" Waveshare Controller',
		group: "Controller",
		link: "https://www.waveshare.com/esp32-s3-touch-lcd-3.5.htm?sku=30733",
	},
	{
		id: "controller_ws3p5b",
		name: '3.5" B Model Waveshare Controller',
		group: "Controller",
		link: "https://www.waveshare.com/esp32-s3-touch-lcd-3.5b.htm?sku=31137",
	},
	{
		id: "controller_ws1p8knob",
		name: '1.8" Rotary Gauge Waveshare Controller',
		group: "Controller",
		link: "https://www.waveshare.com/esp32-s3-knob-touch-lcd-1.8.htm?sku=31623",
	},
	{ id: "controller_cyd3p2", name: '3.2" CYD Capacitive Controller', group: "Legacy" },
	{ id: "controller_cyd3p2_resistive", name: '3.2" CYD Resistive Controller', group: "Legacy" },
];

const GROUP_META: Record<DeviceGroup, { label: string; note?: string }> = {
	Manifold: { label: "Manifold" },
	Controller: { label: "Controller" },
	Legacy: { label: "Legacy Devices", note: "No longer updated" },
};

// ---------------------------------------------------------------------------
// Firmware release types + fetching.
// ---------------------------------------------------------------------------
interface ReleaseAsset {
	name: string;
	size: number;
	downloadUrl: string;
}

interface Release {
	name: string;
	tag: string;
	date: Date;
	body: string;
	assets: ReleaseAsset[];
}

async function fetchFirmwareReleases(): Promise<Release[]> {
	const response = await fetch(
		"https://api.github.com/repos/gopro2027/ArduinoAirSuspensionController/releases"
	);
	const releases = await response.json();
	return releases.map((release: any) => ({
		name: release.name,
		tag: release.tag_name,
		date: new Date(release.created_at),
		body: release.body,
		assets: (release.assets || []).map((asset: any) => ({
			name: asset.name,
			size: asset.size,
			downloadUrl: asset.browser_download_url,
		})),
	}));
}

// GitHub release direct links block CORS, so route through the caching reverse
// proxy (same one the ESP32s use for OTA).
function getReleaseAsset(boardName: string, tagName: string, subscript: string): string {
	return `https://githubreleasebinary-http-proxy.gopro2027.workers.dev/?url=https://github.com/gopro2027/ArduinoAirSuspensionController/releases/download/${tagName}/${boardName}${subscript}`;
}

// Builds an esp-web-tools manifest blob URL. Implemented in manifestgen.js,
// which is loaded as a classic <script> in template.html (global function).
declare function generate_manifest(
	name: string,
	version: string,
	bootloader: string,
	partitions: string,
	firmware: string
): string;

// Markdown rendering for release notes (loaded from CDN in template.html).
declare const marked: { parse(md: string): string };
declare const DOMPurify: { sanitize(html: string): string };

// The manifold runs an ESP32-WROOM; the controllers are ESP32-S3.
function chipFamilyForCategory(category: Category | null): string {
	if (category === "Manifold") return "ESP32";
	if (category === "Controller") return "ESP32-S3";
	return "ESP32";
}

function renderMarkdown(md: string): { __html: string } {
	try {
		return { __html: DOMPurify.sanitize(marked.parse(md || "")) };
	} catch {
		return { __html: "" };
	}
}

// Friendly names for the common USB-serial bridge chips on ESP32 boards.
const USB_VENDORS: Record<number, string> = {
	0x10c4: "Silicon Labs CP210x",
	0x1a86: "WCH CH34x",
	0x0403: "FTDI",
	0x303a: "Espressif (native USB)",
};

// ---------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------
function BackArrow() {
	return (
		<svg viewBox="0 0 24 24" width="20" height="20" fill="none" stroke="currentColor" strokeWidth="2">
			<path d="M19 12H5M12 19l-7-7 7-7" strokeLinecap="round" strokeLinejoin="round" />
		</svg>
	);
}

function Chevron() {
	return (
		<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" strokeWidth="2">
			<path d="M9 18l6-6-6-6" strokeLinecap="round" strokeLinejoin="round" />
		</svg>
	);
}

function Check() {
	return (
		<svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" strokeWidth="3">
			<path d="M20 6L9 17l-5-5" strokeLinecap="round" strokeLinejoin="round" />
		</svg>
	);
}

// ---------------------------------------------------------------------------
// Step header / progress indicator
// ---------------------------------------------------------------------------
const STEP_LABELS = ["Type", "Device", "Version", "Install"];

function Stepper({ step }: { step: number }) {
	return (
		<div className="stepper">
			{STEP_LABELS.map((label, i) => (
				<div className="stepper-item" key={label}>
					<div className={"stepper-dot" + (i < step ? " done" : i === step ? " active" : "")}>
						{i < step ? <Check /> : i + 1}
					</div>
					<span className={"stepper-label" + (i === step ? " active" : "")}>{label}</span>
					{i < STEP_LABELS.length - 1 && (
						<div className={"stepper-line" + (i < step ? " done" : "")} />
					)}
				</div>
			))}
		</div>
	);
}

// ---------------------------------------------------------------------------
// The esp-web-install-button is a custom element; set its `.manifest` property
// imperatively via a ref.
// ---------------------------------------------------------------------------
function InstallButton({ manifest }: { manifest: string | null }) {
	const ref = useRef<any>(null);
	useEffect(() => {
		if (ref.current && manifest) {
			ref.current.manifest = manifest;
		}
	}, [manifest]);

	if (!manifest) return null;

	return React.createElement(
		"esp-web-install-button",
		{ ref, class: "oas-install-btn" },
		React.createElement("button", { slot: "activate", className: "btn btn-primary btn-install" }, "Install firmware"),
		React.createElement(
			"span",
			{ slot: "unsupported", className: "install-note" },
			"Your browser does not support installing firmware. Try Chrome or Edge on desktop."
		),
		React.createElement(
			"span",
			{ slot: "not-allowed", className: "install-note" },
			"This page must be served over https to install firmware."
		)
	);
}

// ---------------------------------------------------------------------------
// Step 1 — category selection (Manifold vs Controller)
// ---------------------------------------------------------------------------
const CATEGORIES: { id: Category; name: string; desc: string }[] = [
	{
		id: "Manifold",
		name: "Manifold",
		desc: "The board bolted to your air manifold — controls the valves, compressor and pressure sensors.",
	},
	{
		id: "Controller",
		name: "Controller",
		desc: "The wireless touchscreen / knob display that remotely controls the manifold.",
	},
];

function CategoryStep({
	selected,
	onSelect,
}: {
	selected: Category | null;
	onSelect: (c: Category) => void;
}) {
	return (
		<div className="step">
			<h2 className="step-title">What are you flashing?</h2>
			<p className="step-sub">Choose the type of device you want to install OAS-Man onto.</p>
			<div className="card-grid">
				{CATEGORIES.map((c) => (
					<button
						key={c.id}
						className={"device-card" + (selected === c.id ? " selected" : "")}
						onClick={() => onSelect(c.id)}
					>
						<div className="device-card-main">
							<span className="device-name">{c.name}</span>
							<span className="device-desc">{c.desc}</span>
						</div>
						<span className="device-chevron">
							<Chevron />
						</span>
					</button>
				))}
			</div>
		</div>
	);
}

// ---------------------------------------------------------------------------
// Step 2 — device selection (filtered by the chosen category)
// ---------------------------------------------------------------------------
function DeviceStep({
	category,
	selected,
	onSelect,
}: {
	category: Category | null;
	selected: Device | null;
	onSelect: (d: Device) => void;
}) {
	// Manifold category -> just the Manifold group. Controller category ->
	// current controllers plus the legacy controllers.
	const groups: DeviceGroup[] =
		category === "Manifold" ? ["Manifold"] : ["Controller", "Legacy"];
	return (
		<div className="step">
			<h2 className="step-title">Select your device</h2>
			<p className="step-sub">Pick the board you want to flash OAS-Man onto.</p>
			{groups.map((g) => (
				<div className="group" key={g}>
					<div className="group-head">
						<span className="group-label">{GROUP_META[g].label}</span>
						{GROUP_META[g].note && <span className="group-note">{GROUP_META[g].note}</span>}
					</div>
					<div className="card-grid">
						{DEVICES.filter((d) => d.group === g).map((d) => (
							<button
								key={d.id}
								className={"device-card" + (selected?.id === d.id ? " selected" : "")}
								onClick={() => onSelect(d)}
							>
								<div className="device-card-main">
									<span className="device-name">{d.name}</span>
									{d.desc && <span className="device-desc">{d.desc}</span>}
									{d.link && (
										<a
											className="device-link"
											href={d.link}
											target="_blank"
											rel="noreferrer"
											onClick={(e) => e.stopPropagation()}
										>
											Product page ↗
										</a>
									)}
								</div>
								<span className="device-chevron">
									<Chevron />
								</span>
							</button>
						))}
					</div>
				</div>
			))}
		</div>
	);
}

// ---------------------------------------------------------------------------
// Step 2 — version selection
// ---------------------------------------------------------------------------
function VersionStep({
	device,
	releases,
	loading,
	error,
	selected,
	onSelect,
}: {
	device: Device | null;
	releases: Release[];
	loading: boolean;
	error: string | null;
	selected: Release | null;
	onSelect: (r: Release) => void;
}) {
	const [showAll, setShowAll] = useState(false);

	// Releases that actually ship a firmware for this device.
	const available = useMemo(() => {
		if (!device) return [];
		return releases.filter((r) => r.assets.some((a) => a.name === `${device.id}_firmware.bin`));
	}, [device, releases]);

	useEffect(() => {
		setShowAll(false);
	}, [device]);

	const visible = showAll ? available : available.slice(0, 5);

	return (
		<div className="step">
			<h2 className="step-title">Select a version</h2>
			<p className="step-sub">
				Firmware versions available for <strong>{device?.name}</strong>.
			</p>

			{loading && <div className="status">Loading versions…</div>}
			{error && <div className="status error">{error}</div>}
			{!loading && !error && available.length === 0 && (
				<div className="status">No firmware releases found for this device.</div>
			)}

			<div className="version-list">
				{visible.map((r, i) => (
					<button
						key={r.tag}
						className={"version-card" + (selected?.tag === r.tag ? " selected" : "")}
						onClick={() => onSelect(r)}
					>
						<div className="version-radio">
							<span className="version-radio-dot" />
						</div>
						<div className="version-main">
							<span className="version-name">
								{r.name || r.tag}
								{i === 0 && <span className="badge">Latest</span>}
							</span>
							<span className="version-date">{r.date.toLocaleDateString()}</span>
						</div>
					</button>
				))}
			</div>

			{!showAll && available.length > 5 && (
				<button className="btn btn-ghost show-more" onClick={() => setShowAll(true)}>
					Show {available.length - 5} more {available.length - 5 === 1 ? "version" : "versions"}
				</button>
			)}
		</div>
	);
}

// ---------------------------------------------------------------------------
// Step 3 — install
// ---------------------------------------------------------------------------
function InstallStep({
	category,
	device,
	release,
	manifest,
}: {
	category: Category | null;
	device: Device | null;
	release: Release | null;
	manifest: string | null;
}) {
	const [showNotes, setShowNotes] = useState(false);
	const notes = release?.body?.trim();

	return (
		<div className="step">
			<h2 className="step-title">Ready to install</h2>
			<p className="step-sub">Connect your device via USB, then click install.</p>

			<div className="summary">
				<div className="summary-row">
					<span className="summary-key">Device</span>
					<span className="summary-val">{device?.name}</span>
				</div>
				<div className="summary-row">
					<span className="summary-key">Chip</span>
					<span className="summary-val">{chipFamilyForCategory(category)}</span>
				</div>
				<div className="summary-row">
					<span className="summary-key">Version</span>
					<span className="summary-val">{release?.name || release?.tag}</span>
				</div>
			</div>

			{notes && (
				<div className="notes">
					<button
						className="notes-toggle"
						onClick={() => setShowNotes((s) => !s)}
						aria-expanded={showNotes}
					>
						<span className={"notes-caret" + (showNotes ? " open" : "")}>
							<Chevron />
						</span>
						What's new in this version
					</button>
					{showNotes && (
						<div className="notes-body markdown" dangerouslySetInnerHTML={renderMarkdown(notes)} />
					)}
				</div>
			)}

			<div className="install-area">
				<InstallButton manifest={manifest} />
			</div>

			<p className="install-hint">
				Use Chrome or Edge on desktop. A USB cable that supports data is required.
			</p>
			
		</div>
	);
}

// ---------------------------------------------------------------------------
// Device Console — a Web Serial debug/management panel (read logs, send
// commands, restart the device). Independent of the install flow.
// ---------------------------------------------------------------------------
const SERIAL_SUPPORTED = typeof navigator !== "undefined" && "serial" in navigator;
const MAX_LOG_CHARS = 60000;

function SerialConsole() {
	const [open, setOpen] = useState(false);
	const [connected, setConnected] = useState(false);
	const [busy, setBusy] = useState(false);
	const [log, setLog] = useState("");
	const [info, setInfo] = useState<string | null>(null);
	const [cmd, setCmd] = useState("");

	const portRef = useRef<any>(null);
	const readerRef = useRef<any>(null);
	const keepReadingRef = useRef(false);
	const logEndRef = useRef<any>(null);

	useEffect(() => {
		if (logEndRef.current) logEndRef.current.scrollTop = logEndRef.current.scrollHeight;
	}, [log]);

	function append(text: string) {
		setLog((prev) => {
			const next = prev + text;
			return next.length > MAX_LOG_CHARS ? next.slice(next.length - MAX_LOG_CHARS) : next;
		});
	}

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
					if (value) append(decoder.decode(value));
				}
			} catch (e) {
				append(`\n[read error: ${e}]\n`);
			} finally {
				reader.releaseLock();
				readerRef.current = null;
			}
		}
	}

	async function connect() {
		setBusy(true);
		try {
			const port = await (navigator as any).serial.requestPort();
			await port.open({ baudRate: 115200 });
			portRef.current = port;
			const i = port.getInfo ? port.getInfo() : {};
			const vendor = i.usbVendorId ? USB_VENDORS[i.usbVendorId] || `USB ${i.usbVendorId.toString(16)}` : null;
			setInfo(vendor ? `${vendor} @ 115200 baud` : "Connected @ 115200 baud");
			setConnected(true);
			keepReadingRef.current = true;
			readLoop();
		} catch (e: any) {
			if (e && e.name !== "NotFoundError") append(`\n[connect error: ${e.message || e}]\n`);
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
			setInfo(null);
			setBusy(false);
		}
	}

	async function writeBytes(data: Uint8Array) {
		const port = portRef.current;
		if (!port || !port.writable) return;
		const writer = port.writable.getWriter();
		try {
			await writer.write(data);
		} finally {
			writer.releaseLock();
		}
	}

	async function sendCommand(e: any) {
		e.preventDefault();
		if (!connected || !cmd) return;
		append(`> ${cmd}\n`);
		await writeBytes(new TextEncoder().encode(cmd + "\n"));
		setCmd("");
	}

	// Pulse EN low (RTS) to reset the ESP32 into a normal boot.
	async function restart() {
		const port = portRef.current;
		if (!port || !port.setSignals) return;
		setBusy(true);
		append("\n[restarting device…]\n");
		try {
			await port.setSignals({ dataTerminalReady: false, requestToSend: true });
			await new Promise((r) => setTimeout(r, 120));
			await port.setSignals({ dataTerminalReady: false, requestToSend: false });
		} catch (e: any) {
			append(`\n[restart error: ${e.message || e}]\n`);
		} finally {
			setBusy(false);
		}
	}

	async function copyLog() {
		try {
			await navigator.clipboard.writeText(log);
		} catch {}
	}

	return (
		<div className="console-panel">
			<button className="console-head" onClick={() => setOpen((o) => !o)} aria-expanded={open}>
				<span className={"console-caret" + (open ? " open" : "")}>
					<Chevron />
				</span>
				<span className="console-title">Device Console</span>
				<span className={"console-status" + (connected ? " on" : "")}>
					{connected ? "Connected" : "Disconnected"}
				</span>
			</button>

			{open && (
				<div className="console-body">
					{!SERIAL_SUPPORTED ? (
						<p className="status">
							Web Serial isn't available in this browser. Use Chrome or Edge on desktop to view
							device logs.
						</p>
					) : (
						<>
							<div className="console-toolbar">
								{!connected ? (
									<button className="btn btn-primary" onClick={connect} disabled={busy}>
										Connect
									</button>
								) : (
									<button className="btn btn-ghost" onClick={disconnect} disabled={busy}>
										Disconnect
									</button>
								)}
								<button className="btn btn-ghost" onClick={restart} disabled={!connected || busy}>
									Restart device
								</button>
								<button className="btn btn-ghost" onClick={() => setLog("")} disabled={!log}>
									Clear
								</button>
								<button className="btn btn-ghost" onClick={copyLog} disabled={!log}>
									Copy
								</button>
								{info && <span className="console-info">{info}</span>}
							</div>

							<div className="console-log" ref={logEndRef}>
								{log || "Logs will appear here once connected. Serial runs at 115200 baud."}
							</div>

							<form className="console-input" onSubmit={sendCommand}>
								<input
									type="text"
									placeholder={connected ? "Type a command and press Enter…" : "Connect to send commands"}
									value={cmd}
									disabled={!connected}
									onChange={(e: any) => setCmd(e.target.value)}
								/>
								<button className="btn btn-ghost" type="submit" disabled={!connected || !cmd}>
									Send
								</button>
							</form>

							<p className="console-note">
								Note: the console and the installer can't use the serial port at the same time —
								disconnect here before flashing.
							</p>
						</>
					)}
				</div>
			)}
		</div>
	);
}

// ---------------------------------------------------------------------------
// Root app — owns step state + slide animation
// ---------------------------------------------------------------------------
function App() {
	const [step, setStep] = useState(0);
	const [releases, setReleases] = useState<Release[]>([]);
	const [loading, setLoading] = useState(true);
	const [error, setError] = useState<string | null>(null);

	const [category, setCategory] = useState<Category | null>(null);
	const [device, setDevice] = useState<Device | null>(null);
	const [release, setRelease] = useState<Release | null>(null);

	useEffect(() => {
		fetchFirmwareReleases()
			.then((list) => {
				setReleases(list);
				setLoading(false);
			})
			.catch((e) => {
				console.error("Error fetching releases:", e);
				setError("Could not load firmware releases. Check your connection and try again.");
				setLoading(false);
			});
	}, []);

	const manifest = useMemo(() => {
		if (!device || !release) return null;
		return generate_manifest(
			device.id,
			release.name || release.tag,
			getReleaseAsset(device.id, release.tag, "_bootloader.bin"),
			getReleaseAsset(device.id, release.tag, "_partitions.bin"),
			getReleaseAsset(device.id, release.tag, "_firmware.bin")
		);
	}, [device, release]);

	function handleSelectCategory(c: Category) {
		setCategory(c);
		setDevice(null);
		setRelease(null);
		setStep(1);
	}

	function handleSelectDevice(d: Device) {
		setDevice(d);
		setRelease(null);
		setStep(2);
	}

	function handleSelectVersion(r: Release) {
		setRelease(r);
		setStep(3);
	}

	function goBack() {
		setStep((s) => Math.max(0, s - 1));
	}

	return (
		<div className="installer">
			<header className="app-header">
				<div className="brand">
					<img
						className="brand-logo"
						src="https://oasman.dev/docs/assets/images/oasman_logo.jpg"
						alt="OAS-Man logo"
					/>
					<span className="brand-name">OAS-Man Installer</span>
				</div>
				<Stepper step={step} />
			</header>

			<div className="viewport">
				<div className="track" style={{ transform: `translateX(-${step * 100}%)` }}>
					<section className="slide">
						<button className="back-btn back-btn-spacer" tabIndex={-1} aria-hidden="true" />
						<CategoryStep selected={category} onSelect={handleSelectCategory} />
					</section>

					<section className="slide">
						<button className="back-btn" onClick={goBack}>
							<BackArrow />
							<span>Back</span>
						</button>
						<DeviceStep category={category} selected={device} onSelect={handleSelectDevice} />
					</section>

					<section className="slide">
						<button className="back-btn" onClick={goBack}>
							<BackArrow />
							<span>Back</span>
						</button>
						<VersionStep
							device={device}
							releases={releases}
							loading={loading}
							error={error}
							selected={release}
							onSelect={handleSelectVersion}
						/>
					</section>

					<section className="slide">
						<button className="back-btn" onClick={goBack}>
							<BackArrow />
							<span>Back</span>
						</button>
						<InstallStep category={category} device={device} release={release} manifest={manifest} />
					</section>
				</div>
			</div>

			<SerialConsole />

			<footer className="app-footer">
				<p>
					<a href="https://github.com/gopro2027/ArduinoAirSuspensionController">OAS-Man</a>
					{" "}&mdash; powered by{" "}
					<a href="https://esphome.github.io/esp-web-tools/">ESP Web Tools</a>.
				</p>
				<p className="footnote">
					* firmware_no_acc: for boards without constant battery + acc input (mostly manifold V2 and below).
				</p>
				<p className="footnote">
					<a href="https://play.google.com/store/apps/details?id=dev.oasman.oasman_mobile" target="_blank" rel="noreferrer">
						Download the Android app
					</a>{" "}
					&middot; iOS coming soon
				</p>
				<p className="footnote">❤️ oasman.dev</p>
				<p className="footnote konami">U,U,D,D,L,R,L,R,A,B</p>
			</footer>
		</div>
	);
}

const root = ReactDOM.createRoot(document.getElementById("root")!);
root.render(<App />);
