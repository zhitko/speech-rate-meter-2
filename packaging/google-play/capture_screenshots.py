#!/usr/bin/env python3
"""Capture Speech Rate Meter 2 screens for the Google Play listing.

Uses the Android emulator and the x86_64 debug APK, not the desktop window.
Build that APK first:

    ./scripts/build_android.sh x86_64 debug

Then, from a desktop session (xdotool is not required):

    python3 packaging/google-play/capture_screenshots.py
    python3 packaging/google-play/capture_screenshots.py phone
    python3 packaging/google-play/capture_screenshots.py tablet7 tablet10

Needs adb, the emulator, pactl, wpctl, paplay, and ImageMagick.
A virtual microphone plays a short synthetic phrase into the emulator.
Pictures are written under packaging/google-play/screenshots/.

The tempo chart in these shots is 30 seconds so a short take is visible.
The default chart window is 5 minutes. Language, theme, and that chart
window are written into the app's settings.ini on the emulator before
the shots; everything else stays as the APK seeded it.
"""

import os
import re
import struct
import subprocess
import sys
import tempfile
import time
import wave
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(os.path.dirname(ROOT))
OUT = os.path.join(ROOT, "screenshots")
SDK = os.environ.get("ANDROID_SDK", os.path.expanduser("~/Android/Sdk"))
ADB = os.path.join(SDK, "platform-tools", "adb")
PACKAGE = "by.intoncore.SpeechRateMeter2"
ACTIVITY = f"{PACKAGE}/org.qtproject.qt.android.bindings.QtActivity"

PROFILES = {
    "phone": {"args": ["Pixel7a_x86_64"], "png": (1080, 2400)},
    "tablet7": {"args": ["--tablet", "7"], "png": (1200, 1920)},
    "tablet10": {"args": ["--tablet", "10"], "png": (1600, 2560)},
}


