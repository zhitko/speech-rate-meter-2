# Speech Rate Meter — technical specification

This document describes the behavior that a reimplementation must reproduce. It is derived from the Qt/QML application and the IntonCore library in this repository. Pitch extraction, manual phonetic labels, and waveform plotting exist in the library but are not part of the running application. They are listed at the end so they are not implemented by accident.

The application does not recognize words and does not use a speech-recognition model. It estimates speaking tempo from vowel-like intensity peaks.

Section 1.2 is the version 2 recorder. A session stays open in the background, metrics update while speech is still going, and each pause-bounded segment becomes its own record. Sections 2–11 apply to each of those segments. Home and History then join every kept segment from Start to Stop into one result. Section 1.3 stores the segments and shows that joined result, plus per-metric charts.

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

Home is the only screen with the record button. While a session is running, every screen shows a `Recording` chip in the toolbar, so leaving Home does not look like the microphone closed. The chip returns to Home.

There is no waveform and no playback of recorded speech. The audio is deleted after the metrics are stored (section 1.3).

### 1.1 Headline results

All five values cover the open session: every phrase kept since Start, plus the phrase still open once it is long enough. They are not the mean of those phrases. Vowel lengths and gap lengths are joined, speech time is added, the longest vowel and gap runs are kept, and section 6 runs once on that collection. A phrase dropped for being too short is left out. After Stop, the numbers stay. Open File is one file and is not joined with a session. Each phrase is still analyzed on its own buffer, range `[0, 1]`. The samples are not kept after that.

Speech rate is the only gauge. Articulation, fillers, pauses, and speech time are a text list under that number.

| On screen | Meaning | Unit | Display |
| --- | --- | --- | --- |
| Speech rate | Pace since Start, pauses inside phrases included | words per minute | Large integer (`toFixed(0)`). The needle clamps to `[Min RS, Max RS]`. The printed number does not. |
| Articulation | Pace while speech is actually going | words per minute | Integer. No needle. |
| Fillers | How much of the speech is stretched sounds | percent | Integer and ` %`, from section 6.5. No needle. |
| Pauses | Extra gaps inside the kept phrases | seconds | Two decimals and ` sec`. No needle. |
| Speech | Analyzed speech since Start | seconds | Integer and ` sec`. Silence that ends a phrase is not included. This is not the `mm:ss` timer. |

The gauge is one semicircle, start angle 90°, span 180°, min to max. A legend behind it runs green (`#066832`) → yellow (`#c8b219`) → red (`#b8181f`), labeled Slow / Average / Fast. The numbers under the arc are Min RS and Max RS in `wpm`. Defaults: 70 and 210. If the printed rate sits outside that range, the needle rests on the near end and the number still shows the real value.

Home, from top to bottom:

1. One status line for the current state (section 1.2).
2. The speech-rate gauge. Before a phrase has been long enough to publish, and only while the session is on, the needle rests at the middle. The speech-rate number and the four values stay hidden. While idle and nothing has been measured, that space holds the idle explanation and no gauge.
3. The speech-rate number, then the other four values in a two-column list.
4. A level meter, visible only while the session is on, so silence and speech are obvious.
5. A round button: microphone and `Start` when idle, stop icon and `Stop` while the session is on. The timer for the open phrase sits with the status line, as `mm:ss`.

Details is inside Advanced. It opens the intermediate statistics in section 7 for the segment still held in memory, using the technical names from that section. Open File is an Advanced action on desktop only. There is no Save button.

### 1.2 Recording

A session stays open. The user does not start and stop each phrase. Capture, pause detection, cutting, and analysis run off the UI thread; the screen only receives finished metric snapshots. The same session keeps running if the user opens another page. Segments that close while Home is hidden are still stored, and the speech-rate gauge catches up when Home is shown again.

**What Home says**

These lines are the only status copy on Home. Each replaces the previous one.

| State | Status line |
| --- | --- |
| Idle, nothing measured yet | `Press Start and speak naturally. A phrase is measured when you pause.` |
| Idle, a previous phrase is still on screen | `Press Start to measure again.` The last numbers stay. |
| Session on, waiting for speech | `Listening…` |
| Speech shorter than the minimum | `Keep speaking. This phrase is still too short to count.` The last published numbers stay. |
| Speech long enough | The timer and the live numbers. No extra sentence. |
| Phrase dropped at a pause | `That phrase was too short and was not saved.` It clears when the next speech starts. |
| Phrase stored | Back to `Listening…`. The stored numbers stay until the next phrase is long enough to replace them. |
| Microphone denied | `The microphone is blocked. Allow access in the system settings, then press Start again.` |

