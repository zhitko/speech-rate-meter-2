# Evaluation on Boris's test WAVs

Direct pause time and both articulation rates, on every file in `data/tests/` from branch `cursor/fix-test-wav-playback-ba86` (PR #5). The same numbers are in [eval-tests.csv](eval-tests.csv).

This run uses the shipped default pause minimum of **200 ms**. k1 stays 0.71 and the silence level stays about 25 dB under the 98th percentile of intensity (raised to the noise-floor gate when that is higher). k2 = 1.2, k3 = 0.30. An earlier pass at 150 ms is summarized under [What changed at 200 ms](#what-changed-at-200-ms).

This branch's application code does not include PR #5's loader. That loader was compiled into a throwaway program. It skips `LIST` and `cue` chunks, mixes to mono, and resamples to 8 kHz on the signed 16-bit scale the analyzer expects. Every file in this set is already 8 kHz, mono, 16-bit PCM, so the loaded sample count equals the data chunk. Eleven of the twenty-three files also carry `LIST` and/or `cue` chunks.

`Rs` in the tables is the legacy whole-buffer rate, k1 × 60 × Nv / Ts. Direct speech rate uses the utterance span and is not tabulated here. `Nv` is the tempo vowel count that `Rs` and `Ra` use. The stricter whole-recording vowel count (voiced peak, tail flushed) is `summary_vowels` in the CSV. Rates are words per minute.

`Tp legacy` is k3 × (Tc − Tm), with Tc the power-mean inter-nucleus gap and Tm the median gap, both in seconds. That product can be negative. The pause score the app still publishes in Legacy mode is the normalized one, P = k3 × (Tc − Tm) / Tm, and it is 0 when Tc < Tm. On this set Tc ≥ Tm for every file, so P = Tp legacy / Tm. `legacy_P` in the CSV is that score. Legacy `Ra` and `Tp legacy` do not use the pause minimum, so they are the same at 150 ms and at 200 ms. Direct `Ra` is k1 × 60 × Nv / (Ts − ΣTp). Leading and trailing silence stay inside Ts and outside ΣTp.

Four pairs are byte-identical, so the table has 23 rows and 19 distinct recordings:

- `8 Poeme_fast.wav` and `9 Poeme_fast.wav`
- `9 Poeme_slow.wav` and `10 Poeme_slow.wav`
- `3 Belarus_Wether (Speaker).wav` and `3 Belarussian_Wether Info.wav`
- `5 Russian_Story (Speaker).wav` and `922сл Dama s sobachkoi _01.wav`

## Results at 200 ms

| File | Ts (s) | Nv | Rs | Ra legacy | Ra direct | Tp legacy (s) | Np | ΣTp (s) | mean / median (ms) | pause ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 240 сл (-) пау (110с) Дама.wav | 110.228 | 349 | 134.9 | 143.5 | 136.7 | 0.050 | 6 | 1.49 | 248 / 255 | 1.4% |
| 63 сл (-) пау 22с лукомо.wav | 21.504 | 83 | 164.4 | 164.4 | 164.4 | 0.009 | 0 | 0.00 | 0 / 0 | 0.0% |
| 36 сл (-8 пау) 17,4с  лукомо.wav | 17.376 | 52 | 127.5 | 127.5 | 129.1 | 0.013 | 1 | 0.21 | 210 / 210 | 1.2% |
| 1612сл Dama s sobachkoi _02.wav | 835.500 | 2241 | 114.3 | 154.1 | 126.5 | 0.180 | 180 | 80.66 | 448 / 330 | 9.7% |
| 1786сл Dama s sobachkoi _03.wav | 912.327 | 2471 | 115.4 | 144.2 | 126.3 | 0.144 | 187 | 78.90 | 422 / 360 | 8.6% |
| 913сл Dama s sobachkoi _04.wav | 479.896 | 1226 | 108.8 | 139.6 | 117.0 | 0.162 | 88 | 33.65 | 382 / 308 | 7.0% |
| 922сл Dama s sobachkoi _01.wav | 467.486 | 1198 | 109.2 | 142.0 | 122.3 | 0.167 | 120 | 50.05 | 417 / 375 | 10.7% |
| 1 English_Fast.wav | 31.712 | 125 | 167.9 | 190.4 | 180.4 | 0.045 | 6 | 2.19 | 365 / 300 | 6.9% |
| 2 English_Slow.wav | 48.304 | 113 | 99.7 | 139.3 | 111.0 | 0.171 | 18 | 4.95 | 275 / 262 | 10.2% |
| 8 Poeme_fast.wav | 12.512 | 47 | 160.0 | 160.0 | 160.0 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 9 Poeme_fast.wav | 12.512 | 47 | 160.0 | 160.0 | 160.0 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 9 Poeme_slow.wav | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 0.072 | 9 | 3.69 | 410 / 345 | 17.0% |
| 10 Poeme_slow.wav | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 0.072 | 9 | 3.69 | 410 / 345 | 17.0% |
| 3 Belarus_Wether (Speaker).wav | 47.216 | 216 | 194.9 | 194.9 | 194.9 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 3 Belarussian_Wether Info.wav | 47.216 | 216 | 194.9 | 194.9 | 194.9 | 0.004 | 0 | 0.00 | 0 / 0 | 0.0% |
| 4 Russian_Story (Helena).wav | 527.360 | 1357 | 109.6 | 126.1 | 134.2 | 0.090 | 184 | 96.50 | 524 / 450 | 18.3% |
| 5 Russian_Story (Speaker).wav | 467.486 | 1198 | 109.2 | 142.0 | 122.3 | 0.167 | 120 | 50.05 | 417 / 375 | 10.7% |
| 6 Russian _Advatasment.wav | 12.864 | 56 | 185.4 | 185.4 | 188.5 | 0.018 | 1 | 0.21 | 210 / 210 | 1.6% |
| 7 Russian_Woman's talking.wav | 39.996 | 158 | 168.3 | 176.4 | 169.3 | 0.027 | 1 | 0.24 | 240 / 240 | 0.6% |
| 8 Russian_Man's talking.wav | 47.192 | 196 | 176.9 | 195.0 | 178.7 | 0.041 | 2 | 0.47 | 232 / 232 | 1.0% |
| 11 Э-Э-Э.wav | 9.056 | 30 | 141.1 | 161.8 | 145.7 | 0.050 | 1 | 0.28 | 285 / 285 | 3.1% |
| 12 Э-Э-Э.wav | 26.944 | 65 | 102.8 | 123.6 | 127.9 | 0.081 | 14 | 5.29 | 378 / 308 | 19.7% |
| 13 Э-Э-Э.wav | 22.528 | 61 | 115.3 | 152.2 | 117.7 | 0.099 | 1 | 0.45 | 450 / 450 | 2.0% |

## What the filenames mean

**сл is words (слов).** On every labeled file the tempo vowel count sits 1.30–1.45 times the number in the name, and 1/0.71 ≈ 1.41 is the vowels-per-word ratio k1 was built to undo. A syllable reading would put Nv next to the printed number. Durations printed in the names match the audio: 17.4 s → 17.376 s, 110 s → 110.228 s, 22 s → 21.504 s (the file is half a second shorter than the rounded label).

Labeled words compared with measured `Rs` on the same audio (labeled wpm = words × 60 / Ts). `Rs` does not depend on the pause minimum:

| File | Words in the name | Nv | words/Nv | labeled wpm | Rs | Rs vs label |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1612сл … _02 | 1612 | 2241 | 0.719 | 115.8 | 114.3 | −1.3% |
| 1786сл … _03 | 1786 | 2471 | 0.723 | 117.5 | 115.4 | −1.8% |
| 240 сл (-) пау (110с) Дама | 240 | 349 | 0.688 | 130.6 | 134.9 | +3.2% |
| 36 сл (-8 пау) 17,4с лукомо | 36 | 52 | 0.692 | 124.3 | 127.5 | +2.6% |
| 913сл … _04 | 913 | 1226 | 0.745 | 114.1 | 108.8 | −4.7% |
| 63 сл (-) пау 22с лукомо | 63 | 83 | 0.759 | 175.8 | 164.4 | −6.5% |
| 922сл … _01 | 922 | 1198 | 0.770 | 118.3 | 109.2 | −7.7% |

The seven labels imply k1 = words/Nv from 0.69 to 0.77. The unweighted mean is 0.73, and weighting by Nv gives 0.73 as well. The two longest readings, 1612 and 1786 words, already land within 2% of `Rs` at k1 = 0.71. That coefficient was left at 0.71.

The stricter summary vowel count is a worse match to these word labels (words/summary ≈ 0.91–0.97 on the long Dama readings). `Rs` should keep using the tempo count.

**(-) пау** is read as "no pauses" (pauses absent or cut out). **(-8 пау)** is read as "eight pauses were removed from this take."

The three `Э-Э-Э` files are distinct hesitation recordings. They are in the table and were left out of the word-count comparison.

## What changed at 200 ms

`Rs`, `Nv`, legacy `Ra`, and `Tp legacy` are unchanged, because they do not use the pause minimum. Direct `Ra`, `Np`, and `ΣTp` are what moved. The 150 ms column is the previous run of this same set.

| File | Np at 150 | Np at 200 | direct Ra at 150 | direct Ra at 200 | Rs |
| --- | ---: | ---: | ---: | ---: | ---: |
| 63 сл (-) пау | 2 | 0 | 166.8 | 164.4 | 164.4 |
| 36 сл (-8 пау) | 5 | 1 | 134.7 | 129.1 | 127.5 |
| 240 сл (-) пау | 41 | 6 | 144.3 | 136.7 | 134.9 |
| English fast | 9 | 6 | 183.7 | 180.4 | 167.9 |
| English slow | 33 | 18 | 117.9 | 111.0 | 99.7 |
| Poeme fast | 1 | 0 | 162.0 | 160.0 | 160.0 |
| Poeme slow | 9 | 9 | 118.6 | 118.6 | 98.4 |

The 63-word file now has Np = 0 and direct Ra = Rs. At 150 ms its two pauses sat exactly on the 150 ms hop. The 36-word file drops from five pauses to one 210 ms gap, and direct Ra is 1.2% above Rs (129.1 vs 127.5) instead of 5.7%. The 240-word file drops from 41 pauses piled at 150–180 ms to 6 pauses (minimum 210 ms, median 255 ms, 1.4% of the file). Direct Ra is 1.4% above Rs (136.7 vs 134.9) instead of 7%. Exact Np = 0 on that file still needs a 300 ms minimum.

The slow poem is unchanged: its shortest pause is 240 ms, so 200 ms keeps all 9. Poeme fast loses its single 150 ms pause and direct Ra falls to Rs. English slow loses more pauses than English fast (33 → 18, against 9 → 6).

## Fast against slow

Both pairs still rank fast above slow on `Rs`, legacy `Ra`, and direct `Ra`.

| Pair | Ts (s) | Nv | Rs | Ra legacy | Ra direct | Np | pause ratio |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| English fast | 31.712 | 125 | 167.9 | 190.4 | 180.4 | 6 | 6.9% |
| English slow | 48.304 | 113 | 99.7 | 139.3 | 111.0 | 18 | 10.2% |
| Poeme fast | 12.512 | 47 | 160.0 | 160.0 | 160.0 | 0 | 0.0% |
| Poeme slow | 21.656 | 50 | 98.4 | 110.4 | 118.6 | 9 | 17.0% |

Vowel counts barely change inside a pair (125 vs 113, 47 vs 50) while duration does. English fast/slow is 1.68 on `Rs` and 1.62 on direct `Ra`. Poeme fast/slow is 1.63 on `Rs` and 1.35 on direct `Ra`.

## Clips labeled as having no pauses

| File | Expected | Np | ΣTp (s) | pause lengths | direct Ra vs Rs |
| --- | --- | ---: | ---: | --- | --- |
| 63 сл (-) пау 22с | Np ≈ 0, Ra ≈ Rs | 0 | 0.00 | none | 164.4 = 164.4 |
| 36 сл (-8 пау) | pauses already removed | 1 | 0.21 | one gap of 210 ms | 129.1 vs 127.5 (+1.2%) |
| 240 сл (-) пау (110с) | Np ≈ 0, Ra ≈ Rs | 6 | 1.49 | min 210, median 255 | 136.7 vs 134.9 (+1.4%) |
| Belarus weather | (no label; continuous read) | 0 | 0.00 | — | 194.9 = 194.9 |

## Why 200 ms, and what was left alone

The sweep that picked 200 ms, at the shipped −25 dB level:

| File | Np at 150 | 200 | 250 | 300 |
| --- | ---: | ---: | ---: | ---: |
| 240 сл (-) пау | 41 | 6 | 4 | 0 |
| 63 сл (-) пау | 2 | 0 | 0 | 0 |
| 36 сл (-8 пау) | 5 | 1 | 0 | 0 |
| English fast | 9 | 6 | 5 | 3 |
| English slow | 33 | 18 | 10 | 7 |
| Poeme fast | 1 | 0 | 0 | 0 |
| Poeme slow | 9 | 9 | 8 | 6 |

200 ms is the first step that clears the one-hop gaps and still keeps every pause in the slow poem. 300 ms is what makes the 240-word label literal, and it also cuts that poem from 9 pauses to 6.

**−25 dB was left as it is.** On the pause-removed clips the silence level does not move from −25 dB to −40 dB, because the noise-floor gate is already above the −25 dB line. Tightening the level does not remove those short gaps. Loosening it to −20 dB adds pauses. On the long narrative takes the −25 dB line is active (the 1612-word reading went from 338 pauses at −25 dB to 173 at −30 dB, both at a 150 ms minimum), so the level is the wrong knob for the short false gaps.

**k1 stays 0.71.** A trial value of 0.73 would match the vowel-weighted word labels. The two longest files are already within 2% at 0.71.
