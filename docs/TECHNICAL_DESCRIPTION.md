# Speech Rate Meter — technical specification

This document describes the behavior that a reimplementation must reproduce. It is derived from the Qt/QML application and the IntonCore library in this repository. Pitch extraction, manual phonetic labels, and waveform plotting exist in the library but are not part of the running application. They are listed at the end so they are not implemented by accident.

The application does not recognize words and does not use a speech-recognition model. It estimates speaking tempo from vowel-like intensity peaks.

Section 1.2 is the version 2 recorder. A session stays open in the background, metrics of the most recent speech update while speech is still going, and each pause-bounded segment becomes its own record. Sections 2–11 apply to each analyzed buffer. After Stop, Home and History show the duration-weighted mean of every kept phrase from Start to Stop. Speech time stays the sum. Section 1.3 stores the segments and shows that mean, plus per-metric charts.

## 1. What the user sees

The window opens on Home. Title: `Speech Rate Meter 2`. The layout is designed at 640×950 and scales down to fit a smaller desktop screen. Android uses the system display scale and does not apply a second scale factor.

One job is on screen at a time. A drawer (and the bottom bar, when that setting is on) holds:

| Item | Screen |
| --- | --- |
| Home | Start or stop a session, and read the session so far. |
| History | Past sessions. A row opens that session’s charts. |
| Settings | Phrase length and appearance. Calibration and coefficients stay behind Advanced. |
| User Guide | How to record. |
| Privacy Policy | What stays on the device. |
| Open-source licences | Bundled components. |

Home is the only screen with the record button. While a session is running, non-Settings screens show a `Recording` chip in the toolbar; the chip returns to Home. Entering Settings is different: it stops the active recording first, applying the normal min-length keep/drop rule to the open segment.

There is no waveform and no playback of recorded speech. The audio is deleted after the metrics are stored (section 1.3).

### 1.1 Headline results

While a session is on, speech rate, articulation, fillers, and pauses describe the analysis window: the most recent Analysis window seconds of kept speech (section 1.2, default 10 s), so a change of pace shows within a few seconds. Speech is the exception: it is the total analyzed speech since Start. After Stop, speech rate, articulation, fillers, and pauses switch to the duration-weighted mean of every phrase kept since Start. A longer phrase counts more than a shorter one. Speech stays the sum of those phrases. The gauge chip is labeled `Mean values`. That is the same result as the session's History row. A phrase dropped for being too short is left out. Open File is one file, is not averaged, and is not labeled Mean values.

Speech rate is the only gauge. Articulation, fillers, pauses, and speech time are four tiles next to it.

| On screen | Meaning | Unit | Display |
| --- | --- | --- | --- |
| Speech rate | Pace of the analysis window while recording; after Stop, the duration-weighted mean of kept phrases | words per minute | Large integer (`toFixed(0)`). While recording, the gauge shows the median of the last Gauge median readings. After Stop the gauge shows the session mean. The gauge marker clamps to `[Min RS, Max RS]`. The printed number does not. |
| Articulation | Pace while speech is actually going | words per minute | Integer. No gauge. |
| Fillers | How much of the speech is stretched sounds | percent | Integer and ` %`, from section 6.5, with a thin bar for the same percent. |
| Pauses | Extra gaps inside the kept phrases | seconds | Two decimals and ` sec`. No gauge. |
| Speech | Analyzed speech since Start | seconds | Integer and ` sec`. Silence that ends a phrase is not included. This is not the `mm:ss` timer. |

The gauge is a 240° arc open at the bottom, min at the lower left and max at the lower right. It is split into three equal zones, Slow / Average / Fast, drawn green → amber → red (light theme `#1E8E5A`, `#D99A00`, `#D2382F`; dark theme `#5DD39E`, `#F4C04E`, `#FF8A80`). Each zone is a faint track that fills in full color up to the value, and a round marker sits at the value. The number, `wpm`, and the name of the current zone are in the middle of the arc. The value animates to each new update. The numbers under the two ends are Min RS and Max RS. Defaults: 70 and 210. If the printed rate sits outside that range, the marker rests on the near end and the number still shows the real value. While a session is on, a microphone mark and a bar sit under the unit, in the theme primary color. The bar is the square root of the same microphone level that grows the Start button halo, so quiet speech still moves it. Silence leaves the bar empty and the mark muted. The meter is hidden when the session is off, and History does not show it.

Home, from top to bottom:

1. The gauge card. A chip in its corner names the state (Ready, Listening, Too short, Measuring, Not saved, Microphone blocked, and Mean values after Stop); while the session is on it has a blinking dot and the open phrase timer, as `mm:ss`, sits on the right. The gauge is always shown. Until something has been measured the arc is empty and the number reads `– –`. Under the gauge, one line explains the current state (section 1.2).
2. Four tiles: Articulation, Fillers, Pauses, Speech. They show `—` until something has been measured.
3. A round button: microphone and `Start` when idle, stop icon and `Stop` while the session is on. While the session is on, a halo around it grows with the microphone level, so silence and speech are obvious.

The layout follows the window. When the page is at least 720 px wide, or landscape and at least 560 px wide, the gauge card is on the left and the tiles and button are on the right. Otherwise everything is one column and the button is pinned to the bottom of the page, so it stays reachable on a phone while the rest scrolls. Content is capped at 1120 px wide and centered.

Details is inside Advanced. It opens the intermediate statistics in section 7. While recording those statistics are the analysis window. After Stop they are the joined vowel and gap collection, except Mean Filler Sounds, which is the same duration-weighted mean as the Fillers tile. Open File uses the opened file. Open File is an Advanced action on desktop only. There is no Save button.

### 1.2 Recording

