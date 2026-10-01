/*
  Spectral Signature -- real-time spectral engraving core.

  The engine deliberately keeps all audio-thread storage fixed. Text and image
  masks are rendered by a one-thread worker and published as immutable buffers.
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <utility>
#include "RealtimeFFT.h"

class SpectralMaskBank : private juce::Thread
{
    struct Mask;
public:
    static constexpr int timeResolution = 256;
    static constexpr int frequencyResolution = 512;

    SpectralMaskBank();
    ~SpectralMaskBank();

    static constexpr int maximumTextLength = 2048;
    static constexpr size_t maximumImageBytes = 8 * 1024 * 1024;
    class ReadView
    {
    public:
        ReadView() = default;
        ReadView (ReadView&& other) noexcept : mask (std::exchange (other.mask, nullptr)) {}
        ~ReadView();
        float sample (float time, float frequency) const noexcept;
        explicit operator bool() const noexcept { return mask != nullptr; }
    private:
        friend class SpectralMaskBank;
        explicit ReadView (Mask* m) noexcept : mask (m) {}
        Mask* mask = nullptr;
        ReadView (const ReadView&) = delete;
        ReadView& operator= (const ReadView&) = delete;
    };

    void requestTextMask (const juce::String& text);
    bool requestImageMask (const juce::MemoryBlock& encodedImage);
    static bool isSupportedImage (const juce::MemoryBlock&) noexcept;
    ReadView acquireReadView() const noexcept;
    void getStateSource (juce::String& text, juce::MemoryBlock& image);
    float sample (float normalisedTime, float normalisedFrequency) const noexcept;
    juce::String getSourceDescription() const;

private:
    struct Mask
    {
        std::array<float, timeResolution * frequencyResolution> pixels {};
        std::atomic<int> readers { 0 }; // -1 while the worker owns the buffer
    };

    void run() override;
    void renderText (Mask&, const juce::String&);
    bool renderImage (Mask&, const juce::MemoryBlock&);
    Mask* acquireWritableMask() noexcept;
    void publish (Mask*, const juce::String&, const juce::String&, juce::MemoryBlock&);

    std::array<std::unique_ptr<Mask>, 3> masks;
    std::atomic<Mask*> activeMask { nullptr };
    juce::CriticalSection requestLock;
    juce::String pendingText;
    juce::MemoryBlock pendingImage;
    std::atomic<uint64_t> requestGeneration { 0 };
    std::atomic<uint64_t> completedGeneration { 0 };
    juce::WaitableEvent renderCompleted;
    bool pending = false;
    mutable juce::CriticalSection sourceLock;
    juce::String sourceDescription { "Default text mask" };
    juce::String currentText { "SPECTRAL\nSIGNATURE" };
    juce::MemoryBlock currentImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralMaskBank)
};

struct SpectralSettings
{
    int mode = 0;                 // 0 Cut, 1 Emboss, 2 Noise-Fill
    int stereoMode = 0;           // 0 linked, 1 mid, 2 side, 3 dual mono
    int quality = 1;              // 1024, 2048, 4096, 8192, 16384
    int overlap = 0;              // 4x, 8x, 16x
    int frequencyMapping = 1;     // linear, log, mel-like
    int audition = 0;             // normal, mask, removed, added, delta, mid, side
    float sampleRate = 44100.0f;
    float intensity = 0.35f;
    float detail = 0.60f;
    float transparency = 0.75f;
    float adaptive = 0.60f;
    float loopSeconds = 8.0f;
    float lowerFrequency = 160.0f;
    float upperFrequency = 16000.0f;
    float frequencySmoothing = 0.40f;
    float timeSmoothing = 0.45f;
    float maskContrast = 0.50f;
    float threshold = 0.08f;
    float maximumCutDb = 3.0f;
    float maximumBoostDb = 1.0f;
    float transientProtection = 0.70f;
    float tonalProtection = 0.50f;
    bool invertMask = false;
    bool lowProtection = true;
    bool highProtection = false;
    bool safeMode = true;
};

class SpectralEngine
{
public:
    static constexpr int minimumFftOrder = 10;
    static constexpr int maximumFftOrder = 14;
    static constexpr int maximumFftSize = 1 << maximumFftOrder;

    explicit SpectralEngine (SpectralMaskBank&);
    void prepare (double sampleRate);
    void configureFor (const SpectralSettings& settings) noexcept;
    void reset() noexcept;
    void resetCycle() noexcept;
    void processSample (float inputLeft, float inputRight, const SpectralSettings&, float& outputLeft, float& outputRight) noexcept;
    // A stable delay keeps host PDC and dry/wet alignment valid during automation.
    int getLatencySamples() const noexcept                         { return maximumFftSize; }
    int getFftSize() const noexcept                                { return fftSize; }
    float getCyclePosition() const noexcept                        { return cyclePosition; }
    uint64_t getConfigurationGeneration() const noexcept           { return configurationGeneration; }

private:
    static constexpr int maximumBins = maximumFftSize / 2 + 1;
    static constexpr int outputRingSize = maximumFftSize * 2;

    void configure (const SpectralSettings&) noexcept;
    void processFrame (const SpectralSettings&) noexcept;
    void rebuildFrequencyMap (const SpectralSettings&) noexcept;
    float transformedMask (int bin, const SpectralSettings&, const SpectralMaskBank::ReadView&) noexcept;
    float randomBipolar (uint32_t&) noexcept;
    void applyEngraving (float& real, float& imaginary, float magnitude, float engraving,
                         float localContrast, const SpectralSettings&, uint32_t&) noexcept;

    SpectralMaskBank& maskBank;
    std::array<std::unique_ptr<RealtimeFFT>, maximumFftOrder - minimumFftOrder + 1> ffts;
    std::array<std::array<float, maximumFftSize>, maximumFftOrder - minimumFftOrder + 1> analysisWindows {};
    std::array<float, maximumFftSize> inputLeft {};
    std::array<float, maximumFftSize> inputRight {};
    std::array<float, outputRingSize> outputRingLeft {};
    std::array<float, outputRingSize> outputRingRight {};
    std::array<float, maximumFftSize * 2> fftLeft {};
    std::array<float, maximumFftSize * 2> fftRight {};
    std::array<float, maximumBins> targetMask {};
    std::array<float, maximumBins> smoothedMask {};
    float synthesisNormalisation = 2.0f / 3.0f;
    std::array<float, maximumBins> frequencyPositions {};
    float mapLowerHz = -1.0f, mapUpperHz = -1.0f;
    int mapMode = -1, mapSampleRate = 0;

    double currentSampleRate = 44100.0;
    int fftOrder = 11;
    int fftSize = 2048;
    int hopSize = 512;
    int inputPosition = 0;
    int outputPosition = 0;
    int samplesSinceFrame = 0;
    int validInputSamples = 0;
    float cyclePosition = 0.0f;
    float previousFrameEnergy = 0.0f;
    uint64_t configurationGeneration = 0;
    uint32_t randomLeft = 0x9e3779b9u;
    uint32_t randomRight = 0x243f6a88u;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEngine)
};
