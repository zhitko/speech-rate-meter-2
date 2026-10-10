# Google Play store listing copy

Paste these fields into Play Console:
**Grow users → Store presence → Main store listing**, plus **Store settings**.

Limits are Play’s published maxima (name 30, short description 80, full
description 4,000, What’s new 500). Counts below include spaces.

Do not add rankings, prices, “free”/“discount”, “#1”/“best”, keyword stuffing,
emojis, or decorative symbols. Keep the default listing in English; add Russian
as a translation.

**What’s new** is written per Play upload / git tag (`MAJOR.MINOR.PATCH`).
Keep an EN+RU block for each tag. The first upload is **1.0.0**
(`versionCode` 1, from `CMakeLists.txt`). When a later tag is uploaded, add a
new What’s new section and paste it into the Console release notes for that AAB.

| Field | EN | RU |
|---|---:|---:|
| App name | 19 / 30 | 23 / 30 |
| Short description | 77 / 80 | 77 / 80 |
| Full description | 2,236 / 4,000 | 2,285 / 4,000 |
| What’s new (1.0.0) | 165 / 500 | 188 / 500 |

---

## Store settings (shared)

| Field | Value |
|---|---|
| Package name | `by.intoncore.SpeechRateMeter2` (set when creating the Play app; cannot change later) |
| Default language | English (United States) |
| Additional language | Russian |
| App type | App |
| Category | Education |
| Tags | Add one only if Play offers it and it fits speech practice |
| Email (required) | zhitko.vladimir@gmail.com |
| Phone | leave empty unless you want a public number |
| Website | https://intontrainer.by/ |
| Privacy policy URL | https://intontrainer.by/speechratemeter2policy.html |

The privacy-policy URL is also required under **App content** and **Data safety**.
It must stay identical to the in-app notice (`docs/privacy_policy_en.md`,
Russian in `docs/privacy_policy_ru.md`). The page to upload is
`speechratemeter2policy.html` in the intontrainer.by site. Do not reuse
https://intontrainer.by/speechratemetterpolicy.html — that is the 2020 notice
for the previous Speech Rate Meter.

This is one Play app. The launcher label and the English listing name are both
**Speech Rate Meter 2**.

---

## English (default listing)

### App name

```
Speech Rate Meter 2
```

19 / 30 characters. Matches the Android launcher label in
`android/AndroidManifest.xml`.

### Short description

```
Measure speaking pace on your device. Audio is deleted when the app closes
```

### Full description

```
Speech Rate Meter 2 measures how fast you are speaking. It does not recognize words. Pace, articulation, fillers, and pauses are estimated on the device from the intensity of your voice. After Stop, Listen plays the recording. The microphone audio is deleted when you leave the app.

How to measure
1. Open Home and press Start.
2. Speak naturally. The microphone stays open for the whole session.
3. The numbers follow your recent speech. A bar inside the gauge shows whether the microphone hears you.
4. Press Stop. Home then shows the averages for that session, labeled Mean values.

What the numbers mean
- Speech rate: overall pace, in words per minute, on a Slow / Average / Fast gauge
- Articulation: pace while you are speaking, with the gaps left out
- Fillers: how much some sounds are drawn out, such as a long "uh", as a percent
- Pauses: the length of the longer gaps, in seconds
- Speech: the total time that counted as speech

While a session is open, the first four numbers use the most recent kept speech (10 seconds by default). Speech keeps the total. After Stop, the first four become averages for the whole session.

History lists finished sessions. Open one to see the same averages and a chart of each measure. Each point is a moment the numbers on Home changed.

Detect speech automatically, in Settings, measures background noise after Start and ends a phrase when you pause. With it off, measuring starts when you press Start, a pause does not end the phrase, and the take is saved in 15-second parts.

The interface is available in English and Russian. Light and dark themes are supported.

Privacy and storage
The microphone is used only to estimate tempo on this device. A recording stays so you can listen after Stop, then it is deleted when you leave the app. Session files keep numbers only, in private app storage. The app does not require Internet permission and does not upload your voice, scores, or settings. Delete user data in Settings removes saved sessions.

For a steady reading, speak in a quiet room and toward the microphone.

Support
Website: https://intontrainer.by/
Email: zhitko.vladimir@gmail.com
Privacy policy: https://intontrainer.by/speechratemeter2policy.html

Open-source licences and source notices are available in the app.
```

### What’s new (version 1.0.0)

```
Initial Android release. Measure speaking pace on the device. Session numbers stay in private storage, and microphone audio is deleted after each phrase is measured.
```

### Graphic alt text (optional, ≤140 characters each)

Use these if Play Console asks for alt text on listing artwork.
Listing order is Home → History → session → Settings → User guide → Privacy.
Filenames: `screenshots/README.md`.

| Asset | Alt text |
|---|---|
| App icon | Speech Rate Meter 2 mark: a white microphone and sound bars on a blue rounded square |
| Feature graphic | Speech-rate gauge in the Average zone, with pace, fillers, and pauses measured on the device |
| Phone screenshot 1 | Home screen with the speech-rate gauge, metric tiles, and the Start button |
| Phone screenshot 2 | History list of finished sessions with date, pace, and phrase counts |
| Phone screenshot 3 | Session screen with mean values and charts of pace, articulation, fillers, and pauses |
| Phone screenshot 4 | Settings for language, theme, and how speech rate is measured |
| Phone screenshot 5 | User guide explaining the five measures and how to record |
| Phone screenshot 6 | Privacy policy: microphone audio stays on the device and is not uploaded |

---

## Russian (translation)

Add locale **Russian** under Main store listing translations. Do not replace the
default English listing.

### App name

```
Измеритель темпа речи 2
```

23 / 30 characters. Localized name for Russian search. The launcher on the
device remains **Speech Rate Meter 2**.