A session stays open. The user does not start and stop each phrase. Capture, pause detection, cutting, and analysis run off the UI thread; the screen only receives finished metric snapshots. The same session keeps running if the user opens another non-Settings page. Entering Settings stops it. Segments that close while Home is hidden are still stored, and the speech-rate gauge catches up when Home is shown again.

**What Home says**

These lines are the only status copy on Home. Each replaces the previous one.

| State | Status line |
| --- | --- |
| Idle, nothing measured yet | `Press Start and speak naturally. The numbers follow your recent speech.` |
| Idle, after a session with kept phrases | `Speech is the total time. Press Start to measure again.` The gauge chip reads `Mean values`. |
| Session on, waiting for speech, or the latest window has no measurable speech (section 5.4) | `Listening…` The last numbers stay. |
| Less than 1.5 s of audio in the window | `Keep speaking. There is not enough speech to measure yet.` The last published numbers stay. |
| Window long enough | `The numbers follow your last N seconds of speech.` with N = Analysis window. |
| Phrase dropped at a pause | `That phrase was too short and was not saved.` It clears when the next speech starts. |
| Phrase stored | Back to `Listening…`. The stored numbers stay until the next phrase is long enough to replace them. |
| Microphone denied | `The microphone is blocked. Allow access in the system settings, then press Start again.` |

**Session**

1. Press Start. On Android, microphone permission is checked and, if necessary, requested before capture or calibration begins. A denial leaves the session stopped. When Use Speech Autodetection is on (`autoCalibrate`, default off), the user then stays quiet for the `vadCalibrationDurationMs` calibration (default 2000 ms); capture remains open afterward and speech and pauses are found by the voice-activity detector. When the setting is off, the whole take is the phrase: measurement starts at the first sample (including silence), a pause does not close it, and the numbers update once the analysis window holds 1.5 s. The choice is fixed for that session. The timer starts at `00:00` and shows the current phrase length (`mm:ss`), incrementing once per second. It resets when the next phrase starts.
2. Press Stop to end the session. Capture stops. An open phrase is kept or dropped by the min-length rule below.
3. `RECORD_AUDIO` is the only Android permission. It is requested on Start, not at application launch.

**Cutting at pauses**

When Use Speech Autodetection is on, speech and pause come from the recorder’s voice-activity detector and its thresholds. Method `0` is energy, `1` is autocorrelation, and `2` is the public Hybrid choice, requiring both detectors (logical AND). The worker also understands internal value `3` as logical OR for compatibility, but the Settings UI does not offer it. A pause is a run of non-speech that lasts at least Silence Duration (`autoStopSilenceDuration`, default 2000 ms). That pause is a record boundary. It is not the Phrase Pauses number in section 6.4, which is still computed inside one segment. When autodetection is off, Silence Duration does not close the take. The 15 s segment limit still splits it.

A segment closes in either of these cases:

- A pause reaches Silence Duration. The speech span is the first speech frame through the last speech frame before that pause.
- Speech has lasted 15 s with no such pause. The span is cut at that limit. The next sample starts the next segment. Samples are not copied into both records.

The compared length for the limits is this speech span, not the padded file and not the `mm:ss` label.

The analyzed buffer includes 300 ms before the first speech sample and 300 ms after the last, when those samples exist. A cut at the 15 s limit is in the middle of speech, so that boundary has no extra pad. The pause that closed a segment is not included past the trailing 300 ms.

**Segment limits**

The limits are fixed and are not settings.

| Limit | Value |
| --- | --- |
| Shortest kept segment | 1 s of speech span |
| Longest segment | 15 s of speech span |
| Audio needed before live numbers appear | 1.5 s in the analysis window |

- A pause closes a segment shorter than 1 s: drop it. Do not store metrics, do not add it to the analysis window, and do not replace the numbers already on screen. The session continues.
- A segment that reaches 15 s is kept.
- Manual stop keeps the open segment when its speech span is at least 1 s, and drops it otherwise.
- A kept segment whose analysis finds no measurable speech (section 5.4) is not stored.

**Live metrics**

The analysis window is the audio of the kept segments of this session, joined end to end, followed by the open segment (speech so far, plus the leading 300 ms when it exists), cut to its last Analysis window seconds (`General/analysisWindowSec`, default 10, range 3…30). Pauses that closed a segment are therefore not in the window; the 300 ms pads are. Section 6 runs once on that buffer.

- Nothing is published until the window holds 1.5 s of audio. Until then the last result stays on screen. If there is none yet, Home shows the gauge with an empty arc and `– –` instead of a speech-rate number.
- After that, a new snapshot is started at most every `60000 / Updates per minute` ms (`General/updatesPerMinute`, default 60, range 6…240). With autodetection on, snapshots start only on speech frames. If a newer buffer is ready before the previous analysis finishes, the older run is abandoned.
- Articulation, fillers, pauses, and speech are shown as computed. They are not joined with earlier phrases and not averaged with earlier snapshots. The speech-rate gauge shows the median of the last Gauge median live readings (`General/gaugeAverageCount`, default 3, range 1…30). Fewer readings than that use the ones already collected. An odd count takes the middle value after sorting. An even count takes the arithmetic mean of the two central values. Start clears those readings. After Stop the gauge and the tiles show the duration-weighted mean of the kept phrases, not that median. Speech stays the total. The gauge chip is labeled Mean values. Details keeps the joined vowel and gap statistics.
- A snapshot with no measurable speech (section 5.4) is not shown. The numbers stay and the state chip reads Listening until a window with speech arrives.
- When a segment closes, its final analysis is stored in the session file and added to the session collection. The screen keeps the window numbers; only Speech grows. The buffer is then released.
- After Stop, once the last segment is stored, Home shows that mean. That final switch is not recorded as a shown change.
- Speech is the sum of the stored segment lengths since Start plus the open segment. It can disagree with the timer by up to one second, by the 300 ms pads, and by the sample-rate rule in section 3.
- Open File is a single whole-file analysis and is not windowed or joined.

