# Privacy Notice

**Last updated October 11, 2026**

This is the same notice as the public policy at [https://intontrainer.by/speechratemeter2policy.html](https://intontrainer.by/speechratemeter2policy.html).

Speech Rate Meter 2 ("we") uses the microphone only to estimate speaking tempo on this device. It does not recognize words and does not send audio anywhere.

## What is stored

- **Microphone.** The app asks for microphone access when a session starts. You can revoke that permission in the system settings.
- **Sessions.** Each kept phrase is stored as numbers (pace, articulation, fillers, pauses, and length) in a private `data/sessions` file. The file has no audio.
- **Recording files.** A phrase is written as a WAV file in `data/records` while those numbers are saved. With **Keep recording files** off (the default), that file is deleted when you leave the app. With the setting on, the WAV stays on the device. You can play the recording from Home after Stop, until the app closes or the file is deleted.
- **Settings.** Language, theme, and measurement options are stored locally in `settings.ini` beside the executable on desktop or in the app-private data directory on Android.

The Android package `by.intoncore.SpeechRateMeter2` requests only `RECORD_AUDIO`. The app does not request camera, network, or storage access. Nothing on this list is sent to a server.

## How long it is kept

Session files stay on the device until you tap **Delete user data** in Settings or uninstall the app. Delete user data also removes every WAV still in `data/records`, including files kept by **Keep recording files**. Uninstalling removes the local settings as well.

## Contact

Questions about this notice: zhitko.vladimir@gmail.com
