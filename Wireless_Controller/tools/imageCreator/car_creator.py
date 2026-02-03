"""
Car image creator tool - align a new car image (PNG/JPG) over the original and export as PNG.
"""
import os
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

try:
    from PIL import Image, ImageTk, ImageDraw
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    ImageDraw = None

try:
    from tkinterdnd2 import DND_FILES, TkinterDnD
    HAS_DND = True
except ImportError:
    HAS_DND = False

# Relative path from each device_lib folder to the preset images
PRESET_CAR_IMAGE = os.path.join("images", "presets", "img_car.png")
PRESET_WHEELS_IMAGE = os.path.join("images", "presets", "img_wheels.png")
PREVIEW_MIN = (400, 300)  # minimum size; canvases scale up with window
# Preview: original at full opacity; new car drawn more opaque so it’s easy to see
ORIGINAL_ALPHA = 1.0    # original car fully opaque (solid reference)
OVERLAY_ALPHA = 0.88    # new car more visible (was 0.5)


def get_script_dir():
    """Return the directory containing this script."""
    return os.path.dirname(os.path.abspath(__file__))


def get_device_libs_path():
    """Return the absolute path to the device_libs directory."""
    script_dir = get_script_dir()
    return os.path.normpath(os.path.join(script_dir, "..", "..", "device_libs"))


def get_img_car_path(folder_name):
    """Return the absolute path to images/presets/img_car.png for the given device_lib folder."""
    return os.path.join(get_device_libs_path(), folder_name, PRESET_CAR_IMAGE)


def get_img_wheels_path(folder_name):
    """Return the absolute path to images/presets/img_wheels.png for the given device_lib folder."""
    return os.path.join(get_device_libs_path(), folder_name, PRESET_WHEELS_IMAGE)


def get_presets_dir(folder_name):
    """Return the absolute path to images/presets for the given device_lib folder."""
    return os.path.join(get_device_libs_path(), folder_name, "images", "presets")


def list_device_lib_folders():
    """List each folder in device_libs (direct subdirectories only). Returns sorted list of names."""
    device_libs_path = get_device_libs_path()
    if not os.path.isdir(device_libs_path):
        return []
    entries = os.listdir(device_libs_path)
    folders = [
        name for name in entries
        if os.path.isdir(os.path.join(device_libs_path, name))
    ]
    return sorted(folders)


def _resample():
    return getattr(Image, "Resampling", Image).LANCZOS if hasattr(Image, "Resampling") else Image.LANCZOS


def _make_rgba(im):
    if im.mode != "RGBA":
        return im.convert("RGBA")
    return im.copy()


def extract_polygon_crop(image_pil, points_xy):
    """
    Crop the image to the polygon: pixels inside the polygon are kept,
    outside become transparent. Returns an RGBA image (bounding box with transparency outside shape).
    points_xy: list of (x, y) in image coordinates.
    """
    if not HAS_PIL or not ImageDraw or len(points_xy) < 3:
        return None
    img = _make_rgba(image_pil)
    w, h = img.size
    mask = Image.new("L", (w, h), 0)
    draw = ImageDraw.Draw(mask)
    draw.polygon(points_xy, fill=255)
    xs = [p[0] for p in points_xy]
    ys = [p[1] for p in points_xy]
    x0, x1 = max(0, int(min(xs))), min(w, int(max(xs)) + 1)
    y0, y1 = max(0, int(min(ys))), min(h, int(max(ys)) + 1)
    if x1 <= x0 or y1 <= y0:
        return None
    crop_img = img.crop((x0, y0, x1, y1))
    crop_mask = mask.crop((x0, y0, x1, y1))
    crop_img.putalpha(crop_mask)
    return crop_img


