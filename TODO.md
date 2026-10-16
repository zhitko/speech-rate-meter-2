# TODO

Feedback from Boris Lobanov, Oct 10, 2026.

## UI
- [x] Make the gauge arcs about 1.5x larger (there is enough space).
- [x] Put the Articulation rate arc on top of the Speech rate arc (it is always longer).
- [x] Remove the Fillers metric (it consistently works poorly). Hidden unless Show Fillers tile is on.
- [x] Put Pauses in the Fillers tile's former place, and Speech rate in the Pauses tile's former place.
- [x] Remove all long explanatory texts (the line under the gauge, the metric-tile hints, and the settings hints).
- [x] Add a "Listen to test" button next to the "Open File" button.
- [x] Tempo dynamics chart (Boris, Oct 13, 2026): jittering arcs do not show tempo dynamics well. Make the arcs smaller and keep them for averaged values only. Add a time chart of tempo above the arcs, like the one in the SIA assistant, updated every 10 s and showing a sliding 300 s (5 min) window.
      Settings → Measurement → While recording chooses Tempo chart or Arcs for the live view. After Stop, and after Open File, Home can show the chart and the arcs together. Each live update adds a chart point at the same gauge median the arcs use; the tip rests at zero when the arcs would. Chart window is the sliding window (default 300 s). The arcs stay the averaged pace.
- [ ] Consistent tempo values (Boris, Oct 16, 2026): the tempo values shown under the arcs differ from those shown above the time chart. Find out why (different windows or averaging?) and make them consistent, or clearly label what each shows.

## Measurements
- [x] Improve measurement methods; current measurements are fairly approximate. Explore alternative approaches on top of the new UI.
  - [x] Research a programmatic approach that, for a long enough speech recording (WAV, ~30 s or more), determines:
    - [x] the total number of vowel sounds
    - [x] the total number of phrasal pauses (at least the phrasal-pause minimum, default 200 ms)
    - [x] statistics of vowel durations (nice to have)
      Whole-file summary after Stop and after Open File: vowel-nucleus count, phrasal-pause count (silent/unvoiced runs of at least `phrasalPauseMs`, default 200), and vowel-duration count, mean, median, min, max, and standard deviation. The live window is unchanged. The default moved from 150 ms to 200 ms after the 23-WAV comparison (`docs/eval-tests.md`): 150 ms was counting 150–180 ms gaps inside clips labeled as having no pauses. k1 stays 0.71 and the silence level stays about −25 dB.
- [x] Measure pauses directly from the signal (Oct 11, 2026).
  - A frame is a pause frame only when it is below an adaptive silence level (about 25 dB under the 98th percentile of intensity, capped so it stays under ordinary speech) and it is not voiced. Stop closures are quiet and unvoiced; the minimum length (default 200 ms) is what keeps a typical closure out. Fricatives stay above the level. Quiet vowels stay because they are voiced. A saved setting of exactly 150 ms, the previous default, loads as 200 ms. Any other saved value is kept.
  - The whole-recording card reports Np, ΣTp, mean, median, 25th and 75th percentiles, a 50 ms histogram, and the pause ratio ΣTp/Ts. The same detector feeds the tempo path.
  - Direct articulation, the default: Ra = k1 * 60 * Nv / (T_span − ΣTp), with the tempo vowel count Nv and the same k1 as speech rate. No pauses means direct Ra equals direct Rs.
  - Settings → Measurement → Articulation method: Direct (default) or Legacy. Legacy keeps Ra = Rs * Ts / (Tv + k2 * Tc_med * Nc) and the pause score P = k3 * (Tc_mean − Tc_med) / Tc_med. The card's Compare block shows both rates and both pause values.
  - Settings → Measurement → Speech rate method: Direct (default) or Legacy. Direct is Rs = k1 * 60 * Nv / T_span, where T_span runs from the first non-silent frame through the last (interior pauses stay in; leading and trailing silence stay out). Legacy keeps Rs = k1 * 60 * Nv / Ts. Direct articulation uses that same span minus ΣTp, so the two direct rates match when there is no pause. The card's Compare block shows both speech rates. A segment saved without a span is read as Ts.
  - Live direct Tp and Ra use the current analysis window (the same audio as Rs), not a total since Start. After Stop the tiles are the duration-weighted mean of the kept phrases. The card is the full take, including silence between phrases.
- [ ] Median option (Boris, Oct 16, 2026): tempo parameters are currently averaged per frame with the arithmetic mean. Add an optional setting to show median values instead.
  