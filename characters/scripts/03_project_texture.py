"""UV-unwrap a Hunyuan mesh and bake a colour texture by projecting the sheet views onto it.

Front view is full-body; left (profile) and back views are partial (cut at thigh/knee), so
each view is registered to the mesh by silhouette matching with the head top anchored.
Texels no view can see are filled from the nearest coloured surface point in 3D.

Usage:  python 03_project_texture.py [names...] [--tex 2048]
Output: meshes/<name>_textured.glb  (+ <name>_albedo.png, _registration.png for checking)
"""
import argparse
from pathlib import Path

import cv2
import numpy as np
import trimesh
import xatlas
from PIL import Image
from scipy.spatial import cKDTree

ROOT = Path(__file__).resolve().parent.parent
ap = argparse.ArgumentParser()
ap.add_argument("names", nargs="*", default=["set1_casual", "set2_traditional"])
ap.add_argument("--tex", type=int, default=2048)
args = ap.parse_args()

RES = 1024  # silhouette / depth buffer resolution


# ---------------------------------------------------------------- orientation
def orient(mesh: trimesh.Trimesh) -> trimesh.Trimesh:
    """Rotate so that +Z is up, character faces -Y (Blender/Unreal front), feet at z=0."""
    v = mesh.vertices
    ext = v.max(0) - v.min(0)
    up = int(np.argmax(ext))
    horiz = [i for i in range(3) if i != up]
    depth = min(horiz, key=lambda i: ext[i])  # body is thinner front-to-back than side-to-side
    side = [i for i in horiz if i != depth][0]
    # Up sign: the head (top) is narrower than the shoulders/arms band; feet are two blobs.
    # Use: the widest slice sits in the upper half of the body.
    h = v[:, up]
    lo, hi = h.min(), h.max()
    bins = np.linspace(lo, hi, 41)
    widths = [np.ptp(v[(h >= a) & (h < b), side]) if ((h >= a) & (h < b)).sum() > 3 else 0
              for a, b in zip(bins[:-1], bins[1:])]
    up_sign = 1 if np.argmax(widths) > 20 else -1
    # Front sign: toes stick out forward of the heels at foot level.
    up_axis = np.zeros(3); up_axis[up] = up_sign
    hh = h * up_sign
    feet = v[hh < hh.min() + 0.06 * np.ptp(hh)]
    body_c = np.median(v[:, depth])
    fwd = feet[:, depth] - body_c
    front_sign = 1 if abs(fwd.max()) > abs(fwd.min()) else -1
    fwd_axis = np.zeros(3); fwd_axis[depth] = front_sign
    # target basis: forward -> -Y, up -> +Z, x = cross(y, z) (right-handed)
    R_src = np.stack([np.cross(-fwd_axis, up_axis), -fwd_axis, up_axis])  # rows map to x,y,z
    T = np.eye(4); T[:3, :3] = R_src
    mesh = mesh.copy(); mesh.apply_transform(T)
    if np.linalg.det(R_src) < 0:
        mesh.invert()
    mesh.apply_translation([-mesh.bounds.mean(0)[0], -mesh.bounds.mean(0)[1], -mesh.bounds[0][2]])
    return mesh


# ---------------------------------------------------------------- views
# Character faces -Y. Each view: (camera direction pointing *toward* the camera from the body,
# image-x axis in world, mirror image?)
VIEWS = {
    "front": dict(to_cam=np.array([0, -1, 0]), ux=np.array([1, 0, 0]), weight=1.0),
    "back": dict(to_cam=np.array([0, 1, 0]), ux=np.array([-1, 0, 0]), weight=1.0),
    # profile shows the character's LEFT side, facing image-left
    "left": dict(to_cam=np.array([1, 0, 0]), ux=np.array([0, 1, 0]), weight=0.8),
    # right side: mirrored left image (low weight; accessories may be mirrored)
    "right": dict(to_cam=np.array([-1, 0, 0]), ux=np.array([0, -1, 0]), weight=0.25, src="left",
                  flip=True),
}


def project(points, view):
    """World -> 2D (image-x right, image-y down), plus depth toward camera (bigger = closer)."""
    u = points @ view["ux"]
    y = -points[:, 2]
    d = points @ view["to_cam"]
    return np.stack([u, y], 1), d


def render_buffers(mesh, view, scale, off, shape):
    """Silhouette + depth buffer (painter's algorithm, far to near) in image pixel space."""
    uv, d = project(mesh.vertices, view)
    px = uv * scale + off
    f = mesh.faces
    fd = d[f].mean(1)
    order = np.argsort(fd)  # far first
    zbuf = np.full(shape, -np.inf, np.float32)
    pts = np.round(px[f] * 4).astype(np.int32)  # 2 bits of subpixel precision
    for i in order:
        cv2.fillConvexPoly(zbuf, pts[i], float(fd[i]), lineType=cv2.LINE_8, shift=2)
    return zbuf