def build_composite_preview(orig_img, orig_size, new_img, overlay_x, overlay_y, overlay_scale, preview_max):
    """
    Build a preview image: original (dimmed) + new car (more opaque), at preview size.
    All coordinates and scale are in original image pixel space.
    """
    ow, oh = orig_size
    nw, nh = new_img.size
    scale_display = min(preview_max[0] / ow, preview_max[1] / oh, 1.0)
    pw, ph = int(ow * scale_display), int(oh * scale_display)

    base = orig_img.resize((pw, ph), _resample())
    base_rgba = _make_rgba(base)
    if ORIGINAL_ALPHA < 1.0:
        base_a = base_rgba.split()[3]
        base_a = base_a.point(lambda x: int(x * ORIGINAL_ALPHA))
        base_rgba.putalpha(base_a)
        background = Image.new("RGBA", (pw, ph), (40, 40, 40, 255))
        base_rgba = Image.alpha_composite(background, base_rgba)

    # New image size in original coords, then to preview coords
    new_w_orig = int(nw * overlay_scale)
    new_h_orig = int(nh * overlay_scale)
    new_w_preview = max(1, int(new_w_orig * scale_display))
    new_h_preview = max(1, int(new_h_orig * scale_display))
    x_preview = int(overlay_x * scale_display)
    y_preview = int(overlay_y * scale_display)

    overlay_resized = new_img.resize((new_w_preview, new_h_preview), _resample())
    overlay_rgba = _make_rgba(overlay_resized)
    a = overlay_rgba.split()[3]
    a = a.point(lambda x: int(x * OVERLAY_ALPHA))
    overlay_rgba.putalpha(a)

    layer = Image.new("RGBA", (pw, ph), (0, 0, 0, 0))
    layer.paste(overlay_rgba, (x_preview, y_preview), overlay_rgba)
    composite = Image.alpha_composite(base_rgba, layer)
    # Composite onto a visible background so transparent pixels don’t become black in RGB
    preview_bg = Image.new("RGBA", (pw, ph), (240, 240, 240, 255))
    composite = Image.alpha_composite(preview_bg, composite)
    return composite.convert("RGB")


def build_export_image(orig_size, new_img, overlay_x, overlay_y, overlay_scale):
    """
    Build the export image: same width/height as original, containing only the new
    car layer at the current position and scale (no original image).
    """
    ow, oh = orig_size
    nw, nh = new_img.size
    new_w = max(1, int(nw * overlay_scale))
    new_h = max(1, int(nh * overlay_scale))

    out = Image.new("RGBA", (ow, oh), (0, 0, 0, 0))
    resized = new_img.resize((new_w, new_h), _resample())
    resized_rgba = _make_rgba(resized)
    out.paste(resized_rgba, (overlay_x, overlay_y), resized_rgba)
    return out  # keep RGBA so PNG has transparent background


def build_wheels_composite_preview(wheels_img, wheels_size, wheel_crops, w1_x, w1_y, w1_scale, w2_x, w2_y, w2_scale, preview_max):
    """Build preview: img_wheels + both wheel overlays at OVERLAY_ALPHA."""
    ow, oh = wheels_size
    scale_display = min(preview_max[0] / ow, preview_max[1] / oh, 1.0)
    pw, ph = int(ow * scale_display), int(oh * scale_display)
    base = wheels_img.resize((pw, ph), _resample())
    base_rgba = _make_rgba(base)
    preview_bg = Image.new("RGBA", (pw, ph), (240, 240, 240, 255))
    composite = Image.alpha_composite(preview_bg, base_rgba)
    for i, (wc, x, y, scale) in enumerate([
        (wheel_crops[0], w1_x, w1_y, w1_scale),
        (wheel_crops[1], w2_x, w2_y, w2_scale),
    ]):
        if wc is None:
            continue
        nw, nh = wc.size
        new_w_orig = max(1, int(nw * scale))
        new_h_orig = max(1, int(nh * scale))
        new_w_preview = max(1, int(new_w_orig * scale_display))
        new_h_preview = max(1, int(new_h_orig * scale_display))
        x_preview = int(x * scale_display)
        y_preview = int(y * scale_display)
        overlay_resized = wc.resize((new_w_preview, new_h_preview), _resample())
        overlay_rgba = _make_rgba(overlay_resized)
        a = overlay_rgba.split()[3]
        a = a.point(lambda v: int(v * OVERLAY_ALPHA))
        overlay_rgba.putalpha(a)
        layer = Image.new("RGBA", (pw, ph), (0, 0, 0, 0))
        layer.paste(overlay_rgba, (x_preview, y_preview), overlay_rgba)
        composite = Image.alpha_composite(composite, layer)
    return composite.convert("RGB")


