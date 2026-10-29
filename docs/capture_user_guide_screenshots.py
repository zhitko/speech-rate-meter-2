#!/usr/bin/env python3
"""Capture the Speech Rate Meter 2 screens used by the user guides.

Run from a desktop session, after the app has been built:

    python3 docs/capture_user_guide_screenshots.py en
    python3 docs/capture_user_guide_screenshots.py ru

Needs xdotool, ImageMagick import, pactl, wpctl, paplay, and Pillow.
The app is copied to a temporary directory so the build's sessions and
settings are left alone. A virtual microphone plays a short synthetic
phrase. Pictures are written to docs/images/<lang>/.

The tempo chart in these shots is 30 seconds so a short take is visible.
The guide text says the default chart window is 5 minutes.
"""

import math
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import time
import wave

from PIL import Image

LANG = sys.argv[1] if len(sys.argv) > 1 else "en"
if LANG not in ("en", "ru"):
    raise SystemExit("usage: capture_user_guide_screenshots.py [en|ru]")

DOCS = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(DOCS)
BUILD = os.path.join(PROJECT, "build")
IMAGES = os.path.join(DOCS, "images", LANG)
WINDOW = (1100, 860)


def run(args, check=True):
    return subprocess.run(
        args,
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def write_phrase(path):
    """A few seconds of vowel-like bursts, loud enough for the meter."""
    rate = 48000
    samples = []

    def silence(seconds):
        samples.extend([0] * int(rate * seconds))

    def vowel(seconds, amplitude=14000, frequency=150):
        count = int(rate * seconds)
        for index in range(count):
            envelope = 0.5 - 0.5 * math.cos(2 * math.pi * index / max(1, count - 1))
            samples.append(int(amplitude * envelope * math.sin(2 * math.pi * frequency * index / rate)))

    silence(0.15)
    for _ in range(16):
        vowel(0.11)
        silence(0.10)
    silence(0.35)
    for _ in range(14):
        vowel(0.11)
        silence(0.10)
    silence(0.4)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "w") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(rate)
        frames = b"".join(struct.pack("<h", max(-32767, min(32767, sample))) for sample in samples)
        handle.writeframes(frames)


def prepare_sandbox(root):
    binary = os.path.join(BUILD, "appspeech-rate-meter-2")
    if not os.path.isfile(binary):
        raise SystemExit(f"build the app first: {binary} is missing")
    shutil.copy2(binary, os.path.join(root, "appspeech-rate-meter-2"))
    os.chmod(os.path.join(root, "appspeech-rate-meter-2"), 0o755)
    for name in ("qt.conf", "speech-rate-meter-2_en.qm", "speech-rate-meter-2_ru.qm", "settings.ini"):
        source = os.path.join(BUILD, name)
        if os.path.isfile(source):
            shutil.copy2(source, os.path.join(root, name if name != "settings.ini" else "settings.base.ini"))
    sessions = os.path.join(BUILD, "data", "sessions")
    destination = os.path.join(root, "data", "sessions")
    os.makedirs(destination, exist_ok=True)
    if os.path.isdir(sessions):
        names = sorted(name for name in os.listdir(sessions) if name.endswith(".json"))
        for name in names[:3]:
            shutil.copy2(os.path.join(sessions, name), os.path.join(destination, name))
    write_phrase(os.path.join(root, "data", "tests", "guide-sample.wav"))


def patch_settings(root):
    title = "English" if LANG == "en" else "Русский"
    base = os.path.join(root, "settings.base.ini")
    text = open(base, encoding="utf-8", errors="surrogateescape").read()
    lines = text.splitlines(keepends=True)
    out = []
    # Qt stores beginGroup("General") in the [%General] section.
    in_general = False
    wanted = {
        "language": LANG,
        "languageTitle": title,
        "theme": "light",
        "showNavigationMenu": "true",
        "fontSizeMultiplier": "1",
        "gaugeMode": "0",
        "gaugeAverageCount": "3",
        "analysisWindowSec": "10",
        "updatesPerMinute": "60",
        "chartUseAllReadings": "true",
        "speechRateMethod": "0",
        "articulationMethod": "0",
        "showFillersTile": "false",
        "showArticulationRateTile": "true",
        "showPausesTile": "true",
        "showSpeechTile": "true",
        "showWholeRecordingTile": "true",
        "tempoChartWindowSec": "30",
    }
    seen = set()
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            in_general = stripped == "[%General]"
        if in_general:
            key = stripped.split("=", 1)[0]
            if key in wanted:
                line = f"{key}={wanted[key]}\n"
                seen.add(key)
        out.append(line)
    missing = set(wanted) - seen
    if missing:
        raise SystemExit(f"settings keys missing from {base}: {missing}")
    open(os.path.join(root, "settings.ini"), "w", encoding="utf-8", errors="surrogateescape").write("".join(out))