def run(args, check=True, text=True):
    return subprocess.run(
        args,
        check=check,
        text=text,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def adb(*args, check=True, text=True):
    return run([ADB, *args], check=check, text=text)


def write_phrase(path):
    """Long enough for a few chart points at the default update rate."""
    rate = 48000
    samples = []

    def silence(seconds):
        samples.extend([0] * int(rate * seconds))

    def vowel(seconds, amplitude=14000, frequency=150):
        count = int(rate * seconds)
        for index in range(count):
            envelope = 0.5 - 0.5 * math_cos(index, count)
            samples.append(int(amplitude * envelope * math_sin(frequency, index, rate)))

    silence(0.2)
    for _ in range(40):
        vowel(0.12)
        silence(0.11)
    silence(0.45)
    for _ in range(36):
        vowel(0.12)
        silence(0.11)
    silence(0.4)
    with wave.open(path, "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        frames = b"".join(struct.pack("<h", max(-32767, min(32767, sample))) for sample in samples)
        handle.writeframes(frames)


def math_cos(index, count):
    import math
    return math.cos(2 * math.pi * index / max(1, count - 1))


def math_sin(frequency, index, rate):
    import math
    return math.sin(2 * math.pi * frequency * index / rate)


class Mic:
    def __init__(self):
        self.modules = []
        self.old_source = None
        self.old_wp = None

    def source_id(self, label):
        status = run(["wpctl", "status"]).stdout.split("Video", 1)[0]
        in_sources = False
        for line in status.splitlines():
            if "Sources:" in line and "endpoints" not in line:
                in_sources = True
                continue
            if in_sources and "Source endpoints" in line:
                break
            if not in_sources:
                continue
            if label == "*" and "*" not in line:
                continue
            if label != "*" and label not in line:
                continue
            match = re.search(r"(\d+)\.", line)
            if match:
                return match.group(1)
        return None

    def start(self):
        self.old_source = run(["pactl", "get-default-source"]).stdout.strip()
        self.old_wp = self.source_id("*")
        sink = run([
            "pactl", "load-module", "module-null-sink",
            "sink_name=srm_play",
            "sink_properties=device.description=SRMPlay",
        ]).stdout.strip().splitlines()[-1].strip()
        source = run([
            "pactl", "load-module", "module-remap-source",
            "master=srm_play.monitor",
            "source_name=srm_play_mic",
            "source_properties=device.description=SRMPlayMic",
        ]).stdout.strip().splitlines()[-1].strip()
        self.modules = [source, sink]
        time.sleep(0.4)
        mic_id = self.source_id("SRMPlayMic")
        if not mic_id:
            raise SystemExit("SRMPlayMic did not appear in wpctl status")
        run(["wpctl", "set-default", mic_id])
        run(["pactl", "set-default-source", "srm_play_mic"])
        print("virtual mic", mic_id, "was", self.old_source, self.old_wp)

    def restore(self):
        if self.old_source:
            run(["pactl", "set-default-source", self.old_source], check=False)
        if self.old_wp:
            run(["wpctl", "set-default", self.old_wp], check=False)
        for module in self.modules:
            run(["pactl", "unload-module", module], check=False)
        self.modules = []
        if self.old_source:
            run(["pactl", "set-default-source", self.old_source], check=False)
        if self.old_wp:
            run(["wpctl", "set-default", self.old_wp], check=False)


def boot(profile):
    """Start the AVD, install the debug APK, and launch the app."""
    script = os.path.join(PROJECT, "scripts", "run_emulator.sh")
    print("starting", profile, PROFILES[profile]["args"], flush=True)
    result = subprocess.run([script, *PROFILES[profile]["args"]], cwd=PROJECT)
    if result.returncode != 0:
        raise SystemExit(f"emulator launch failed for {profile}")


def launch():
    adb("shell", "pm", "grant", PACKAGE, "android.permission.RECORD_AUDIO", check=False)
    adb("shell", "am", "start", "-n", ACTIVITY)


def allow_microphone(tree):
    found = find_nodes(tree, "While using the app")
    if not found:
        return tree
    tap_node(found[0])
    time.sleep(1)
    return dump_ui()


def force_stop():
    adb("shell", "am", "force-stop", PACKAGE)


def wait_for_settings():
    deadline = time.time() + 40
    while time.time() < deadline:
        found = adb(
            "shell", "run-as", PACKAGE, "find", ".", "-name", "settings.ini",
            check=False,
        )
        paths = [line.strip() for line in found.stdout.splitlines() if line.strip().endswith("settings.ini")]
        if paths:
            return paths[0].lstrip("./")
        time.sleep(1)
    raise SystemExit("settings.ini did not appear in the app data directory")


def patch_settings(relative):
    pulled = adb("exec-out", "run-as", PACKAGE, "cat", relative, text=False)
    text = pulled.stdout.decode("utf-8", errors="surrogateescape")
    if not text.strip():
        raise SystemExit(f"could not read {relative}")
    wanted = {
        "language": "en",
        "languageTitle": "English",
        "theme": "light",
        "showNavigationMenu": "true",
        "fontSizeMultiplier": "1",
        "tempoChartWindowSec": "30",
    }
    lines = text.splitlines(keepends=True)
    if lines and not lines[-1].endswith("\n"):
        lines[-1] += "\n"
    out = []
    section = None
    seen = set()
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            section = stripped
        if section in ("[General]", "[%General]"):
            key = stripped.split("=", 1)[0]
            if key in wanted:
                line = f"{key}={wanted[key]}\n"
                seen.add((section, key))
        out.append(line)
    for section in ("[General]", "[%General]"):
        missing = [key for key in wanted if (section, key) not in seen]
        if not missing:
            continue
        if section + "\n" not in "".join(out) and f"{section}\r\n" not in "".join(out):
            continue
        # Keys live in one of the two General sections. Add any this
        # section did not already have, just after its header.
        inserted = []
        for index, line in enumerate(out):
            if line.strip() == section:
                insert_at = index + 1
                for key in missing:
                    inserted.append(f"{key}={wanted[key]}\n")
                out[insert_at:insert_at] = inserted
                break
    patched = "".join(out).encode()
    remote = "/data/local/tmp/srm-settings.ini"
    handle, local = tempfile.mkstemp(prefix="srm-settings-", suffix=".ini")
    try:
        os.write(handle, patched)
        os.close(handle)
        pushed = run([ADB, "push", local, remote])
        if pushed.returncode != 0:
            raise SystemExit(pushed.stderr or "adb push failed")
        # The APK seeds settings.ini as read-only, so a plain copy cannot replace it.
        adb("shell", "run-as", PACKAGE, "chmod", "u+w", relative, check=False)
        copied = adb(
            "shell", "run-as", PACKAGE, "cp", remote, relative,
            check=False,
        )
        if copied.returncode != 0:
            raise SystemExit(copied.stderr or copied.stdout or "could not copy settings.ini")
    finally:
        os.unlink(local)
    print("patched", relative)


def dump_ui():
    last = ""
    for _ in range(6):
        remote = "/sdcard/srm-uidump.xml"
        dumped = adb("shell", "uiautomator", "dump", remote, check=False)
        last = (dumped.stdout or "") + (dumped.stderr or "")
        if dumped.returncode == 0 and "dumped" in last.lower():
            pulled = adb("exec-out", "cat", remote, check=False, text=False)
            body = pulled.stdout.decode("utf-8", errors="replace").strip()
            if body.startswith("<?xml") or body.startswith("<hierarchy"):
                return ET.fromstring(body)
        time.sleep(0.7)
    raise SystemExit(f"uiautomator dump failed: {last.strip()}")


def bounds_of(node):
    match = re.match(r"\[(\d+),(\d+)\]\[(\d+),(\d+)\]", node.attrib.get("bounds", ""))
    if not match:
        return None
    x0, y0, x1, y1 = (int(value) for value in match.groups())
    return x0, y0, x1, y1


def center_of(node):
    box = bounds_of(node)
    if not box:
        return None
    x0, y0, x1, y1 = box
    return (x0 + x1) // 2, (y0 + y1) // 2


def nodes(tree):
    return list(tree.iter("node"))


def label(node):
    return node.attrib.get("text") or node.attrib.get("content-desc") or ""


def tap(x, y):
    adb("shell", "input", "tap", str(int(x)), str(int(y)))
    time.sleep(0.7)


def tap_node(node):
    point = center_of(node)
    if not point:
        raise SystemExit("node has no bounds")
    print("tap", label(node), point)
    tap(*point)


def find_nodes(tree, text):
    found = []
    for node in nodes(tree):
        if label(node) == text or label(node).startswith(text):
            if bounds_of(node):
                found.append(node)
    return found


def tap_text(tree, text, prefer="bottom"):
    found = find_nodes(tree, text)
    if not found:
        raise SystemExit(f"could not find {text!r} on screen")
    found.sort(key=lambda node: center_of(node)[1])
    tap_node(found[-1] if prefer == "bottom" else found[0])


def open_menu(tree):
    """The toolbar menu is the control just left of the page title."""
    titles = []
    for node in nodes(tree):
        if label(node) in ("Home", "History", "Settings", "User Guide", "Privacy Policy"):
            box = bounds_of(node)
            if box and box[1] < 400:
                titles.append(node)
    if not titles:
        raise SystemExit("toolbar title not found")
    titles.sort(key=lambda node: bounds_of(node)[1])
    box = bounds_of(titles[0])
    x = max(24, box[0] - 48)
    y = (box[1] + box[3]) // 2
    print("menu", x, y)
    tap(x, y)


def wait_until(predicate, timeout, message):
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        last = dump_ui()
        if predicate(last):
            return last
        time.sleep(0.8)
    raise SystemExit(message)


def capture(profile, filename):
    raw = os.path.join(tempfile.gettempdir(), f"srm-{profile}-{filename}")
    shot = adb("exec-out", "screencap", "-p", check=False, text=False)
    if shot.returncode != 0 or not shot.stdout.startswith(b"\x89PNG"):
        raise SystemExit(f"screencap failed for {filename}")
    open(raw, "wb").write(shot.stdout)
    folder = os.path.join(OUT, profile)
    os.makedirs(folder, exist_ok=True)
    dest = os.path.join(folder, filename)
    converted = run([
        "convert", raw, "-alpha", "remove", "-alpha", "off", f"PNG24:{dest}",
    ], check=False)
    if converted.returncode != 0:
        os.replace(raw, dest)
    else:
        os.remove(raw)
    identified = run(["identify", "-format", "%w %h", dest], check=False)
    size = identified.stdout.strip()
    expected = PROFILES[profile]["png"]
    print("saved", dest, size)
    if size != f"{expected[0]} {expected[1]}":
        print(f"warning: expected {expected[0]}x{expected[1]}")
    return dest


def play_phrase(path):
    return subprocess.Popen(
        ["paplay", "--device=srm_play", path],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def capture_profile(profile, wav):
    boot(profile)
    adb("shell", "pm", "clear", PACKAGE)
    launch()
    relative = wait_for_settings()
    force_stop()
    patch_settings(relative)
    launch()
    home = wait_until(
        lambda tree: find_nodes(tree, "Start recording") or find_nodes(tree, "Start"),
        45,
        "Home did not show Start",
    )
    home = allow_microphone(home)
    start_label = "Start recording" if find_nodes(home, "Start recording") else "Start"
    tap_text(home, start_label, prefer="bottom")
    time.sleep(1)
    home = allow_microphone(dump_ui())
    if find_nodes(home, start_label):
        tap_text(home, start_label, prefer="bottom")
    player = play_phrase(wav)
    try:
        time.sleep(12)
        recording = wait_until(
            lambda tree: find_nodes(tree, "Stop recording") or find_nodes(tree, "Stop"),
            20,
            "Stop did not appear",
        )
        tap_text(recording, "Stop recording" if find_nodes(recording, "Stop recording") else "Stop", prefer="bottom")
    finally:
        if player.poll() is None:
            player.wait(timeout=30)
    time.sleep(6)
    capture(profile, "1_home.png")

    history = dump_ui()
    tap_text(history, "History")
    opened = wait_until(
        lambda tree: any(label(node).startswith("Open session") for node in nodes(tree)),
        15,
        "history has no sessions",
    )
    capture(profile, "2_history.png")
    sessions = [node for node in nodes(opened) if label(node).startswith("Open session")]
    sessions.sort(key=lambda node: center_of(node)[1])
    tap_node(sessions[0])
    time.sleep(1.2)
    capture(profile, "3_session.png")

    listing = dump_ui()
    tap_text(listing, "Settings")
    time.sleep(0.8)
    settings = dump_ui()
    capture(profile, "4_settings.png")

    open_menu(settings)
    guide = wait_until(
        lambda tree: find_nodes(tree, "User Guide"),
        10,
        "drawer did not show User Guide",
    )
    tap_text(guide, "User Guide", prefer="top")
    time.sleep(0.8)
    guide_page = dump_ui()
    capture(profile, "5_user_guide.png")

    open_menu(guide_page)
    privacy = wait_until(
        lambda tree: find_nodes(tree, "Privacy Policy"),
        10,
        "drawer did not show Privacy Policy",
    )
    tap_text(privacy, "Privacy Policy", prefer="top")
    time.sleep(0.8)
    capture(profile, "6_privacy.png")


def main():
    names = sys.argv[1:] or list(PROFILES)
    unknown = [name for name in names if name not in PROFILES]
    if unknown:
        raise SystemExit("usage: capture_screenshots.py [phone|tablet7|tablet10] ...")
    if not os.path.isfile(ADB):
        raise SystemExit(f"adb not found: {ADB}")
    apk_root = os.path.join(PROJECT, "build_android_x86_64")
    if not os.path.isdir(apk_root):
        raise SystemExit("build the emulator APK first: ./scripts/build_android.sh x86_64 debug")
    wav = os.path.join(tempfile.gettempdir(), "srm-play-phrase.wav")
    write_phrase(wav)
    mic = Mic()
    try:
        mic.start()
        # The emulator captures whichever source was default when it booted.
        adb("emu", "kill", check=False)
        time.sleep(2)
        for name in names:
            capture_profile(name, wav)
    finally:
        mic.restore()


if __name__ == "__main__":
    main()
