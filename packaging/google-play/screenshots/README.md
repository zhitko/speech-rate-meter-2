# Play Console screenshots

Android captures for the **Speech Rate Meter 2** listing
(`by.intoncore.SpeechRateMeter2`).

Light theme, English UI, portrait. Each folder has the same six screens.
Filenames are `N_screen_name.png` and match the phone listing order.

| # | Screen | Phone `1080×2400` | 7-inch `1200×1920` | 10-inch `1600×2560` |
|---|---|---|---|---|
| 1 | Home (tempo chart, Start) | `phone/1_home.png` | `tablet7/1_home.png` | `tablet10/1_home.png` |
| 2 | History | `phone/2_history.png` | `tablet7/2_history.png` | `tablet10/2_history.png` |
| 3 | Session (means and charts) | `phone/3_session.png` | `tablet7/3_session.png` | `tablet10/3_session.png` |
| 4 | Settings | `phone/4_settings.png` | `tablet7/4_settings.png` | `tablet10/4_settings.png` |
| 5 | User guide | `phone/5_user_guide.png` | `tablet7/5_user_guide.png` | `tablet10/5_user_guide.png` |
| 6 | Privacy policy | `phone/6_privacy.png` | `tablet7/6_privacy.png` | `tablet10/6_privacy.png` |

Build the emulator APK, then capture. The script starts the matching AVD,
installs that APK, and drives the six screens. A 30-second chart window is
used so a short take is visible; the default in the app is 5 minutes.

```bash
./scripts/build_android.sh x86_64 debug
python3 packaging/google-play/capture_screenshots.py
python3 packaging/google-play/capture_screenshots.py phone
```

`phone` uses `Pixel7a_x86_64` (3-button navigation, `1080×2400`).
`tablet7` and `tablet10` use the Play tablet AVDs.

One screen can still be captured by hand after the emulator is showing it:

```bash
./scripts/run_emulator.sh Pixel7a_x86_64
./scripts/run_emulator.sh --screenshot home
```

Known `--screenshot` names are `home`, `history`, `session`, `settings`,
`guide`, and `privacy`. Phone files go to `phone/`. After `--tablet 7` or
`--tablet 10`, the same command writes `tablet7/` or `tablet10/`.
