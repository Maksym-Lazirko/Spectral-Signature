# Build Spectral Signature 1.0.1

The supported build is 64-bit Windows VST3 using CMake and Visual Studio 2022
Build Tools (Desktop development with C++, MSVC v143 and a Windows 10/11 SDK).
C++20 is required. No installer, global PATH change, or plugin installation is
performed by this project.

Use a local checkout of official JUCE **8.0.12**, commit
`29396c22c93392d6738e021b83196283d6e4d850`. CMake deliberately does not download
dependencies. JUCE licensing still applies; review the license included with that
checkout before distributing a derivative product.

From this source directory in a developer PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_SOURCE_DIR="C:/path/to/JUCE"
cmake --build build --config Release --target SpectralSignature_VST3 SpectralTests --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The plugin bundle is written to
`build/SpectralSignature_artefacts/Release/VST3/Spectral Signature.vst3`.
Copy the complete `.vst3` directory when installing it in a host's VST3 location.
The tests run from `build/SpectralTests_artefacts/Release/SpectralTests.exe`.

The CMake Windows build uses the static MSVC runtime to avoid requiring an extra
VC++ Redistributable installation. It retains standard Windows system-library
dependencies. No auxiliary DLL or external FFT library is added. The radix-2 FFT
tables and processing buffers are prepared before audio starts.

The original Projucer identity is retained: manufacturer `Manu`, plugin `Dc8f`,
bundle `com.yourcompany.SpectralCarver`, and all existing parameter IDs and order.
The `.jucer` file also includes the previously omitted spectral source files and
a VS2022 exporter. Its module path is a workspace convenience; update it to your
local JUCE checkout if using Projucer. CMake is the tested build route. Generated
VS2026 projects and old precompiled outputs from the original archive are not a
release source of truth.

The native regression executable covers all five FFT sizes, three overlaps and
five sample rates, mono/stereo reconstruction, variable blocks, clean bypass,
quality automation, bounded sample-peak safety, malformed/non-finite input,
Unicode and blank text recall, embedded image recall, and concurrent mask
publication. Its heap check counts C++ `new`/`delete` on the processing thread;
it is complemented by source review of the callback path, rather than presented
as a general OS allocation or real-time scheduling proof.

The native suite also checks editor bindings and control bounds. Loading the
compiled VST3 is verified separately by the included host check.
macOS/AU, Linux, ARM64, legacy Windows versions, and clean-machine installation
are not validated by the Windows x64 build.
