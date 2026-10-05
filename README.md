# Speech Rate Meter 2

Desktop and Android recorder based on the Intonation Trainer 2 audio stack.
Package id: `by.intoncore.SpeechRateMeter2`.

The interface is Home, Settings, User Guide, Privacy Policy, and Open-source licences.
Home records from the microphone, writes a WAV file, and plays it back.
DSP services (VAD, pitch, spectrum, amplitude, UMP, DTW) stay in the tree for later measurement work.
QML module URI: `SpeechRateMeter2`.
Controllers expose only what those screens and the recorder use.

## Build

Qt 6.11+ (Quick, Multimedia, LinguistTools), CMake 3.16+, OpenMP, C++17.

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target appspeech-rate-meter-2 -- -j2
```

Executable: `build/appspeech-rate-meter-2`.

## Android

`android/AndroidManifest.xml` requests `RECORD_AUDIO` only.
`QT_ANDROID_PACKAGE_NAME` is `by.intoncore.SpeechRateMeter2`.
Native library: `libappspeech-rate-meter-2_<abi>.so`.
