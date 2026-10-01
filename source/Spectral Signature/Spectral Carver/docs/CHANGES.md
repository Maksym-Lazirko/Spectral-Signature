# Changes from Maksym's supplied archive

## Audio and host behavior

- Initialize reconstruction normalization for the default configuration and correct FFT scaling and DC/Nyquist indexing.
- Use an instance-owned radix-2 FFT with preallocated tables. The processing path does not take JUCE's fallback FFT spinlock.
- Keep latency at 16,384 samples across FFT and overlap changes. Align dry/wet paths and fade through dry audio while a new processing configuration fills.
- Preserve timing in host and parameter bypass, with raw dry audio independent of gain controls.
- Keep mask buffers valid while audio and UI readers use them. Coalesce rapid mask requests on the worker thread.
- Bound image and state input, reject malformed/non-finite values, embed accepted image data in saved state, and clear stale image selection when text is applied.
- Use a 20% default wet mix and retain the existing host parameter IDs and plugin identity.

## Interface

- Replace the flat control grid with source, mask, signal, character, spectrum, and protection/output sections.
- Draw the actual two-dimensional mask instead of a column silhouette. Add a cursor driven by the engine's cycle position.
- Add smoothed meter bars, depth and hover feedback, legible units, percentage entry, and explanatory tooltips.
- Support image drag-and-drop and use a lifetime-safe asynchronous file chooser callback.
- Keep peak labels accurate and display a specific note when additive noise-fill is selected.
- Keep all visual rendering and animation on the UI thread.

## Build and documentation

- Add a pinned, offline CMake build for Windows x64 VST3 with Visual Studio 2022 and JUCE 8.0.12.
- Include the missing spectral-engine source files and repair the Projucer source list/exporter.
- Use the static Microsoft C++ runtime and inspect the resulting dynamic-library imports.
- Add native DSP/state regressions and a separate VST3 host smoke test.
- Rewrite all nine guideline pages into a concise product brief and a guide that distinguishes current behavior from future features.

The untouched original ZIP, its hash, and the original guideline images are retained in the delivery. Old generated projects, build intermediates, and bundled precompiled binaries are not used as release outputs.
