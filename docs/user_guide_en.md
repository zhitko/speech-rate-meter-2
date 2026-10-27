# Speech Rate Meter 2

Speech Rate Meter 2 estimates how fast you are speaking. It does not recognize words. While you speak, the arcs and the tempo chart follow your recent speech. The numbers appear when you stop.

## What the numbers mean

- **Speech rate** is the overall pace, in words per minute, pauses included. By default the gauge shows it and names the zone Slow, Average, or Fast. With **Speech rate method** set to Direct, silence before the first sound and after the last is left out of the time. Legacy uses the whole buffer. The Whole recording card shows both.
- **Articulation** is the pace while you are actually speaking, with the gaps left out. It is also in words per minute. With **Articulation method** set to Direct, those gaps are silent unvoiced stretches measured in the signal, taken out of the same span Direct speech rate uses. With no such gaps the two direct rates are equal. Window uses the whole measured buffer instead of that span, so silence before the first sound and after the last stays in the time, and it can sit below the speech rate. Legacy keeps the older estimate. A saved result shows it as a card when the gauge is set to speech rate and **Show Articulation rate tile** is on.
- **Fillers** is how much some sounds are drawn out, such as a long “uh”. It is a percent, not a count of words. A saved result shows the card when **Show Fillers tile** is on.
- **Pauses** is the length of the longer gaps, in seconds. Direct mode shows their average length. Legacy shows the older score. It is not the total time you were silent. A saved result shows the card when **Show Pauses tile** is on.
- **Speech** is the total time that counted as speech. A saved result shows the card when **Show Speech tile** is on. While you are recording, the red badge shows the time since **Start**. Silence before speech is detected is not included, and a pause is included only up to the Pause setting. From one minute the card reads `1m 30s`.

While you are recording, speech rate, articulation, fillers, and pauses use the **Analysis window**: the most recent kept speech (10 seconds by default). Direct speech rate, direct articulation, window articulation, and the pause length use that same window. At the start of a session the window is only as long as the audio collected so far. They are not a running total from **Start**. **Speech** keeps the total since **Start**. Home does not show these numbers during the session: the arcs and the tempo chart move. After **Stop** and after **Open File**, the numbers appear in the arc centre, and the cards whose switches are on appear with them. While you are recording, those cards stay hidden. After **Stop**, the first four become averages for the whole session, and **Speech** stays the total. The gauge chip then reads **Mean values**. The **Whole recording** card is separate: it measures the entire take, including silence between phrases, and shows how the pause lengths are spread out, plus both speech rates and all three articulation estimates.

## Record

1. Open **Home**.
2. Press **Start** and speak naturally. The microphone stays open. Home shows the tempo chart and the arcs together. While you record, both move and the numbers stay hidden. Until the first point appears, the chart says that everything is working and that the tempo appears about 2 seconds after you start speaking. The line grows from the left; when it reaches the right edge, older points slide off. A halo around **Stop** follows the microphone: if you are speaking and the halo stays still, the microphone is not being heard. With **Detect speech automatically** on, stay quiet while background noise is measured, then speak. With it off, measuring starts when you press **Start**. The chart adds a point on each live update, at the same median pace the arcs use, and keeps the last **Chart window** (5 minutes by default). Older points slide off.
3. With **Detect speech automatically** on, a pause ends the phrase; it is saved, and the next one starts when you speak again. With it off, a pause does not end the phrase, and the recording is saved in 15-second parts.
4. Press **Stop** when the session is finished. Home then shows the averages on the arcs and the cards whose switches are on, with the chip **Mean values**, and keeps the tempo chart above the arcs. History shows the same averages and the same cards.

On a computer, **Open File** sits to the right of **Start**. It measures one existing WAV recording and does not save a session. PCM and 32-bit float files are accepted, including other sample rates, stereo, and files with extra header data. The app converts them to 8000 Hz mono before measuring. **Listen to test** then plays that recording. If a file cannot be read, the page says why. Android does not show this button.

Silence and background noise are not counted as speech. When the window holds no speech, the numbers stay and the chip reads **Listening**. A phrase shorter than one second is not saved; the chip then reads **Too short to save**. After **Stop**, **Listen** sits to the left of **Start** when a recording was saved, and plays that recording. Press it again to stop playback. The audio is deleted when you leave the app, unless **Keep recording files** is on.

While a session is running, Home outlines the gauge card in red and shows a red badge with a blinking dot and the time since **Start** (`mm:ss`). The timer does not reset between phrases. Other pages show a red **Recording** chip with the same time in the toolbar. Tap it to return to Home.

