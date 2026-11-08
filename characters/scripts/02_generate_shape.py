"""Generate an untextured mesh per character with Hunyuan3D-2mv (multi-view shape model).

Usage:  python 02_generate_shape.py [set1_casual|set2_traditional ...] [--views front,back,left]
Runs on an 8 GB GPU: fp16 weights, Turbo step-distilled model, FlashVDM decoding.
"""
import argparse
import sys
import time
from pathlib import Path

import torch
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools" / "Hunyuan3D-2"))

from hy3dgen.shapegen import (  # noqa: E402
    DegenerateFaceRemover,
    FaceReducer,
    FloaterRemover,
    Hunyuan3DDiTFlowMatchingPipeline,
)

ap = argparse.ArgumentParser()
ap.add_argument("names", nargs="*", default=["set1_casual", "set2_traditional"])
# Only the front views on the sheet are full-body, so default to the single-image model.
ap.add_argument("--views", default="front")
ap.add_argument("--steps", type=int, default=30)
ap.add_argument("--octree", type=int, default=380)
ap.add_argument("--faces", type=int, default=60000, help="target face count after reduction")
ap.add_argument("--seed", type=int, default=1234)
args = ap.parse_args()
views = [v.strip() for v in args.views.split(",") if v.strip()]
multiview = views != ["front"]

repo, sub = (
    ("tencent/Hunyuan3D-2mv", "hunyuan3d-dit-v2-mv") if multiview
    else ("tencent/Hunyuan3D-2", "hunyuan3d-dit-v2-0")
)
pipe = Hunyuan3DDiTFlowMatchingPipeline.from_pretrained(repo, subfolder=sub, variant="fp16")
try:
    pipe.enable_flashvdm(mc_algo="mc")
except Exception as e:  # FlashVDM is an optimisation only
    print("flashvdm unavailable:", e)

out_dir = ROOT / "meshes"
out_dir.mkdir(exist_ok=True)
for name in args.names:
    images = {v: Image.open(ROOT / "views" / f"{name}_{v}.png").convert("RGBA") for v in views}
    if not multiview:
        images = images["front"]
    t = time.time()
    mesh = pipe(
        image=images,
        num_inference_steps=args.steps,
        octree_resolution=args.octree,
        num_chunks=8000,
        generator=torch.manual_seed(args.seed),
        output_type="trimesh",
    )[0]
    raw = out_dir / f"{name}_shape_raw.glb"
    mesh.export(raw)
    mesh = FloaterRemover()(mesh)
    mesh = DegenerateFaceRemover()(mesh)
    mesh = FaceReducer()(mesh, max_facenum=args.faces)
    mesh.export(out_dir / f"{name}_shape.obj")
    print(f"{name}: {len(mesh.faces)} faces, {time.time() - t:.1f}s")
    torch.cuda.empty_cache()
