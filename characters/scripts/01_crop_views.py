"""Crop the front / left / back views of each character out of the turnaround sheet,
remove the background, and save square RGBA PNGs for Hunyuan3D-2mv.

Usage:  python 01_crop_views.py [--debug]
Box coordinates are in pixels of source/character_sheet.png (1408 x 768).
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
SHEET = ROOT / "source" / "character_sheet.png"
OUT = ROOT / "views"

# (x0, y0, x1, y1). "blank" = inset panels that overlap the crop and must be erased.
CHARACTERS = {
    "set1_casual": {
        "front": (40, 20, 350, 744),
        "left": (355, 20, 510, 452),
        "back": (512, 460, 698, 740),
        # face close-up panel, title text, "BACK" label
        "blank": [(320, 498, 505, 768), (0, 0, 150, 98), (560, 742, 650, 768)],
    },
    "set2_traditional": {
        "front": (735, 20, 1045, 744),
        "left": (1035, 20, 1200, 452),
        "back": (1202, 460, 1406, 740),
        "blank": [(1015, 498, 1197, 768), (706, 0, 848, 98), (1250, 742, 1340, 768)],
    },
}


class BiRefNet:
    """Background removal with the BiRefNet ONNX model (no rembg/numba needed)."""

    def __init__(self, path: Path):
        import onnxruntime as ort

        self.sess = ort.InferenceSession(
            str(path), providers=["CUDAExecutionProvider", "CPUExecutionProvider"]
        )
        self.inp = self.sess.get_inputs()[0].name

    def __call__(self, img: Image.Image) -> Image.Image:
        x = np.asarray(img.convert("RGB").resize((1024, 1024), Image.BICUBIC), np.float32) / 255
        x = (x - [0.485, 0.456, 0.406]) / [0.229, 0.224, 0.225]
        x = x.transpose(2, 0, 1)[None].astype(np.float32)
        logits = self.sess.run(None, {self.inp: x})[0][0, 0]
        mask = 1 / (1 + np.exp(-logits))
        mask = Image.fromarray((mask * 255).astype(np.uint8)).resize(img.size, Image.BICUBIC)
        out = img.convert("RGBA")
        out.putalpha(mask)
        return out


def debug_overlay(img: Image.Image) -> None:
    d = ImageDraw.Draw(img)
    colors = {"front": "red", "left": "lime", "back": "cyan"}
    for spec in CHARACTERS.values():
        for k, c in colors.items():
            d.rectangle(spec[k], outline=c, width=3)
        for b in spec["blank"]:
            d.rectangle(b, outline="yellow", width=2)
    img.save(OUT / "_debug_boxes.png")


def square_pad(rgba: Image.Image, size: int = 1024, margin: float = 0.08) -> Image.Image:
    """Tight-crop to alpha, then centre on a transparent square canvas."""
    a = np.array(rgba)[:, :, 3]
    ys, xs = np.where(a > 16)
    rgba = rgba.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    w, h = rgba.size
    scale = size * (1 - 2 * margin) / max(w, h)
    rgba = rgba.resize((max(1, round(w * scale)), max(1, round(h * scale))), Image.LANCZOS)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(rgba, ((size - rgba.width) // 2, (size - rgba.height) // 2), rgba)
    return canvas


def main() -> None:
    OUT.mkdir(exist_ok=True)
    sheet = Image.open(SHEET).convert("RGB")
    if "--debug" in sys.argv:
        debug_overlay(sheet.copy())
        print("wrote", OUT / "_debug_boxes.png")
        return

    session = BiRefNet(ROOT / "tools" / "models" / "birefnet-general.onnx")
    bg =tuple(int(v) for v in np.array(sheet)[5, 5])  # sheet grey
    for name, spec in CHARACTERS.items():
        work = sheet.copy()
        d = ImageDraw.Draw(work)
        for b in spec["blank"]:
            d.rectangle(b, fill=bg)
        for view in ("front", "left", "back"):
            crop = work.crop(spec[view])
            cut = session(crop)  # RGBA, same size as crop box
            cut.save(OUT / f"{name}_{view}_raw.png")  # keeps sheet scale (used for texturing)
            square_pad(cut).save(OUT / f"{name}_{view}.png")
            print("saved", name, view)


if __name__ == "__main__":
    main()
