# Speech Rate Meter 2

Desktop and Android meter of speaking tempo. A session stays open, each pause-bounded phrase is measured from vowel-like intensity peaks, and the audio is deleted once the numbers are stored.

Package id: `by.intoncore.SpeechRateMeter2`. Version name `1.2`, version code `18`.

The interface is Home, History, Settings, User Guide, Privacy Policy, and Open-source licences. Home shows the current session on one speech-rate gauge. History charts past sessions. There is no waveform and no playback.

All processing is local. The app requests only microphone access (`RECORD_AUDIO`), creates no network connection, and deletes scratch audio after its numeric result is safely stored.

QML module URI: `SpeechRateMeter2`.

## Build

Qt 6.11+ (Quick, Multimedia, LinguistTools), CMake 3.16+, OpenMP, C++17.

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target appspeech-rate-meter-2 -- -j2
```

Executable: `build/appspeech-rate-meter-2`.

The analysis check is `speechrate-analysis-test`.

## Android

`android/AndroidManifest.xml` requests only `RECORD_AUDIO`.
`QT_ANDROID_PACKAGE_NAME` is `by.intoncore.SpeechRateMeter2`.
Native library: `libappspeech-rate-meter-2_<abi>.so`.