**Files**

Capture format requested from the device:

| Parameter | Value |
| --- | --- |
| Sample rate | 8000 Hz |
| Channels | 1 |
| Sample format | signed 16-bit little-endian PCM |
| Codec name | `audio/pcm` |

If the device rejects that format, capture uses the nearest supported format. Samples are resampled to 8000 Hz mono s16le before analysis, so the samples match the time base in section 3.

The resampled segment lives in memory. It is not kept as a recording the user can play later. A temporary WAV under `data/records/` (executable directory on desktop, application-local data on Android) is allowed only as a scratch file for that one segment. The file name is local time `dd.MM.yyyy.HH.mm.ss.zzz` plus `.wav`.

Delete that file as soon as the segment’s metrics are in the session file. Release the memory buffer in the same step. Do not open the next segment’s scratch file until the previous one is gone. After a segment is stored, and whenever the app is idle, `data/records/` is empty. If writing the session file fails, keep the scratch file and retry; do not delete the only copy of a result that was not stored. Open File is not a scratch file and is never deleted.

Open File (desktop): a WAV dialog starting at `<executable>/data/tests/`. The chosen file must be PCM WAV at exactly 8000 Hz, mono, signed 16-bit little-endian. Any other sample rate, channel count, encoding, or bit depth is rejected rather than converted. A valid file is analyzed once as a whole file; it is not cut at pauses or updated live. QML exposes availability through the `openFileAvailable` property/API; it is `false` on Android and `true` on desktop.

### 1.3 Saved history

History is a list of recording sessions. A session is the span from press-record to press-stop in section 1.2. It keeps every segment that was written, and every change of the five numbers Home actually showed from Start until Stop. A phrase dropped for being shorter than 1 s is not stored and does not change those numbers. An opened file is not a session.

The session file is created by whichever happens first: the first shown change or the first kept segment. Each later shown change is appended when the on-screen labels change. Each kept segment is appended as soon as its final analysis finishes, and only then is that segment’s audio deleted. Stopping the session sets the session end time. A session that ends with no shown changes and no kept segments is not listed. Delete user data removes the session files and any scratch WAV still in `data/records/`.

**File**

One JSON file per session, under `data/sessions/` next to `data/records/` (executable directory on desktop, application-local data on Android). The file name is the session id: local start time `yyyyMMdd-HHmmss-zzz.json`.

```json
{
  "id": "20261005-231001-042",
  "startedAt": "2026-10-05T23:10:01.042",
  "endedAt": "2026-10-05T23:12:18.110",
  "shown": [
    {
      "at": "2026-10-05T23:10:05.250",
      "speechRate": 126.8,
      "articulationRate": 149.6,
      "phrasePauses": 0.18,
      "speechDuration": 3.72,
      "fillerPercent": 12
    }
  ],
  "segments": [
    {
      "startedAt": "2026-10-05T23:10:04.100",
      "endedAt": "2026-10-05T23:10:09.400",
      "speechRate": 128.4,
      "articulationRate": 151.2,
      "phrasePauses": 0.18,
      "speechDuration": 4.86,
      "fillerPercent": 12,
      "vowelLengths": [4, 10],
      "gapLengths": [40],
      "vowelMaxFrames": 11,
      "gapMaxFrames": 40,
      "frame": 240,
      "shift": 120,
      "smooth": 120,
      "minLengthMs": 5,
      "degree": 3,
      "k1": 0.71,
      "k2": 1.2,
      "k3": 0.3,
      "k4": 100,
      "fillerMin": 120,
      "fillerMax": 240
    }
  ]
}
```

Times are local, zero-padded, use a 24-hour `HH` field, and include milliseconds. The session file has no audio path. `shown` contains the five values Home displayed; each entry is timestamped by `at`. While recording, its speech rate is the gauge median from section 1.2, rounded half away from zero. The other four values are that snapshot as computed. Every segment contains both its finalized headline result and the analysis inputs needed to recompute a combined session result. The five headline numbers are stored before display rounding except `fillerPercent`, which is already the integer label value:

| Field | Value |
| --- | --- |
| `speechRate` | `R_s`, not clamped |
| `articulationRate` | `R_a`, not clamped |
| `phrasePauses` | `P` |
| `speechDuration` | `T_s` |
| `fillerPercent` | the 0…100 label value from section 6.5, after clamping |

`startedAt` on a segment is the moment its speech began. `endedAt` is the moment that segment closed. `endedAt` on the session is the moment the user stopped. If the process stops before that, the session file already holds the segments written so far and `endedAt` stays the last segment’s end.

**Session list**

Newest session first. A row is scannable without opening it:

- Title: local date and start–end clock time, such as `5 Oct 2026, 23:10–23:12`.
- Subtitle: phrase count, how many times the on-screen numbers changed, and the session speech rate, such as `8 phrases · 46 updates · 128 wpm`.
- Third line: the other four session values, in the Home order, with the same rounding as the live labels.

Those five values are the duration-weighted mean of the kept phrases’ headline numbers, except speech, which is the sum of their speech durations. A longer phrase therefore counts more than a shorter one. Each segment also stores `vowelLengths`, `gapLengths`, `vowelMaxFrames`, `gapMaxFrames`, and the coefficients used (`frame`, `shift`, `smooth`, `minLengthMs`, `degree`, `k1`–`k4`, `fillerMin`, `fillerMax`). The filler percent is the duration-weighted mean of the phrases’ filler percents. An empty list says `No sessions yet. Start on Home and speak.`