**Session**

1. Press Start. When Use Speech Autodetection is on (`autoCalibrate`, default off), the VAD calibration dialog runs first. The user stays quiet for `vadCalibrationDurationMs` (default 2000 ms), the measured threshold is stored, and then the microphone opens and stays open. Speech and pauses are found by the voice-activity detector. When the setting is off, Start opens the microphone immediately and the whole take is the phrase: measurement starts at the first sample, a pause does not close it, and the numbers update once the speech span reaches Min recording time. The choice is fixed for that session. If microphone permission is denied during calibration, the session does not start. The timer starts at `00:00` and shows the speech length of the current phrase (`mm:ss`), incrementing once per second. It resets when the next phrase starts.
2. Press Stop to end the session. Capture stops. An open phrase is kept or dropped by the min-length rule below.
3. `RECORD_AUDIO` is requested when the session starts, if it is not already granted. If it is denied, the session does not start.

**Cutting at pauses**

When Use Speech Autodetection is on, speech and pause come from the recorder’s voice-activity detector (energy, autocorrelation, or hybrid) and its thresholds. A pause is a run of non-speech that lasts at least Silence Duration (`autoStopSilenceDuration`, default 2000 ms). That pause is a record boundary. It is not the Phrase Pauses number in section 6.4, which is still computed inside one segment. When the setting is off, Silence Duration does not close the take. Max recording time still splits it.

A segment closes in either of these cases:

- A pause reaches Silence Duration. The speech span is the first speech frame through the last speech frame before that pause.
- Speech has lasted Max recording time with no such pause. The span is cut at that limit. The next sample starts the next segment. Samples are not copied into both records.

The compared length for min and max is this speech span, not the padded file and not the `mm:ss` label.

The analyzed buffer includes 300 ms before the first speech sample and 300 ms after the last, when those samples exist. A cut at Max recording time is in the middle of speech, so that boundary has no extra pad. The pause that closed a segment is not included past the trailing 300 ms.

**Min and max recording time**

Both sit in the Recording settings group. The UI edits them in seconds. The file stores milliseconds. Max must stay greater than min: if a save would make max ≤ min, max is stored as min + 1 s.

| Setting | Default | INI key |
| --- | --- | --- |
| Min recording time | 1 s | `minRecordingTimeMs` = 1000 |
| Max recording time | 15 s | `maxRecordingTimeMs` = 15000 |

- A pause closes a segment shorter than Min recording time: drop it. Do not store metrics and do not replace the numbers already on screen. The session continues.
- A segment that reaches Max recording time is kept. Because max is longer than min, it always qualifies.
- Manual stop keeps the open segment when its speech span is at least min, and drops it otherwise.

**Live metrics**

Section 6 runs on the samples that would be written if the segment closed now: speech so far, plus the leading 300 ms when it exists.

- Nothing is published until the speech span reaches Min recording time. Until then the last finalized result stays on screen. If there is none yet, Home shows the gauge with the needle at the middle and does not show a speech-rate number.
- After that, a new snapshot is started at most every 250 ms. If a newer buffer is ready before the previous analysis finishes, the older run is abandoned.
- Each published snapshot is joined with the phrases already kept in this session, then shown. While the phrase is still open, Home shows the arithmetic mean of the last N of those joined snapshots. N is Display average (section 9), default 4. N = 1 shows each snapshot unchanged. Until N snapshots exist, the mean uses the ones received so far. A new phrase starts a new window.
- When a segment closes, the final analysis of that buffer replaces its live snapshots inside the collection. The screen shows the collection, not the phrase alone and not the display average. The session file stores the phrase. The buffer is then released. Open File is a single analysis and is not averaged or joined.
- Speech is the sum of analyzed phrase lengths since Start. It can disagree with the timer by up to one second, by the 300 ms pads, and by the sample-rate rule in section 3.

**Files**

Capture format requested from the device:

| Parameter | Value |
| --- | --- |
| Sample rate | 8000 Hz |
| Channels | 1 |
| Sample format | signed 16-bit little-endian PCM |
| Codec name | `audio/pcm` |

If the device rejects that format, capture uses the nearest supported format. Samples are resampled to 8000 Hz mono s16le before analysis, so the samples match the time base in section 3.

The resampled segment lives in memory. It is not kept as a recording the user can play later. A temporary WAV under `data/records/` (executable directory on desktop, application-local data on Android) is allowed only as a scratch file for that one segment. The file name is local time `dd.MM.yyyy.hh.mm.ss.zzz` plus `.wav`.

