# Privacy Notice

**Last updated October 6, 2026**

Speech Rate Meter 2 ("we") uses the microphone only to estimate speaking tempo on this device. It does not recognize words and does not send audio anywhere.

## What is stored

- **Microphone.** The app asks for microphone access when a session starts. You can revoke that permission in the system settings.
- **Sessions.** Each kept phrase is stored as numbers (pace, articulation, fillers, pauses, and length) in a private `data/sessions` file. The file has no audio.
- **Scratch audio.** A phrase may be written briefly as a WAV file in `data/records` while those numbers are saved. That file is deleted as soon as the save succeeds. After a session, and whenever the app is idle, that folder is empty unless a save failed and the file is still needed.
- **Settings.** Language, theme, and measurement options are stored locally in `settings.ini`.

The Android package also declares camera, network, and storage permissions. The speech-rate flow does not use them. Nothing on this list is sent to a server.

## How long it is kept

Session files stay on the device until you tap **Delete user data** in Settings or uninstall the app. Delete user data also removes any scratch WAV still in `data/records`. Uninstalling removes the local settings as well.

## Contact

Questions about this notice: zhitko.vladimir@gmail.com
