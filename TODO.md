# TODO

Feedback from Boris Lobanov, Oct 10, 2026.

## UI
- [ ] Make the gauge arcs about 1.5x larger (there is enough space).
- [ ] Put the Articulation rate arc on top of the Speech rate arc (it is always longer).
- [ ] Remove the Fillers metric (it consistently works poorly).
- [ ] Put Pauses in the Fillers tile's former place, and Speech rate in the Pauses tile's former place.
- [ ] Remove all long explanatory texts (the line under the gauge, the metric-tile hints, and the settings hints).
- [ ] Add a "Listen to test" button next to the "Open File" button.

## Measurements
- [ ] Improve measurement methods; current measurements are fairly approximate. Explore alternative approaches on top of the new UI.
  - [ ] Research a programmatic approach that, for a long enough speech recording (WAV, ~30 s or more), determines:
    - [ ] the total number of vowel sounds
    - [ ] the total number of phrasal pauses (at least 150 ms long)
    - [ ] statistics of vowel durations (nice to have)

## Waiting on Boris
- [ ] Speech samples with slow, medium, and fast tempo for testing "Open File".
- [ ] Recommended default settings (Boris is continuing to tune them).
