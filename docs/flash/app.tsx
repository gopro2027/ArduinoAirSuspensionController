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
	device,
	release,
	manifest,
}: {
	device: Device | null;
	release: Release | null;
	manifest: string | null;
}) {
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
					<span className="summary-key">Version</span>
					<span className="summary-val">{release?.name || release?.tag}</span>
				</div>
			</div>

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
						<InstallStep device={device} release={release} manifest={manifest} />
					</section>
				</div>
			</div>

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
