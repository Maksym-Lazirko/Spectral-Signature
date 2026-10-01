# Using Spectral Signature

Spectral Signature applies a text or image mask to a stereo signal. Its mask preview shows the source pattern, not a measured spectrogram of the output. The cursor follows the processing cycle.

## First session

1. Load the complete `Spectral Signature.vst3` bundle in a compatible 64-bit Windows VST3 host. Use the host's supported plugin-location or scanning workflow. This package does not install files globally.
2. Place the effect near the end of the mastering chain. It reports 16,384 samples of delay, so enable the host's plugin delay compensation. This version is intended for mastering and rendering, not low-latency live monitoring.
3. Enter text and choose **Apply text**, or choose **Import image**. You can also drop one PNG, JPEG, or GIF file onto the editor. Image input is limited to 8 MiB and 4096 by 4096 pixels. The decoded image is converted into a grayscale mask on a worker thread.
4. Start with **Cut**, **Linked stereo**, **Safe mode**, and **Protect lows** enabled. The default wet mix is 20%. Increase intensity or depth gradually while comparing against bypass.
5. Use **Listen > Difference** to hear the change. Return to **Normal** before export. Check the rendered file with your normal loudness and true-peak tools.

## Interface

**Source** contains multiline text input, limited to 2,048 characters, and image import. Brighter mask values cause stronger changes. The plugin stores imported image data in its saved state so recall does not require the original file.

**Engraving mask** displays the two-dimensional source mask. Time runs left to right and higher frequencies are at the top. **Restart cycle** returns the scan to the beginning. The source label reports the current input or processing status.

**Signal** displays input, output, and difference sample peaks. Meter motion is smoothed for readability. **Peak hold** is a sample-peak indicator; it is not an oversampled true-peak measurement.

**Character** controls mode, stereo placement, intensity, wet mix, cycle length, detail, transparency, and adaptive scaling. Cycle length is free-running, from 0.25 to 60 seconds. It is not synchronized to host tempo.

**Spectrum** controls frequency mapping, audition selection, frequency bounds, maximum attenuation and boost, and smoothing. Cut removes energy. Emboss applies bounded local contrast; it does not guarantee equal perceived loudness. Noise-fill adds sound and can be clearly audible.

The **Mid** and **Side** audition choices solo the processed mid or side signal. **Difference** auditions the signed change from the original signal. Audition level follows Wet mix without adding the dry signal.

**Protection & output** groups FFT/overlap settings, transient and tonal protection, mask contrast/threshold, and input/output gain. Larger FFT sizes and higher overlap cost more CPU. Quality changes preserve the reported host delay and briefly fade through the aligned dry path while the new window fills.

Hover over a control for its purpose. Knobs support direct value entry; percentage controls use 0–100% in the interface while retaining the original normalized parameter values for automation. The protection switches are independent of the source mask.

## Recall and bypass

The original plugin identity and parameter IDs are retained. Save a new copy of an important host session when trying the revised build, since corrected DSP changes the rendered sound compared with the faulty original.

Bypass preserves processing delay and returns the unprocessed signal. The plugin supports mono and stereo processing paths; the specific host still needs to expose the corresponding bus layout correctly. MIDI input and output are not used. Processing is single precision; the plugin does not advertise a native double-precision path.

The fixed delay is approximately 371.5 ms at 44.1 kHz, 341.3 ms at 48 kHz, 185.8 ms at 88.2 kHz, 170.7 ms at 96 kHz, and 85.3 ms at 192 kHz.

## Limits

Spectrogram visibility depends on the source material and analyzer settings. It is not a guarantee of inaudibility or watermark recovery. The safety stage bounds sample peaks; it does not replace a true-peak limiter or mastering review.

This version does not include host-tempo sync, selectable analysis windows, font styling controls, a factory preset bank, A/B slots, automatic loudness compensation, or an oversampled true-peak meter/limiter. The original broader product notes are retained in `PRODUCT-NOTES.md`.