### Short description

```
Измеряйте темп речи на устройстве. Звук удаляется, как только числа сохранены
```

### Full description

```
«Измеритель темпа речи 2» измеряет, насколько быстро вы говорите. Слова он не распознаёт. Темп, артикуляция, заполнители и паузы оцениваются на устройстве по интенсивности голоса. Звук с микрофона удаляется, как только числа сохранены.

Как измерить
1. Откройте «Главная» и нажмите «Старт».
2. Говорите естественно. Микрофон остаётся включённым весь сеанс.
3. Числа следуют за недавней речью. Полоса внутри шкалы показывает, слышит ли микрофон звук.
4. Нажмите «Стоп». На главной появятся средние за этот сеанс с подписью «Средние значения».

Что означают числа
- Темп речи: общий темп в словах в минуту на шкале «Медленно / Средне / Быстро»
- Артикуляция: темп, пока вы говорите, без промежутков
- Заполнители: насколько некоторые звуки затянуты, например долгое «э-э», в процентах
- Паузы: длина более долгих промежутков, в секундах
- Речь: суммарное время, засчитанное как речь

Пока сеанс открыт, первые четыре числа считаются по последней сохранённой речи (по умолчанию 10 секунд). «Речь» копит всё время. После «Стоп» первые четыре становятся средними за весь сеанс.

«История» показывает законченные сеансы. Откройте строку, чтобы увидеть те же средние и график каждого показателя. Каждая точка — момент, когда числа на главной менялись.

«Автоопределение речи» в настройках измеряет фоновый шум после «Старт» и завершает фразу на паузе. Если переключатель выключен, измерение начинается в момент «Старт», пауза фразу не заканчивает, а запись сохраняется частями по 15 секунд.

Интерфейс доступен на английском и русском языках. Есть светлая и тёмная темы.

Конфиденциальность и хранение
Микрофон используется только чтобы оценить темп на этом устройстве. Пока фраза сохраняется, звук может ненадолго лежать во временном файле, затем он удаляется. Файлы сеансов хранят только числа, в закрытом хранилище приложения. Разрешение Интернет не требуется: голос, оценки и настройки никуда не отправляются. «Удалить данные пользователя» в настройках стирает сохранённые сеансы.

Для устойчивого результата говорите в тихом помещении и в сторону микрофона.

Поддержка
Сайт: https://intontrainer.by/
Почта: zhitko.vladimir@gmail.com
Политика конфиденциальности: https://intontrainer.by/speechratemeter2policy.html

Лицензии открытого ПО и сведения об исходном коде доступны в приложении.
```

### What’s new (version 1.0.0)

```
Первый выпуск для Android. Темп речи, артикуляция, заполнители и паузы считаются на устройстве. Числа сеанса остаются в закрытом хранилище, а звук с микрофона удаляется после каждой фразы.
```

### Graphic alt text (optional, ≤140 characters each)

| Asset | Alt text |
|---|---|
| Иконка | Знак «Измеритель темпа речи 2»: белый микрофон и полосы звука на синем скруглении |
| Рекламный графический файл | Шкала темпа речи в зоне «Средне»: темп, заполнители и паузы считаются на устройстве |
| Скриншот 1 | Главный экран со шкалой темпа, карточками показателей и кнопкой «Старт» |
| Скриншот 2 | История сеансов с датой, темпом и числом фраз |
| Скриншот 3 | Сеанс: средние значения и графики темпа, артикуляции, заполнителей и пауз |
| Скриншот 4 | Настройки языка, темы и измерения темпа речи |
| Скриншот 5 | Руководство: пять показателей и как записывать |
| Скриншот 6 | Политика конфиденциальности: звук остаётся на устройстве и не отправляется |

---

## Data safety and declarations

Play treats data as **collected** only when it leaves the device. This app
processes the microphone on the device and does not send voice, scores, or
settings to a server. It has no account, no ads, and no analytics SDK.

| Console question | Answer |
|---|---|
| Privacy policy URL | https://intontrainer.by/speechratemeter2policy.html |
| Does the app collect or share any of the required user data types? | No |
| Ads | No |
| In-app purchases | No |
| Designed for Families / children under 13 | No. Do not opt in |
| Microphone (`RECORD_AUDIO`) | App functionality, foreground session only. Audio is not kept after the phrase is measured and is not shared |
| Other sensitive permissions | None. Camera, location, contacts, storage, and Internet are not requested |
| Account deletion | No accounts. Saved sessions are removed with **Delete user data** in Settings, or by uninstalling |

Do not declare the app as a medical device. It estimates speaking tempo; it
does not diagnose.

---

## Listing graphics (not text)

These files live next to this document. Keep replacements in the same folders.
Index: `screenshots/README.md`.

| Asset | Spec |
|---|---|
| App icon | `icon-512.png` (512×512, 32-bit PNG with alpha). Rendered from `res/icons/src/app-icon.svg` |
| Feature graphic | `feature-graphic.png` (1024×500, 24-bit sRGB PNG, no alpha); editable source: `feature-graphic.svg` |
| Phone screenshots | 6 portrait captures in `screenshots/phone/` (`1080×2400`) |
| 7-inch tablet screenshots | 6 portrait captures in `screenshots/tablet7/` (`1200×1920`) |
| 10-inch tablet screenshots | 6 portrait captures in `screenshots/tablet10/` (`1600×2560`) |

The listing shots are Android emulator captures, in portrait, light theme,
English UI.

Official field limits:
[Create and set up your app](https://support.google.com/googleplay/android-developer/answer/9859152),
[Store listing assets](https://support.google.com/googleplay/android-developer/answer/9866151),
[Metadata policy](https://support.google.com/googleplay/android-developer/answer/9898842).
