# Spectral Signature

**Text and image spectral engraving for audio**

**Co-authors: Maksym Lazirko and Henmoro™**

Spectral Signature turns text or an image into a time–frequency mask and uses it to shape an audio signal. It is designed for controlled spectral engraving near the end of a mastering chain, with tools for auditioning the change and keeping the source material audible and recognizable.

The pattern can become visible in a spectrogram, depending on the audio, settings, and analyzer. Visibility does not guarantee inaudibility, reliable watermark recovery, or proof of ownership. Listen to the processed result and check the rendered master.

<img width="1153" height="641" alt="image" src="https://github.com/user-attachments/assets/97c474da-0387-40b2-a3ee-7c1445fa41e4" />

| Release detail | Current status |
| --- | --- |
| Version | **1.0.1** |
| Built format | **Windows x64 VST3** |
| Plugin name / manufacturer metadata | Spectral Signature / Lazirko Records |
| Processing | Single-precision audio; mono and stereo layouts |
| Host parameters | 30, with the original IDs and order retained |
| Reported latency | **16,384 samples**, fixed across FFT settings |
| Native regression result | **232/232 checks passed** |
| Compiled-plugin validation | Independent VST3 host checks passed |
| Other platforms and commercial DAWs | Not verified by this release |

Paths in this README assume the layout of the combined source-and-VST3 package, with this file at its root. The same layout can be retained in a repository.

## How it works

The mask is read from left to right over a repeating cycle. Its vertical axis maps to frequency, with higher frequencies at the top. Brighter mask regions produce stronger engraving after threshold, contrast, smoothing, and protection settings are applied.

The audio engine uses a short-time Fourier transform, overlapping Hann windows, spectral processing, and normalized overlap-add reconstruction. Cut and Emboss apply real-valued gains to complex frequency bins, preserving the source phase. Noise-fill adds a signal-dependent carrier.

The main signal path is:

```text
Input → input gain → spectral processing → aligned wet/dry mix
      → output gain → optional sample-peak safety → output

Text or image → worker-thread rasterization → published spectral mask
```

The dry path is delayed to match the processed signal. Bypass returns the original delayed input, independently of the input/output gain controls and safety stage. Audition modes isolate selected signals instead of adding the dry signal back in.

## Implemented features

- **Text input:** multiline text, up to 2,048 characters. Unicode text is retained in state; visible glyph coverage depends on the installed font.
- **Image input:** PNG, JPEG, and GIF through the file picker or drag-and-drop. Files are limited to 8 MiB and 4,096 × 4,096 pixels and converted into a grayscale mask.
- **Three processing modes:** Cut, Emboss, and Noise-fill.
- **Stereo placement:** Linked stereo, Mid only, Side only, and Dual mono.
- **Frequency mapping:** Linear, Logarithmic, and Mel-like, with adjustable lower and upper bounds.
- **Mask shaping:** inversion, threshold, contrast, detail, and smoothing across frequency and time.
- **Signal-dependent protection:** adaptive scaling, transient protection, tonal protection, and optional low/high-frequency protection.
- **Quality choices:** FFT sizes of 1,024, 2,048, 4,096, 8,192, and 16,384 samples; overlaps of 4×, 8×, and 16×.
- **Audition tools:** Normal, Mask, Removed, Added, Difference, Mid, and Side.
- **Interface:** grouped controls, a live source-mask preview, cycle cursor, smoothed level indicators, peak hold, tooltips, direct numeric entry, and image drag-and-drop.
- **Host state:** parameter recall, text recall, and embedded image data that does not depend on the original image file remaining on disk.
- **Stable automation identity:** the original 30 parameter IDs and their order are preserved.

### Processing modes

| Mode | Behavior | Practical use |
| --- | --- | --- |
| **Cut** | Attenuates selected spectral regions within the configured bounds. | Starting point for restrained engraving. |
| **Emboss** | Applies bounded attenuation and boost derived from local mask contrast. | Emphasizes mask edges and contrast; does not guarantee equal perceived loudness. |
| **Noise-fill** | Adds a conservative carrier proportional to existing spectral energy. | An additive creative effect that can be audible. It does not create a carrier during digital silence. |

Linked stereo applies matching spectral gain changes to both channels. Mid and Side placement target those components before reconstructing left and right. The Dual mono option is available, but it is not a fully independent adaptive-analysis model for each channel.

### Audition modes

