/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float meterDecay = 0.92f;
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralCarverAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto choice = [] (const char* id, const char* name, const juce::StringArray& items, int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID (id, 1), name, items, def);
    };
    auto norm = [] (const char* id, const char* name, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (id, 1), name,
                                                            juce::NormalisableRange<float> (0.0f, 1.0f), def);
    };
    auto toggle = [] (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID (id, 1), name, def);
    };

    params.push_back (choice (ParamIds::mode,           "Mode",              { "Cut", "Emboss", "Noise-Fill" }, 0));
    params.push_back (choice (ParamIds::stereoMode,     "Stereo Mode",       { "Linked", "Mid", "Side", "Dual Mono" }, 0));
    params.push_back (choice (ParamIds::quality,        "Quality (FFT)",     { "1024", "2048", "4096", "8192", "16384" }, 1));
    params.push_back (choice (ParamIds::overlap,        "Overlap",           { "4x", "8x", "16x" }, 0));
    params.push_back (choice (ParamIds::frequencyMapping,"Frequency Mapping",{ "Linear", "Log", "Mel-like" }, 1));
    params.push_back (choice (ParamIds::audition,       "Audition",          { "Normal", "Mask", "Removed", "Added", "Delta", "Mid", "Side" }, 0));

    params.push_back (norm (ParamIds::intensity,          "Intensity",           0.35f));
    params.push_back (norm (ParamIds::detail,             "Detail",              0.60f));
    params.push_back (norm (ParamIds::transparency,       "Transparency",        0.75f));
    params.push_back (norm (ParamIds::adaptive,           "Adaptive",            0.60f));
    params.push_back (norm (ParamIds::frequencySmoothing, "Frequency Smoothing", 0.40f));
    params.push_back (norm (ParamIds::timeSmoothing,      "Time Smoothing",      0.45f));
    params.push_back (norm (ParamIds::maskContrast,       "Mask Contrast",       0.50f));
    params.push_back (norm (ParamIds::threshold,          "Threshold",           0.08f));
    params.push_back (norm (ParamIds::transientProtection,"Transient Protection",0.70f));
    params.push_back (norm (ParamIds::tonalProtection,    "Tonal Protection",    0.50f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ParamIds::loopSeconds, 1), "Loop Seconds",
                                                                   juce::NormalisableRange<float> (0.25f, 60.0f, 0.01f, 0.5f), 8.0f,
                                                                   juce::AudioParameterFloatAttributes().withLabel ("s")));

    auto freqSkew = [] (const char* id, const char* name, float min, float max, float def)
    {
        auto range = juce::NormalisableRange<float> (min, max, 0.01f, 0.35f);
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (id, 1), name, range, def,
                                                            juce::AudioParameterFloatAttributes().withLabel ("Hz"));
    };
    params.push_back (freqSkew (ParamIds::lowerFrequency, "Lower Frequency", 10.0f, 20000.0f, 160.0f));
    params.push_back (freqSkew (ParamIds::upperFrequency, "Upper Frequency", 100.0f, 24000.0f, 16000.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ParamIds::maximumCutDb, 1), "Maximum Cut",
                                                                   juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 3.0f,
                                                                   juce::AudioParameterFloatAttributes().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ParamIds::maximumBoostDb, 1), "Maximum Boost",
                                                                   juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), 1.0f,
                                                                   juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (toggle (ParamIds::invertMask,     "Invert Mask",     false));
    params.push_back (toggle (ParamIds::lowProtection,  "Low Protection",  true));
    params.push_back (toggle (ParamIds::highProtection, "High Protection", false));
    params.push_back (toggle (ParamIds::safeMode,       "Safe Mode",       true));
    params.push_back (toggle (ParamIds::bypass,         "Bypass",          false));
    params.push_back (toggle (ParamIds::resetCycle,     "Reset Cycle",     false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ParamIds::inputGainDb, 1), "Input Gain",
                                                                   juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                                                   juce::AudioParameterFloatAttributes().withLabel ("dB")));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID (ParamIds::outputGainDb, 1), "Output Gain",
                                                                   juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
                                                                   juce::AudioParameterFloatAttributes().withLabel ("dB")));
    params.push_back (norm (ParamIds::mix, "Wet Mix", 0.20f));

    return { params.begin(), params.end() };
}

