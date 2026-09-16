#!/usr/bin/env python3
"""Bake real 3D mesh assets (OBJ) into content/baked/*.exg for APK packaging."""
from __future__ import annotations
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "content" / "baked"

def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    print(f"wrote {path.relative_to(ROOT)} ({len(text)} bytes)")

def box_obj(name: str, sx: float, sy: float, sz: float) -> str:
    hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
    corners = [(-hx,-hy,-hz),(hx,-hy,-hz),(hx,hy,-hz),(-hx,hy,-hz),
               (-hx,-hy,hz),(hx,-hy,hz),(hx,hy,hz),(-hx,hy,hz)]
    faces = [(0,1,2,3),(5,4,7,6),(4,0,3,7),(1,5,6,2),(3,2,6,7),(4,5,1,0)]
    normals = [(0,0,-1),(0,0,1),(-1,0,0),(1,0,0),(0,1,0),(0,-1,0)]
    lines = [f"# EXWORLD baked mesh: {name}", f"o {name}"]
    for c in corners:
        lines.append(f"v {c[0]:.5f} {c[1]:.5f} {c[2]:.5f}")
    for n in normals:
        lines.append(f"vn {n[0]} {n[1]} {n[2]}")
    for fi, face in enumerate(faces):
        ni = fi + 1
        a,b,c,d = [x+1 for x in face]
        lines.append(f"f {a}//{ni} {b}//{ni} {c}//{ni}")
        lines.append(f"f {a}//{ni} {c}//{ni} {d}//{ni}")
    return "\n".join(lines) + "\n"

def capsule_obj(name: str, radius: float, height: float, segs: int = 12, rings: int = 6) -> str:
    verts, norms, faces = [], [], []
    cyl_h = max(0.01, height - 2 * radius)
    for i in range(rings + 1):
        t = i / rings
        y = -cyl_h * 0.5 + t * cyl_h
        for j in range(segs):
            a = 2 * math.pi * j / segs
            x, z = radius * math.cos(a), radius * math.sin(a)
            verts.append((x, y, z))
            norms.append((x / radius, 0.0, z / radius))
    for i in range(rings):
        for j in range(segs):
            a = i * segs + j
            b = i * segs + (j + 1) % segs
            c = (i + 1) * segs + (j + 1) % segs
            d = (i + 1) * segs + j
            faces.append((a, b, c, d))
    lines = [f"# EXWORLD baked mesh: {name}", f"o {name}"]
    for v in verts:
        lines.append(f"v {v[0]:.5f} {v[1]:.5f} {v[2]:.5f}")
    for n in norms:
        lines.append(f"vn {n[0]:.5f} {n[1]:.5f} {n[2]:.5f}")
    for f in faces:
        a,b,c,d = [x+1 for x in f]
        lines.append(f"f {a}//{a} {b}//{b} {c}//{c}")
        lines.append(f"f {a}//{a} {c}//{c} {d}//{d}")
    return "\n".join(lines) + "\n"

def character_bundle() -> str:
    h, build = 1.75, 1.0
    shoulder = 0.38 * build
    leg, torso_h, head_r = h * 0.46, h * 0.31, h * 0.105
    limb_r = 0.075 * build
    parts = [
        ("torso", capsule_obj("torso", shoulder * 0.72, torso_h)),
        ("head", capsule_obj("head", head_r, head_r * 1.6)),
        ("left_leg", capsule_obj("left_leg", limb_r * 1.15, leg * 0.92)),
        ("right_leg", capsule_obj("right_leg", limb_r * 1.15, leg * 0.92)),
        ("left_arm", capsule_obj("left_arm", limb_r, torso_h * 0.92)),
        ("right_arm", capsule_obj("right_arm", limb_r, torso_h * 0.92)),
        ("left_foot", box_obj("left_foot", 0.18*build, 0.10*h, 0.30*build)),
        ("right_foot", box_obj("right_foot", 0.18*build, 0.10*h, 0.30*build)),
    ]
    header = [
        "# EXWORLD character mesh package (real 3D, meters)",
        "format = obj_multipart", "height = 1.75", "unit = meters",
        "parts = torso,head,left_leg,right_leg,left_arm,right_arm,left_foot,right_foot",
        f"offset.torso = 0,{leg + torso_h * 0.5:.4f},0",
        f"offset.head = 0,{leg + torso_h + head_r * 1.2:.4f},0",
        f"offset.left_leg = {-0.12*build:.4f},{leg*0.5:.4f},0",
        f"offset.right_leg = {0.12*build:.4f},{leg*0.5:.4f},0",
        f"offset.left_arm = {-shoulder-limb_r:.4f},{leg+torso_h*0.55:.4f},0",
        f"offset.right_arm = {shoulder+limb_r:.4f},{leg+torso_h*0.55:.4f},0",
        f"offset.left_foot = {-0.12*build:.4f},{0.05*h:.4f},{0.045*h:.4f}",
        f"offset.right_foot = {0.12*build:.4f},{0.05*h:.4f},{0.045*h:.4f}",
        "material.torso = fabric", "material.head = skin",
        "material.left_leg = denim", "material.right_leg = denim",
        "material.left_arm = fabric", "material.right_arm = fabric",
        "material.left_foot = leather", "material.right_foot = leather",
        "BEGIN_MESH",
    ]
    body = "\n".join(header) + "\n"
    for name, obj in parts:
        body += f"# --- part {name} ---\n" + obj
    return body + "END_MESH\n"

