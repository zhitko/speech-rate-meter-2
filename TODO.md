# TODO

Feedback from Boris Lobanov, Oct 10, 2026.

## UI
- [x] Make the gauge arcs about 1.5x larger (there is enough space).
- [x] Put the Articulation rate arc on top of the Speech rate arc (it is always longer).
- [x] Remove the Fillers metric (it consistently works poorly). Hidden unless Show Fillers tile is on.
- [x] Put Pauses in the Fillers tile's former place, and Speech rate in the Pauses tile's former place.
- [x] Remove all long explanatory texts (the line under the gauge, the metric-tile hints, and the settings hints).
- [x] Add a "Listen to test" button next to the "Open File" button.

## Measurements
- [x] Improve measurement methods; current measurements are fairly approximate. Explore alternative approaches on top of the new UI.
  - [x] Research a programmatic approach that, for a long enough speech recording (WAV, ~30 s or more), determines:
    - [x] the total number of vowel sounds
    - [x] the total number of phrasal pauses (at least 150 ms long)
    - [x] statistics of vowel durations (nice to have)
      Whole-file summary after Stop and after Open File: vowel-nucleus count, phrasal-pause count (silent/unvoiced runs of at least `phrasalPauseMs`, default 150), and vowel-duration count, mean, median, min, max, and standard deviation. The live window is unchanged. On Boris's 23 WAVs the 150 ms cut still counts 150–180 ms gaps inside clips labeled as having no pauses; 200–250 ms fits those labels better (`docs/eval-tests.md`). Tightening the −25 dB level did not remove those gaps.
- [x] Measure pauses directly from the signal (Oct 11, 2026).
  - A frame is a pause frame only when it is below an adaptive silence level (about 25 dB under the 98th percentile of intensity, capped so it stays under ordinary speech) and it is not voiced. Stop closures are quiet and unvoiced; the existing minimum length (default 150 ms) is what keeps a typical closure out. Fricatives stay above the level. Quiet vowels stay because they are voiced.
  - The whole-recording card reports Np, ΣTp, mean, median, 25th and 75th percentiles, a 50 ms histogram, and the pause ratio ΣTp/Ts. The same detector feeds the tempo path.
  - Direct articulation, the default: Ra = k1 * 60 * Nv / (Ts − ΣTp), with the tempo vowel count Nv and the same k1 as speech rate. No pauses means Ra = Rs.
  - Settings → Measurement → Articulation method: Direct (default) or Legacy. Legacy keeps Ra = Rs * Ts / (Tv + k2 * Tc_med * Nc) and the pause score P = k3 * (Tc_mean − Tc_med) / Tc_med. The card's Compare block shows both rates and both pause values.
  - Live direct Tp and Ra use the current analysis window (the same audio as Rs), not a total since Start. After Stop the tiles are the duration-weighted mean of the kept phrases. The card is the full take, including silence between phrases.

## Waiting on Boris
- [ ] Speech samples with slow, medium, and fast tempo for testing "Open File". The 23 WAVs are on branch `cursor/fix-test-wav-playback-ba86` under `data/tests/`. Direct-pause numbers, filename ground truth, and calibration notes are in `docs/eval-tests.md`. Playing those files in the app still depends on that branch's loader, which this branch does not include.
- [ ] Recommended default settings (Boris is continuing to tune them). The test-set notes suggest trying a 200–250 ms pause minimum, and leaving k1 near 0.71 (the filename word counts imply about 0.73) and the −25 dB silence rule as they are.