//==============================================================================
SpectralCarverAudioProcessor::SpectralCarverAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    static_assert (std::atomic<float>::is_always_lock_free);
    for (size_t i = 0; i < cachedParameters.size(); ++i)
    {
        auto* parameter = parameters.getParameter (ParamIds::all[i]);
        const auto& range = parameter->getNormalisableRange();
        cachedParameters[i] = { parameters.getRawParameterValue (ParamIds::all[i]), range.start, range.end,
                                parameter->convertFrom0to1 (parameter->getDefaultValue()) };
    }
    inputGain .reset (256);
    outputGain.reset (256);
    wetMix    .reset (256);
    bypassMix .reset (256);
}

SpectralCarverAudioProcessor::~SpectralCarverAudioProcessor()
{
}

//==============================================================================
const juce::String SpectralCarverAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpectralCarverAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectralCarverAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectralCarverAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectralCarverAudioProcessor::getTailLengthSeconds() const
{
    const auto rate = getSampleRate();
    return (double) SpectralEngine::maximumFftSize / (rate > 1000.0 ? rate : 44100.0);
}

int SpectralCarverAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectralCarverAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpectralCarverAudioProcessor::setCurrentProgram (int)
{
}

const juce::String SpectralCarverAudioProcessor::getProgramName (int)
{
    return {};
}

void SpectralCarverAudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void SpectralCarverAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto rate = std::isfinite (sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
    for (auto* smooth : { &inputGain, &outputGain, &wetMix, &bypassMix, &qualityBlend })
        smooth->reset (rate, 0.02);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (parameterValue (ParamIds::inputGainDbIndex)));
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (parameterValue (ParamIds::outputGainDbIndex)));
    wetMix.setCurrentAndTargetValue (parameterValue (ParamIds::mixIndex));
    bypassMix.setCurrentAndTargetValue (parameterValue (ParamIds::bypassIndex) > 0.5f ? 0.0f : 1.0f);
    qualityBlend.setCurrentAndTargetValue (1.0f);
    auto settings = readSettings();
    activeQuality = settings.quality;
    activeOverlap = settings.overlap;
    spectralEngine.prepare (rate);
    spectralEngine.configureFor (settings);
    setLatencySamples (spectralEngine.getLatencySamples());
    dryDelayLeft.fill (0.0f);
    dryDelayRight.fill (0.0f);
    gainedDryDelayLeft.fill (0.0f);
    gainedDryDelayRight.fill (0.0f);
    dryWritePosition = qualityRefillSamples = 0;
    previousReset = false;
    stateResetRequested.store (false, std::memory_order_relaxed);
    blockInputPeak = blockOutputPeak = blockDeltaPeak = 0.0f;
    inputMeter.store (0.0f);
    outputMeter.store (0.0f);
    deltaMeter.store (0.0f);
    truePeakMeter.store (0.0f);
    juce::ignoreUnused (samplesPerBlock);
}