**Session charts**

Choosing a row opens that session. The header repeats the list title and the five session values. Under it: `Each point is one change shown on Home.`

Five charts follow, speech rate first and taller than the others. Titles are the Home labels. Each point is one moment when a printed number changed: speech rate or articulation to the nearest word per minute, fillers to the nearest percent, pauses to two decimals, or speech to the nearest second. The horizontal axis is that moment as clock time, in order. The vertical axis is the value Home was showing (`wpm`, `%`, or `sec`) and is not clamped to the gauge. A session whose labels never changed is a single point. Older files that have no shown changes plot one point per phrase instead. These are metric charts, not a waveform. Back returns to the list.

## 2. Processing pipeline

```
PCM s16le mono
  → raw sample vector (int16 promoted to float, not scaled to [-1, 1])
  → short-time mean absolute amplitude (intensity)
  → min-max normalize intensity to [0, 1]
  → moving-average smooth
  → vowel nuclei = runs where raw normalized intensity exceeds smoothed intensity by 0.009
  → consonants/pauses = gaps between those nuclei
  → durations, counts, power mean, median
  → speech rate, articulation rate, phrase pauses, filler score
```

Every derived series is cached on a loaded file path. Changing settings does not invalidate that cache. A new segment, a live snapshot of the open segment, or a different file path creates a new analysis and therefore picks up the current settings. Re-opening the same saved path on the same backend object returns the old numbers.

The sample rate used to convert frame counts into seconds is the constant **8000**, matching recorder output and the strict Open File validation. Open File rejects a WAV whose header does not specify 8000 Hz mono signed 16-bit PCM.

Samples are decoded as signed 16-bit integers regardless of other bit depths: sample count is `data_bytes / (bitDepth/8)`, then each step reads an `int16`. For the files this app writes, bit depth is 16, so this is just “little-endian int16 to float”.

## 3. Time base

```
SAMPLE_RATE = 8000          # used everywhere seconds are computed
FRAME       = 240           # intensity window, in samples. Default.
SHIFT       = 120           # intensity hop, in samples. Default.
SMOOTH      = 120           # smoother length, in intensity samples. Default.
MIN_LENGTH_MS = 5           # minimum vowel length, milliseconds. Default.
PEAK_MARGIN = 0.009         # DATA_NORMALIZED_LIMIT
```

One intensity sample represents `SHIFT / SAMPLE_RATE` seconds of audio. At the defaults that is `120/8000 = 0.015` s = 15 ms.

Conversion used by every duration that comes from a segment statistic:

```
seconds(frame_count) = floor_toward_zero(frame_count) * SHIFT / SAMPLE_RATE
```

`frame_count` is passed through an integer parameter. Fractional means are truncated before the conversion. A mean of 10.9 intensity frames becomes 10 frames = 0.150 s, not 0.1635 s. Medians of integer lengths are already integers. Reproduce the truncation.

Speech duration does not use this conversion. It uses the PCM length:

```
T_s = sample_count / SAMPLE_RATE          # UI always analyzes 100% of the file
```

`sample_count` is the number of int16 samples in the data chunk.

## 4. Intensity

### 4.1 Short-time mean absolute amplitude

Let `x[n]` be the raw float samples (int16 values, typically about −32768…32767). With `half = round(FRAME / 2)` (120 when FRAME is 240):

```
start = -half
stop  = +half
```

While `stop < sample_count - half`:

```
acc = 0
for j in [start, stop):
    if 0 <= j < sample_count:
        acc += abs(x[j])
I.append(acc / FRAME)          # divisor is FRAME, not the number of in-range samples
start += SHIFT
stop  += SHIFT
```

Consequences that affect the contour:

- The first window only sees samples `[0, half)` but still divides by the full `FRAME`, so the first intensity value is about half of a centered window.
- Samples with negative indexes are skipped.
- The loop stops once `stop` reaches `sample_count - half`, so the tail of the waveform is not emitted.
- Output length is the number of hops that satisfy the loop, not `ceil(N / SHIFT)`.

### 4.2 Normalize

```
I_norm[i] = (I[i] - min(I)) / (max(I) - min(I))     # target range [0, 1]
```

If `max == min` the current code divides by zero. An empty recording produces an empty vector and the UI aborts the result (`waveLength` is 0).

### 4.3 Linear smooth

The smooth length is forced to an even count:

```
half_s = ceil(SMOOTH / 2)
W = half_s * 2
```

Default SMOOTH 120 → `half_s = 60`, `W = 120`.

For each index `i` in `0 .. len(I_norm)-1`:

```
acc = 0
for j in 0 .. W-1:
    k = j + i - half_s
    if k > 0 and k < len(I_norm):      # k == 0 is excluded; unsigned wraparound is excluded
        acc += I_norm[k]
S[i] = acc / W                         # divisor stays W even when edge samples were skipped
```

`k` is an unsigned index in the original code. When `i < half_s`, `k` underflows and the comparison fails, so those taps contribute 0. Index 0 is also never added, because the test is `k > 0` rather than `k >= 0`. Edges of `S` are therefore biased low. That bias matters: vowels are detected by `I_norm - S`.

There is a second smoothing pass in the library (`intensityDoubleSmoothFrame`, default 120, applied to `S`). The speech-rate metrics do not use it.

## 5. Vowel nuclei and gaps

### 5.1 Detector used by the metrics

Compare the normalized intensity with its smoothed version. A sample belongs to a vowel when it rises above the smooth contour by a fixed margin:

```
d[i] = I_norm[i] - S[i] - PEAK_MARGIN
```

