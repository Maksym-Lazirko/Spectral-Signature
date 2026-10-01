/*
  Spectral Signature audio processor.
  Parameters are held in an APVTS; all spectral buffers are preallocated by
  SpectralEngine before the audio callback starts.
*/

#pragma once

#include <JuceHeader.h>
#include "SpectralEngine.h"

namespace ParamIds
{
    static constexpr auto* mode               = "mode";
    static constexpr auto* stereoMode         = "stereoMode";
    static constexpr auto* quality            = "quality";
    static constexpr auto* overlap            = "overlap";
    static constexpr auto* frequencyMapping   = "frequencyMapping";
    static constexpr auto* audition           = "audition";
    static constexpr auto* intensity          = "intensity";
    static constexpr auto* detail             = "detail";
    static constexpr auto* transparency       = "transparency";
    static constexpr auto* adaptive           = "adaptive";
    static constexpr auto* loopSeconds        = "loopSeconds";
    static constexpr auto* lowerFrequency     = "lowerFrequency";
    static constexpr auto* upperFrequency     = "upperFrequency";
    static constexpr auto* frequencySmoothing = "frequencySmoothing";
    static constexpr auto* timeSmoothing      = "timeSmoothing";
    static constexpr auto* maskContrast       = "maskContrast";
    static constexpr auto* threshold          = "threshold";
    static constexpr auto* maximumCutDb       = "maximumCutDb";
    static constexpr auto* maximumBoostDb     = "maximumBoostDb";
    static constexpr auto* transientProtection= "transientProtection";
    static constexpr auto* tonalProtection    = "tonalProtection";
    static constexpr auto* invertMask         = "invertMask";
    static constexpr auto* lowProtection      = "lowProtection";
    static constexpr auto* highProtection     = "highProtection";
    static constexpr auto* safeMode           = "safeMode";
    static constexpr auto* inputGainDb        = "inputGainDb";
    static constexpr auto* outputGainDb       = "outputGainDb";
    static constexpr auto* mix                = "mix";
    static constexpr auto* bypass             = "bypass";
    static constexpr auto* resetCycle         = "resetCycle";
    enum Index { modeIndex, stereoModeIndex, qualityIndex, overlapIndex, frequencyMappingIndex, auditionIndex, intensityIndex, detailIndex, transparencyIndex, adaptiveIndex, loopSecondsIndex, lowerFrequencyIndex, upperFrequencyIndex, frequencySmoothingIndex, timeSmoothingIndex, maskContrastIndex, thresholdIndex, maximumCutDbIndex, maximumBoostDbIndex, transientProtectionIndex, tonalProtectionIndex, invertMaskIndex, lowProtectionIndex, highProtectionIndex, safeModeIndex, inputGainDbIndex, outputGainDbIndex, mixIndex, bypassIndex, resetCycleIndex, count };
    static constexpr std::array<const char*, count> all { mode, stereoMode, quality, overlap, frequencyMapping, audition, intensity, detail, transparency, adaptive, loopSeconds, lowerFrequency, upperFrequency, frequencySmoothing, timeSmoothing, maskContrast, threshold, maximumCutDb, maximumBoostDb, transientProtection, tonalProtection, invertMask, lowProtection, highProtection, safeMode, inputGainDb, outputGainDb, mix, bypass, resetCycle };
}

class SpectralCarverAudioProcessor : public juce::AudioProcessor
{
public:
    SpectralCarverAudioProcessor();
    ~SpectralCarverAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() noexcept             { return parameters; }
    const juce::AudioProcessorValueTreeState& getParameters() const noexcept { return parameters; }
    void setTextMask (const juce::String& text);
    bool loadImageMask (const juce::File& imageFile);
    juce::String getMaskText() const;
    juce::String getMaskSourceDescription() const;
    float getInputMeter() const noexcept                                     { return inputMeter.load(); }
    float getOutputMeter() const noexcept                                    { return outputMeter.load(); }
    float getDeltaMeter() const noexcept                                     { return deltaMeter.load(); }
    float getTruePeakMeter() const noexcept                                  { return truePeakMeter.load(); }
    float getMaskValue (float time, float frequency) const noexcept;
    float getEngravingCyclePosition() const noexcept { return engravingCyclePosition.load (std::memory_order_relaxed); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    SpectralSettings readSettings() const noexcept;
    float parameterValue (ParamIds::Index) const noexcept;
    void processAudio (juce::AudioBuffer<float>&, juce::MidiBuffer&, bool hostBypass);
    struct CachedParameter
    {
        std::atomic<float>* value = nullptr;
        float minimum = 0.0f, maximum = 1.0f, defaultValue = 0.0f;
    };
    std::array<CachedParameter, ParamIds::count> cachedParameters {};
    void updateMeter (std::atomic<float>& meter, float peak) noexcept;
    float applySafetyLimiter (float sample, float ceiling) const noexcept;

    juce::AudioProcessorValueTreeState parameters;
    SpectralMaskBank maskBank;
    SpectralEngine spectralEngine { maskBank };
    std::array<float, SpectralEngine::maximumFftSize> dryDelayLeft {};
    std::array<float, SpectralEngine::maximumFftSize> dryDelayRight {};
    std::array<float, SpectralEngine::maximumFftSize> gainedDryDelayLeft {}, gainedDryDelayRight {};
    int dryWritePosition = 0;
    int activeQuality = 1, activeOverlap = 0, qualityRefillSamples = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> qualityBlend;
    std::atomic<bool> stateResetRequested { false };
    std::atomic<float> engravingCyclePosition { 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> wetMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> bypassMix;
    std::atomic<float> inputMeter { 0.0f }, outputMeter { 0.0f }, deltaMeter { 0.0f }, truePeakMeter { 0.0f };
    mutable juce::CriticalSection maskStateLock;
    juce::String maskText { "SPECTRAL\nSIGNATURE" };
    bool previousReset = false;
    float blockInputPeak = 0.0f, blockOutputPeak = 0.0f, blockDeltaPeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralCarverAudioProcessor)
};