void SpectralCarverAudioProcessor::releaseResources()
{
    spectralEngine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralCarverAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

SpectralSettings SpectralCarverAudioProcessor::readSettings() const noexcept
{
    SpectralSettings s;

    s.mode                = juce::jlimit (0, 2, (int) parameterValue (ParamIds::modeIndex));
    s.stereoMode          = juce::jlimit (0, 3, (int) parameterValue (ParamIds::stereoModeIndex));
    s.quality             = juce::jlimit (0, 4, (int) parameterValue (ParamIds::qualityIndex));
    s.overlap             = juce::jlimit (0, 2, (int) parameterValue (ParamIds::overlapIndex));
    s.frequencyMapping    = juce::jlimit (0, 2, (int) parameterValue (ParamIds::frequencyMappingIndex));
    s.audition            = juce::jlimit (0, 6, (int) parameterValue (ParamIds::auditionIndex));
    s.sampleRate          = (float) getSampleRate();
    if (! std::isfinite (s.sampleRate) || s.sampleRate < 1000.0f)
        s.sampleRate = 44100.0f;
    s.intensity           = parameterValue (ParamIds::intensityIndex);
    s.detail              = parameterValue (ParamIds::detailIndex);
    s.transparency        = parameterValue (ParamIds::transparencyIndex);
    s.adaptive            = parameterValue (ParamIds::adaptiveIndex);
    s.loopSeconds         = parameterValue (ParamIds::loopSecondsIndex);
    s.lowerFrequency      = parameterValue (ParamIds::lowerFrequencyIndex);
    s.upperFrequency      = parameterValue (ParamIds::upperFrequencyIndex);
    s.frequencySmoothing  = parameterValue (ParamIds::frequencySmoothingIndex);
    s.timeSmoothing       = parameterValue (ParamIds::timeSmoothingIndex);
    s.maskContrast        = parameterValue (ParamIds::maskContrastIndex);
    s.threshold           = parameterValue (ParamIds::thresholdIndex);
    s.maximumCutDb        = parameterValue (ParamIds::maximumCutDbIndex);
    s.maximumBoostDb      = parameterValue (ParamIds::maximumBoostDbIndex);
    s.transientProtection = parameterValue (ParamIds::transientProtectionIndex);
    s.tonalProtection     = parameterValue (ParamIds::tonalProtectionIndex);
    s.invertMask          = parameterValue (ParamIds::invertMaskIndex)     > 0.5f;
    s.lowProtection       = parameterValue (ParamIds::lowProtectionIndex)  > 0.5f;
    s.highProtection      = parameterValue (ParamIds::highProtectionIndex) > 0.5f;
    s.safeMode            = parameterValue (ParamIds::safeModeIndex)       > 0.5f;
    return s;
}

float SpectralCarverAudioProcessor::parameterValue (ParamIds::Index index) const noexcept
{
    const auto& p = cachedParameters[(size_t) index];
    const auto value = p.value->load (std::memory_order_relaxed);
    return std::isfinite (value) ? juce::jlimit (p.minimum, p.maximum, value) : p.defaultValue;
}

float SpectralCarverAudioProcessor::applySafetyLimiter (float sample, float ceiling) const noexcept
{
    // A bounded sample-peak soft knee, not an oversampled true-peak limiter.
    const auto knee = ceiling * 0.9f;
    const auto magnitude = std::abs (sample);
    if (magnitude <= knee)
        return sample;
    const auto limited = knee + (ceiling - knee) * std::tanh ((magnitude - knee) / (ceiling - knee));
    return std::copysign (limited, sample);
}

juce::AudioProcessorParameter* SpectralCarverAudioProcessor::getBypassParameter() const
{
    return parameters.getParameter (ParamIds::bypass);
}

void SpectralCarverAudioProcessor::updateMeter (std::atomic<float>& meter, float peak) noexcept
{
    auto previous = meter.load (std::memory_order_relaxed);
    const auto next = peak > previous ? peak : previous * meterDecay;
    meter.store (next, std::memory_order_relaxed);
}

void SpectralCarverAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    processAudio (buffer, midi, false);
}

void SpectralCarverAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    processAudio (buffer, midi, true);
}

