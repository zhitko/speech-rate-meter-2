# Evaluation on Boris's test WAVs

Direct pause time and both articulation rates, on every file in `data/tests/` from branch `cursor/fix-test-wav-playback-ba86` (PR #5). The same numbers are in [eval-tests.csv](eval-tests.csv).

This branch's application code is unchanged. PR #5's WAV loader was compiled into a throwaway program and was not merged here. The loader skips `LIST` and `cue` chunks, mixes to mono, and resamples to 8 kHz on the signed 16-bit scale the analyzer expects. Every file in this set is already 8 kHz, mono, 16-bit PCM, so the loaded sample count equals the data chunk. Eleven of the twenty-three files also carry `LIST` and/or `cue` chunks.

Defaults, the same ones the app ships: k1 = 0.71, k2 = 1.2, k3 = 0.30, phrasal pause 150 ms, silence about 25 dB under the 98th percentile of intensity (raised to the noise-floor gate when that is higher). `Nv` is the tempo vowel count that `Rs` and `Ra` use. The stricter whole-recording vowel count (voiced peak, tail flushed) is `summary_vowels` in the CSV. Rates are words per minute.

`Tp legacy` is k3 × (Tc − Tm), with Tc the power-mean inter-nucleus gap and Tm the median gap, both in seconds. That product can be negative. The pause score the app still publishes in Legacy mode is the normalized one, P = k3 × (Tc − Tm) / Tm, and it is 0 when Tc < Tm. On this set Tc ≥ Tm for every file, so P = Tp legacy / Tm. `legacy_P` in the CSV is that score. Direct `Ra` is k1 × 60 × Nv / (Ts − ΣTp). Leading and trailing silence stay inside Ts and outside ΣTp.

Four pairs are byte-identical, so the table has 23 rows and 19 distinct recordings:

- `8 Poeme_fast.wav` and `9 Poeme_fast.wav`
- `9 Poeme_slow.wav` and `10 Poeme_slow.wav`
- `3 Belarus_Wether (Speaker).wav` and `3 Belarussian_Wether Info.wav`
- `5 Russian_Story (Speaker).wav` and `922сл Dama s sobachkoi _01.wav`

## Results

| File | Ts (s) | Nv | Rs | Ra legacy | Ra direct | Tp legacy (s) | Np | ΣTp (s) | mean / median (ms) | pause ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 240 сл (-) пау (110с) Дама.wav | 110.228 | 349 | 134.9 | 143.5 | 144.3 | 0.050 | 41 | 7.23 | 176 / 165 | 6.6% |
| 63 сл (-) пау 22с лукомо.wav | 21.504 | 83 | 164.4 | 164.4 | 166.8 | 0.009 | 2 | 0.30 | 150 / 150 | 1.4% |
| 36 сл (-8 пау) 17,4с  лукомо.wav | 17.376 | 52 | 127.5 | 127.5 | 134.7 | 0.013 | 5 | 0.93 | 186 / 195 | 5.3% |
| 1612сл Dama s sobachkoi _02.wav | 835.500 | 2241 | 114.3 | 154.1 | 131.1 | 0.180 | 338 | 107.30 | 317 / 210 | 12.8% |
| 1786сл Dama s sobachkoi _03.wav | 912.327 | 2471 | 115.4 | 144.2 | 129.3 | 0.144 | 305 | 98.39 | 323 / 240 | 10.8% |
| 913сл Dama s sobachkoi _04.wav | 479.896 | 1226 | 108.8 | 139.6 | 119.6 | 0.162 | 144 | 43.24 | 300 / 232 | 9.0% |
| 922сл Dama s sobachkoi _01.wav | 467.486 | 1198 | 109.2 | 142.0 | 126.4 | 0.167 | 201 | 63.67 | 317 / 225 | 13.6% |
| 1 English_Fast.wav | 31.712 | 125 | 167.9 | 190.4 | 183.7 | 0.045 | 9 | 2.73 | 303 / 270 | 8.6% |
| 2 English_Slow.wav | 48.304 | 113 | 99.7 | 139.3 | 117.9 | 0.171 | 33 | 7.47 | 226 / 210 | 15.5% |
| 8 Poeme_fast.wav | 12.512 | 47 | 160.0 | 160.0 | 162.0 | 0.004 | 1 | 0.15 | 150 / 150 | 1.2% |
| 9 Poeme_fast.wav | 12.512 | 47 | 160.0 | 160.0 | 162.0 | 0.004 | 1 | 0.15 | 150 / 150 | 1.2% |
| 9 Poeme_slow.wav | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 0.072 | 9 | 3.69 | 410 / 345 | 17.0% |
| 10 Poeme_slow.wav | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 0.072 | 9 | 3.69 | 410 / 345 | 17.0% |
| 3 Belarus_Wether (Speaker).wav | 47.216 | 216 | 194.9 | 194.9 | 194.9 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 3 Belarussian_Wether Info.wav | 47.216 | 216 | 194.9 | 194.9 | 194.9 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 4 Russian_Story (Helena).wav | 527.360 | 1357 | 109.6 | 126.1 | 140.0 | 0.090 | 290 | 114.54 | 395 / 255 | 21.7% |
| 5 Russian_Story (Speaker).wav | 467.486 | 1198 | 109.2 | 142.0 | 126.4 | 0.167 | 201 | 63.67 | 317 / 225 | 13.6% |
| 6 Russian _Advatasment.wav | 12.864 | 56 | 185.4 | 185.4 | 191.0 | 0.018 | 2 | 0.38 | 188 / 188 | 2.9% |
| 7 Russian_Woman's talking.wav | 39.996 | 158 | 168.3 | 176.4 | 171.5 | 0.027 | 4 | 0.75 | 188 / 172 | 1.9% |
| 8 Russian_Man's talking.wav | 47.192 | 196 | 176.9 | 195.0 | 182.0 | 0.041 | 7 | 1.32 | 189 / 180 | 2.8% |
| 11 Э-Э-Э.wav | 9.056 | 30 | 141.1 | 161.8 | 148.2 | 0.050 | 2 | 0.43 | 218 / 218 | 4.8% |
| 12 Э-Э-Э.wav | 26.944 | 65 | 102.8 | 123.6 | 130.1 | 0.081 | 16 | 5.66 | 353 / 300 | 21.0% |
| 13 Э-Э-Э.wav | 22.528 | 61 | 115.3 | 152.2 | 118.7 | 0.099 | 2 | 0.63 | 315 / 315 | 2.8% |

## What the filenames mean

**сл is words (слов).** On every labeled file the tempo vowel count sits 1.30–1.45 times the number in the name, and 1/0.71 ≈ 1.41 is the vowels-per-word ratio k1 was built to undo. A syllable reading would put Nv next to the printed number. Durations printed in the names match the audio: 17.4 s → 17.376 s, 110 s → 110.228 s, 22 s → 21.504 s (the file is half a second shorter than the rounded label).

Labeled words compared with measured `Rs` on the same audio (labeled wpm = words × 60 / Ts):

| File | Words in the name | Nv | words/Nv | labeled wpm | Rs | Rs vs label |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1612сл … _02 | 1612 | 2241 | 0.719 | 115.8 | 114.3 | −1.3% |
| 1786сл … _03 | 1786 | 2471 | 0.723 | 117.5 | 115.4 | −1.8% |
| 240 сл (-) пау (110с) Дама | 240 | 349 | 0.688 | 130.6 | 134.9 | +3.2% |
| 36 сл (-8 пау) 17,4с лукомо | 36 | 52 | 0.692 | 124.3 | 127.5 | +2.6% |
| 913сл … _04 | 913 | 1226 | 0.745 | 114.1 | 108.8 | −4.7% |
| 63 сл (-) пау 22с лукомо | 63 | 83 | 0.759 | 175.8 | 164.4 | −6.5% |
| 922сл … _01 | 922 | 1198 | 0.770 | 118.3 | 109.2 | −7.7% |

The seven labels imply k1 = words/Nv from 0.69 to 0.77. The unweighted mean is 0.73, and weighting by Nv gives 0.73 as well. The two longest readings, 1612 and 1786 words, already land within 2% of `Rs` at k1 = 0.71.

The stricter summary vowel count is a worse match to these word labels (words/summary ≈ 0.91–0.97 on the long Dama readings). `Rs` should keep using the tempo count.

**(-) пау** is read as "no pauses" (pauses absent or cut out). **(-8 пау)** is read as "eight pauses were removed from this take." The 36-word file's duration and word count both match the name, and the pauses that remain are five gaps of 150–195 ms, which is the same short-gap population as the other pause-removed clip.

The three `Э-Э-Э` files are distinct hesitation recordings. They are in the table and were left out of the word-count comparison.

## Fast against slow

Both pairs rank fast above slow on `Rs`, legacy `Ra`, and direct `Ra`.

| Pair | Ts (s) | Nv | Rs | Ra legacy | Ra direct | Np | pause ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| English fast | 31.712 | 125 | 167.9 | 190.4 | 183.7 | 9 | 8.6% |
| English slow | 48.304 | 113 | 99.7 | 139.3 | 117.9 | 33 | 15.5% |
| Poeme fast | 12.512 | 47 | 160.0 | 160.0 | 162.0 | 1 | 1.2% |
| Poeme slow | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 9 | 17.0% |

Vowel counts barely change inside a pair (125 vs 113, 47 vs 50) while duration does, which is what a same-text pair should do. English fast/slow is 1.68 on `Rs` and 1.56 on direct `Ra`. Poeme fast/slow is 1.63 on `Rs` and 1.37 on direct `Ra`. The slow poem's pauses start at 240 ms (median 345 ms), so they are well clear of the 150 ms floor.

## Clips labeled as having no pauses

| File | Expected | Np | ΣTp (s) | pause lengths | direct Ra vs Rs |
| --- | --- | ---: | ---: | --- | --- |
| 63 сл (-) пау 22с | Np ≈ 0, Ra ≈ Rs | 2 | 0.30 | both exactly 150 ms | 166.8 vs 164.4 (+1.4%) |
| 36 сл (-8 пау) | pauses already removed | 5 | 0.93 | 150–195 ms, median 195 | 134.7 vs 127.5 (+5.7%) |
| 240 сл (-) пау (110с) | Np ≈ 0, Ra ≈ Rs | 41 | 7.23 | p25 150, median 165, p75 180 | 144.3 vs 134.9 (+7.0%) |
| Belarus weather | (no label; continuous read) | 0 | 0.00 | — | 194.9 = 194.9 |

The 63-word file is essentially there: two pauses sitting on the 150 ms floor, and direct `Ra` within 1.4% of `Rs`. The 240-word file is not. Its 41 pauses pile up between 150 and 180 ms, so the detector is counting gaps one or two hops above the minimum (the hop is 15 ms). The weather reading shows the same settings can return Np = 0 on continuous speech.

## Calibration

Defaults were not changed. These are suggestions from the sweeps below.

**Pause minimum: try 200–250 ms.** Raising only the minimum, at the shipped −25 dB level:

| File | Np at 150 | 200 | 250 | 300 |
| --- | ---: | ---: | ---: | ---: |
| 240 сл (-) пау | 41 | 6 | 4 | 0 |
| 63 сл (-) пау | 2 | 0 | 0 | 0 |
| 36 сл (-8 пау) | 5 | 1 | 0 | 0 |
| English fast | 9 | 6 | 5 | 3 |
| English slow | 33 | 18 | 10 | 7 |
| Poeme fast | 1 | 0 | 0 | 0 |
| Poeme slow | 9 | 9 | 8 | 6 |

At 200 ms the 63-word file is exactly Np = 0 and Ra = Rs, the 36-word file drops to one pause, and the 240-word file drops to 6 pauses (1.3% of the file, direct Ra 136.7 against Rs 134.9). The slow poem keeps all 9 pauses, because its shortest is 240 ms, and fast stays above slow on every rate. At 250 ms the 36-word file is clean and the 240-word file still has 4 pauses. Exact Np = 0 on the 240-word file needs 300 ms, which also cuts the slow poem from 9 pauses to 6 and the slow English reading from 33 to 7. 200 ms is the first setting that clears the one-hop false pauses without eating the poem. 300 ms is the setting that makes the "no pauses" label literal.

**−25 dB: leave it.** The same files at the 150 ms minimum, varying only the relative level:

| File | Np at −20 dB | −25 | −30 | −35 | −40 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 240 сл (-) пау | 47 | 41 | 41 | 41 | 41 |
| 63 сл (-) пау | 2 | 2 | 2 | 2 | 2 |
| 36 сл (-8 пау) | 5 | 5 | 5 | 5 | 5 |
| Poeme slow | 10 | 9 | 9 | 9 | 9 |
| English slow | 39 | 33 | 32 | 32 | 32 |
| 1612сл Dama _02 | 505 | 338 | 173 | 67 | 67 |
| Helena | 320 | 290 | 258 | 219 | 182 |
| Belarus weather | 0 | 0 | 0 | 0 | 0 |

On the pause-removed clips the silence level does not move from −25 dB to −40 dB. The noise-floor gate (10th percentile × speech-over-noise) is already above the −25 dB line, so a deeper relative level never gets used. Those short gaps are deep enough that duration is the lever, which matches the 150–180 ms pile-up. Loosening to −20 dB adds pauses (240 goes from 41 to 47, the long Dama reading from 338 to 505).

On the long narrative takes the −25 dB line is active. Moving to −30 dB cuts the 1612-word reading from 338 pauses to 173 and Helena from 290 to 258. That is a large change in pause time on speech the filenames do not mark as pause-free, so it is a poor way to fix the 240-word clip.

**k1: 0.71 is already in the right place.** A single trial value of 0.73 matches the vowel-weighted word labels and would lift the readings that currently sit a few percent low (913 words −4.7%, 922 words −7.7%). It would also push the 240-word and 36-word clips, which are already slightly high, further up. The two longest files are within 2% at 0.71. Leave k1 until Boris wants that half-step.