def find_blob(path, predicate, y0=0.0, y1=1.0, x0=0.0, x1=1.0):
    image = Image.open(path).convert("RGB")
    width, height = image.size
    pixels = image.load()
    points = []
    for y in range(int(height * y0), int(height * y1), 2):
        for x in range(int(width * x0), int(width * x1), 2):
            if predicate(pixels[x, y]):
                points.append((x, y))
    if len(points) < 8:
        return None
    cx = sum(point[0] for point in points) / len(points)
    cy = sum(point[1] for point in points) / len(points)
    return int(cx), int(cy), len(points), (width, height)


def near(color, target, tolerance):
    return all(abs(channel - aim) <= tolerance for channel, aim in zip(color, target))


def blue_button(color):
    return near(color, (0x41, 0x5F, 0x91), 28)


def red_button(color):
    red, green, blue = color
    return red > 170 and red > green + 50 and red > blue + 50


def publish(path):
    """Write a 600-pixel-wide copy next to the guide images."""
    os.makedirs(IMAGES, exist_ok=True)
    image = Image.open(path).convert("RGB")
    height = round(image.height * 600 / image.width)
    destination = os.path.join(IMAGES, os.path.basename(path).split("-", 1)[1])
    image.resize((600, height), Image.Resampling.LANCZOS).save(destination, "PNG", optimize=True)
    print("guide image", destination)