def build_wheels_export_image(wheels_size, wheel_crops, w1_x, w1_y, w1_scale, w2_x, w2_y, w2_scale):
    """Build export: same size as img_wheels, both wheels pasted at their positions."""
    ow, oh = wheels_size
    out = Image.new("RGBA", (ow, oh), (0, 0, 0, 0))
    for wc, x, y, scale in [(wheel_crops[0], w1_x, w1_y, w1_scale), (wheel_crops[1], w2_x, w2_y, w2_scale)]:
        if wc is None:
            continue
        nw, nh = wc.size
        new_w = max(1, int(nw * scale))
        new_h = max(1, int(nh * scale))
        resized = wc.resize((new_w, new_h), _resample())
        resized_rgba = _make_rgba(resized)
        out.paste(resized_rgba, (int(x), int(y)), resized_rgba)
    return out


def run_gui():
    """Main GUI: device lib selector, outline steps (wheels + car), then align car, then align wheels."""
    result = [None]

    root_class = TkinterDnD.Tk if HAS_DND else tk.Tk
    root = root_class()
    root.title("Car Creator – Align & Export")
    root.minsize(420, 560)
    root.resizable(True, True)

    folders = list_device_lib_folders()
    if not folders:
        folders = ["(no device_libs folders found)"]
        default = folders[0]
    else:
        default = folders[0]

    main = ttk.Frame(root, padding=12)
    main.pack(fill=tk.BOTH, expand=True)

    # --- Device lib ---
    ttk.Label(main, text="Device lib:").pack(anchor=tk.W)
    chosen = tk.StringVar(value=default)
    combo = ttk.Combobox(
        main, textvariable=chosen, values=folders,
        state="readonly", width=40,
    )
    combo.pack(fill=tk.X, pady=(0, 6))

    # --- Content holder: one of drop_frame, outline_frame, align_car_frame, align_wheels_frame visible ---
    content_frame = ttk.Frame(main)
    content_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 6))

    # --- State ---
    state = {
        "orig_img": None,
        "orig_size": (0, 0),
        "new_img": None,
        "overlay_x": 0,
        "overlay_y": 0,
        "overlay_scale": 1.0,
        "source_image": None,
        "wheel_crops": [],
        "car_crop": None,
        "workflow_step": None,  # None | "outline_wheel1" | "outline_wheel2" | "outline_car" | "align_car" | "align_wheels"
        "wheels_img": None,
        "wheels_size": (0, 0),
        "wheel1_x": 0, "wheel1_y": 0, "wheel1_scale": 1.0,
        "wheel2_x": 0, "wheel2_y": 0, "wheel2_scale": 1.0,
    }
    photo_ref = [None]
    controls_frame = [None]
    outline_photo_ref = [None]
    wheels_photo_ref = [None]

    def get_canvas_preview_size(c):
        """Return (width, height) for preview; use canvas size when mapped, else PREVIEW_MIN."""
        w = max(c.winfo_width(), 1)
        h = max(c.winfo_height(), 1)
        if w <= 1 or h <= 1:
            return PREVIEW_MIN
        return (w, h)

    # --- Drop frame (initial load) ---
    drop_frame = ttk.LabelFrame(content_frame, text="img_car.png — drop PNG/JPG or click to browse", padding=4)
    drop_canvas = tk.Canvas(drop_frame, width=PREVIEW_MIN[0], height=PREVIEW_MIN[1], bg="#2d2d2d", highlightthickness=1)
    drop_canvas.pack(fill=tk.BOTH, expand=True)
    drop_hint = drop_canvas.create_text(
        PREVIEW_MIN[0] // 2, PREVIEW_MIN[1] // 2,
        text="Select a device lib, then drop a PNG or JPG\nor click to load a new car image",
        fill="#888", font=("Segoe UI", 10), justify=tk.CENTER,
    )

    def on_drop_canvas_configure(event):
        if event.width > 10 and event.height > 10:
            drop_canvas.coords(drop_hint, event.width // 2, event.height // 2)
    drop_canvas.bind("<Configure>", on_drop_canvas_configure)

    # --- Outline frame (polygon tool) ---
    outline_frame = ttk.Frame(content_frame)
    outline_instruction = ttk.Label(outline_frame, text="", font=("Segoe UI", 10))
    outline_instruction.pack(anchor=tk.W, pady=(0, 4))
    outline_canvas = tk.Canvas(outline_frame, width=PREVIEW_MIN[0], height=PREVIEW_MIN[1], bg="#2d2d2d", highlightthickness=1)
    outline_canvas.pack(fill=tk.BOTH, expand=True)
    outline_btn_frame = ttk.Frame(outline_frame)
    outline_btn_frame.pack(fill=tk.X, pady=(6, 0))
    outline_done_btn = ttk.Button(outline_btn_frame, text="Done")
    outline_done_btn.pack(side=tk.LEFT)

    # --- Align car frame (existing car overlay UI) ---
    align_car_frame = ttk.Frame(content_frame)
    preview_lf = ttk.LabelFrame(align_car_frame, text="img_car.png — align new car", padding=4)
    preview_lf.pack(fill=tk.BOTH, expand=True, pady=(0, 6))
    canvas = tk.Canvas(preview_lf, width=PREVIEW_MIN[0], height=PREVIEW_MIN[1], bg="#2d2d2d", highlightthickness=1)
    canvas.pack(fill=tk.BOTH, expand=True)

    # --- Align wheels frame ---
    align_wheels_frame = ttk.Frame(content_frame)
    wheels_preview_lf = ttk.LabelFrame(align_wheels_frame, text="img_wheels.png — align wheels", padding=4)
    wheels_preview_lf.pack(fill=tk.BOTH, expand=True, pady=(0, 6))
    wheels_canvas = tk.Canvas(wheels_preview_lf, width=PREVIEW_MIN[0], height=PREVIEW_MIN[1], bg="#2d2d2d", highlightthickness=1)
    wheels_canvas.pack(fill=tk.BOTH, expand=True)
    wheels_controls_frame = ttk.LabelFrame(align_wheels_frame, text="Wheel position & size", padding=6)
    wheels_controls_frame.pack(fill=tk.X, pady=(0, 6))

    def show_content():
        """Show the appropriate content frame based on workflow_step and state."""
        for child in content_frame.winfo_children():
            child.pack_forget()
        step = state["workflow_step"]
        if step is None or state["source_image"] is None:
            drop_frame.pack(fill=tk.BOTH, expand=True)
            return
        if step in ("outline_wheel1", "outline_wheel2", "outline_car"):
            outline_frame.pack(fill=tk.BOTH, expand=True)
            if step == "outline_wheel1":
                outline_instruction.config(text="Step 1: Outline the first wheel. Click to add points, then click Done.")
            elif step == "outline_wheel2":
                outline_instruction.config(text="Step 2: Outline the second wheel. Click to add points, then click Done.")
            else:
                outline_instruction.config(text="Step 3: Outline the car. Click to add points, then click Done.")
            start_outline_step()
            return
        if step == "align_car":
            align_car_frame.pack(fill=tk.BOTH, expand=True)
            refresh_preview()
            if state["new_img"] is not None and controls_frame[0] is None:
                show_overlay_controls()
            return
        if step == "align_wheels":
            align_wheels_frame.pack(fill=tk.BOTH, expand=True)
            show_wheels_controls()
            refresh_wheels_preview()
            return
        drop_frame.pack(fill=tk.BOTH, expand=True)

    def load_original():
        folder = chosen.get()
        if folder.startswith("("):
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            show_content()
            return
        path = get_img_car_path(folder)
        if not HAS_PIL or not os.path.isfile(path):
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            show_content()
            return
        try:
            img = Image.open(path).convert("RGBA")
            state["orig_img"] = img
            state["orig_size"] = img.size
            state["new_img"] = None
            if controls_frame[0]:
                controls_frame[0].pack_forget()
                controls_frame[0] = None
            show_content()
        except Exception as e:
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            show_content()
            messagebox.showerror("Error", str(e))

    def load_new_png(filepath):
        if not HAS_PIL:
            messagebox.showerror("Error", "Pillow is required for image handling.")
            return
        if not state["orig_img"]:
            messagebox.showinfo("Info", "Select a device lib with a valid img_car.png first.")
            return
        try:
            source = Image.open(filepath).convert("RGBA")
        except Exception as e:
            messagebox.showerror("Error", f"Could not open image: {e}")
            return
        state["source_image"] = source
        state["wheel_crops"] = []
        state["car_crop"] = None
        state["workflow_step"] = "outline_wheel1"
        show_content()

    outline_points_ref = []  # stored in image coordinates
    outline_canvas_size_ref = [PREVIEW_MIN[0], PREVIEW_MIN[1]]
    outline_img_size_ref = [0, 0]

    def redraw_outline_view():
        """Redraw outline image and polygon at current canvas size (for resize)."""
        if state["source_image"] is None or state["workflow_step"] not in ("outline_wheel1", "outline_wheel2", "outline_car"):
            return
        src = state["source_image"]
        w, h = outline_img_size_ref[0], outline_img_size_ref[1]
        if w <= 0 or h <= 0:
            return
        pw, ph = outline_canvas_size_ref[0], outline_canvas_size_ref[1]
        if pw <= 0 or ph <= 0:
            return
        outline_canvas.delete("all")
        thumb = src.resize((pw, ph), _resample())
        outline_photo_ref[0] = ImageTk.PhotoImage(thumb)
        outline_canvas.create_image(0, 0, anchor=tk.NW, image=outline_photo_ref[0], tags="outline_img")
        pts_img = outline_points_ref
        if len(pts_img) >= 2:
            for i in range(len(pts_img) - 1):
                x1 = int(pts_img[i][0] * pw / w)
                y1 = int(pts_img[i][1] * ph / h)
                x2 = int(pts_img[i + 1][0] * pw / w)
                y2 = int(pts_img[i + 1][1] * ph / h)
                outline_canvas.create_line(x1, y1, x2, y2, fill="black", width=4, tags="polygon")
                outline_canvas.create_line(x1, y1, x2, y2, fill="lime", width=2, tags="polygon")
            for pt in pts_img:
                cx = int(pt[0] * pw / w)
                cy = int(pt[1] * ph / h)
                outline_canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, outline="black", width=2, fill="black", tags="polygon")
                outline_canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4, outline="lime", fill="green", tags="polygon")

    def on_outline_canvas_configure(event):
        if event.width > 10 and event.height > 10:
            outline_canvas_size_ref[0] = event.width
            outline_canvas_size_ref[1] = event.height
            redraw_outline_view()

    outline_canvas.bind("<Configure>", on_outline_canvas_configure)

    def start_outline_step():
        """Display source_image on outline_canvas and set up polygon collection."""
        outline_canvas.delete("all")
        outline_points_ref.clear()
        src = state["source_image"]
        if src is None:
            return
        w, h = src.size
        outline_img_size_ref[0], outline_img_size_ref[1] = w, h
        root.update_idletasks()
        pw, ph = get_canvas_preview_size(outline_canvas)
        outline_canvas_size_ref[0], outline_canvas_size_ref[1] = pw, ph
        thumb = src.resize((pw, ph), _resample())
        outline_photo_ref[0] = ImageTk.PhotoImage(thumb)
        outline_canvas.create_image(0, 0, anchor=tk.NW, image=outline_photo_ref[0], tags="outline_img")

        def canvas_to_image(cx, cy):
            iw, ih = outline_img_size_ref[0], outline_img_size_ref[1]
            pw2, ph2 = outline_canvas_size_ref[0], outline_canvas_size_ref[1]
            return (int(cx * iw / pw2), int(cy * ih / ph2))

        def redraw_polygon():
            outline_canvas.delete("polygon")
            pts_img = outline_points_ref
            iw, ih = outline_img_size_ref[0], outline_img_size_ref[1]
            pw2, ph2 = outline_canvas_size_ref[0], outline_canvas_size_ref[1]
            if len(pts_img) < 2:
                return
            for i in range(len(pts_img) - 1):
                x1 = int(pts_img[i][0] * pw2 / iw)
                y1 = int(pts_img[i][1] * ph2 / ih)
                x2 = int(pts_img[i + 1][0] * pw2 / iw)
                y2 = int(pts_img[i + 1][1] * ph2 / ih)
                outline_canvas.create_line(x1, y1, x2, y2, fill="black", width=4, tags="polygon")
                outline_canvas.create_line(x1, y1, x2, y2, fill="lime", width=2, tags="polygon")
            for pt in pts_img:
                cx = int(pt[0] * pw2 / iw)
                cy = int(pt[1] * ph2 / ih)
                outline_canvas.create_oval(cx - 5, cy - 5, cx + 5, cy + 5, outline="black", width=2, fill="black", tags="polygon")
                outline_canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4, outline="lime", fill="green", tags="polygon")

        def on_click(event):
            outline_points_ref.append(canvas_to_image(event.x, event.y))
            redraw_polygon()

        def on_done():
            if len(outline_points_ref) < 3:
                messagebox.showwarning("Outline", "Add at least 3 points, then click Done.")
                return
            points_img = list(outline_points_ref)
            crop = extract_polygon_crop(state["source_image"], points_img)
            if crop is None:
                messagebox.showerror("Error", "Could not extract crop.")
                return
            step = state["workflow_step"]
            if step == "outline_wheel1":
                state["wheel_crops"].append(crop)
                state["workflow_step"] = "outline_wheel2"
            elif step == "outline_wheel2":
                state["wheel_crops"].append(crop)
                state["workflow_step"] = "outline_car"
            elif step == "outline_car":
                state["car_crop"] = crop
                state["new_img"] = crop
                ow, oh = state["orig_size"]
                nw, nh = crop.size
                scale = min(ow / nw, oh / nh, 1.0)
                scale = max(0.1, min(2.0, scale))
                new_w = int(nw * scale)
                new_h = int(nh * scale)
                state["overlay_x"] = (ow - new_w) // 2
                state["overlay_y"] = (oh - new_h) // 2
                state["overlay_scale"] = scale
                state["workflow_step"] = "align_car"
            outline_canvas.unbind("<Button-1>")
            outline_done_btn.config(command=lambda: None)
            root.after(0, show_content)

        outline_canvas.bind("<Button-1>", on_click)
        outline_done_btn.config(command=on_done)

    def show_overlay_controls():
        if controls_frame[0] is not None:
            return
        cf = ttk.LabelFrame(align_car_frame, text="Align new car (position & size)", padding=6)
        cf.pack(fill=tk.X, pady=(0, 6))
        controls_frame[0] = cf
        ow, oh = state["orig_size"]

        def make_slider(parent, label, from_, to, var, resolution=1, command=None):
            f = ttk.Frame(parent)
            f.pack(fill=tk.X, pady=2)
            ttk.Label(f, text=label, width=8).pack(side=tk.LEFT)
            s = ttk.Scale(f, from_=from_, to=to, variable=var, orient=tk.HORIZONTAL, length=200, command=lambda _: command and command())
            s.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 4))
            return s

        v_x = tk.DoubleVar(value=state["overlay_x"])
        v_y = tk.DoubleVar(value=state["overlay_y"])
        v_scale = tk.DoubleVar(value=state["overlay_scale"])

        def sync_from_sliders(*_):
            state["overlay_x"] = int(v_x.get())
            state["overlay_y"] = int(v_y.get())
            state["overlay_scale"] = max(0.05, min(3.0, v_scale.get()))
            refresh_preview()

        make_slider(cf, "X:", -ow, ow, v_x, command=sync_from_sliders)
        make_slider(cf, "Y:", -oh, oh, v_y, command=sync_from_sliders)
        make_slider(cf, "Scale:", 0.05, 3.0, v_scale, command=sync_from_sliders)

        def export():
            folder = chosen.get()
            if folder.startswith("("):
                messagebox.showwarning("Warning", "Select a valid device lib first.")
                return
            out_dir = get_presets_dir(folder)
            path = filedialog.asksaveasfilename(
                title="Export image (original size, new car only)",
                initialdir=out_dir,
                initialfile="img_car.png",
                defaultextension=".png",
                filetypes=[("PNG", "*.png"), ("All files", "*.*")],
            )
            if not path:
                return
            try:
                exp = build_export_image(
                    state["orig_size"],
                    state["new_img"],
                    state["overlay_x"],
                    state["overlay_y"],
                    state["overlay_scale"],
                )
                exp.save(path)
                messagebox.showinfo("Export", f"Saved to:\n{path}\n\nNow align the wheels.")
                wheels_path = get_img_wheels_path(folder)
                if not os.path.isfile(wheels_path):
                    messagebox.showwarning("Wheels", "img_wheels.png not found for this device lib. Skipping wheels step.")
                    return
                state["workflow_step"] = "align_wheels"
                try:
                    state["wheels_img"] = Image.open(wheels_path).convert("RGBA")
                    state["wheels_size"] = state["wheels_img"].size
                except Exception as e:
                    messagebox.showerror("Wheels", f"Could not load img_wheels.png: {e}")
                    return
                ww, wh = state["wheels_size"]
                for i, wc in enumerate(state["wheel_crops"]):
                    if i == 0:
                        nw, nh = wc.size
                        s = min(ww / 2 / max(nw, 1), wh / max(nh, 1), 1.0)
                        s = max(0.1, min(2.0, s))
                        state["wheel1_x"] = 0
                        state["wheel1_y"] = (wh - int(nh * s)) // 2
                        state["wheel1_scale"] = s
                    else:
                        nw, nh = wc.size
                        s = min(ww / 2 / max(nw, 1), wh / max(nh, 1), 1.0)
                        s = max(0.1, min(2.0, s))
                        state["wheel2_x"] = ww - int(nw * s)
                        state["wheel2_y"] = (wh - int(nh * s)) // 2
                        state["wheel2_scale"] = s
                show_content()
            except Exception as e:
                messagebox.showerror("Export error", str(e))

        ttk.Button(cf, text="Export image (original size, new car only)", command=export).pack(pady=(6, 0))

        state["_v_x"] = v_x
        state["_v_y"] = v_y
        state["_v_scale"] = v_scale

    def update_control_values():
        if not controls_frame[0] or "_v_x" not in state:
            return
        state["_v_x"].set(state["overlay_x"])
        state["_v_y"].set(state["overlay_y"])
        state["_v_scale"].set(state["overlay_scale"])

    def refresh_preview(_=None):
        canvas.delete("preview")
        if not state["orig_img"]:
            return
        pw, ph = get_canvas_preview_size(canvas)
        if state["new_img"] is None:
            img = state["orig_img"]
            img = img.resize((pw, ph), _resample())
            photo = ImageTk.PhotoImage(img)
            photo_ref[0] = photo
            canvas.create_image(0, 0, anchor=tk.NW, image=photo, tags="preview")
        else:
            comp = build_composite_preview(
                state["orig_img"],
                state["orig_size"],
                state["new_img"],
                state["overlay_x"],
                state["overlay_y"],
                state["overlay_scale"],
                (pw, ph),
            )
            photo = ImageTk.PhotoImage(comp)
            photo_ref[0] = photo
            canvas.create_image(0, 0, anchor=tk.NW, image=photo, tags="preview")

    def on_car_canvas_configure(event):
        if event.width > 10 and event.height > 10:
            refresh_preview()
    canvas.bind("<Configure>", on_car_canvas_configure)

    wheels_controls_created = [False]

    def refresh_wheels_preview(_=None):
        wheels_canvas.delete("preview")
        wimg = state.get("wheels_img")
        wcrops = state.get("wheel_crops") or []
        if wimg is None or len(wcrops) < 2:
            return
        pw, ph = get_canvas_preview_size(wheels_canvas)
        comp = build_wheels_composite_preview(
            wimg, state["wheels_size"],
            wcrops,
            state["wheel1_x"], state["wheel1_y"], state["wheel1_scale"],
            state["wheel2_x"], state["wheel2_y"], state["wheel2_scale"],
            (pw, ph),
        )
        wheels_photo_ref[0] = ImageTk.PhotoImage(comp)
        wheels_canvas.create_image(0, 0, anchor=tk.NW, image=wheels_photo_ref[0], tags="preview")

    def on_wheels_canvas_configure(event):
        if event.width > 10 and event.height > 10:
            refresh_wheels_preview()
    wheels_canvas.bind("<Configure>", on_wheels_canvas_configure)

    def show_wheels_controls():
        if wheels_controls_created[0]:
            return
        wheels_controls_created[0] = True
        ww, wh = state["wheels_size"]

        def make_slider(parent, label, from_, to, var, command=None):
            f = ttk.Frame(parent)
            f.pack(fill=tk.X, pady=2)
            ttk.Label(f, text=label, width=8).pack(side=tk.LEFT)
            s = ttk.Scale(f, from_=from_, to=to, variable=var, orient=tk.HORIZONTAL, length=180, command=lambda _: command and command())
            s.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(4, 4))
            return s

        w1f = ttk.LabelFrame(wheels_controls_frame, text="Wheel 1", padding=4)
        w1f.pack(fill=tk.X, pady=(0, 4))
        v_w1_x = tk.DoubleVar(value=state["wheel1_x"])
        v_w1_y = tk.DoubleVar(value=state["wheel1_y"])
        v_w1_s = tk.DoubleVar(value=state["wheel1_scale"])

        def sync_w1(*_):
            state["wheel1_x"] = int(v_w1_x.get())
            state["wheel1_y"] = int(v_w1_y.get())
            state["wheel1_scale"] = max(0.05, min(3.0, v_w1_s.get()))
            refresh_wheels_preview()

        make_slider(w1f, "X:", -ww, ww, v_w1_x, command=sync_w1)
        make_slider(w1f, "Y:", -wh, wh, v_w1_y, command=sync_w1)
        make_slider(w1f, "Scale:", 0.05, 3.0, v_w1_s, command=sync_w1)

        w2f = ttk.LabelFrame(wheels_controls_frame, text="Wheel 2", padding=4)
        w2f.pack(fill=tk.X, pady=(0, 4))
        v_w2_x = tk.DoubleVar(value=state["wheel2_x"])
        v_w2_y = tk.DoubleVar(value=state["wheel2_y"])
        v_w2_s = tk.DoubleVar(value=state["wheel2_scale"])

        def sync_w2(*_):
            state["wheel2_x"] = int(v_w2_x.get())
            state["wheel2_y"] = int(v_w2_y.get())
            state["wheel2_scale"] = max(0.05, min(3.0, v_w2_s.get()))
            refresh_wheels_preview()

        make_slider(w2f, "X:", -ww, ww, v_w2_x, command=sync_w2)
        make_slider(w2f, "Y:", -wh, wh, v_w2_y, command=sync_w2)
        make_slider(w2f, "Scale:", 0.05, 3.0, v_w2_s, command=sync_w2)

        def export_wheels():
            folder = chosen.get()
            if folder.startswith("("):
                messagebox.showwarning("Warning", "Select a valid device lib first.")
                return
            out_dir = get_presets_dir(folder)
            path = filedialog.asksaveasfilename(
                title="Export img_wheels.png (original size, wheels only)",
                initialdir=out_dir,
                initialfile="img_wheels.png",
                defaultextension=".png",
                filetypes=[("PNG", "*.png"), ("All files", "*.*")],
            )
            if not path:
                return
            try:
                exp = build_wheels_export_image(
                    state["wheels_size"],
                    state["wheel_crops"],
                    state["wheel1_x"], state["wheel1_y"], state["wheel1_scale"],
                    state["wheel2_x"], state["wheel2_y"], state["wheel2_scale"],
                )
                exp.save(path)
                messagebox.showinfo("Export", f"Saved to:\n{path}")
            except Exception as e:
                messagebox.showerror("Export error", str(e))

        ttk.Button(wheels_controls_frame, text="Export img_wheels.png", command=export_wheels).pack(pady=(6, 0))

    def browse_png():
        path = filedialog.askopenfilename(
            title="Select new car image",
            filetypes=[
                ("PNG and JPG", "*.png;*.jpg;*.jpeg"),
                ("PNG", "*.png"),
                ("JPEG", "*.jpg;*.jpeg"),
                ("All files", "*.*"),
            ],
        )
        if path and path.lower().endswith((".png", ".jpg", ".jpeg")):
            load_new_png(path)
        elif path:
            messagebox.showinfo("Info", "Please choose a PNG or JPG file.")

    def on_drop(event):
        data = event.data
        if isinstance(data, str) and data.strip():
            path = data.strip().strip("{}")
            if path.lower().endswith((".png", ".jpg", ".jpeg")) and os.path.isfile(path):
                load_new_png(path)
            else:
                messagebox.showinfo("Info", "Please drop a PNG or JPG file.")

    drop_canvas.bind("<Button-1>", lambda e: browse_png())
    if HAS_DND:
        try:
            drop_canvas.drop_target_register(DND_FILES)
            drop_canvas.dnd_bind("<<Drop>>", on_drop)
        except Exception:
            pass

    combo.bind("<<ComboboxSelected>>", lambda e: load_original())
    load_original()

    # --- Bottom buttons ---
    btns = ttk.Frame(main)
    btns.pack(fill=tk.X)
    ttk.Button(btns, text="Done", command=lambda: (result.__setitem__(0, chosen.get()), root.quit(), root.destroy())).pack(side=tk.RIGHT, padx=(6, 0))
    ttk.Button(btns, text="Cancel", command=lambda: (result.__setitem__(0, None), root.quit(), root.destroy())).pack(side=tk.RIGHT)

    root.protocol("WM_DELETE_WINDOW", lambda: (result.__setitem__(0, None), root.quit(), root.destroy()))
    root.mainloop()
    return result[0]


if __name__ == "__main__":
    selected = run_gui()
    if selected:
        print(f"Selected: {selected}")
    else:
        print("Cancelled")