## History

**History** lists each session, newest first. A row shows the date, the speech rate, how many phrases were kept, how many times the numbers changed, and the other four averages from **Start** to **Stop**. Open a row to see those same averages, labeled **Mean values**, then the value cards that Settings leaves on, then a chart. Each point is one Home update. A pace already on the gauge is not repeated as a card.

## Settings

Opening **Settings** stops an active recording first. An open phrase shorter than one second is dropped; a longer one is saved. **Settings** holds language, theme, color, font size, and the navigation bar. **Show Navigation Menu** adds Home, History, and Settings along the bottom. The menu button stays in the toolbar.

Under **Measurement**:

- **Analysis window** is how many recent seconds of speech Home uses while recording. Shorter reacts faster but jumps more. The shortest value is 3 seconds. There is no upper limit.
- **Updates per minute** is how often the numbers are recalculated. The shortest value is 1, so the curve can be updated once a minute instead of several times a minute. There is no upper limit. Above 60000 the wait stays 1 ms. The tempo chart adds a point on each of those updates. **Gauge median** still takes the middle of the last few readings (default 3), and **Chart window** still keeps the last 300 seconds by default.
- **Chart window** is how many seconds of the tempo chart stay on screen. The default is 300 seconds (5 minutes). Older points slide off. With **Use all readings** on, each point is the median of every reading since Start. With it off, the values match the arcs: the gauge median while measuring, and zero at rest. Home shows this chart together with the arcs while you record and after **Stop**.
- **Gauge** chooses the pace on the arc. **Speech rate** draws that pace. **Articulation rate** draws articulation. **Both** draws speech rate on the outer arc and articulation on the inner arc. A pace already on the gauge is not also a card.
- **Show Speech rate tile** and **Show Articulation rate tile** turn those cards on after Stop, after Open File, and on a saved result. Each switch cannot be changed, and the card stays hidden, while that pace is already on the gauge. The cards stay hidden while you are recording.
- **Show Fillers tile** is off until you turn it on. The card appears after Stop, after Open File, and on a saved result.
- **Show Pauses tile** and **Show Speech tile** are on by default. They appear after Stop, after Open File, and on a saved result. **Show Whole recording tile** is on by default and shows the summary on Home after Stop or Open File.
- **Use all readings** is on by default. Each tempo-chart point is then the middle of every live reading since Start, and **Gauge median** cannot be changed. The arcs still use **Gauge median**. Turn it off and each chart point is that gauge median, with the tip at zero while the arcs are at rest.
- **Gauge median** is how many latest readings of each pace on the gauge it takes the middle of. The tempo chart uses that same middle value only when **Use all readings** is off. The default is 3. Set it to 1 to show the current reading.
- **Pause** is how much silence ends a phrase and saves it. This is used only when **Detect speech automatically** is on.
- **Phrasal pause** is the shortest silent, unvoiced gap that counts as a pause. The default is 200 ms.
- **Speech rate method** chooses Direct or Legacy. Direct leaves out silence before the first sound and after the last. Legacy uses the whole buffer. The Whole recording card shows both.
- **Articulation method** chooses Direct, Legacy, or Window. Direct measures pauses in the signal and removes them from that same span. Window removes those same pauses from the measured buffer, and keeps silence at the ends of it. Legacy keeps the older estimate. The Whole recording card shows all three. **Window scale** (K5), under Advanced, changes only the Window rate. Its default is 0.71.
- **Detect speech automatically** measures background noise after **Start**, then waits for you to speak. When it is off, measuring starts when you press **Start**, a pause does not end the phrase, and the recording is saved in 15-second parts.

**Slow** and **Fast** are the ends of the gauge. The arc between them is split equally into Slow, Average, and Fast.

**Advanced** opens the controls that change how the numbers are calculated, including **Keep recording files** and **Shortest vowel**. The checkbox turns off when you leave the app. **Keep recording files** itself stays as you set it. On, each phrase is left as a WAV file in `data/records`. Off, those files are deleted when you leave the app. **Shortest vowel** is the shortest peak that still counts, in milliseconds. The default is 8 ms: a single intensity point and a one-frame run (4 ms at the default shift) are ignored, and a peak of 8 ms or longer is kept. 0 ms also counts that single point. The step is 15 ms, from 0 to 2000, and the same limit is used while recording and in the whole-recording summary.

**Delete user data** removes saved sessions and any recording files still kept in `data/records`. These settings are kept.

## Other pages

- **User Guide** is this text.
- **Privacy Policy** describes what stays on the device.
- **Open-source licences** lists the components built into the application.
