# Spectral Signature 1.0.1 — engineering and validation

The Windows x64 VST3 release restores a working spectral path, safe mask
publication, self-contained image recall, stable host timing and a redesigned
editor. The original manufacturer/plugin identity (`Manu` / `Dc8f`), bundle ID,
30 parameter IDs and parameter order are retained. New instances default to a
20% wet mix; existing saved parameter values remain authoritative.

## Processing design

The processor snapshots cached atomic parameter values once per block. Audio
passes through smoothed input gain, a stereo short-time Fourier transform,
spectral processing, overlap-add synthesis, wet/dry mixing, smoothed output gain
and optional sample-peak safety. Bypass fades to the original delayed input and
is unaffected by input gain, output gain or the safety stage. Mono and stereo
layouts are supported; double-precision processing is not advertised.

The engine uses preallocated radix-2 FFTs for 1024, 2048, 4096, 8192 and 16384
samples, with 4x, 8x and 16x overlap. Bit-reversal and twiddle tables are created
before processing. Unlike the pinned JUCE fallback FFT, these transforms do not
take a per-transform spin lock. Their analytic DC, Nyquist, phase and inverse
scaling are tested. Periodic Hann analysis/synthesis windows use the squared
overlap normalization `8 / (3 * overlap)`; inverse FFT normalization is applied
exactly once.

Latency is fixed at **16384 samples**. Shorter FFT modes delay their synthesized
output to the same timeline as the dry path. Changing quality or overlap fades
to delayed dry, resets/refills the engine and fades back. This avoids changing
host delay compensation in the audio callback. The tradeoff is mastering-scale
latency even at smaller FFT settings:

| Sample rate | Reported delay |
| --- | ---: |
| 44.1 kHz | 371.52 ms |
| 48 kHz | 341.33 ms |
| 88.2 kHz | 185.76 ms |
| 96 kHz | 170.67 ms |
| 192 kHz | 85.33 ms |

Cut and Emboss multiply complex bins by positive real gains, preserving their
phase. Cut attenuates the mask; Emboss applies bounded local edge contrast.
Noise-Fill adds a conservative carrier proportional to existing bin energy and
produces no carrier during digital silence. Linked noise uses the same random
phase on both channels. Mid/Side placement and audition are supported; the
dual-mono option does not constitute a fully separate per-channel adaptive
analysis model.

Mask audition solos the mask-weighted original spectrum. Removed, Added and
Delta isolate their corresponding spectral contributions, scaled by Wet Mix,
without leaking dry audio. Mid and Side audition solo processed channel
components. Emboss is not a measured loudness- or energy-compensated process.

The safety stage is a bounded **sample-peak soft knee** after output gain, with
a -0.5 dBFS ceiling while active. It is not an oversampled true-peak limiter.
The editor's peak hold is also a sample peak. Non-finite audio becomes zero;
abnormally large finite input is bounded to ±64. Invalid parameter values use
bounded defaults so an isolated bad value cannot poison the FFT history.

## Masks, threads and state

One worker coalesces requests into a single latest pending mask. Text is limited
to 2048 characters; PNG, JPEG and GIF inputs to 8 MiB and 4096 × 4096 pixels.
File reading is bounded even if a file grows between its size check and read.
Image decoding, resizing, text layout and pixel conversion stay outside the
audio callback. Text and image rasterization explicitly use CPU image storage,
avoiding unfinished native Windows drawing when pixels are sampled.

Three fixed mask buffers use atomic reader counts and exclusive writer claims.
A frame holds one read view while processing; a buffer cannot be reused while
that view exists. Reader acquisition has a bounded retry count. Audio processing
does not wait for the worker, copy image data, allocate/free FFT storage, access
files, post GUI updates or take request/state locks.