void SpectralCarverAudioProcessor::processAudio (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, bool hostBypass)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midi);
    const auto channels = juce::jmin (buffer.getNumChannels(), getTotalNumInputChannels());
    const auto numSamples = buffer.getNumSamples();
    for (int channel = channels; channel < buffer.getNumChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);
    if (channels == 0 || numSamples == 0)
        return;

    auto settings = readSettings();
    const auto requestedQuality = settings.quality, requestedOverlap = settings.overlap;
    if (requestedQuality != activeQuality || requestedOverlap != activeOverlap)
        qualityBlend.setTargetValue (0.0f);
    else if (qualityRefillSamples == 0)
        qualityBlend.setTargetValue (1.0f);
    if (stateResetRequested.exchange (false, std::memory_order_acq_rel))
    {
        spectralEngine.reset();
        spectralEngine.resetCycle();
        qualityBlend.setCurrentAndTargetValue (0.0f);
        qualityRefillSamples = SpectralEngine::maximumFftSize + 1;
    }
    const auto resetRequested = parameterValue (ParamIds::resetCycleIndex) > 0.5f;
    if (resetRequested != previousReset)
        spectralEngine.resetCycle();
    previousReset = resetRequested;
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (parameterValue (ParamIds::inputGainDbIndex)));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (parameterValue (ParamIds::outputGainDbIndex)));
    wetMix.setTargetValue (parameterValue (ParamIds::mixIndex));
    bypassMix.setTargetValue (hostBypass || parameterValue (ParamIds::bypassIndex) > 0.5f ? 0.0f : 1.0f);
    const auto stereo = channels >= 2;
    auto* left = buffer.getWritePointer (0);
    auto* right = stereo ? buffer.getWritePointer (1) : left;
    constexpr auto ceiling = 0.944060876f;
    float inputPeak = blockInputPeak, outputPeak = blockOutputPeak, deltaPeak = blockDeltaPeak;
    auto heldPeak = truePeakMeter.load (std::memory_order_relaxed);
    const auto finiteSample = [] (float value)
    {
        return std::isfinite (value) ? juce::jlimit (-64.0f, 64.0f, value) : 0.0f;
    };
    for (int i = 0; i < numSamples; ++i)
    {
        if (! qualityBlend.isSmoothing() && qualityBlend.getCurrentValue() == 0.0f
            && (requestedQuality != activeQuality || requestedOverlap != activeOverlap))
        {
            spectralEngine.configureFor (settings);
            activeQuality = requestedQuality;
            activeOverlap = requestedOverlap;
            qualityRefillSamples = SpectralEngine::maximumFftSize + 1;
        }
        const auto rawL = finiteSample (left[i]), rawR = finiteSample (right[i]);
        const auto gainIn = inputGain.getNextValue();
        const auto inL = rawL * gainIn, inR = rawR * gainIn;
        float wetL = 0.0f, wetR = 0.0f;
        spectralEngine.processSample (inL, inR, settings, wetL, wetR);
        const auto position = (size_t) dryWritePosition;
        const auto dryL = dryDelayLeft[position], dryR = dryDelayRight[position];
        const auto gainedDryL = gainedDryDelayLeft[position], gainedDryR = gainedDryDelayRight[position];
        dryDelayLeft[position] = rawL;
        dryDelayRight[position] = rawR;
        gainedDryDelayLeft[position] = inL;
        gainedDryDelayRight[position] = inR;
        dryWritePosition = (dryWritePosition + 1) % SpectralEngine::maximumFftSize;
        if (qualityRefillSamples > 0 && --qualityRefillSamples == 0
            && requestedQuality == activeQuality && requestedOverlap == activeOverlap)
            qualityBlend.setTargetValue (1.0f);
        const auto mixAmount = wetMix.getNextValue();
        const auto qualityAmount = qualityBlend.getNextValue();
        const auto contributionAudition = settings.audition >= 2 && settings.audition <= 4;
        const auto wet = (settings.audition == 0 || contributionAudition ? mixAmount : 1.0f) * qualityAmount;
        const auto dry = settings.audition == 0 ? 1.0f - wet : 0.0f;
        const auto active = bypassMix.getNextValue();
        const auto gainOut = outputGain.getNextValue();
        auto outL = (wetL * wet + gainedDryL * dry) * gainOut;
        auto outR = (wetR * wet + gainedDryR * dry) * gainOut;
        if (settings.safeMode)
        {
            outL = applySafetyLimiter (outL, ceiling);
            outR = applySafetyLimiter (outR, ceiling);
        }
        outL = outL * active + dryL * (1.0f - active);
        outR = outR * active + dryR * (1.0f - active);
        left[i] = outL;
        if (stereo)
            right[i] = outR;
        inputPeak = juce::jmax (juce::jmax (std::abs (inL), std::abs (inR)), inputPeak * 0.9999f);
        outputPeak = juce::jmax (juce::jmax (std::abs (outL), std::abs (outR)), outputPeak * 0.9999f);
        heldPeak = juce::jmax (heldPeak, juce::jmax (std::abs (outL), std::abs (outR)));
        deltaPeak = juce::jmax (juce::jmax (std::abs (outL - dryL), std::abs (outR - dryR)), deltaPeak * 0.9999f);
    }
    truePeakMeter.store (heldPeak, std::memory_order_relaxed);
    engravingCyclePosition.store (spectralEngine.getCyclePosition(), std::memory_order_relaxed);
    blockInputPeak = inputPeak;
    blockOutputPeak = outputPeak;
    blockDeltaPeak = deltaPeak;
    updateMeter (inputMeter, inputPeak);
    updateMeter (outputMeter, outputPeak);
    updateMeter (deltaMeter, deltaPeak);
}

