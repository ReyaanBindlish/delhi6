r"""
setup_all.py - run every OldDelhi editor setup script in the right order, each in its own headless
UnrealEditor-Cmd process. Run with any plain Python 3 (no unreal module needed):

    python Tools\ue\setup_all.py                 # everything
    python Tools\ue\setup_all.py --only input,physics
    python Tools\ue\setup_all.py --skip crowd --limit 50   # smoke test: 50 buildings per location
    python Tools\ue\setup_all.py --list

Prerequisites: the OldDelhiEditor target has compiled (see docs/BUILD_AND_PLAY.md) and the full
UnrealEditor GUI is CLOSED (a second editor on the same project fights over locks).

Order: input -> characters -> crowd -> import (gulbahar, nizamuddin; creates materials + levels)
       -> physics (needs MI_* materials) -> lighting (one run per level; needs the levels).
A failed step is reported and the run continues; exit code is the number of failed steps.
"""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(HERE, "..", ".."))
UPROJECT = os.path.join(PROJECT_DIR, "OldDelhi.uproject")
UE_ROOT = os.environ.get("UE_ROOT", r"C:\Program Files\Epic Games\UE_5.8")
EDITOR_CMD = os.path.join(UE_ROOT, "Engine", "Binaries", "Win64", "UnrealEditor-Cmd.exe")
LOG_DIR = os.path.join(PROJECT_DIR, "Saved", "Logs", "setup_all")

# name, script, script args, map to open first (or None), extra env
STEPS = [
    ("input",       "setup_input.py",      "", None, {}),
    ("characters",  "setup_characters.py", "", None, {}),
    ("crowd",       "setup_crowd.py",      "", None, {}),
    ("import_gulbahar",    "import_location.py", "gulbahar_lines", None, {"OLDDELHI_NO_LIGHTING": "1"}),
    ("import_nizamuddin",  "import_location.py", "nizamuddin",     None, {"OLDDELHI_NO_LIGHTING": "1"}),
    ("physics",     "setup_physics.py",    "", None, {}),
    ("lighting_gulbahar",   "setup_lighting.py", "", "/Game/Maps/Gulbahar_Lines", {"DELHI_CITY": "karachi", "DELHI_DATETIME": "2019-10-14 16:30"}),
    ("lighting_nizamuddin", "setup_lighting.py", "", "/Game/Maps/Nizamuddin",     {"DELHI_CITY": "delhi",   "DELHI_DATETIME": "2019-10-14 16:30"}),
]


def run_step(name, script, args, map_path, env_extra, limit):
    script_path = os.path.join(HERE, script)
    if not os.path.isfile(script_path):
        print("[%s] MISSING %s" % (name, script_path))
        return 1
    os.makedirs(LOG_DIR, exist_ok=True)
    log = os.path.join(LOG_DIR, "%s_%s.log" % (name, time.strftime("%Y%m%d_%H%M%S")))
    script_arg = '"%s %s"' % (script_path, args) if args else '"%s"' % script_path
    cmd = ['"%s"' % EDITOR_CMD, '"%s"' % UPROJECT]
    if map_path:
        cmd.append(map_path)
    cmd += ["-run=pythonscript", "-script=" + script_arg, "-unattended", "-nop4", "-nosplash",
            "-NoSound", "-stdout", "-FullStdOutLogOutput", '-abslog="%s"' % log]
    env = dict(os.environ)
    env.update(env_extra)
    if limit and name.startswith("import_"):
        env["OLDDELHI_LIMIT"] = str(limit)
    print("[%s] %s" % (name, " ".join(cmd)))
    t0 = time.time()
    p = subprocess.run(" ".join(cmd), env=env, shell=True)
    bad = []
    if os.path.isfile(log):
        with open(log, "r", errors="replace") as f:
            for line in f:
                if "Traceback" in line or "LogPython: Error" in line or "Error: " in line and "LogPython" in line:
                    bad.append(line.strip())
    print("[%s] exit %d in %.0fs, %d python error lines. log: %s" % (name, p.returncode, time.time() - t0, len(bad), log))
    for b in bad[:8]:
        print("    " + b)
    return 0 if p.returncode == 0 and not bad else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--only", default="", help="comma list of step names or prefixes (input,physics,lighting)")
    ap.add_argument("--skip", default="", help="comma list of step names or prefixes to skip")
    ap.add_argument("--limit", type=int, default=0, help="import only N buildings per location (smoke test)")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    if a.list:
        for s in STEPS:
            print(s[0], "->", s[1], s[2])
        return 0
    if not os.path.isfile(EDITOR_CMD):
        print("UnrealEditor-Cmd.exe not found at", EDITOR_CMD, "(set UE_ROOT)")
        return 99
    only = [x for x in a.only.split(",") if x]
    skip = [x for x in a.skip.split(",") if x]
    failed = 0
    for name, script, args, map_path, env in STEPS:
        if only and not any(name.startswith(o) for o in only):
            continue
        if any(name.startswith(s) for s in skip):
            continue
        failed += run_step(name, script, args, map_path, env, a.limit)
    print("Done. failed steps: %d" % failed)
    return failed


if __name__ == "__main__":
    sys.exit(main())