Delete that file as soon as the segment’s metrics are in the session file. Release the memory buffer in the same step. Do not open the next segment’s scratch file until the previous one is gone. After a segment is stored, and whenever the app is idle, `data/records/` is empty. If writing the session file fails, keep the scratch file and retry; do not delete the only copy of a result that was not stored. Open File is not a scratch file and is never deleted.

Open File (desktop): a WAV dialog starting at `<executable>/data/tests/`. The chosen file is analyzed once, as a whole file. It is not cut at pauses and it is not updated live. The button is hidden on Android (`isOpenAvailable() == false`).

### 1.3 Saved history

History is a list of recording sessions. A session is the span from press-record to press-stop in section 1.2. It keeps every segment that was written, and every change of the five numbers Home actually showed from Start until Stop. A phrase dropped for being shorter than Min recording time is not stored and does not change those numbers. An opened file is not a session.

The session file is created on the first shown change. Each later change is appended when the on-screen labels change. Each kept segment is appended as soon as its final analysis finishes, and only then is that segment’s audio deleted. Stopping the session sets the session end time. A session that ends with no shown changes and no kept segments is not listed. Delete user data removes the session files and any scratch WAV still in `data/records/`.

**File**

One JSON file per session, under `data/sessions/` next to `data/records/` (executable directory on desktop, application-local data on Android). The file name is the session id: local start time `yyyyMMdd-HHmmss-zzz.json`.

```json
{
  "id": "20261005-231001-042",
  "startedAt": "2026-10-05T23:10:01.042",
  "endedAt": "2026-10-05T23:12:18.110",
  "segments": [
    {
      "startedAt": "2026-10-05T23:10:04.100",
      "endedAt": "2026-10-05T23:10:09.400",
      "speechRate": 128.4,
      "articulationRate": 151.2,
      "phrasePauses": 0.18,
      "speechDuration": 4.86,
      "fillerPercent": 12
    }
  ]
}
```

Times are local, zero-padded, with milliseconds. The session file has no audio path. The five numbers are the finalized analysis of the memory buffer, before display rounding:

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

Those five values are section 6 on the joined vowel lengths, gap lengths, and speech durations of the kept phrases. Speech rate therefore weights a longer phrase more than a shorter one. Pauses and fillers use the joined runs, so they are not the mean of the per-phrase numbers. Speech is the sum. Each segment also stores `vowelLengths`, `gapLengths`, `vowelMaxFrames`, `gapMaxFrames`, and the coefficients used (`frame`, `shift`, `smooth`, `minLengthMs`, `degree`, `k1`–`k4`, `fillerMin`, `fillerMax`). The filler percent uses the last phrase’s filler range. If those lists are missing, or the coefficients differ inside one session, the row falls back to a duration-weighted mean of the stored headline values and the sum of the speech durations. An empty list says `No sessions yet. Start on Home and speak.`

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

The sample rate used to convert frame counts into seconds is the constant **8000**, not the rate stored in the WAV header (`getWaveFrameRate()` is hard-coded). Duration of an opened file that is not 8000 Hz will be wrong in the current program. Match the constant if the goal is numerical compatibility. Use the real header rate only if you intentionally fix that.

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

Gauge needle: clamp `R_s` into `[MinRS, MaxRS]`, default `[70, 210]`.

### 6.3 Articulation rate

```
K2 = 1.2           # default
R_a = R_s * T_s / (T_v + K2 * T_c_med * N_c)
```

If `R_s > R_a`, the function returns `R_s`. Articulation rate is never shown below speech rate. That happens when the denominator is larger than `T_s`, and also guards some short-file cases.

If `N_c` is 0 the denominator is just `T_v`. If that is also 0 the current code divides by zero.

Printed and gauge behavior match speech rate. Min/max articulation limits are not edited separately: the settings screen writes Min RS into both minima and Max RS into both maxima.

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

The on-screen label is `percent` as an integer followed by ` %`. The gauge value is `F_clamped`, and the gauge’s own min/max are `F_min` and `F_max`. A raw score below 120 therefore prints `0 %`. A raw score above 240 prints `100 %`.

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

Stored in `settings.ini` beside the executable (desktop) or in application-local data (Android), INI format. The file is read only when the key `date_v3` exists. Until the user changes something, every value below is the in-code default and the file may be absent. The first save writes `date_v3` as an empty `QDate`, which is enough to make later launches load the file.

Advanced is a process-global boolean. It is not written to the INI and resets to off on restart. When it is off, Settings shows General, the phrase controls (including Use Speech Autodetection), and the speech-rate gauge range. Display average, coefficients, filler calibration, signal-processing parameters, voice-activity calibration, and Open File are hidden, not reset.

