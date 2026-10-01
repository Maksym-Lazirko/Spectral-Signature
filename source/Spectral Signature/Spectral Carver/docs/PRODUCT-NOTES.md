# Spectral Signature: product notes

These notes consolidate all nine supplied guideline pages. The original page images are preserved alongside the project. They describe the intended product; the implementation and validation notes identify which parts are delivered.

## Purpose

Spectral Signature applies a text or image pattern to the frequency content of a stereo master. Time runs horizontally through the pattern; frequency runs vertically. Brighter parts of the mask produce stronger changes. The pattern may become visible in a spectrogram, depending on the source, settings, and display resolution.

The source audio takes priority over pattern visibility. Spectrogram visibility does not establish inaudibility or provide a robust ownership or authentication system. Audition the difference signal and inspect the rendered master.

The intended placement is near the end of a stereo mastering chain. Cut mode, a modest depth, and a 10–25% wet mix are the starting point. A final true-peak check remains part of the mastering workflow.

## Signal processing

Use a short-time Fourier transform with overlapping windows. Preserve the source phase in Cut and Emboss modes, alter magnitudes within bounded limits, and reconstruct the waveform with overlap-add normalization. Align the dry and wet paths and report the complete processing delay to the host.

The normalized mask is `M(time, frequency)` in the range 0 to 1. In Cut mode, a depth `D` in dB and an adaptive weight `A` produce the target gain `10^(-D * M * A / 20)`. Emboss derives local mask contrast from the difference between the mask and a smoothed version, then applies bounded positive and negative gain. Noise-fill adds a level-limited carrier within selected mask regions; it is an audible creative option rather than the default mastering mode.

Support linked stereo, mid-only, side-only, and independent stereo processing. Use identical gain changes for linked stereo to preserve interchannel relationships. Mid and side processing must reconstruct left and right correctly. Do not introduce DC energy.

Smooth the mask across time and frequency. Adaptive masking should reduce changes in exposed or quiet regions, near dominant tonal components, and during strong transients. Low-frequency protection should reduce engraving below roughly 100–200 Hz. These are practical signal-dependent safeguards, not a validated perceptual model.

## Input and timing

Text input is multiline and uses Unicode where the installed font supports it. Image input is converted to grayscale. Rasterization and image decoding belong on a worker thread; audio processing reads a completed mask. An invalid or unavailable source must produce a useful error without damaging the current mask.

The engraving cycle ranges from 0.25 to 60 seconds and can be restarted. The broader design also proposes host-tempo synchronization, fixed or scrolling text, and several image fitting modes. Those features require explicit implementation and testing before being advertised.

Host state must preserve all parameters and the input needed to reproduce the mask. An imported image should remain recallable after its original file is moved. Text and image modes need an explicit source selection so a stale image path cannot override newer text.

## Controls and monitoring

The core control groups are:

- Input and output gain, wet mix, bypass, engraving mode, intensity, stereo placement, and cycle duration.
- FFT size, overlap, frequency range and mapping, time and frequency smoothing, mask contrast, threshold, inversion, and maximum cut or boost.
- Adaptive masking, transient and tonal protection, low and high frequency protection, and a conservative safety setting.
- Input, output, and difference meters; audition modes for checking what processing changes.

The proposed expanded interface adds font styling, mask transformations, drag-and-drop, spectrogram overlays, frequency handles, presets, A/B comparison, and explicit CPU/latency warnings. Clear labels, readable units, tooltips, and a visible distinction between sample peak and true peak take priority over decorative UI features.

The guideline preset ideas cover subtle text and image watermarks, logo engraving, broad or midrange signatures, high-frequency patterns, QR-like textures, aggressive carving, embossed artwork, and experimental noise-fill. Preset names are not evidence of mastering suitability; a finished preset must define its settings and intended use.

## Engineering requirements

The audio callback must avoid heap allocation, locks, waits, file access, image decoding, text rendering, logging, and GUI work. Preallocate FFT plans, windows, delay lines, overlap-add rings, masks, and scratch storage. Publish mask changes through a bounded handoff that keeps every reader's data valid.

Use float processing safely unless a separately tested double-precision path is implemented. Validate values from automation and saved state, including non-finite numbers. Preserve stable parameter identifiers and plugin identity for host recall. Keep bypass timing consistent and handle sample-rate, block-size, transport, and processing-configuration changes without unsafe reinitialization.

The requested formats are VST3 and macOS Audio Unit, with an optional standalone application. A Windows VST3 build does not demonstrate macOS, AU, ARM64, 32-bit, or DAW-specific compatibility. AAX, VST2, and other formats require their own build and validation decisions.

## Acceptance and remaining product work

Core acceptance covers delayed null and bypass behavior, stereo reconstruction, reported latency, all supported FFT and overlap choices, representative sample rates and block sizes, finite output, automation, concurrent mask replacement, and state recall. Malformed data and missing source files must leave a usable state.

Musical acceptance additionally needs quiet acoustic audio, dense and limited masters, voices, cymbals, guitars, bass-heavy material, and transient-rich drums. Check CPU use, offline bounce, mono fold-down, audible artifacts, true peak, and session recall in the DAWs being supported.

The following are separate product features unless the validation report explicitly confirms them: oversampled true-peak metering/limiting, automatic loudness compensation, selectable windows, host-tempo synchronization, dedicated offline-quality modes, extensive image/font transformations, a complete factory preset bank, and A/B state controls. Preserve a stable core before adding them.
