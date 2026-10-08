# Speech Rate Meter 2

**Speech Rate Meter 2** is a cross-platform desktop and Android application that measures how fast you are speaking. A session stays open while you talk. Each pause-bounded phrase is measured from vowel-like intensity peaks, and the microphone audio is deleted as soon as the numbers are stored.

It does not recognize words and does not use a speech-recognition model. Pace, articulation, fillers, pauses, and speech time are estimated on the device from the intensity contour of the recording.

This repository is a rebuild of the earlier [Speech Rate Meter](https://github.com/zhitko/speech-rate-meter) (Qt 5, separate Inton Core library). Version 2 keeps the same four tempo measures, adds a filler score and a live analysis window, and keeps only the numeric result.

Designed for speakers, announcers, call-center operators, language learners, and clinicians who want an objective, repeatable reading of speaking tempo — for rehearsal, self-control, or observation of oral speech.

Package id: `by.intoncore.SpeechRateMeter2`. Version name `1.2`, version code `18`.

---

## What It Does

### Core Concept

1. Press **Start**. The microphone stays open for the whole session.
2. With **Use Speech Autodetection** on, a pause ends the current phrase, the phrase is measured, and the next one starts when you speak again. With it off, the take is measured from the first sample and is split into 15-second parts.
3. While you speak, speech rate, articulation, fillers, and pauses follow the **analysis window** — the most recent kept speech (10 seconds by default) — so a change of pace shows within a few seconds. **Speech** is the total time measured since Start.
4. Press **Stop**. Home and History then show one result for every phrase kept from Start to Stop. A phrase shorter than one second is dropped.
5. The scratch WAV is deleted once that phrase’s numbers are in the session file. There is no waveform and no playback.

All processing is local. The Android package requests only microphone access (`RECORD_AUDIO`).

### The five measures

| On screen | Meaning | Unit |
| --- | --- | --- |
| **Speech rate** | Pace since Start, pauses inside phrases included | words per minute |
| **Articulation** | Pace while speech is actually going | words per minute |
| **Fillers** | How much of the speech is stretched sounds | percent |
| **Pauses** | Extra gaps inside the kept phrases | seconds |
| **Speech** | Analyzed speech since Start | seconds |

Speech rate is vowels per minute scaled into words per minute (`K1`, default 0.71). Articulation removes the typical gap between vowel nuclei. Pauses are the excess of long gaps over the typical gap. Fillers are the excess of long vowel nuclei over the typical nucleus, shown as a percent of a calibrated range. The full formulas are in [docs/TECHNICAL_DESCRIPTION.md](docs/TECHNICAL_DESCRIPTION.md).

---

## User Experience Flow

### 1. Home

Home is the only screen with the record button.

- A **speech-rate gauge** (240° arc, Slow / Average / Fast) shows the current pace. The marker sits between Slow and Fast (70 and 210 wpm by default). The printed number is the real value. While the session is on, a microphone mark and a bar inside the gauge follow the input level, so it is obvious whether the microphone is hearing sound.
- Four tiles beside it: Articulation, Fillers, Pauses, and Speech.
- A round **Start / Stop** button. While the session is on, a halo around it grows with the microphone level.
- A status chip names the state (Ready, Listening, Too short, Measuring, Not saved, Microphone blocked) and, during a session, the open phrase timer (`mm:ss`).

On a wide or landscape window the gauge sits on the left and the tiles and button on the right. On a phone the button stays pinned to the bottom of the page.

While a session is running, a **Recording** chip stays in the toolbar on every page except Settings and returns to Home. Opening Settings stops the session first.

### 2. History

History lists every finished session, newest first. A row shows the date and clock span, how many phrases were kept, how many times the numbers changed, and the pace from Start to Stop.

Opening a row repeats those five session values, then five charts — one point for each change that appeared on Home. These are metric charts, not a waveform.

### 3. Settings

An **Advanced** toggle at the top of the page is not saved and turns off when you leave the app. With Advanced off, only the everyday controls are shown.

| Section | What you control |
| --- | --- |
| **General (always visible)** | UI language (EN/RU), light/dark/system theme, accent color (Blue/Green/Purple/Orange/Red), navigation menu, font size, delete user data |
| **Measurement (always visible)** | Analysis window, updates per minute, pause length, Use Speech Autodetection, Slow and Fast ends of the gauge |
| **Advanced (hidden behind the toggle)** | Voice-activity method and thresholds, calibration, intensity frame/shift/smooth, vowel-length limit, power-mean degree, coefficients K1–K4 and the filler range, Open File (desktop) |

**Use Speech Autodetection** measures background noise after Start, then finds speech and pauses. When it is off, the recording is analyzed from the first sample and a pause does not end the phrase.

**Delete user data** removes saved sessions. Recorded audio is already gone. Settings are kept.

**Open File** (desktop, Advanced) measures one WAV without saving a session. It accepts only 8000 Hz, mono, signed 16-bit PCM.

Settings take effect on the next phrase. The in-app [User Guide](docs/user_guide_en.md) describes the same flow for someone using the app.

### 4. Other pages

- **User Guide** — how to record, read History, and change settings. English and Russian.
- **Privacy Policy** — what stays on the device. Microphone audio is not kept, and nothing is sent off the device.
- **Open-source licences** — Qt, ALGLIB, SPTK, Font Awesome, and the other bundled components.

---

## Key Technical Features

| Feature | Description |
| --- | --- |
| **Intensity nuclei** | Short-time mean absolute amplitude, normalized and smoothed. Vowel-like peaks are runs that rise above the smooth contour. Tempo is counted from those nuclei, not from recognized words. |
| **Live analysis window** | While a session is open, four of the five numbers describe the last N seconds of kept speech (default 10 s, 3…30). Snapshots are published up to the configured updates per minute (default 60). |
| **Pause cutting** | With autodetection on, a silence of Silence Duration (default 2 s) closes a phrase. Speech longer than 15 s is cut and continued in the next phrase. Phrases under 1 s are dropped. |
| **Voice activity detection** | Energy, autocorrelation, or hybrid (both). Optional calibration measures background noise before the session listens for speech. |
| **Session file** | One JSON file per session under `data/sessions/`: every on-screen change and every kept phrase, including the vowel and gap lengths needed to recompute the joined result. No audio path. |
| **Scratch audio** | A temporary WAV under `data/records/` exists only while that phrase is being stored, then is deleted. After a successful save the folder is empty. |
| **Speech gate** | A buffer whose nuclei do not rise above the noise floor is not published and not stored, so silence does not read as fast speech. |
| **Open File** | Desktop-only whole-file analysis of an 8000 Hz mono s16le WAV. It is not cut into phrases and is not written to History. |
| **i18n** | English and Russian UI via Qt Linguist. |

---

## Application Pages

```
Home
├── Start / Stop session
│   └── (Advanced) Details — intermediate vowel and gap statistics
├── History
│   └── [session] → five metric charts
├── Settings
└── User Guide, Privacy Policy, Open-source licences
```

---

## Requirements

### Build Requirements

- **CMake**: 3.16 or higher
- **Qt**: 6.12 or higher with the following modules:
  - Qt Quick
  - Qt Multimedia
  - Qt Linguist Tools
- **C++ Compiler**: Supporting C++17 or higher
- **OpenMP**: For parallelized DSP routines
- **Git**: For cloning the repository

### Android Build Requirements (optional)

- **Android NDK**: 26+ (with LLVM/clang toolchain; r27c is what the current package links for `libomp`)
- **Java**: 17 (required by Gradle for AAB/APK packaging)
- **Android SDK**: compileSdk 36, minSdk 26

### Runtime Requirements

- Operating System: Windows, Linux, macOS, or Android 8+ (API 26+)
- A microphone

---

## Build Instructions

### Clone the Repository

```bash
git clone https://github.com/zhitko/speech-rate-meter-2.git
cd speech-rate-meter-2
```

### Clone 3rd-Party Dependencies

The following libraries are required. They are **not** included in the repository (`3rdparty/*` is gitignored) and must be fetched separately.

#### SPTK (Speech Signal Processing Toolkit 4.3)

Audio feature processing. The build uses commit `68f4158`.

```bash
git clone https://github.com/sp-nitech/SPTK.git 3rdparty/SPTK
cd 3rdparty/SPTK && git checkout 68f4158 && cd ../..
```

#### ALGLIB 4.06.0

Sample moments used by the vowel and gap statistics.

```bash
wget https://www.alglib.net/translator/re/alglib-4.06.0.cpp.gpl.zip
unzip alglib-4.06.0.cpp.gpl.zip -d 3rdparty/alglib-cpp
```

### Build Steps

#### Linux/macOS

```bash
mkdir build
cd build
cmake ..
cmake --build . --target appspeech-rate-meter-2 -- -j2
```

#### Linux AppImage

`scripts/build_appimage.sh` builds an x86_64 AppImage with the program, Qt, QML modules, the FFmpeg multimedia plugin, the OpenMP runtime, and `settings.ini`.

It needs the Qt **gcc_64** kit. The image runs on systems whose glibc is at least as new as the machine that built it. The microphone uses the host PulseAudio or PipeWire library.

```bash
QT_ROOT="$HOME/Qt/6.12.0" ./scripts/build_appimage.sh release
```

The image is written to:

```
build_appimage/SpeechRateMeter2-<version>-x86_64.AppImage
```

Keep that file in a writable directory. `settings.ini` and `data/sessions` are stored beside it.

#### Windows

`scripts/build_windows.sh` builds a 64-bit folder and zip that includes the program, Qt, QML modules, multimedia plugins, the OpenMP runtime, and `settings.ini`.

On Windows, run it from Git Bash with the Qt **MSVC 2022 64-bit** or **MinGW 13.1 64-bit** kit installed. On Linux it cross-compiles. 64-bit Wine (`wine64`) must be installed, and the Qt MinGW kit must sit next to the matching `gcc_64` kit.

The released `aqtinstall` 3.3.0 cannot download Qt 6.12 Windows kits. It looks for `qt6_6120/qt6_6120/Updates.xml`, while Qt publishes the MinGW kit at `qt6_6120/qt6_6120_mingw/Updates.xml`. Install aqt from git, then install the kit:

```bash
sudo apt install g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 wine64
sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

python3 -m pip install --user --upgrade --force-reinstall --no-cache-dir "git+https://github.com/miurahr/aqtinstall.git"
python3 -m aqt install-qt --outputdir "$HOME/Qt" windows desktop 6.12.0 win64_mingw -m qtmultimedia qttasktree
ln -sfn win64_mingw "$HOME/Qt/6.12.0/mingw_64"
```

The compiler must be GCC 13 with the posix thread model. aqt writes the kit to `~/Qt/6.12.0/win64_mingw`; the symlink is the path the script expects. Then:

```bash
QT_ROOT="$HOME/Qt/6.12.0" ./scripts/build_windows.sh release
```

The zip is written to:

```
build_windows/SpeechRateMeter2-<version>-win64.zip
```

Extract it to a writable directory. Settings and sessions are stored beside `SpeechRateMeter2.exe`.

To compile on Windows without packaging, from a Visual Studio 2022 x64 prompt:

```bash
cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
cmake --build build --config Release --target appspeech-rate-meter-2
```

#### Android (arm64-v8a)

From the project root, with Qt 6.12.0, NDK r27, and JDK 17 (see [scripts/android_build_guide.md](scripts/android_build_guide.md)):

```bash
./scripts/build_android.sh arm64-v8a release
./scripts/build_android.sh x86_64 debug
./scripts/run_emulator.sh Pixel7a_x86_64
```

The project already sets:

- package name `by.intoncore.SpeechRateMeter2`
- version name from `project(VERSION)` (currently `1.2`)
- `QT_ANDROID_VERSION_CODE` `18` — increment this in `CMakeLists.txt` for every store upload
- minSdk 26, target and compile SDK 36
- 16 KB ELF page alignment for Play devices

`android/AndroidManifest.xml` requests only `RECORD_AUDIO`. The native library is `libappspeech-rate-meter-2_<abi>.so`.

### Tests

Desktop builds register two checks:

```bash
cd build
ctest --output-on-failure
```

`speechrate-analysis-test` checks the tempo formulas. `sessionstore-test` checks session files.

### Run the Application

After building, the executable is in the build directory:

- **Linux/macOS**: `./appspeech-rate-meter-2`
- **Windows**: `Release\appspeech-rate-meter-2.exe`

`settings.ini` is copied next to the executable at build time. Session files are written to `data/sessions/` beside it.

---

## Project Structure

```
speech-rate-meter-2/
├── src/
│   ├── api/                            # QML-exposed C++ backends
│   │   ├── audioapi.{h,cpp}           # Microphone capture, VAD, live level
│   │   ├── sessionapi.{h,cpp}         # Session list and charts
│   │   ├── settingsapi.{h,cpp}        # Settings exposed to QML
│   │   ├── fileapi.{h,cpp}            # Open File (desktop WAV)
│   │   ├── qmllogger.{h,cpp}
│   │   └── helpers/                    # App settings, beep
│   └── services/
│       ├── speechrateanalysis.{h,cpp} # Intensity nuclei and the five metrics
│       ├── sessionstore.{h,cpp}       # JSON sessions, scratch WAV lifetime
│       ├── vadenergryservice.{h,cpp}
│       ├── vadautocorrelationservice.{h,cpp}
│       ├── wavfileservice.{h,cpp}
│       └── helpers/
├── ui/
│   ├── pages/                          # Home, History, Session, Settings, Guide, Privacy, Licences
│   ├── components/                     # Gauge, tiles, charts, dialogs
│   └── utils/                          # Theme, icons, scale, logger
├── res/                                # Font Awesome, icons
├── i18n/                               # English and Russian (.ts)
├── android/                            # Manifest, Gradle, launcher icon
├── scripts/                            # Android, Windows, and Linux AppImage builds
├── docs/                               # Technical description, user guide, privacy policy
├── licenses/                           # Third-party notices bundled in the app
├── packaging/                          # Desktop entry and icons
├── tests/                              # Analysis and session-store checks
├── 3rdparty/                           # SPTK and ALGLIB (fetched separately)
├── CMakeLists.txt
├── settings.ini
└── README.md
```

QML module URI: `SpeechRateMeter2`.

---

## License

The application's own source files are offered under the [MIT License](LICENSE).
The built executable also incorporates GPL-licensed ALGLIB, so distribution of
the combined executable is subject to the GNU GPL. Qt is dynamically linked
under LGPLv3. See [Open-source notices](licenses/THIRD_PARTY_NOTICES.md) for
complete attributions, source availability, and Qt relinking information.

---

## Third-Party Libraries

| Library | Purpose |
| --- | --- |
| **[SPTK](https://github.com/sp-nitech/SPTK)** (Speech Signal Processing Toolkit 4.3) | Audio feature processing. Commit `68f4158`, Apache License 2.0. |
| **[ALGLIB](https://www.alglib.net/)** 4.06.0 | Sample moments for vowel and gap statistics. GPL edition, compiled into the application. |
| **[Font Awesome](https://fontawesome.com/)** (Free 7.2.0) | Icon font used throughout the UI. SIL Open Font License 1.1. |
| **LLVM OpenMP** | Android package links `libomp.so` from the NDK. Apache License 2.0 with LLVM Exceptions. |

---

## Authors

- **Boris Lobanov** — Scientific — [LinkedIn](https://www.linkedin.com/in/boris-lobanov-50628384/)
- **Vladimir Zhitko** — Development — [LinkedIn](https://www.linkedin.com/in/zhitko-vladimir-92662255/)