class App:
    def __init__(self, root, wav):
        self.root = root
        self.wav = wav
        self.exe = os.path.join(root, "appspeech-rate-meter-2")
        self.shots = os.path.join(root, "shots")
        os.makedirs(self.shots, exist_ok=True)
        self.proc = None
        self.wid = None
        self.pulse_modules = []
        self.old_source = None
        self.old_wp_source = None

    def audio_source_id(self, label):
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

    def setup_audio(self):
        self.old_source = run(["pactl", "get-default-source"]).stdout.strip()
        self.old_wp_source = self.audio_source_id("*")
        sink = run([
            "pactl", "load-module", "module-null-sink",
            "sink_name=srm_guide",
            "sink_properties=device.description=SRMGuide",
        ]).stdout.strip().splitlines()[-1].strip()
        source = run([
            "pactl", "load-module", "module-remap-source",
            "master=srm_guide.monitor",
            "source_name=srm_mic",
            "source_properties=device.description=SRMMic",
        ]).stdout.strip().splitlines()[-1].strip()
        self.pulse_modules = [source, sink]
        time.sleep(0.4)
        mic_id = self.audio_source_id("SRMMic")
        if not mic_id:
            raise SystemExit("SRMMic did not appear in wpctl status")
        run(["wpctl", "set-default", mic_id])
        run(["pactl", "set-default-source", "srm_mic"])
        print("virtual mic", mic_id, "was", self.old_source, self.old_wp_source)

    def restore_audio(self):
        if self.old_source:
            run(["pactl", "set-default-source", self.old_source], check=False)
        if self.old_wp_source:
            run(["wpctl", "set-default", self.old_wp_source], check=False)
        for module in self.pulse_modules:
            run(["pactl", "unload-module", module], check=False)
        self.pulse_modules = []
        if self.old_source:
            run(["pactl", "set-default-source", self.old_source], check=False)
        if self.old_wp_source:
            run(["wpctl", "set-default", self.old_wp_source], check=False)

    def launch(self):
        env = os.environ.copy()
        env["DISPLAY"] = os.environ.get("DISPLAY", ":0.0")
        env["PULSE_SOURCE"] = "srm_mic"
        env["QT_SCALE_FACTOR"] = "1"
        env["QT_AUTO_SCREEN_SCALE_FACTOR"] = "0"
        self.proc = subprocess.Popen(
            [self.exe],
            cwd=self.root,
            env=env,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        deadline = time.time() + 15
        while time.time() < deadline:
            result = run(["xdotool", "search", "--pid", str(self.proc.pid), "--onlyvisible"], check=False)
            ids = [line for line in result.stdout.split() if line]
            if ids:
                self.wid = ids[-1]
                break
            time.sleep(0.2)
        if not self.wid:
            raise SystemExit("app window did not appear")
        run(["xdotool", "windowsize", self.wid, str(WINDOW[0]), str(WINDOW[1])])
        run(["xdotool", "windowmove", self.wid, "40", "30"])
        run(["xdotool", "windowactivate", "--sync", self.wid])
        time.sleep(0.8)
        print("window", self.wid)

    def close(self):
        if self.proc and self.proc.poll() is None:
            self.proc.send_signal(signal.SIGTERM)
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.proc = None

    def capture(self, name):
        run(["xdotool", "windowactivate", "--sync", self.wid])
        time.sleep(0.35)
        path = os.path.join(self.shots, f"{LANG}-{name}.png")
        run(["import", "-window", self.wid, path])
        print("saved", path)
        if not name.startswith("_"):
            publish(path)
        return path

    def click_xy(self, x, y):
        run(["xdotool", "mousemove", "--window", self.wid, str(int(x)), str(int(y)), "click", "1"])
        time.sleep(0.45)

    def click_blob(self, path, predicate, **kwargs):
        found = find_blob(path, predicate, **kwargs)
        if not found:
            raise SystemExit(f"blob not found in {path}")
        x, y, count, size = found
        print(f"click {path} at {x},{y} n={count} size={size}")
        self.click_xy(x, y)
        return found

    def nav(self, which):
        path = self.capture("_navtmp")
        image = Image.open(path)
        width, height = image.size
        x = {"home": 0.17, "history": 0.50, "settings": 0.83}[which] * width
        y = height - 36
        print("nav", which, int(x), int(y), width, height)
        self.click_xy(x, y)


def main():
    sandbox = tempfile.mkdtemp(prefix="srm-guide-")
    print("sandbox", sandbox)
    prepare_sandbox(sandbox)
    patch_settings(sandbox)
    wav = os.path.join(sandbox, "data", "tests", "guide-sample.wav")
    app = App(sandbox, wav)
    try:
        app.setup_audio()
        app.launch()
        app.nav("history")
        time.sleep(0.5)
        history = app.capture("05-history")
        image = Image.open(history)
        width, _height = image.size
        app.click_xy(int(width * 0.45), 170)
        time.sleep(0.7)
        app.capture("06-session")
        app.nav("settings")
        time.sleep(0.5)
        app.capture("07-settings")
        app.nav("home")
        time.sleep(0.5)

        home = app.capture("01-home-ready")
        start = find_blob(home, blue_button, y0=0.3)
        if not start:
            raise SystemExit("Start button not found")
        print("start at", start[0], start[1], flush=True)
        # Open File is the pale button to the right of Start.
        before = set(run(["xdotool", "search", "--onlyvisible", "."], check=False).stdout.split())
        app.click_xy(start[0] + 118, start[1])
        time.sleep(1.2)
        after = set(run(["xdotool", "search", "--onlyvisible", "."], check=False).stdout.split())
        new_windows = [wid for wid in after - before if wid != app.wid]
        print("dialog windows", new_windows, flush=True)
        dialog = new_windows[-1] if new_windows else app.wid
        run(["xdotool", "windowactivate", "--sync", dialog], check=False)
        run(["xdotool", "key", "--window", dialog, "ctrl+l"], check=False)
        time.sleep(0.25)
        run(["xdotool", "type", "--window", dialog, "--delay", "5", wav], check=False)
        time.sleep(0.2)
        run(["xdotool", "key", "--window", dialog, "Return"], check=False)
        time.sleep(8.0)
        app.capture("04-open-file")

        app.nav("home")
        time.sleep(0.4)
        home = app.capture("01-home-ready")
        app.click_blob(home, blue_button, y0=0.3)
        time.sleep(0.6)
        player = subprocess.Popen(
            ["paplay", "--device=srm_guide", wav],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        time.sleep(6.0)
        recording = app.capture("02-home-recording")
        # The gauge arc is red on the right. The Stop button is further right.
        stop = find_blob(recording, red_button, y0=0.3, y1=0.7, x0=0.85)
        player.wait(timeout=30)
        time.sleep(2.6)
        if not stop:
            raise SystemExit("Stop button not found")
        print("stop at", stop[0], stop[1], flush=True)
        app.click_xy(stop[0], stop[1])
        time.sleep(5.0)
        app.capture("03-home-result")
    finally:
        app.close()
        app.restore_audio()
        print("left sandbox at", sandbox)


if __name__ == "__main__":
    main()