State machine, `got = false`:

| Condition | Action |
| --- | --- |
| `d[i] > 0` and not `got` | Start a nucleus at `i`. Store `start = i`, `length = 0`, `got = true`. |
| `d[i] > 0` and `got` | `length += 1` |
| `d[i] < 0` and `got` | End the nucleus. Keep it only if `length > min_length_frames`. `got = false`. |
| `d[i] == 0` | Do nothing. An open nucleus is not extended and not closed. |
| Loop ends while `got` is still true | The trailing nucleus is dropped. There is no flush. |

`min_length_frames` is not `MIN_LENGTH_MS` directly:

```
min_length_frames = uint32( (SAMPLE_RATE / SHIFT) / 1000 * MIN_LENGTH_MS )
```

`SAMPLE_RATE` here is the constant 8000, and the division is the intensity rate, not a per-sample rate. With the defaults:

```
(8000 / 120) / 1000 * 5 = 0.333…  →  truncated to 0
```

So the default test is `length > 0`. A nucleus of one intensity sample has `length == 0` and is discarded. A nucleus of two or more samples is kept.

The stored record is the pair `(start, length)`:

- The run covers intensity indexes `start, start+1, …, start+length` (that is `length + 1` samples).
- Statistics in section 6 use the integer `length`, not `length + 1`. A kept two-sample nucleus contributes a duration of 1 frame = 15 ms.

### 5.2 Gaps (consonants and silence)

Gaps are only the interiors between nuclei. Silence before the first nucleus and after the last nucleus is ignored. If there are fewer than two nuclei, the gap list is empty.

Sort is not required; nuclei are already in time order. Let nucleus `a` be `(start, length)`.

```
cursor = nuclei[0].start + nuclei[0].length
for each later nucleus b:
    gaps.append( (cursor, b.start - cursor) )
    cursor = b.start + b.length
```

Gap `length` is `next.start - (prev.start + prev.length)`. It does not include a leading or trailing tail. These gap lengths are what the UI calls “consonants and silence”.

### 5.3 Mask (only for the “max” detail fields)

Build an array of size `len(I_norm)` filled with `2`.

For each nucleus `(start, length)`, write `1` at indexes `start .. start+length` inclusive.

Let `lo` be the smallest `start` and `hi` the largest `start+length`. For indexes from `lo` through `hi`, replace any remaining `2` with `0`. Indexes outside `[lo, hi]` stay `2`.

- `1` = vowel
- `0` = gap inside the outer vowel span
- `2` = audio outside that span (not counted as pause)

Longest vowel = longest consecutive run of `1`, in intensity frames, then converted with the truncation rule in section 3.

Longest consonant/silence = longest consecutive run of `0`, same conversion. The `2` region is not a pause.

### 5.4 Speech gate

Normalization (section 4.2) stretches any buffer to `[0, 1]`, including one that holds only background noise. Without a gate, noise wiggles cross the 0.009 margin dozens of times per second and silence reads as fast speech with 100 % fillers. So each nucleus from section 5.1 is checked against the raw intensity `I` before section 5.2 builds the gaps:

```
floor = I sorted ascending, element at index trunc(0.10 * (len(I) - 1))
gate  = max(80, floor * 3)
keep nucleus when max(I[start .. start+length]) >= gate
```

Gaps, the mask, and every statistic use only the kept nuclei. If fewer than 3 nuclei are kept, the buffer has no measurable speech and the result is invalid: no metrics are published or stored. The constants (`speechOverNoise` = 3, `minSpeechLevel` = 80, `noiseFloorPercentile` = 0.10, `minVowels` = 3) live in the analysis config and are not settings.

## 6. Statistics and the four formulas

Notation:

| Symbol | Meaning |
| --- | --- |
| `N_v` | number of vowel nuclei |
| `L_v[k]` | stored `length` of nucleus k, in intensity frames |
| `N_c` | number of gaps |
| `L_c[k]` | stored `length` of gap k, in intensity frames |
| `T_s` | speech duration, seconds, `sample_count / 8000` |
| `d` | power-mean degree, default `3` |

Empty lists yield 0 for counts, sums, means, and medians.

### 6.1 Power mean and median

Power mean of order `d` (this is what the UI labels “mean”; the arithmetic mean exists in the library and is not shown):

```
M_d(L) = ( mean( L[k] ** d ) ) ** (1 / d)
```

Then convert to seconds with truncation:

```
T_v_mean = seconds(M_d(L_v))
T_c_mean = seconds(M_d(L_c))
```

The degree is not part of the cache key. The first degree used for a loaded file sticks until the file is reloaded.

Median, after sorting ascending:

- Odd count: the middle element.
- Even count: the arithmetic mean of the two central elements, computed with **integer** division (the lengths are integers). For lengths 4 and 5 the median is `4`, not `4.5`.
- Empty: 0.

```
T_v_med = seconds(median(L_v))
T_c_med = seconds(median(L_c))
```

Sum of vowel lengths, also truncated as a whole after the sum (the sum is accumulated as a float of integers, then passed through the integer converter):

```
T_v = seconds( sum(L_v) )
```

The same applies to the gap sum `T_c`, which is shown on the details screen but is not used in the headline formulas.

### 6.2 Speech rate

```
K1 = 0.71          # default
R_s = K1 * N_v * 60 / T_s        # words per minute
```

`K1` folds “vowels per minute” into “words per minute”. It is not derived at runtime. If `T_s` is 0 the current code divides by zero; the UI returns before that when duration is 0.

Printed value: `round-half-away-from-zero` via `toFixed(0)` (JavaScript, not C++ truncation).

Gauge marker: clamp `R_s` into `[MinRS, MaxRS]`, default `[70, 210]`.