def register(mesh, view, alpha, partial):
    """Find scale/offset mapping mesh projection to the view image by silhouette IoU.
    Head top is anchored; for partial views only rows that exist in the image are compared."""
    uv, _ = project(mesh.vertices, view)
    H, W = alpha.shape
    ys, xs = np.where(alpha > 0.5)
    top, cx = ys.min(), xs.mean()
    full_h = uv[:, 1].max() - uv[:, 1].min()
    if not partial:
        cands = [(ys.max() - top) / full_h]
    else:
        base = (ys.max() - top) / full_h  # lower bound (image shows less than full height)
        cands = np.linspace(base, base * 3.0, 120)
    best = None
    for s in cands:
        px = uv * s
        oy = top - px[:, 1].min()
        # horizontal: align silhouette centroid of the rows in common
        rows = (px[:, 1] + oy) < ys.max()
        if rows.sum() < 50:
            continue
        ox = cx - px[rows, 0].mean()
        z = render_buffers(mesh, view, s, np.array([ox, oy]), (H, W))
        sil = np.isfinite(z)
        a = alpha > 0.5
        iou = (sil & a).sum() / max(1, (sil | a).sum())
        if best is None or iou > best[0]:
            best = (iou, s, np.array([ox, oy]))
    return best


def load_alpha(name, src, flip=False):
    a = np.asarray(Image.open(ROOT / "views" / f"{name}_{src}_raw.png").convert("RGBA"))[..., 3]
    return (a[:, ::-1] if flip else a).astype(np.float32) / 255


def oriented(mesh, fwd, up):
    """Rotate so `fwd` -> -Y (faces viewer of the front image), `up` -> +Z, feet on z=0."""
    R = np.stack([np.cross(-fwd, up), -fwd, up]).astype(float)
    T = np.eye(4); T[:3, :3] = R
    m = mesh.copy(); m.apply_transform(T)
    m.apply_translation([-m.bounds.mean(0)[0], -m.bounds.mean(0)[1], -m.bounds[0][2]])
    return m


def orient_by_views(mesh, name):
    """Try all 24 axis-aligned orientations: front-view IoU picks up/forward axes,
    profile-view IoU breaks the front/back mirror ambiguity."""
    fa = load_alpha(name, "front")
    axes = [np.eye(3)[i] * s for i in range(3) for s in (1, -1)]
    scored = []
    for up in axes:
        for fwd in axes:
            if abs(up @ fwd) > 0.5:
                continue
            m = oriented(mesh, fwd, up)
            scored.append((register(m, VIEWS["front"], fa, False)[0], tuple(fwd), tuple(up)))
    scored.sort(reverse=True)
    la = load_alpha(name, "left")
    best = None
    for iou_f, fwd, up in scored[:2]:  # the top two are the front/back mirror pair
        m = oriented(mesh, np.array(fwd), np.array(up))
        iou_l = register(m, VIEWS["left"], la, True)[0]
        print(f"  candidate fwd={fwd} up={up}: front IoU {iou_f:.3f}, profile IoU {iou_l:.3f}")
        if best is None or iou_f + iou_l > best[0]:
            best = (iou_f + iou_l, m)
    return best[1]


def bilinear(img, q):
    H, W = img.shape[:2]
    x = np.clip(q[:, 0] - 0.5, 0, W - 1.001); y = np.clip(q[:, 1] - 0.5, 0, H - 1.001)
    x0, y0 = x.astype(int), y.astype(int); fx, fy = (x - x0)[:, None], (y - y0)[:, None]
    return (img[y0, x0] * (1 - fx) * (1 - fy) + img[y0, x0 + 1] * fx * (1 - fy)
            + img[y0 + 1, x0] * (1 - fx) * fy + img[y0 + 1, x0 + 1] * fx * fy)