Version-2 state stores standard Base64 image bytes rather than an external image
path. Serialization uses the last successfully rendered source. If a queued
decode is unfinished, saving may wait up to five seconds outside the processing
callback; on failure or timeout it retains the previous valid source. A
valid-header/corrupt-body PNG is specifically tested to ensure that a failed
import cannot replace the audible mask in the next saved session. Empty text,
Unicode text and switching from image back to text are also retained correctly.
Legacy local image paths can be imported once when available; missing legacy
images fall back to their stored text. State input is capped at 12 MiB and
embedded image decoding is size-bounded. Text/image edits notify the host that
non-parameter state has changed.

## Verified results

The native regression executable completed **232 / 232 checks with zero
failures**. Evidence: `processor-tests-final.log` and `Tests/ProcessorTests.cpp`.

| Check | Result |
| --- | --- |
| Five sample rates × five FFT sizes × three overlaps | 75 unity reconstruction cases passed |
| Variable blocks | 1, 3, 17, 64, 128, 257, 511 and 1024 samples |
| Worst unity reconstruction error | 7.45058 × 10⁻⁸ absolute sample amplitude |
| Quality/overlap automation at unity | Worst error 4.47035 × 10⁻⁸ |
| Full white-mask Cut, requested -12 dB | Measured amplitude gain 0.251189 |
| Mask / Removed / Added / Delta audition gains | 1 / 0.748811 / 0 / -0.748811 |
| Delta at 20% wet | Correct scaled contribution with no dry leakage |
| C++ heap operations in monitored callbacks | 0 `new`, 0 `delete` |
| Bypass with non-unity gains and safety enabled | Exact delayed original input |
| Silence, non-finite input, sample ceiling, mono | Passed |
| Image recall after deleting original file | Passed |
| Corrupt image import then save/reopen | Prior valid image retained |
| 1000 rapid mask requests with concurrent reads | Latest request published; pinned mask unchanged |
| UI inventory and bounds | 18 sliders, 6 selectors, 5 toggles plus reset; controls contained |
| UI actions | Slider/parameter updates in both directions, process selector, bypass, Apply Text and Restart Cycle passed |

The fresh native editor screenshot shows the actual rendered text mask and the
complete interface without clipped or overlapping controls. The UI test invokes
the real button callbacks through queued clicks and verifies parameter state;
it is not a pixel-only mockup.

An independent JUCE VST3 host loaded the actual module and passed scanning,
instantiation, five sample rates, variable/zero-sized blocks, dry and bypass
nulls, three finite-output modes, a strong Cut effect and state close/reopen.
Its strongest Cut render differed measurably from dry (maximum difference
0.0748813); dry and bypass maximum error was zero. See
`host-validation-final.log` in the validation evidence.

## Build and practical limits

The build uses MSVC 19.44 / Visual Studio 2022 Build Tools, Windows SDK
10.0.26100.0, CMake 3.31.6 and official JUCE 8.0.12 at commit
`29396c22c93392d6738e021b83196283d6e4d850`. See `BUILD.md` for reproduction.
The CMake route is tested; the updated Projucer project is provided as source
convenience and was not independently exported with Projucer.

The x64 module uses the static MSVC runtime. Import inspection found 21 Windows
system DLLs and no separate VC runtime DLL requirement. The binary has ASLR,
high-entropy addressing, NX and control-flow protection. These checks do not
replace code signing, a clean-machine install test or platform certification.

The allocation counter instruments C++ `new`/`delete` on the processing thread,
not every operating-system or C allocation API. Source review complements it;
there is no claim of universal hard real-time scheduling or zero system
allocation. The suite is deterministic functional coverage, not a proof that
every DAW, device, image decoder path or automation combination works.

macOS/AU, Linux, ARM64, older Windows versions, physical file-picker interaction,
commercial DAW compatibility, long-session CPU profiling, codec survival and
listening-based transparency remain unverified. The source mask preview is not
a measured output spectrogram. Host-tempo synchronization, reference-spectrum
matching, true-peak metering/limiting, loudness compensation and the broader
guideline feature wishlist remain future work. JUCE's applicable license still
governs distribution; no new external FFT dependency was introduced.
