# Spectral Signature

A professional VST plugin for spectrum analysis and audio mastering built with JUCE framework.

## Overview

**Spectral Signature** is a powerful VST plugin designed for audio engineers and producers who need advanced spectral analysis capabilities for mixing and mastering. The plugin provides real-time visualization and analysis of audio frequency content, helping you make informed decisions about your mix.

## Features

- 🎚️ Real-time spectral analysis
- 📊 Interactive frequency visualization
- 🎵 Professional audio processing
- 🔌 VST plugin format support
- ⚡ Low-latency operation

## Technology Stack

- **C++** (60.6%) - Core audio processing engine
- **CMake** (38.8%) - Build system configuration
- **JUCE Framework** - Cross-platform audio plugin development framework

## Building from Source

### Requirements

- CMake 3.22 or later
- C++17 compatible compiler
- JUCE 8.0.12 framework
- Platform SDK for your target OS

### Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/Maksym-Lazirko/Spectral-Signature.git
cd Spectral-Signature
```

2. Create a build directory:
```bash
mkdir build
cd build
```

3. Configure with CMake:
```bash
cmake ..
```

4. Build the plugin:
```bash
cmake --build . --config Release
```

### Compiled Binaries

Pre-built VST plugin binaries are available in the [`extras/Build`](extras/Build) directory after compilation.

## Project Structure

```
Spectral-Signature/
├── CMakeLists.txt              # Main build configuration
├── Plugin/                      # Plugin source code and implementation
├── source/                      # Core audio processing source files
├── extras/
│   └── Build/                   # Build utilities and compiled binaries
├── LICENSE                      # MIT License
└── README.md                    # This file
```

## Installation

### On macOS
Copy the compiled `.vst3` or `.vst` plugin to:
```
~/Library/Audio/Plug-Ins/VST3/
```

### On Windows
Copy the compiled plugin to:
```
C:\Program Files\Common Files\VST3\
```

### On Linux
Copy the compiled plugin to:
```
~/.vst/
```

## Usage

1. Open your DAW (Digital Audio Workstation)
2. Scan for new plugins
3. Insert **Spectral Signature** on an audio track or bus
4. Use the frequency visualization to analyze your audio
5. Adjust parameters as needed for your mix

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### JUCE Framework License

This project uses the JUCE framework which is subject to the JUCE End User Licence Agreement. Additional licensing options are available:
- Open source licensing under AGPLv3
- Commercial licensing available at https://juce.com

For more information, see:
- [JUCE End User Licence Agreement](https://juce.com/legal/juce-8-licence/)
- [JUCE Privacy Policy](https://juce.com/juce-privacy-policy)
- [JUCE Website Terms of Service](https://juce.com/juce-website-terms-of-service/)

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Author

**Maksym Lazirko**
- GitHub: [@Maksym-Lazirko](https://github.com/Maksym-Lazirko)

## Support

For issues, questions, or feature requests, please open an [issue](https://github.com/Maksym-Lazirko/Spectral-Signature/issues) on GitHub.

---

**Note:** This project is actively maintained. Check back regularly for updates and improvements!
