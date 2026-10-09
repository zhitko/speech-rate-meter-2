# Speech Rate Meter 2

Speech Rate Meter 2 estimates how fast you are speaking. It does not recognize words. While you speak, the numbers follow your recent speech.

## What the numbers mean

- **Speech rate** is the overall pace, in words per minute. The gauge shows it and names the zone Slow, Average, or Fast.
- **Articulation** is the pace while you are actually speaking, with the gaps left out. It is also in words per minute, and it is never below the speech rate.
- **Fillers** is how much some sounds are drawn out, such as a long “uh”. It is a percent, not a count of words.
- **Pauses** is the length of the longer gaps, in seconds. It is not the total time you were silent.
- **Speech** is the total time that counted as speech, in seconds.

While you are recording, speech rate, articulation, fillers, and pauses use the **Analysis window**: the most recent kept speech (10 seconds by default). **Speech** keeps the total since **Start**. After **Stop**, the first four become averages for the whole session, and **Speech** stays the total. The gauge chip then reads **Mean values**.

## Record

1. Open **Home**.
2. Press **Start** and speak naturally. The microphone stays open. A meter inside the speech-rate gauge follows the microphone: if you are speaking and the bar stays empty, the microphone is not being heard. With **Detect speech automatically** on, stay quiet while background noise is measured, then speak. With it off, measuring starts when you press **Start**.
3. With **Detect speech automatically** on, a pause ends the phrase; it is saved, and the next one starts when you speak again. With it off, a pause does not end the phrase, and the recording is saved in 15-second parts.
4. Press **Stop** when the session is finished. Home then shows the averages, with the chip **Mean values**. History shows the same averages.

On a computer, **Open File** sits to the right of **Start**. It measures one existing WAV recording and does not save a session. The file must be 8000 Hz, mono, signed 16-bit PCM. Android does not show this button.

Silence and background noise are not counted as speech. When the window holds no speech, the numbers stay and the chip reads **Listening**. A phrase shorter than one second is not saved; the chip then reads **Too short to save**. The audio is deleted once the numbers are stored. There is no playback.

While a session is running, **Recording** stays in the toolbar on every page. Tap it to return to Home.

## History

**History** lists each session, newest first. A row shows the date, the speech rate, how many phrases were kept, how many times the numbers changed, and the other four averages from **Start** to **Stop**. Open a row to see those same averages, labeled **Mean values**, then a chart. Each point is one moment the numbers on Home changed.

## Settings

Opening **Settings** stops an active recording first. An open phrase shorter than one second is dropped; a longer one is saved. **Settings** holds language, theme, color, font size, and the navigation bar. **Show Navigation Menu** adds Home, History, and Settings along the bottom. The menu button stays in the toolbar.

Under **Measurement**:

- **Analysis window** is how many recent seconds of speech Home uses while recording. Shorter reacts faster but jumps more.
- **Updates per minute** is how often the numbers are recalculated.
- **Gauge median** is how many latest speech-rate readings the gauge takes the middle of. The default is 3. Set it to 1 to show the current reading.
- **Pause** is how much silence ends a phrase and saves it. This is used only when **Detect speech automatically** is on.
- **Detect speech automatically** measures background noise after **Start**, then waits for you to speak. When it is off, measuring starts when you press **Start**, a pause does not end the phrase, and the recording is saved in 15-second parts.

**Slow** and **Fast** are the ends of the speech-rate gauge. The arc between them is split equally into Slow, Average, and Fast.

**Show advanced settings** opens the controls that change how the numbers are calculated. The switch turns off when you leave the app.

**Delete user data** removes saved sessions. Recordings are not kept, so there is no audio to delete. These settings are kept.

## Other pages

- **User Guide** is this text.
- **Privacy Policy** describes what stays on the device.
- **Open-source licences** lists the components built into the application.
