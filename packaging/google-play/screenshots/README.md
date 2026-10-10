# Play Console screenshots

Android captures for the **Speech Rate Meter 2** listing
(`by.intoncore.SpeechRateMeter2`).

Light theme, English UI, portrait. Each folder has the same six screens.
Filenames are `N_screen_name.png` and match the phone listing order.

| # | Screen | Phone `1080×2400` | 7-inch `1200×1920` | 10-inch `1600×2560` |
|---|---|---|---|---|
| 1 | Home (gauge, Start) | `phone/1_home.png` | `tablet7/1_home.png` | `tablet10/1_home.png` |
| 2 | History | `phone/2_history.png` | `tablet7/2_history.png` | `tablet10/2_history.png` |
| 3 | Session (means and charts) | `phone/3_session.png` | `tablet7/3_session.png` | `tablet10/3_session.png` |
| 4 | Settings | `phone/4_settings.png` | `tablet7/4_settings.png` | `tablet10/4_settings.png` |
| 5 | User guide | `phone/5_user_guide.png` | `tablet7/5_user_guide.png` | `tablet10/5_user_guide.png` |
| 6 | Privacy policy | `phone/6_privacy.png` | `tablet7/6_privacy.png` | `tablet10/6_privacy.png` |

Build the emulator APK first:

```bash
./scripts/build_android.sh x86_64 debug
```

Recapture with the phone AVD (3-button navigation, `1080×2400`):

```bash
./scripts/run_emulator.sh Pixel7a_x86_64
./scripts/run_emulator.sh --screenshot home
./scripts/run_emulator.sh --screenshot history
./scripts/run_emulator.sh --screenshot session
./scripts/run_emulator.sh --screenshot settings
./scripts/run_emulator.sh --screenshot guide
./scripts/run_emulator.sh --screenshot privacy
```

Then the tablet slots:

```bash
./scripts/run_emulator.sh --tablet 7
./scripts/run_emulator.sh --tablet 10
```

Known `--screenshot` names write the numbered files above. Phone files go to
`phone/`. After `--tablet 7` or `--tablet 10`, the same commands write
`tablet7/` or `tablet10/`.