| Listen setting | What it exposes |
| --- | --- |
| **Normal** | The normal processed/dry mix. |
| **Mask** | The mask-weighted original spectrum. |
| **Removed** | The contribution removed by processing. |
| **Added** | The contribution added by processing. |
| **Difference** | The signed processed-minus-original contribution; called **Delta** in the host parameter choices. |
| **Mid / Side** | The processed mid or side component. These are not difference-only signals. |

Audition level follows Wet mix without adding the dry signal. Return **Listen** to **Normal** before exporting a finished master.

## Quick start

1. Extract the package and load the complete VST3 bundle in a compatible 64-bit Windows VST3 host.
2. Insert Spectral Signature near the end of the mastering chain and enable the host's plugin delay compensation.
3. Enter text and click **Apply text**, or use **Import image** / drag-and-drop.
4. Start with **Cut**, **Linked stereo**, **Safe mode**, and **Protect lows** enabled. The default Wet mix is **20%**.
5. Set the cycle length and frequency range, then adjust Intensity and Maximum cut gradually while comparing with bypass.
6. Use **Listen → Difference** to hear the change. Return to **Normal** for export.
7. Render a short test and inspect it with the spectrogram, loudness, and true-peak tools used in your mastering workflow.

The preview shows the **source mask**, not a measured spectrogram of the output. A visible preview does not mean the same pattern will be equally visible in every rendered signal.

## Controls and defaults

The editor exposes 18 rotary controls, six selectors, five switches, and a Restart cycle button. Percentage controls display 0–100% while retaining normalized host parameter values.

### Selectors

| Control | Choices | Default |
| --- | --- | --- |
| Process | Cut, Emboss, Noise-fill | Cut |
| Placement | Linked stereo, Mid only, Side only, Dual mono | Linked stereo |
| Mapping | Linear, Logarithmic, Mel-like | Logarithmic |
| Listen | Normal, Mask, Removed, Added, Difference, Mid, Side | Normal |
| FFT size | 1,024 / 2,048 / 4,096 / 8,192 / 16,384 | 2,048 |
| Overlap | 4× / 8× / 16× | 4× |

### Continuous controls

| Control | Range / default | Purpose |
| --- | --- | --- |
| Intensity | 0–100%; **35%** | Overall engraving strength. |
| Wet mix | 0–100%; **20%** | Balance between processed and aligned original audio in Normal mode. |
| Cycle length | 0.25–60 s; **8 s** | Duration of one complete horizontal mask scan. |
| Detail | 0–100%; **60%** | Retention of fine mask detail. |
| Transparency | 0–100%; **75%** | Reduces engraving strength; does not guarantee inaudibility. |
| Adaptive | 0–100%; **60%** | Adjusts the change according to local signal energy. |
| Lower bound | 10–20,000 Hz; **160 Hz** | Lower engraving-frequency bound. |
| Upper bound | 100–24,000 Hz; **16,000 Hz** | Upper engraving-frequency bound. |
| Maximum cut | 0–24 dB; **3 dB** | Nominal maximum attenuation before further processing limits. |
| Maximum boost | 0–12 dB; **1 dB** | Nominal maximum boost for Emboss. |
| Frequency smooth | 0–100%; **40%** | Smooths changes between neighboring frequency bins. |
| Time smooth | 0–100%; **45%** | Smooths changes between spectral frames. |
| Transient protect | 0–100%; **70%** | Reduces engraving during sudden changes in signal energy. |
| Tonal protect | 0–100%; **50%** | Reduces changes near prominent tonal components. |
| Mask contrast | 0–100%; **50%** | Shapes grayscale mask contrast. |
| Mask threshold | 0–100%; **8%** | Suppresses lower mask values. |
| Input gain | −24 to +24 dB; **0 dB** | Gain before spectral processing. |
| Output gain | −24 to +24 dB; **0 dB** | Gain after the wet/dry mix. |

Frequency bounds are constrained by the current sample rate. Safe mode and the adaptive/protection controls can reduce the effective processing depth below the nominal cut/boost settings.

### Switches and actions

| Control | Default / behavior |
| --- | --- |
| Invert mask | Off; reverses the mask. |
| Protect lows | On; reduces engraving in low-frequency content. |
| Protect highs | Off; reduces engraving near the upper frequency range. |
| Safe mode | On; applies conservative spectral limits and sample-peak safety. |
| Bypass | Off; fades to the delayed original signal. |
| Restart cycle | Restarts the mask scan; can be used repeatedly. |

The cycle is free-running. It is not synchronized to host tempo or promised to restart automatically with the transport.

### Meters and safety

Input, output, and difference meters show sample peaks, with smoothing for readability. Peak hold is also a sample-peak measurement.

