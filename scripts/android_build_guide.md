# Android Build Guide — Speech Rate Meter 2

Package id: `by.intoncore.SpeechRateMeter2`. Native library: `libappspeech-rate-meter-2_<abi>.so`.

`versionName` comes from `project(... VERSION ...)` in `CMakeLists.txt` (currently `1.2`). `versionCode` is `QT_ANDROID_VERSION_CODE` (currently `18`). Increment the version code for every Play upload of this package.

## Prerequisites

Override any of these with environment variables; the values below are the script defaults.

| Item | Default / notes |
|---|---|
| Qt for Android | `$QT_ROOT/android_arm64_v8a` (also `android_x86_64`, `android_armv7`) |
| Qt host tools | `$QT_ROOT/gcc_64` |
| `QT_ROOT` | `$HOME/Qt/6.12.0` |
| Android SDK | `$ANDROID_SDK` → `$HOME/Android/Sdk` |
| Android NDK | `$ANDROID_NDK` → `$ANDROID_SDK/ndk/27.2.12479018` (Clang 18) |
| Build tools | `36.0.0` |
| Target / compile SDK | **36** (`platforms;android-36` must be installed) |
| Min SDK | 26 (Android 8.0) |
| Java | OpenJDK 17 (`$JAVA_HOME`, default `/usr/lib/jvm/java-17-openjdk-amd64`) |
| Tools on `PATH` | `cmake`, `ninja`, `java` |
| OpenMP | NDK `libomp.so` (imported as `AndroidOMP`) |
| Local AVDs | `Pixel7a_x86_64` (3-button nav); `Pixel7a_gesture_x86_64` (gesture nav / SafeArea, created by `run_emulator.sh --gesture`); tablet screenshots: `PlayTablet7_x86_64`, `PlayTablet10_x86_64` (created by `run_emulator.sh --tablet`) |

Install the Android 36 platform if CMake/Gradle fails looking for it:

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
yes | "$HOME/Android/Sdk/cmdline-tools/latest/bin/sdkmanager" "platforms;android-36"
```

All commands below run from the **project root**.

Play Store uploads should use **arm64-v8a only**.

---

## Quick start

### Device / Play ARM64 (APK + AAB)

```bash
./scripts/build_android.sh arm64-v8a release
```

The script configures CMake, compiles `appspeech-rate-meter-2`, then packages both Qt targets: **`apk`** (sideload / emulator) and **`aab`** (Play). Gradle uses Java 17 via `GRADLE_OPTS`.

If a manual `cmake --build ... --target apk` reports that Gradle is using Java 11, export both variables before retrying:

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export GRADLE_OPTS="-Dorg.gradle.java.home=${JAVA_HOME}"
```

Typical outputs:

```
build_android_arm64-v8a/android-build/build/outputs/apk/release/android-build-release.apk
build_android_arm64-v8a/android-build/build/outputs/bundle/release/android-build-release.aab
build_android_arm64-v8a/android-build/build/outputs/native-debug-symbols/release/native-debug-symbols.zip
```

The script also lists `BUNDLE-METADATA/com.android.tools.build.debugsymbols/` inside the release AAB. Play extracts those automatically; do not upload the ZIP unless that metadata is missing.

Build directory names follow the NDK ABI: `build_android_arm64-v8a`, `build_android_x86_64`, `build_android_armeabi-v7a`. They are covered by `/build*/` in `.gitignore`.

### x86_64 emulator (local development)

```bash
./scripts/build_android.sh x86_64 debug
./scripts/run_emulator.sh Pixel7a_x86_64          # 3-button nav
./scripts/run_emulator.sh --gesture               # Pixel7a_gesture_x86_64, gesture nav
```

An ARM64 AVD cannot run on an x86_64 host. Use `Pixel7a_x86_64` or `--gesture` for local testing. `--gesture` is the emulator stand-in for a physical Pixel with gesture navigation (the system bars overlay the app, which is what the SafeArea padding in `ui/Main.qml` is for).

---

## Scripts

### `scripts/build_android.sh`

```bash
./scripts/build_android.sh [arm64-v8a|armeabi-v7a|x86_64|all]  [debug|release]
```

Defaults: `arm64-v8a`, `release`. `armv7` is accepted as an alias for `armeabi-v7a`.

| Env var | Purpose |
|---|---|
| `QT_ROOT` | Qt install root (kits live in `android_*` subdirs) |
| `ANDROID_SDK` | SDK root (`ANDROID_HOME` is set to this) |
| `ANDROID_NDK` | NDK root |
| `JAVA_HOME` | JDK 17 for Gradle |
| `QT_ANDROID_KEYSTORE_PATH` | Upload keystore (enables `-DQT_ANDROID_SIGN_AAB=ON`) |
| `QT_ANDROID_KEYSTORE_ALIAS` | Key alias |
| `QT_ANDROID_KEYSTORE_STORE_PASS` | Keystore password |
| `QT_ANDROID_KEYSTORE_KEY_PASS` | Key password |

