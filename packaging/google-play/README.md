# Google Play listing assets

Files for the Play Console store listing of **Speech Rate Meter 2**
(`by.intoncore.SpeechRateMeter2`).

| File | Role |
|---|---|
| `store-listing.md` | All required listing **text** (EN + RU), contact fields, category, What’s new, data-safety answers, alt text |
| `icon-512.png` | Play icon (512×512 RGBA PNG), rendered from `res/icons/src/app-icon.svg` |
| `feature-graphic.png` | Upload-ready feature graphic (1024×500, 24-bit sRGB PNG, no alpha) |
| `feature-graphic.svg` | Editable vector source for the feature graphic; do not upload this file |
| `screenshots/phone/` | 6 portrait phone captures (`1080×2400`) |
| `screenshots/tablet7/` | 6 portrait 7-inch tablet captures (`1200×1920`) |
| `screenshots/tablet10/` | 6 portrait 10-inch tablet captures (`1600×2560`) |
| `screenshots/README.md` | Filename-to-screen index and recapture commands |

Launcher icons for the APK/AAB live in `android/res/` (legacy, round, and adaptive). The Play icon is the same artwork as `res/icons/src/app-icon.svg`.

## Paste order

1. Publish the privacy page, then confirm
   https://intontrainer.by/speechratemeter2policy.html
   matches `docs/privacy_policy_en.md`. Source file:
   `speechratemeter2policy.html` in the intontrainer.by site
   (`gsutil rsync` from that site’s `upload.sh`).
2. Open **Grow users → Store presence → Main store listing**.
3. Copy the English name, short description, and full description from
   `store-listing.md`.
4. Add a **Russian** translation and paste the RU fields.
5. Upload `icon-512.png`.
6. Upload `feature-graphic.png`.
7. Upload the phone, 7-inch, and 10-inch screenshots from `screenshots/`
   (index in `screenshots/README.md`).
8. Open **Store settings** and fill category, tags, email, and website.
9. Paste the privacy-policy URL and the data-safety answers from
   `store-listing.md` under **App content** / **Data safety**.

## Release tags

Every Play AAB is built from an annotated git tag (`MAJOR.MINOR.PATCH`, no
`v` prefix). The first upload is tag **`1.0.0`** (`versionCode` 1). Add a
**What’s new** block for that tag in `store-listing.md` before uploading.
Full procedure: `scripts/android_build_guide.md` (Signing for Google Play).

## Rebuild the graphics

From the project root, with Inkscape and ImageMagick:

```bash
inkscape res/icons/src/app-icon.svg \
  --export-type=png \
  --export-filename=packaging/google-play/icon-512.png \
  --export-width=512 --export-height=512

inkscape packaging/google-play/feature-graphic.svg \
  --export-type=png \
  --export-filename=/tmp/feature-graphic-raw.png \
  --export-width=1024 --export-height=500
convert /tmp/feature-graphic-raw.png -background white -alpha remove -alpha off \
  PNG24:packaging/google-play/feature-graphic.png
```

`feature-graphic.svg` links `icon-512.png` beside it. Re-export the icon
before the feature graphic.
