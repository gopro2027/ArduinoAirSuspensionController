"""
Car image creator tool - align a new car image (PNG/JPG) over the original and export as PNG.
"""
import os
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

try:
    from tkinterdnd2 import DND_FILES, TkinterDnD
    HAS_DND = True
except ImportError:
    HAS_DND = False

# Relative path from each device_lib folder to the car preset image
PRESET_CAR_IMAGE = os.path.join("images", "presets", "img_car.png")
PREVIEW_MAX = (500, 400)
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


def build_composite_preview(orig_img, orig_size, new_img, overlay_x, overlay_y, overlay_scale, preview_max=PREVIEW_MAX):
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


def run_gui():
    """Main GUI: device lib selector, preview with overlay, position/scale controls, export."""
    result = [None]

    root_class = TkinterDnD.Tk if HAS_DND else tk.Tk
    root = root_class()
    root.title("Car Creator – Align & Export")
    root.minsize(420, 520)
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

    # --- State (all in original image pixel space) ---
    state = {
        "orig_img": None,
        "orig_size": (0, 0),
        "new_img": None,
        "overlay_x": 0,
        "overlay_y": 0,
        "overlay_scale": 1.0,
    }
    photo_ref = [None]
    controls_frame = [None]  # hide/show when new_img set/cleared

    # --- Preview: canvas (drop target) + placeholder text ---
    preview_lf = ttk.LabelFrame(main, text="img_car.png — drop PNG/JPG or click to browse", padding=4)
    preview_lf.pack(fill=tk.BOTH, expand=True, pady=(0, 6))

    canvas = tk.Canvas(preview_lf, width=PREVIEW_MAX[0], height=PREVIEW_MAX[1], bg="#2d2d2d", highlightthickness=1)
    canvas.pack(expand=True)
    drop_hint = canvas.create_text(
        PREVIEW_MAX[0] // 2, PREVIEW_MAX[1] // 2,
        text="Select a device lib, then drop a PNG or JPG\nor click to load a new car image",
        fill="#888", font=("Segoe UI", 10), justify=tk.CENTER,
    )

    def load_original():
        folder = chosen.get()
        if folder.startswith("("):
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            refresh_preview()
            return
        path = get_img_car_path(folder)
        if not HAS_PIL or not os.path.isfile(path):
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            refresh_preview()
            return
        try:
            img = Image.open(path).convert("RGBA")
            state["orig_img"] = img
            state["orig_size"] = img.size
            state["new_img"] = None
            if controls_frame[0]:
                controls_frame[0].pack_forget()
                controls_frame[0] = None
            refresh_preview()
        except Exception as e:
            state["orig_img"] = None
            state["orig_size"] = (0, 0)
            refresh_preview()
            messagebox.showerror("Error", str(e))

    def load_new_png(filepath):
        if not HAS_PIL:
            messagebox.showerror("Error", "Pillow is required for image handling.")
            return
        if not state["orig_img"]:
            messagebox.showinfo("Info", "Select a device lib with a valid img_car.png first.")
            return
        try:
            new_img = Image.open(filepath).convert("RGBA")
        except Exception as e:
            messagebox.showerror("Error", f"Could not open image: {e}")
            return
        ow, oh = state["orig_size"]
        nw, nh = new_img.size
        scale = min(ow / nw, oh / nh, 1.0)
        scale = max(0.1, min(2.0, scale))
        new_w = int(nw * scale)
        new_h = int(nh * scale)
        overlay_x = (ow - new_w) // 2
        overlay_y = (oh - new_h) // 2
        state["new_img"] = new_img
        state["overlay_x"] = overlay_x
        state["overlay_y"] = overlay_y
        state["overlay_scale"] = scale
        show_overlay_controls()
        refresh_preview()
        update_control_values()

    def show_overlay_controls():
        if controls_frame[0] is not None:
            return
        cf = ttk.LabelFrame(main, text="Align new car (position & size)", padding=6)
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
            default_name = os.path.join(out_dir, "img_car.png")
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
                messagebox.showinfo("Export", f"Saved to:\n{path}")
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

    def refresh_preview():
        canvas.delete("preview")
        if not state["orig_img"]:
            canvas.itemconfig(drop_hint, state=tk.NORMAL)
            canvas.tag_lower(drop_hint)
            return
        canvas.itemconfig(drop_hint, state=tk.HIDDEN)
        if state["new_img"] is None:
            # Show original only
            img = state["orig_img"]
            img.thumbnail(PREVIEW_MAX, _resample())
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
            )
            photo = ImageTk.PhotoImage(comp)
            photo_ref[0] = photo
            canvas.create_image(0, 0, anchor=tk.NW, image=photo, tags="preview")

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

    canvas.bind("<Button-1>", lambda e: browse_png())
    if HAS_DND:
        try:
            canvas.drop_target_register(DND_FILES)
            canvas.dnd_bind("<<Drop>>", on_drop)
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