Everyday labels use plain units. Silence Duration is edited in seconds and stored as milliseconds.

| On screen | Stored setting | Default | Hint |
| --- | --- | --- | --- |
| Shortest phrase | Min recording time | 1 s | Shorter speech is ignored. |
| Longest phrase | Max recording time | 15 s | A longer stretch is split even without a pause. |
| Pause | Silence Duration | 2 s | Silence that ends a phrase. |
| Use Speech Autodetection | `autoCalibrate` | off | After Start, measure background noise, then listen for speech. Off measures the recording from the first sample. |
| Slow | Min RS | 70 wpm | Left end of the speech-rate gauge. Also copies to articulation min. |
| Fast | Max RS | 210 wpm | Right end of the speech-rate gauge. Also copies to articulation max. |

General (language, theme, color, font size, navigation bar) stays visible. Delete user data stays at the bottom of General, asks for confirmation, and says that it deletes saved sessions. Recorded audio is already gone.

Double-valued settings are edited as a spin box with 2 decimal places (internal integer = value × 100) and stored as the real coefficient.

| UI label | Symbol | Default | INI key | Visible without Advanced |
| --- | --- | --- | --- | --- |
| Display average | N | 4 | `General/metricAverageCount` | no. Spin range 1…30. While a phrase is open, Home averages the last N joined snapshots. The closing collection is shown as computed. |
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

The three phrase controls are always visible. They are not hidden with Advanced, and they are not speech-rate coefficients. On screen they are Shortest phrase, Longest phrase, and Pause. The stored keys stay:

| Stored setting | Default | INI key |
| --- | --- | --- |
| Min recording time | 1 s | `minRecordingTimeMs` |
| Max recording time | 15 s | `maxRecordingTimeMs` |
| Silence Duration | 2 s | `autoStopSilenceDuration` = 2000 |

Silence Duration is the pause that closes a phrase. Min and Max recording time bound the speech span that is kept.

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
articulation_needle = clamp(R_a, MinRS, MaxRS)
pause_label         = format(P, 2) + " sec"
duration_label      = format(T_s, 0) + " sec"
F_c                 = clamp(F, F_min, F_max)
filler_label        = format((F_c - F_min) / (F_max - F_min) * 100, 0) + " %"
filler_needle       = F_c
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

- Capture, pause cutting, and analysis run off the UI thread. Headline metrics update during an open segment (section 1.2), at most every 250 ms. Open File is still a single whole-file analysis.
- Each screen constructs its own analysis backend. Settings and the audio recorder are process-wide singletons. Details uses the analysis already computed for the current segment. It does not re-read a WAV; the segment audio has been deleted.
- Logging: every Qt debug line is appended to `logs.txt` in the process working directory, prefixed with `dd.MM.yyyy hh:mm:ss:zzz`.
- Android package id `by.intoncore.SpeechRateMeter`, versionName `1.2`, versionCode `18`. Permissions include `RECORD_AUDIO`. Camera, network, and storage permissions are declared and unused by the speech-rate flow.
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

1. Keep a background session. Cut a kept segment at a pause or at Max recording time, drop a segment shorter than Min recording time, resample to 8000 Hz mono s16le, and analyze that buffer with sections 2–11, including live snapshots. Delete the segment audio once its metrics are in the session file.
2. Intensity uses mean absolute amplitude, full-window divisor, hop 120, window 240, and the exact loop bounds.
3. Normalize to [0, 1], then the moving average with even length, full-window divisor, and the `index > 0` edge rule.
4. Nuclei are `I_norm - S > 0.009`, stored length is `run_samples - 1`, runs of one sample are dropped at the default minimum, and a nucleus still open at the last sample is dropped.
5. Gaps are only interiors between nuclei.
6. Durations truncate fractional frame counts before multiplying by `SHIFT/8000`.
7. Means on the details screen and in the formulas are power means of degree 3, not arithmetic means.
8. Even-count medians use integer division.
9. `R_s`, `R_a` (with the `R_s` floor), `P`, and `F` match section 6, including the filler remap onto 120…240 → 0…100%.
10. Gauges clamp; printed wpm and the pause do not, except filler, which is clamped before the percent conversion.
11. Settings defaults and the Advanced visibility rules match section 9. Advanced itself is not persisted.
12. Each recording session is a JSON file of its kept segments, without audio, plus each change Home showed from Start to Stop. History lists sessions with the five metrics recomputed on all kept phrases together. Opening a session shows those values and one chart per metric against the times the numbers changed.