Safe mode includes a sample-peak soft knee with a −0.5 dBFS ceiling while active. It is **not an oversampled true-peak limiter** and does not replace a true-peak check after rendering.

## Installing the Windows VST3

The combined package is named:

```text
Spectral-Signature-1.0.1-Source-and-VST3.zip
```

After extraction, the ready-to-load bundle is:

```text
Plugin/
└── Spectral Signature.vst3/
    └── Contents/
        ├── Resources/
        └── x86_64-win/
            └── Spectral Signature.vst3
```

1. Keep the **outer `Spectral Signature.vst3` directory and all its contents together**. Do not copy only the inner binary or rename it to `.dll`.
2. Put the bundle in a VST3 location supported by the host. Common Windows locations are `%LOCALAPPDATA%\Programs\Common\VST3` and `C:\Program Files\Common Files\VST3`; use the host's documented workflow, since supported locations can differ.
3. Rescan plugins in the host and load **Spectral Signature**, listed with manufacturer **Lazirko Records**.
4. Enable plugin delay compensation and test a short render before using it in an important session.

The VST3 bundle is not a standalone application. Double-clicking it is not a functional plugin test. The package has no installer and the build does not copy files into global plugin folders automatically.

The release uses the static Microsoft C++ runtime. Import inspection found Windows system-library dependencies and no separate VC runtime DLL requirement; this is not a clean-machine compatibility guarantee.

## Latency and quality settings

Spectral Signature reports **16,384 samples of latency for every FFT and overlap setting**. Shorter FFT modes align their output to the same timeline as the dry path.

Changing FFT size or overlap briefly fades through the aligned dry signal while the engine resets and fills the new processing configuration. Host latency remains unchanged during that transition.

| Sample rate | Fixed delay |
| --- | ---: |
| 44.1 kHz | 371.52 ms |
| 48 kHz | 341.33 ms |
| 88.2 kHz | 185.76 ms |
| 96 kHz | 170.67 ms |
| 192 kHz | 85.33 ms |

This is a mastering/rendering tradeoff, not a low-latency monitoring design. A smaller FFT changes spectral resolution and processing cost; it does not reduce the reported delay. Larger FFT sizes and higher overlap increase processing work.

## Saved state and session compatibility

- All 30 original parameter IDs and their order are retained.
- Manufacturer code `Manu`, plugin code `Dc8f`, and bundle ID `com.yourcompany.SpectralCarver` are retained for host identity.
- New instances start at 20% Wet mix. Restored parameter values remain authoritative.
- Text and successfully rendered image data are stored with the plugin state. Image recall does not require the original image file.
- Applying text replaces image mode, including when the new text is intentionally blank.
- A failed image decode retains the previous valid mask and its saved source.
- Available legacy image paths can be imported; a missing legacy image falls back to its stored text.

Version-2 state uses bounded embedded image data. State input is capped at 12 MiB. Saving can wait up to five seconds for a pending mask operation on the control/state path; failure or timeout retains the last valid source. This wait is not part of the audio processing callback.

Preserving identity does not mean preserving the sound of faulty processing. The reconstruction corrections change the output relative to the supplied implementation, so keep a copy of important sessions before substituting this build.

## Building from source

### Prerequisites

- Windows and an x64 build target.
- Visual Studio 2022 Build Tools with **Desktop development with C++**, the **MSVC v143** toolset, and a Windows 10/11 SDK.
- **CMake 3.24 or newer**.
- **C++20** support.
- A local copy of **JUCE 8.0.12**, pinned to commit `29396c22c93392d6738e021b83196283d6e4d850`.

The delivered build used MSVC **19.44.35228**, Windows SDK **10.0.26100.0**, and CMake **3.31.6-msvc6**. Those are the verified tool versions, rather than a claim that every compatible toolchain has been tested.

The package includes JUCE's root files, modules, and `extras/Build` helpers. Optional JUCE example applications and other tools are omitted; leave `JUCE_BUILD_EXAMPLES` and `JUCE_BUILD_EXTRAS` off. Configuration and building do not download dependencies.

### Configure, build, and run the native suite

Run these commands from the extracted package root in a developer PowerShell:

```powershell
$spectralProject = "source/Spectral Signature/Spectral Carver"
$juceSource = (Resolve-Path "dependencies/JUCE").Path

cmake -S "$spectralProject" -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR="$juceSource"
cmake --build build --config Release --target SpectralSignature_VST3 SpectralTests --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Build outputs:

```text
build/SpectralSignature_artefacts/Release/VST3/Spectral Signature.vst3/
build/SpectralTests_artefacts/Release/SpectralTests.exe
```

To use a different local JUCE checkout, pass its absolute path as `JUCE_SOURCE_DIR`. The project checks for version 8.0.12. The `.jucer` project is included as a convenience; **CMake is the validated build route**. The updated Projucer exporter was not independently regenerated and tested.

### Build and run the independent VST3 host check

From the package root:

```powershell
$juceSource = (Resolve-Path "dependencies/JUCE").Path