def vehicle_sedan() -> str:
    body = box_obj("body", 4.5, 1.2, 1.85)
    cabin = box_obj("cabin", 2.2, 0.9, 1.7)
    wheel = capsule_obj("wheel", 0.32, 0.22, 10, 4)
    header = [
        "# EXWORLD vehicle sedan (real 3D, meters)",
        "format = obj_multipart", "type = sedan",
        "length = 4.50", "width = 1.85", "height = 1.45",
        "parts = body,cabin,wheel_fl,wheel_fr,wheel_rl,wheel_rr",
        "offset.body = 0,0.70,0", "offset.cabin = -0.2,1.25,0",
        "offset.wheel_fl = 1.45,0.32,0.85", "offset.wheel_fr = 1.45,0.32,-0.85",
        "offset.wheel_rl = -1.45,0.32,0.85", "offset.wheel_rr = -1.45,0.32,-0.85",
        "material.body = paint", "material.cabin = glass",
        "material.wheel_fl = rubber", "material.wheel_fr = rubber",
        "material.wheel_rl = rubber", "material.wheel_rr = rubber",
        "BEGIN_MESH",
    ]
    text = "\n".join(header) + "\n" + body + "# cabin\n" + cabin
    for w in ("wheel_fl","wheel_fr","wheel_rl","wheel_rr"):
        text += f"# {w}\n" + wheel.replace("o wheel", f"o {w}")
    return text + "END_MESH\n"

def building_block() -> str:
    shell = box_obj("shell", 14.0, 15.0, 12.0)
    return "\n".join([
        "# EXWORLD building shell (real 3D, meters)",
        "format = obj_multipart", "floors = 5", "floor_height = 3.00",
        "width = 14.00", "depth = 12.00", "parts = shell",
        "offset.shell = 0,7.5000,0", "material.shell = concrete", "BEGIN_MESH",
    ]) + "\n" + shell + "END_MESH\n"

def ground_plate() -> str:
    plate = box_obj("ground", 800.0, 0.15, 800.0)
    return "\n".join([
        "# EXWORLD ground plate (real 3D, meters)",
        "format = obj_multipart", "parts = ground",
        "offset.ground = 0,-0.05,0", "material.ground = asphalt", "BEGIN_MESH",
    ]) + "\n" + plate + "END_MESH\n"

def main() -> None:
    write(OUT / "character.exg", character_bundle())
    write(OUT / "vehicle_sedan.exg", vehicle_sedan())
    write(OUT / "building.exg", building_block())
    write(OUT / "ground.exg", ground_plate())
    index = """# EXWORLD baked package index (pre-generated, shipped in APK)
package = downtown,baked/downtown.exg,1.0,city
package = forest,baked/forest.exg,1.0,biome
package = lake,baked/lake.exg,1.0,biome
package = swamp,baked/swamp.exg,1.0,biome
package = vehicles,baked/vehicles.exg,1.0,props
package = animation,baked/animation.exg,1.0,anim
package = sound,baked/sound.exg,1.0,audio
package = character,baked/character.exg,1.0,mesh
package = vehicle_sedan,baked/vehicle_sedan.exg,1.0,mesh
package = building,baked/building.exg,1.0,mesh
package = ground,baked/ground.exg,1.0,mesh
scale = meters
player_height = 1.75
car_length = 4.50
car_width = 1.85
floor_height = 3.00
block_size = 96.00
stream_radius = 400
"""
    write(OUT / "index.exg", index)
    print("bake complete")

if __name__ == "__main__":
    main()