def bake(name):
    mesh = trimesh.load(ROOT / "meshes" / f"{name}_shape.obj", force="mesh")
    mesh = orient_by_views(mesh, name)
    print(name, "faces", len(mesh.faces), "bounds", mesh.bounds.round(3).tolist())

    # UV unwrap
    vmap, idx, uvs = xatlas.parametrize(mesh.vertices, mesh.faces)
    V = mesh.vertices[vmap]
    N = mesh.vertex_normals[vmap]
    F = idx
    T = args.tex
    face_id = np.full((T, T), -1, np.int32)
    tri_px = uvs[F] * T
    tri_px[..., 1] = T - tri_px[..., 1]  # image rows go down
    pts = np.round(tri_px * 4).astype(np.int32)
    for i in range(len(F)):
        cv2.fillConvexPoly(face_id, pts[i], int(i), lineType=cv2.LINE_8, shift=2)
    ty, tx = np.where(face_id >= 0)
    fid = face_id[ty, tx]
    # barycentrics of texel centres
    a, b, c = tri_px[fid, 0], tri_px[fid, 1], tri_px[fid, 2]
    p = np.stack([tx + 0.5, ty + 0.5], 1)
    v0, v1, v2 = b - a, c - a, p - a
    den = v0[:, 0] * v1[:, 1] - v1[:, 0] * v0[:, 1]
    den[np.abs(den) < 1e-12] = 1e-12
    l1 = (v2[:, 0] * v1[:, 1] - v1[:, 0] * v2[:, 1]) / den
    l2 = (v0[:, 0] * v2[:, 1] - v2[:, 0] * v0[:, 1]) / den
    l0 = 1 - l1 - l2
    bary = np.clip(np.stack([l0, l1, l2], 1), 0, 1)
    bary /= bary.sum(1, keepdims=True)
    P = (V[F[fid]] * bary[..., None]).sum(1)
    Nn = (N[F[fid]] * bary[..., None]).sum(1)
    Nn /= np.linalg.norm(Nn, axis=1, keepdims=True) + 1e-9

    acc = np.zeros((len(P), 3)); wsum = np.zeros(len(P))
    dbg = []
    for vname, view in VIEWS.items():
        src = view.get("src", vname)
        img = Image.open(ROOT / "views" / f"{name}_{src}_raw.png").convert("RGBA")
        arr = np.asarray(img).astype(np.float32) / 255
        if view.get("flip"):
            arr = arr[:, ::-1]
        alpha = arr[..., 3]
        partial = src != "front"
        iou, s, off = register(mesh, view, alpha, partial)
        print(f"  {vname}: IoU {iou:.3f} scale {s:.1f}")
        H, W = alpha.shape
        z = render_buffers(mesh, view, s, off, (H, W))
        q, d = project(P, view)
        q = q * s + off
        qi = np.round(q).astype(int)
        inb = (qi[:, 0] >= 0) & (qi[:, 0] < W) & (qi[:, 1] >= 0) & (qi[:, 1] < H)
        qx = np.clip(qi[:, 0], 0, W - 1); qy = np.clip(qi[:, 1], 0, H - 1)
        eps = 0.01 * mesh.extents.max()
        vis = inb & (d >= z[qy, qx] - eps) & (alpha[qy, qx] > 0.6)
        facing = np.clip(Nn @ view["to_cam"], 0, 1)
        w = view["weight"] * facing ** 3 * vis
        col = bilinear(arr[..., :3], q)
        acc += col * w[:, None]; wsum += w
        # registration debug: mesh silhouette edge over the photo
        sil = np.isfinite(z).astype(np.uint8)
        edge = cv2.morphologyEx(sil, cv2.MORPH_GRADIENT, np.ones((3, 3), np.uint8)) > 0
        rgb = (arr[..., :3] * 255).astype(np.uint8).copy(); rgb[edge] = (255, 0, 255)
        dbg.append(cv2.resize(rgb, (int(W * 400 / H), 400)))

    good = wsum > 0.02
    colors = np.zeros((len(P), 3))
    colors[good] = acc[good] / wsum[good, None]
    # Unseen texels (mostly back of lower legs - the back view stops at the knees):
    # project the front view straight through, ignoring occlusion.
    fview = VIEWS["front"]
    farr = np.asarray(Image.open(ROOT / "views" / f"{name}_front_raw.png").convert("RGBA")).astype(np.float32) / 255
    _, fs, foff = register(mesh, fview, farr[..., 3], False)
    fq = project(P, fview)[0] * fs + foff
    fa = bilinear(farr[..., 3:], fq)[:, 0]
    through = (~good) & (fa > 0.8)
    colors[through] = bilinear(farr[..., :3], fq[through])
    good = good | through
    # fill unseen texels from nearest seen surface point in 3D
    if (~good).any():
        tree = cKDTree(P[good])
        _, nn = tree.query(P[~good], k=4)
        colors[~good] = colors[good][nn].mean(1)
    print(f"  coverage {good.mean() * 100:.1f}% of texels seen directly")

    tex = np.zeros((T, T, 3), np.float32)
    tex[ty, tx] = colors
    # pad UV islands (avoid seams under mipmapping)
    filled = (face_id >= 0).astype(np.uint8)
    _, labels = cv2.distanceTransformWithLabels(1 - filled, cv2.DIST_L2, 5,
                                                labelType=cv2.DIST_LABEL_PIXEL)
    src_idx = np.zeros(labels.max() + 1, np.int64)
    fy, fx = np.where(filled > 0)
    src_idx[labels[fy, fx]] = fy * T + fx
    flat = tex.reshape(-1, 3)
    tex = flat[src_idx[labels]].reshape(T, T, 3)
    tex8 = (np.clip(tex, 0, 1) * 255).astype(np.uint8)
    Image.fromarray(tex8).save(ROOT / "meshes" / f"{name}_albedo.png")
    Image.fromarray(np.concatenate(dbg, 1)).save(ROOT / "meshes" / f"{name}_registration.png")

    material = trimesh.visual.material.PBRMaterial(
        baseColorTexture=Image.fromarray(tex8), metallicFactor=0.0, roughnessFactor=0.85
    )
    out = trimesh.Trimesh(V, F, visual=trimesh.visual.TextureVisuals(uv=uvs, material=material),
                          process=False)
    # glTF is Y-up: rotate Z-up -> Y-up so it imports upright everywhere
    out.apply_transform(trimesh.transformations.rotation_matrix(-np.pi / 2, [1, 0, 0]))
    out.export(ROOT / "meshes" / f"{name}_textured.glb")
    print("  wrote", f"meshes/{name}_textured.glb")


for n in args.names:
    bake(n)