cmake -S "validation/host-source" -B build-host -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="$juceSource"
cmake --build build-host --config Release --parallel 2

$vst3Bundle = (Resolve-Path "Plugin/Spectral Signature.vst3").Path
& ".\build-host\SpectralSignatureHostCheck_artefacts\Release\SpectralSignatureHostCheck.exe" "$vst3Bundle"
```

That command checks the delivered bundle. To check a fresh build instead, set `$vst3Bundle` to `build/SpectralSignature_artefacts/Release/VST3/Spectral Signature.vst3` using `Resolve-Path`.

The host is a focused test application, not a commercial DAW or a certification validator.

## Source and package layout

```text
Plugin/
  Spectral Signature.vst3/       Ready-to-load Windows x64 VST3 bundle
source/Spectral Signature/Spectral Carver/
  CMakeLists.txt                Validated build configuration
  Spectral Carver.jucer         Projucer project
  BUILD.md                     Additional build notes
  Source/
    PluginProcessor.*          Parameters, host integration, gain/mix, state
    PluginEditor.*             Native interface and parameter bindings
    SpectralEngine.*           Masks, STFT processing and reconstruction
    RealtimeFFT.h              Preallocated radix-2 FFT implementation
  Tests/ProcessorTests.cpp     Native DSP, state and UI regression checks
  docs/                        User guide, product notes and engineering report
dependencies/JUCE/             Pinned framework modules and build helpers
validation/
  host-source/                 Independent VST3 host-check source
  *-final.log                  Test evidence
  Spectral-Signature-editor-final.png
