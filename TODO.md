# TODO

Feedback from Boris Lobanov, Oct 10, 2026.

## UI
- [x] Make the gauge arcs about 1.5x larger (there is enough space).
- [x] Put the Articulation rate arc on top of the Speech rate arc (it is always longer).
- [x] Remove the Fillers metric (it consistently works poorly). Hidden unless Show Fillers tile is on.
- [x] Put Pauses in the Fillers tile's former place, and Speech rate in the Pauses tile's former place.
- [ ] Remove all long explanatory texts (the line under the gauge, the metric-tile hints, and the settings hints).
- [x] Add a "Listen to test" button next to the "Open File" button.

## Measurements
- [x] Improve measurement methods; current measurements are fairly approximate. Explore alternative approaches on top of the new UI.
  - [x] Research a programmatic approach that, for a long enough speech recording (WAV, ~30 s or more), determines:
    - [x] the total number of vowel sounds
    - [x] the total number of phrasal pauses (at least 150 ms long)
    - [x] statistics of vowel durations (nice to have)
      Whole-file summary after Stop and after Open File: vowel-nucleus count, phrasal-pause count (silent/unvoiced runs of at least `phrasalPauseMs`, default 150), and vowel-duration count, mean, median, min, max, and standard deviation. The live window is unchanged. The 150 ms cut and the voicing threshold still need tuning on the slow, medium, and fast samples.

## Waiting on Boris
- [ ] Speech samples with slow, medium, and fast tempo for testing "Open File".
- [ ] Recommended default settings (Boris is continuing to tune them).