`ANDROID_KEYSTORE_PATH`, `ANDROID_KEYSTORE_ALIAS`, and `ANDROID_KEYSTORE_PASSWORD` are aliases that fill the `QT_ANDROID_*` vars when those are unset.

`all` builds each ABI in turn, then packages from the **arm64-v8a** build dir only.

Without a keystore the AAB is still produced but is **not** signed for Play.

Release packaging also runs `scripts/check_16kb_alignment.sh` on the APK and AAB (ELF LOAD alignment, `zipalign -P 16`, AAB `PAGE_ALIGNMENT_16K`). The same check runs after a debug package.

### `scripts/check_16kb_alignment.sh`

```bash
./scripts/check_16kb_alignment.sh path/to/app.apk [path/to/app.aab]
VERBOSE=1 ./scripts/check_16kb_alignment.sh path/to/app.apk   # list every .so
```

Fails if any 64-bit `.so` has ELF `LOAD` alignment below `2**14`, if APK zip alignment is not 16 KB, or if the AAB BundleConfig is not `PAGE_ALIGNMENT_16K` with uncompressed native libraries enabled. `armeabi-v7a` is reported but does not fail (32-bit is exempt).

### `scripts/run_emulator.sh`

```bash
./scripts/run_emulator.sh [avd_name]
./scripts/run_emulator.sh --tablet 7
./scripts/run_emulator.sh --tablet 10
./scripts/run_emulator.sh --gesture
./scripts/run_emulator.sh --screenshot home
./scripts/run_emulator.sh --logcat
```

| Behavior | Detail |
|---|---|
| AVD | Argument, or the first name from `emulator -list-avds` |
| Tablets | `--tablet 7` / `--tablet 10` create and boot Play listing AVDs (`PlayTablet7_x86_64`, `PlayTablet10_x86_64`) if they do not exist, then force portrait `1200×1920` or `1600×2560` |
| Gesture nav | `--gesture` creates and boots `Pixel7a_gesture_x86_64` (Pixel 7a, `1080×2400`) and switches SystemUI to gesture navigation (`navigation_mode=2`). `Pixel7a_x86_64` stays on 3-button nav. |
| Screenshots | `--screenshot [name]` writes a PNG under `packaging/google-play/screenshots/{phone,tablet7,tablet10}/` (`home` → `1_home.png`, `history` → `2_history.png`, `session` → `3_session.png`, `settings` → `4_settings.png`, `guide` → `5_user_guide.png`, `privacy` → `6_privacy.png`) |
| Pre-flight | Requires executable `emulator` and `adb` under `$ANDROID_SDK` |
| Boot timeout | `BOOT_TIMEOUT_SEC` (default 120; tablet boots use 240 if the default is still in effect) |
| Emulator log | `$BUILD_DIR/emulator.log` (dumped if the emulator process dies) |
| Logcat | Started before `am start`; crash-filtered dump if the process is gone |
| APK search | `build_android_x86_64` first, then `build_android_arm64-v8a` (debug, then release, then unsigned) |
| Launch | `adb shell am start -n by.intoncore.SpeechRateMeter2/org.qtproject.qt.android.bindings.QtActivity` |

`--logcat` streams live logcat and always writes `build_android_arm64-v8a/logcat.log`, even if you installed an x86_64 APK. After a normal launch, inspect `$BUILD_DIR/logcat.log` (the dir of the APK that was found).

The APK ABI must match the emulator ABI:

| AVD | Build |
|---|---|
| `Pixel7a_x86_64` | `./scripts/build_android.sh x86_64 debug` |
| `Pixel7a_gesture_x86_64` | `./scripts/build_android.sh x86_64 debug` |
| `PlayTablet7_x86_64` / `PlayTablet10_x86_64` | `./scripts/build_android.sh x86_64 debug` |

### `scripts/clean_android_build.sh`

Removes every `build_android_*` directory in the project root:

```bash
./scripts/clean_android_build.sh            # prompt before deleting
./scripts/clean_android_build.sh --force    # skip confirmation
./scripts/clean_android_build.sh --dry-run  # preview only
```

---

## Manual CMake configure + build

```bash
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export GRADLE_OPTS="-Dorg.gradle.java.home=${JAVA_HOME}"

cmake \
  -S . \
  -B build_android_arm64-v8a \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$HOME/Android/Sdk/ndk/27.2.12479018/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26 \
  -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON \
  -DANDROID_NDK="$HOME/Android/Sdk/ndk/27.2.12479018" \
  -DCMAKE_ANDROID_NDK="$HOME/Android/Sdk/ndk/27.2.12479018" \
  -DCMAKE_FIND_ROOT_PATH="$HOME/Qt/6.12.0/android_arm64_v8a" \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/android_arm64_v8a" \
  -DQT_HOST_PATH="$HOME/Qt/6.12.0/gcc_64" \
  -DQT_HOST_PATH_CMAKE_DIR="$HOME/Qt/6.12.0/gcc_64/lib/cmake" \
  -DCMAKE_BUILD_TYPE=Release \
  -DANDROID_SDK_ROOT="$HOME/Android/Sdk"

cmake --build build_android_arm64-v8a --target appspeech-rate-meter-2 -- -j"$(nproc)"
cmake --build build_android_arm64-v8a --target apk -- -j"$(nproc)"
cmake --build build_android_arm64-v8a --target aab -- -j"$(nproc)"
```