### 6.3 Articulation rate

```
K2 = 1.2           # default
R_a = R_s * T_s / (T_v + K2 * T_c_med * N_c)
```

If `R_s > R_a`, the function returns `R_s`. Articulation rate is never shown below speech rate. That happens when the denominator is larger than `T_s`, and also guards some short-file cases.

If `N_c` is 0 the denominator is just `T_v`. If that is also 0 the current code divides by zero.

The articulation value is printed as an integer only; there is no articulation gauge. Legacy articulation min/max settings are still synchronized with Min/Max RS but do not control a visible gauge.

### 6.4 Phrase pauses

```
K3 = 0.30          # default
if T_c_mean < T_c_med:
    P = 0
else:
    P = K3 * (T_c_mean - T_c_med) / T_c_med
```

`P` is in seconds. If `T_c_med` is 0 and `T_c_mean` is not smaller, the current code divides by zero (no nuclei pair, or a zero median). Display: two decimal places. No clamp.

The scientific reading: the power mean of inter-nucleus gaps sits above the median when a few gaps are much longer than a typical consonant. That excess, scaled by `K3`, is reported as the average inter-phrase pause.

### 6.5 Filler sounds

```
K4 = 100           # default
if T_v_mean < T_v_med:
    F = 0
else:
    F = K4 * (T_v_mean - T_v_med) / T_v_med
```

`F` is a dimensionless score. Long nuclei relative to the typical nucleus (filled pauses such as extended vowels) push the power mean above the median.

Display, with defaults `F_min = 120` and `F_max = 240`:

```
F_clamped = clamp(F, F_min, F_max)
percent   = (F_clamped - F_min) / (F_max - F_min) * 100
```

The on-screen label is `percent` as an integer followed by ` %`. There is no filler gauge. A raw score below 120 therefore prints `0 %`; a raw score above 240 prints `100 %`.

## 7. Details screen

Shown only from the recorder, and only while Advanced is checked. Same collection Home is showing: the session since Start, or the opened file.

| Label | Value | Format |
| --- | --- | --- |
| Record Length | `T_s` | 2 decimals |
| Consonants & Silence Length | `seconds(sum(L_c))` | 2 decimals |
| Consonants & Silence Count | `N_c` | integer |
| Consonants & Silence Max | longest run of mask value `0`, in seconds | 2 decimals |
| Consonants & Silence Mean Duration | `T_c_mean` (power mean, not arithmetic) | 2 decimals |
| Consonants & Silence Median Duration | `T_c_med` | default number-to-string, no fixed decimals |
| Vowels Length | `T_v` | 2 decimals |
| Vowels Count | `N_v` | integer |
| Vowels Max | longest run of mask value `1`, in seconds | 2 decimals |
| Vowels Mean Duration | `T_v_mean` | 2 decimals |
| Vowels Median Duration | `T_v_med` | default number-to-string |
| Vowels Speaking Rate | `N_v / T_s` nuclei per second | 2 decimals |
| Mean Filler Sounds | raw `F` from section 6.5, not the percent | 3 decimals |

Variance, skewness, and kurtosis are implemented (section 8) and are not shown.

## 8. Moments that the UI does not show

For completeness, `alglib::samplemoments` on the integer lengths:

```
mean = sum(L) / n
variance = sum((L - mean)^2) / (n - 1)     # with the usual two-pass correction; 0 when n < 2
skewness = mean(z^3)                        # z = (L - mean) / stddev ; 0 when stddev is 0
kurtosis = mean(z^4) - 3                    # excess kurtosis
```

`stddev` is `sqrt(variance)` with the `n - 1` variance. These stay in intensity-frame units. They are not passed through `seconds()`.

## 9. Settings

Stored in `settings.ini` beside the executable (desktop) or in application-local data (Android), INI format. The file is read only when the root key `date_v3` exists. Until the user changes something, every value below is the in-code default and the file may be absent. The first save writes `date_v3` as an empty `QDate`, which is enough to make later launches load the file. General application and recorder values are under the `[General]` group; analysis coefficients retain their named groups.

Advanced is a process-global boolean. It is not written to the INI and resets to off on restart. When it is off, Settings shows General, the phrase controls (including Use Speech Autodetection), and the speech-rate gauge range. Coefficients, filler calibration, signal-processing parameters, voice-activity calibration, and Open File are hidden, not reset.

Everyday labels use plain units. Silence Duration is edited in seconds and stored as milliseconds.

| On screen | Stored setting | Default | Hint |
| --- | --- | --- | --- |
| Analysis window (s) | `General/analysisWindowSec` | 10 s | While recording, Home shows the pace of this much recent speech. Range 3…30. |
| Updates per minute | `General/updatesPerMinute` | 60 | How often the numbers on Home are recalculated. Range 6…240. |
| Gauge median | `General/gaugeAverageCount` | 3 | The speech-rate gauge shows the median of this many latest readings. Range 1…30. 1 shows the current reading. |
| Pause | `General/autoStopSilenceDuration` | 2 s | Silence that ends a phrase. |
| Use Speech Autodetection | `General/autoCalibrate` | off | After Start, measure background noise, then listen for speech. Off measures the recording from the first sample. |
| Slow | Min RS | 70 wpm | Left end of the speech-rate gauge. Also copies to articulation min. |
| Fast | Max RS | 210 wpm | Right end of the speech-rate gauge. Also copies to articulation max. |

General (language, theme, color, font size, navigation bar) stays visible. Delete user data stays at the bottom of General, asks for confirmation, and says that it deletes saved sessions. Recorded audio is already gone.

Double-valued settings are edited as a spin box with 2 decimal places (internal integer = value × 100) and stored as the real coefficient.