Licenses/                      Dependency license notices
VALIDATION.md                  Detailed validation and limitations
THIRD-PARTY-NOTICES.txt         Dependency/provenance notes
SHA256-MANIFEST.json            Packaged-file hashes and sizes
```

The combined ZIP excludes the original input archive, old guideline images, source diff, temporary builds, IDE caches, credentials, and standalone validation executables. The original input is preserved separately.

## Validation results

Validation was performed on Windows on **1 October 2026**.

### Native processor and interface tests

The native suite passed **232/232 assertions**, with zero failures. CTest passed its **1/1 registered regression test**; that test runs the native suite.

| Check | Recorded result |
| --- | --- |
| Five sample rates × five FFT sizes × three overlap settings | 75 unity-reconstruction cases passed |
| Worst unity-reconstruction error | `7.45058e-08` absolute sample amplitude |
| Worst FFT/overlap automation error at unity | `4.47035e-08` |
| Requested 12 dB Cut on the test mask | Measured gain `0.251189` |
| Instrumented audio-callback C++ heap operations | `0` allocations / `0` deallocations |
| Mono, bypass, silence, nonfinite input, sample ceiling | Passed |
| Unicode text, empty text, image recall after source-file deletion | Passed |
| Corrupt image import followed by save/reopen | Previous valid image retained |
| 1,000 rapid mask requests with concurrent readers | Latest mask published; pinned mask unchanged |
| UI inventory and layout bounds | 18 sliders, six selectors, five toggles plus restart verified |
| UI actions and automation bindings | Slider updates in both directions, selector, bypass, Apply text and Restart cycle passed |

The interface snapshot was also visually inspected for mask content, readability, clipping, and overlaps.

The allocation counter covers C++ `new`/`delete` on monitored processing paths. It does not measure every operating-system or C allocation API, and it does not prove hard real-time scheduling. The mask stress test is not a thread-sanitizer proof.

### Actual VST3 module checks

A separate source-built JUCE host scanned and loaded the compiled release bundle, then verified:

- One Spectral Signature effect, stereo I/O, 30 parameters, and single-precision processing.
- Rendering at 44.1, 48, 88.2, 96, and 192 kHz.
- Zero-length callbacks and varying blocks of 1, 17, 64, 255, 512, 1,024, and 2,048 samples.
- A constant 16,384-sample reported delay.
- Zero measured error against delayed dry audio in the dry-path and bypass checks.
- Finite output in all three modes and a measurable spectral change under a strong Cut setting.
- Parameter recall after serializing state, destroying the instance, creating a new instance, and restoring state.

These checks establish tested behavior in the custom host. They do **not** establish compatibility with every DAW, scanner, automation workflow, or offline-render implementation.

### Build and archive checks

The x64 module was inspected for ASLR, high-entropy addressing, NX, Control Flow Guard, and dynamic-library imports. No separate Microsoft C++ runtime DLL was required by its inspected imports.

Archive integrity and SHA-256 manifests were checked. The combined package's source matches the finished working copy and its VST3 matches the tested binary. A fresh extraction of the source package configured successfully and built JUCE's build helper from the packaged source; the full plugin test results above come from the verified release build.

See [VALIDATION.md](VALIDATION.md), [native results](validation/processor-tests-final.log), [VST3 host results](validation/host-validation-final.log), and [CTest results](validation/ctest-final.log).

## What changed in 1.0.1

- Corrected reconstruction normalization, inverse-FFT scaling, and DC/Nyquist handling.
- Replaced the fallback FFT path with an instance-owned implementation using preallocated tables.
- Aligned dry and processed paths and made latency stable across quality changes.
- Corrected bypass timing and removed dry leakage from contribution audition modes.
- Protected mask buffers while readers use them and coalesced worker requests.
- Bounded image/state input and made image recall independent of external file paths.
- Retained the last valid image after decode failure, including through save/reopen.
- Corrected Windows text/image raster readback so the mask contains completed pixels.
- Redesigned the interface and retained all existing parameter IDs and ordering.
- Added an offline CMake build, regression tests, independent host checks, and clearer documentation.

## Limits and deferred features

**Not verified:** commercial DAW sessions, host-specific automation and offline export, physical file-picker interaction, HiDPI behavior across hosts, long-session CPU profiling, older Windows versions, clean-machine installation, macOS/AU, Linux, Windows ARM64, Apple Silicon, and 32-bit Windows. No pluginval or Steinberg validator certification is claimed.

**Not implemented in this build:** host-tempo synchronization, selectable analysis windows, font styling controls, extensive mask transformations, a factory preset bank, A/B slots, automatic loudness compensation, reference-spectrum matching, dedicated offline-quality modes, or oversampled true-peak metering/limiting.

The preview is not an output spectrogram. Adaptive and protection controls are practical signal-dependent measures, not a validated perceptual model. Emboss does not guarantee equal loudness. Noise-fill can be audible. Musical transparency, survival through codecs, and watermark recovery require separate evaluation and are not promised by the test results.

The broader ideas in [PRODUCT-NOTES.md](source/Spectral%20Signature/Spectral%20Carver/docs/PRODUCT-NOTES.md) describe intended or future product scope, not an additional list of delivered features.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| The host does not find the plugin | Use a 64-bit Windows VST3 host; extract the ZIP; keep the complete outer `.vst3` folder intact; check the host's supported scan location and rescan. |
| Processing sounds unchanged | Confirm Bypass is off and Listen is Normal. Check Wet mix, Intensity, frequency bounds, and whether the scan is passing through a blank mask region. Protection and transparency settings can make the change subtle. |
| An audition mode is silent | Removed/Added depend on the selected processing mode and material. Wet mix also controls audition level. Noise-fill does not generate sound from digital silence. |
| The mask is visible but not obvious in an output spectrogram | The preview shows the source mask. Source energy, mapping, depth, protection settings, and analyzer resolution all affect the rendered pattern. |
| Monitoring feels delayed | The delay is fixed at 16,384 samples. Enable host compensation for mixing/rendering; use a workflow suited to mastering rather than live monitoring. |
| Image import fails | Use a valid PNG, JPEG, or GIF within the size/dimension limits. A failed decode retains the previous valid mask. |
| New text is not active | Click Apply text after editing. It also switches the source from image to text. |
| CMake cannot find JUCE or rejects its version | Point `JUCE_SOURCE_DIR` to the local 8.0.12 directory containing `CMakeLists.txt`. Keep the packaged modules and build helpers together. |
| The host-check build cannot find JUCE | Pass the explicit `JUCE_PATH` shown in the host-check command. |
| CPU use rises with quality settings | Reduce FFT size or overlap and compare the result. Higher settings cost more processing time; they do not lower the fixed latency. |

For a reproducible issue, record the plugin version, Windows version, host/version, sample rate, block size, bus layout, settings, and steps to reproduce. Include relevant test output and a minimal non-sensitive example where possible. No issue-tracker or repository URL is assumed here.

## Co-authors

- **Maksym Lazirko**
- **Henmoro™**

Both are credited as co-authors of Spectral Signature. Existing source attribution and third-party notices are retained.