Do not call `androiddeployqt` by hand. Qt’s `apk` / `aab` CMake targets stage libs and generate the Gradle project.

---

## Signing for Google Play

Google Play needs a **signed AAB**. Generate an upload keystore once, keep a backup offline, and never commit it (`*.jks` and `*.keystore` are gitignored).

```bash
keytool -genkey -v \
  -keystore "$HOME/speech-rate-meter-2.jks" \
  -alias speech-rate-meter-2 \
  -keyalg RSA -keysize 2048 -validity 10000
```

Then:

```bash
export QT_ANDROID_KEYSTORE_PATH="$HOME/speech-rate-meter-2.jks"
export QT_ANDROID_KEYSTORE_ALIAS=speech-rate-meter-2
export QT_ANDROID_KEYSTORE_STORE_PASS='<password>'
export QT_ANDROID_KEYSTORE_KEY_PASS='<key-password>'

./scripts/build_android.sh arm64-v8a release
```

The script passes `-DQT_ANDROID_SIGN_AAB=ON` (and `QT_ANDROID_SIGN_APK`) at configure time. The keystore env vars must stay exported during the Gradle/`androiddeployqt` step.

See [QT_ANDROID_SIGN_AAB](https://doc.qt.io/qt-6.12/cmake-variable-qt-android-sign-aab.html) and [Publishing to Google Play](https://doc.qt.io/qt-6.12/android-publishing-to-googleplay.html).

---

## Native debug symbols

`android/build.gradle` sets `android.buildTypes.release.ndk.debugSymbolLevel = 'FULL'`. `CMakeLists.txt` adds `-g` to Android Release objects so DWARF exists for `libappspeech-rate-meter-2_<abi>.so`. Gradle still strips the `.so` files that ship inside the APK/AAB; the extra data goes into AAB metadata.

Confirm after a release build:

```bash
unzip -l build_android_arm64-v8a/android-build/build/outputs/bundle/release/android-build-release.aab \
  | grep com.android.tools.build.debugsymbols
```

---

## 16 KB page-size compatibility

Google Play requires 64-bit native apps that target Android 15+ (API 35+) to support **16 KB memory pages**. See [Support 16 KB page sizes](https://developer.android.com/guide/practices/page-sizes).

What the repo does:

1. **ELF 16 KB alignment (NDK r27).** `CMakeLists.txt` adds `-Wl,-z,max-page-size=16384` and `-Wl,-z,common-page-size=16384` to `appspeech-rate-meter-2`. `scripts/build_android.sh` also passes `-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON`. SPTK and ALGLIB are compiled into `libappspeech-rate-meter-2_<abi>.so`, so they inherit that alignment.
2. **Uncompressed JNI libs (AGP 9.0.0).** `android/build.gradle` parses `legacyPackaging` with `Boolean.parseBoolean`. The app target sets `QT_ANDROID_LEGACY_PACKAGING FALSE`.
3. **Verification.** After packaging, `build_android.sh` runs `scripts/check_16kb_alignment.sh` on the APK and AAB.

Expect every `arm64-v8a` / `x86_64` `.so` to report `ALIGNED (2**14)`, `zipalign: Verification successful`, and AAB `alignment=PAGE_ALIGNMENT_16K` with `uncompress_native_libraries.enabled=1`.

`armeabi-v7a` is exempt from the 16 KB ELF rule (32-bit). Play uploads from this project are **arm64-v8a only**.

---

## Runtime assets

On Android, `CMakeLists.txt` copies `settings.ini` into `android/assets/` so `androiddeployqt` packs it into the APK (extracted under `AppDataLocation` on device). `android/assets/` is gitignored. Session files are written to app-private storage; scratch WAV files are deleted after each phrase is stored.

---

## Package source (`android/`)

- Package: `by.intoncore.SpeechRateMeter2`
- Permission: `RECORD_AUDIO` only
- Required feature: `android.hardware.microphone`
- Portrait only (`android:screenOrientation="portrait"`)
- Min SDK 26, target / compile SDK 36
- Version placeholders filled from CMake (`QT_ANDROID_VERSION_CODE` and `PROJECT_VERSION`)
- `<meta-data android:name="android.app.lib_name" android:value="appspeech-rate-meter-2"/>`
- Launcher icons: `@mipmap/ic_launcher` and `@mipmap/ic_launcher_round`