//==============================================================================
bool SpectralCarverAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SpectralCarverAudioProcessor::createEditor()
{
    return new SpectralCarverAudioProcessorEditor (*this);
}

//==============================================================================
void SpectralCarverAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    {
        const juce::ScopedLock lock (maskStateLock);
        juce::String renderedText;
        juce::MemoryBlock imageData;
        maskBank.getStateSource (renderedText, imageData);
        state.setProperty ("stateVersion", 2, nullptr);
        state.setProperty ("maskText", imageData.getSize() > 0 ? maskText : renderedText, nullptr);
        state.setProperty ("maskSource", imageData.getSize() > 0 ? "image" : "text", nullptr);
        state.setProperty ("maskImageData", juce::Base64::toBase64 (imageData.getData(), imageData.getSize()), nullptr);
        state.removeProperty ("imagePath", nullptr);
    }
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SpectralCarverAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    constexpr int maximumStateBytes = 12 * 1024 * 1024;
    if (data == nullptr || sizeInBytes <= 0 || sizeInBytes > maximumStateBytes)
        return;
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;
    const auto text = xml->getStringAttribute ("maskText", "SPECTRAL\nSIGNATURE").substring (0, SpectralMaskBank::maximumTextLength);
    juce::MemoryBlock restoredImage;
    const auto source = xml->getStringAttribute ("maskSource");
    if (source == "image")
    {
        const auto encoded = xml->getStringAttribute ("maskImageData");
        if (encoded.length() > (int) ((SpectralMaskBank::maximumImageBytes + 2) / 3 * 4))
            return;
        juce::MemoryOutputStream decoded (restoredImage, false);
        const auto decodedSuccessfully = juce::Base64::convertFromBase64 (decoded, encoded);
        decoded.flush();
        if (! decodedSuccessfully || ! SpectralMaskBank::isSupportedImage (restoredImage))
            return;
    }
    auto restored = juce::ValueTree::fromXml (*xml);
    if (! restored.isValid())
        return;
    parameters.replaceState (restored);
    {
        const juce::ScopedLock lock (maskStateLock);
        maskText = text;
        if (restoredImage.getSize() > 0)
            maskBank.requestImageMask (restoredImage);
        else
            maskBank.requestTextMask (maskText);
    }
    // Legacy v1 sessions stored a path. Import only a local existing image once;
    // subsequent saves are self-contained. Missing legacy images fall back to text.
    if (source.isEmpty())
    {
        const auto path = xml->getStringAttribute ("imagePath");
        if (path.isNotEmpty() && ! path.startsWith ("\\") && juce::File::isAbsolutePath (path))
            loadImageMask (juce::File (path));
    }
    stateResetRequested.store (true, std::memory_order_release);
}

void SpectralCarverAudioProcessor::setTextMask (const juce::String& text)
{
    {
        const juce::ScopedLock lock (maskStateLock);
        maskText = text.substring (0, SpectralMaskBank::maximumTextLength);
        maskBank.requestTextMask (maskText);
    }
    updateHostDisplay (ChangeDetails().withNonParameterStateChanged (true));
}

bool SpectralCarverAudioProcessor::loadImageMask (const juce::File& file)
{
    if (! file.existsAsFile() || file.getSize() <= 0 || file.getSize() > (int64_t) SpectralMaskBank::maximumImageBytes)
        return false;
    juce::MemoryBlock data;
    auto stream = file.createInputStream();
    if (stream == nullptr)
        return false;
    stream->readIntoMemoryBlock (data, (int) SpectralMaskBank::maximumImageBytes + 1);
    if (! SpectralMaskBank::isSupportedImage (data))
        return false;
    {
        const juce::ScopedLock lock (maskStateLock);
        if (! maskBank.requestImageMask (data))
            return false;
    }
    updateHostDisplay (ChangeDetails().withNonParameterStateChanged (true));
    return true;
}

juce::String SpectralCarverAudioProcessor::getMaskText() const
{
    const juce::ScopedLock lock (maskStateLock);
    return maskText;
}

juce::String SpectralCarverAudioProcessor::getMaskSourceDescription() const
{
    return maskBank.getSourceDescription();
}

float SpectralCarverAudioProcessor::getMaskValue (float time, float frequency) const noexcept
{
    return maskBank.sample (time, frequency);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralCarverAudioProcessor();
}