With Advanced on, the extra settings are shown as numbered cards in the order a phrase is processed. Each card names its stage and says in one line what it does:

| Stage | Card | Settings |
| --- | --- | --- |
| 1 | Speech detection | VAD Method, Energy Threshold, Autocorr. Threshold, Autocorr. Threshold K, Autocorr Min/Max F0, Calibrate. Only used when Use Speech Autodetection is on. Energy shows only the energy threshold, Autocorrelation shows only the autocorrelation fields, Hybrid shows both. |
| 2 | Intensity | Frame, Shift, Smooth Frame (section 4) |
| 3 | Vowel detection | Segment length limit (section 5.1) |
| 4 | Statistics | Mean value degree (section 6.1) |
| 5 | Metrics | K1 under Speech rate, K2 under Articulation, K3 under Pauses, K4 / Min FS / Max FS under Fillers (sections 6.2–6.5) |

| UI label | Symbol | Default | INI key | Visible without Advanced |
| --- | --- | --- | --- | --- |
| Mean value degry | `d` | 3 | `speechRate/MeanValueDegry` | no |
| K1 | `K1` | 0.71 | `speechRate/K1` | no |
| Min RS | `MinRS` | 70 | `speechRate/Min` | yes, labeled Slow. Also copies to articulation min. |
| Max RS | `MaxRS` | 210 | `speechRate/Max` | yes, labeled Fast. Also copies to articulation max. |
| K2 | `K2` | 1.2 | `articulationRate/K2` | no |
| K3 | `K3` | 0.30 | `meanPauses/Max` | no |
| Frame | `FRAME` | 240 | `intensity/frame` | no. Spin range 0…1024. |
| Shift | `SHIFT` | 120 | `intensity/shift` | no. Spin range 0…512. |
| Smooth Frame | `SMOOTH` | 120 | `intensity/smoothFrame` | no. Spin range 0…1024. |
| Segment length limit (millisec) | `MIN_LENGTH_MS` | 5 | `segmentsByIntensity/minimumLength` | no. Spin range 0…2000. |
| K4 | `K4` | 100 | `fillerSounds/K4` | no |
| Min FS | `F_min` | 120 | `fillerSounds/Min` | no |
| Max FS | `F_max` | 240 | `fillerSounds/Max` | no |

The Measurement card (Analysis window, Updates per minute, Gauge median, Pause, Use Speech Autodetection) is always visible. It is not hidden with Advanced, and its values are not speech-rate coefficients. Silence Duration (`General/autoStopSilenceDuration` = 2000) is the pause that closes a phrase. The segment length limits are fixed (section 1.2). Older INI files may still hold `metricAverageCount`, `minRecordingTimeMs`, and `maxRecordingTimeMs`; they are ignored and removed on the next save.

Also present in the config object but not on this screen, and not used by the headline path:

| Parameter | Default | Role |
| --- | --- | --- |
| Absolute intensity threshold | 0.5 | Alternate segmenter, unused by the UI |
| Relative intensity threshold | 0.1 | Alternate segmenter, unused by the UI |
| Double-smooth frame | 120 | Second moving average, unused by the UI |
| Double-smooth minimum length | 15 ms | Unused by the UI |

Articulation min/max default to 70 and 210 and are persisted (`articulationRate/Min`, `articulationRate/Max`) but the settings UI overwrites them whenever Min RS or Max RS changes.

## 10. Reference implementation of the analysis

Inputs: `samples` as a float array of int16 values, and the settings structure. Output: the numbers in sections 6 and 7.

```
function analyze(samples, cfg):
    if samples.length == 0:
        return empty            # UI shows nothing

    I = intensity(samples, cfg.FRAME, cfg.SHIFT)
    if I.length == 0:
        return empty
    I_norm = minmax_normalize(I, 0, 1)
    S = moving_average(I_norm, cfg.SMOOTH)          # section 4.3, including the k > 0 rule

    min_frames = uint32( (8000 / cfg.SHIFT) / 1000 * cfg.MIN_LENGTH_MS )
    nuclei = peaks(I_norm, S, 0.009, min_frames)    # section 5.1, no trailing flush
    nuclei = gate(nuclei, I)                        # section 5.4
    if nuclei.length < 3:
        return empty
    gaps = interiors(nuclei)                        # section 5.2

    T_s = samples.length / 8000
    N_v = nuclei.length
    N_c = gaps.length

    T_v      = seconds(sum of lengths)
    T_v_mean = seconds(power_mean(lengths, cfg.d))
    T_v_med  = seconds(integer_median(lengths))
    T_c_mean = seconds(power_mean(gap lengths, cfg.d))
    T_c_med  = seconds(integer_median(gap lengths))

    R_s = cfg.K1 * N_v * 60 / T_s

    denom = T_v + cfg.K2 * T_c_med * N_c
    R_a = R_s * T_s / denom
    if R_s > R_a:
        R_a = R_s

    P = 0 if T_c_mean < T_c_med else cfg.K3 * (T_c_mean - T_c_med) / T_c_med
    F = 0 if T_v_mean < T_v_med else cfg.K4 * (T_v_mean - T_v_med) / T_v_med

    return { T_s, R_s, R_a, P, F, N_v, N_c, T_v, T_v_mean, T_v_med, T_c, T_c_mean, T_c_med, max runs }
```

`seconds(v)` is `trunc_toward_zero(v) * SHIFT / 8000`.

Display layer, separate from `analyze`:

```
speech_label        = format(R_s, 0) + " wpm"
speech_needle       = clamp(R_s, MinRS, MaxRS)
articulation_label  = format(R_a, 0) + " wpm"
pause_label         = format(P, 2) + " sec"
duration_label      = format(T_s, 0) + " sec"
F_c                 = clamp(F, F_min, F_max)
filler_label        = format((F_c - F_min) / (F_max - F_min) * 100, 0) + " %"
```

## 11. Worked numeric skeleton

Defaults, one nucleus of stored length 4 and a second of stored length 10, one gap of stored length 40, `sample_count = 80000` (10.0 s), `SHIFT = 120`.

```
T_s = 10
N_v = 2
N_c = 1
power mean vowels, d=3:
    ((4^3 + 10^3) / 2) ^ (1/3) = (532) ^ (1/3) ≈ 8.105
    seconds(8.105) = 8 * 120 / 8000 = 0.120
median vowels = integer mean of 4 and 10 = 7
    seconds = 7 * 120 / 8000 = 0.105
T_v = seconds(14) = 14 * 120 / 8000 = 0.210
T_c_mean = seconds(40) = 0.600
T_c_med  = seconds(40) = 0.600

R_s = 0.71 * 2 * 60 / 10 = 8.52     → label "9 wpm", needle clamped up to 70
R_a = 8.52 * 10 / (0.210 + 1.2 * 0.600 * 1) = 85.2 / 0.930 ≈ 91.61
    91.61 > 8.52, so articulation stays 91.61 → label "92 wpm"
P = 0, because 0.600 < 0.600 is false and the numerator is 0
F = 100 * (0.120 - 0.105) / 0.105 ≈ 14.286
    clamped to 120 → label "0 %"
```

This example is only a check of the formula wiring. Real nuclei are much more numerous; `K1 = 0.71` is what brings a normal vowel count into the 70–210 wpm band.

## 12. Platform behavior worth copying

- Capture, pause cutting, and analysis run off the UI thread. Headline metrics of the analysis window update during an open segment (section 1.2), at most Updates per minute times per minute. Open File is still a single whole-file analysis.
- One application-level `SessionApi` instance owns capture, analysis queues, the open-session accumulator, history access, and the result exposed to every page. Pages obtain that shared instance from the application window; they do not construct per-screen analysis backends. `SettingsApi` is likewise application-level. Details reads the metrics already published by `SessionApi` and does not re-read deleted scratch audio.
- Logging: one Qt message handler appends each Qt log line to `logs.txt` in the process working directory, prefixed with 24-hour local time `dd.MM.yyyy HH:mm:ss:zzz`. `main.cpp` does not initialize a second `FileLogger` sink.
- Android package id `by.intoncore.SpeechRateMeter2`, versionName `1.2`, versionCode `18`. The only declared permission is `RECORD_AUDIO`.
- WAV container is PCM with the standard header, format chunk, and data chunk. No cue points are written for a new recording. Manual segments marked `P` (pre-nucleus), `N` (nucleus), and `T` (post-nucleus) can be read from cue/label chunks by the library; the application never displays them.

## 13. Present in the repository and unused by this application

Do not implement these for behavioral parity:

- Pitch tracks (RAPT, SWIPE, REAPER, WORLD via SPTK) and octave normalization.
- The absolute/relative threshold segmenter (`intensityToSegments`). It scans with threshold 0.5, then re-scans each hit using `min + (max-min) * 0.1` inside that hit, and drops pieces shorter than the minimum. The live metrics use the smooth-difference detector in section 5.1 instead.
- Double-smoothed intensity and the segments derived from it.
- Waveform and intensity series downsampled to 1000 points for charts (`resizeVector`, `resizeVectorByMinMax`). No QML page calls them.
- Segment-variance helper in the backend (`mx2 - mx*mx` over mask indexes where the mask is non-zero). It is not shown.
- The legacy `QAudioRecorder` wrapper (`recorder.cpp`). Capture goes through `PcmRecorder`.

## 14. Parity checklist

1. Keep a background session. Cut a kept segment at a pause or at 15 s, drop a segment shorter than 1 s, resample to 8000 Hz mono s16le, and analyze that buffer with sections 2–11. Live snapshots analyze the last Analysis window seconds of kept plus open audio. Articulation, fillers, pauses, and speech are shown unjoined and unaveraged. The speech-rate gauge shows the median of the last Gauge median live readings (default 3). After Stop, the gauge and tiles show the duration-weighted mean of the kept phrases, speech stays the total, and the screen is labeled Mean values. Silence must not produce nuclei (section 5.4). Delete the segment audio once its metrics are in the session file.
2. Intensity uses mean absolute amplitude, full-window divisor, hop 120, window 240, and the exact loop bounds.
3. Normalize to [0, 1], then the moving average with even length, full-window divisor, and the `index > 0` edge rule.
4. Nuclei are `I_norm - S > 0.009`, stored length is `run_samples - 1`, runs of one sample are dropped at the default minimum, and a nucleus still open at the last sample is dropped.
5. Gaps are only interiors between nuclei.
6. Durations truncate fractional frame counts before multiplying by `SHIFT/8000`.
7. Means on the details screen and in the formulas are power means of degree 3, not arithmetic means.
8. Even-count medians use integer division.
9. `R_s`, `R_a` (with the `R_s` floor), `P`, and `F` match section 6, including the filler remap onto 120…240 → 0…100%.
10. The single speech-rate gauge clamps its marker; printed wpm and the pause do not. Filler is clamped before percent conversion.
11. Settings defaults and the Advanced visibility rules match section 9. Advanced itself is not persisted.
12. Each recording session is a JSON file of its kept segments, without audio, plus each change Home showed from Start to Stop. History lists sessions with the five metrics recomputed on all kept phrases together. Opening a session shows those values and one chart per metric against the times the numbers changed.
