#include "PluginProcessor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
#include <vector>

namespace
{
    thread_local bool trackAllocations = false;
    thread_local int audioAllocations = 0, audioDeallocations = 0;
}

void* operator new (std::size_t size)
{
    if (trackAllocations) ++audioAllocations;
    if (auto* memory = std::malloc (size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t size) { return ::operator new (size); }
void operator delete (void* memory) noexcept
{
    if (trackAllocations && memory != nullptr) ++audioDeallocations;
    std::free (memory);
}
void operator delete[] (void* memory) noexcept { ::operator delete (memory); }
void operator delete (void* memory, std::size_t) noexcept { ::operator delete (memory); }
void operator delete[] (void* memory, std::size_t) noexcept { ::operator delete (memory); }

namespace
{
    int failures = 0, checks = 0;
    void require (bool condition, const char* description)
    {
        ++checks;
        if (! condition)
        {
            ++failures;
            std::cout << "FAIL: " << description << '\n';
        }
    }

    void set (SpectralCarverAudioProcessor& processor, const char* id, float value)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }

    float signal (int index, int channel, double sampleRate)
    {
        if (index < 0) return 0.0f;
        const auto tone = std::sin (6.283185307179586 * 997.0 * (double) index / sampleRate + channel * 0.7);
        const auto noise = ((uint32_t) index * 1664525u + (uint32_t) channel * 1013904223u) & 65535u;
        return (float) (tone * 0.15 + 0.013 + ((index & 1) == 0 ? 0.017 : -0.017))
             + ((float) noise / 65535.0f - 0.5f) * 0.035f;
    }

    void fftReference()
    {
        for (int order = 10; order <= 14; ++order)
        {
            const auto size = 1 << order;
            RealtimeFFT fft (order);
            std::vector<float> data ((size_t) size * 2), original ((size_t) size);
            for (int index = 0; index < size; ++index)
                original[(size_t) index] = data[(size_t) index] = 0.1f + ((index & 1) == 0 ? 0.05f : -0.05f)
                    + (float) (0.3 * std::sin (6.283185307179586 * 7.0 * index / size));
            fft.performRealOnlyForwardTransform (data.data());
            require (std::abs (data[0] / size - 0.1f) < 1.0e-6f, "FFT reference DC bin");
            require (std::abs (data[(size_t) size] / size - 0.05f) < 1.0e-6f, "FFT reference Nyquist bin");
            require (std::abs (data[15] / size + 0.15f) < 1.0e-6f && std::abs (data[14] / size) < 1.0e-6f,
                     "FFT reference complex sine phase and amplitude");
            fft.performRealOnlyInverseTransform (data.data());
            float error = 0.0f;
            for (int index = 0; index < size; ++index)
                error = juce::jmax (error, std::abs (data[(size_t) index] - original[(size_t) index]));
            require (error < 1.0e-6f, "FFT reference inverse scaling");
        }
        std::cout << "FFT analytic DC, Nyquist, phase and inverse checks complete\n";
    }

    void prepare (SpectralCarverAudioProcessor& p, double rate, int channels = 2)
    {
        p.setPlayConfigDetails (channels, channels, rate, 1024);
        p.prepareToPlay (rate, 1024);
    }

    float nullRender (SpectralCarverAudioProcessor& processor, double rate, bool hostBypass = false, int channels = 2)
    {
        constexpr int blocks[] { 1, 17, 64, 257, 511, 1024, 3, 128 };
        constexpr int samples = SpectralEngine::maximumFftSize + 8192;
        juce::AudioBuffer<float> block (channels, 1024);
        juce::MidiBuffer midi;
        float maximumError = 0.0f;
        int position = 0, blockIndex = 0;
        while (position < samples)
        {
            const auto length = juce::jmin (blocks[blockIndex++ % 8], samples - position);
            block.setSize (channels, length, false, false, true);
            for (int channel = 0; channel < channels; ++channel)
                for (int sample = 0; sample < length; ++sample)
                    block.setSample (channel, sample, signal (position + sample, channel, rate));
            trackAllocations = true;
            if (hostBypass) processor.processBlockBypassed (block, midi);
            else processor.processBlock (block, midi);
            trackAllocations = false;
            for (int channel = 0; channel < channels; ++channel)
                for (int sample = 0; sample < length; ++sample)
                    maximumError = juce::jmax (maximumError,
                        std::abs (block.getSample (channel, sample)
                            - signal (position + sample - processor.getLatencySamples(), channel, rate)));
            position += length;
        }
        return maximumError;
    }

    void nullMatrix()
    {
        auto processor = std::make_unique<SpectralCarverAudioProcessor>();
        set (*processor, ParamIds::intensity, 0.0f);
        set (*processor, ParamIds::safeMode, 0.0f);
        set (*processor, ParamIds::mix, 1.0f);
        float worst = 0.0f;
        for (const auto rate : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
            for (int quality = 0; quality < 5; ++quality)
                for (int overlap = 0; overlap < 3; ++overlap)
                {
                    set (*processor, ParamIds::quality, (float) quality);
                    set (*processor, ParamIds::overlap, (float) overlap);
                    prepare (*processor, rate);
                    const auto error = nullRender (*processor, rate);
                    worst = juce::jmax (worst, error);
                    if (error >= 2.0e-5f)
                        std::cout << "Null matrix rate=" << rate << " quality=" << quality << " overlap=" << overlap << " error=" << error << '\n';
                    require (error < 2.0e-5f, "unity STFT reconstruction including DC/Nyquist and startup");
                    require (processor->getLatencySamples() == SpectralEngine::maximumFftSize, "stable reported latency");
                    processor->releaseResources();
                }
        std::cout << "Unity reconstruction: 75 rate/FFT/overlap cases; worst absolute error=" << worst << '\n';
        prepare (*processor, 48000.0, 1);
        require (nullRender (*processor, 48000.0, false, 1) < 2.0e-5f, "mono reconstruction");
    }

    void bypassAndSafety()
    {
        auto p = std::make_unique<SpectralCarverAudioProcessor>();
        set (*p, ParamIds::inputGainDb, 18.0f);
        set (*p, ParamIds::outputGainDb, 24.0f);
        set (*p, ParamIds::mix, 1.0f);
        set (*p, ParamIds::bypass, 1.0f);
        prepare (*p, 48000.0);
        require (nullRender (*p, 48000.0) == 0.0f, "parameter bypass preserves raw delayed input despite gains and safe mode");
        set (*p, ParamIds::bypass, 0.0f);
        prepare (*p, 48000.0);
        require (nullRender (*p, 48000.0, true) == 0.0f, "host bypass preserves raw delayed input");
        prepare (*p, 48000.0);
        juce::AudioBuffer<float> block (2, 1024);
        juce::MidiBuffer midi;
        float peak = 0.0f;
        for (int index = 0; index < 40; ++index)
        {
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < 1024; ++sample)
                    block.setSample (channel, sample, signal (index * 1024 + sample, channel, 48000.0) * 8.0f);
            p->processBlock (block, midi);
            peak = juce::jmax (peak, block.getMagnitude (0, 1024));
        }
        require (peak <= 0.944062f && peak > 0.9f, "sample safety ceiling is bounded after output gain");
        for (int index = 0; index < 24; ++index)
        {
            block.clear();
            if (index == 0)
            {
                block.setSample (0, 0, std::numeric_limits<float>::quiet_NaN());
                block.setSample (1, 1, std::numeric_limits<float>::infinity());
                p->getParameters().getRawParameterValue (ParamIds::intensity)->store (std::numeric_limits<float>::quiet_NaN());
            }
            p->processBlock (block, midi);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < 1024; ++sample)
                    if (! std::isfinite (block.getSample (channel, sample)))
                    {
                        require (false, "non-finite audio/parameter recovery");
                        return;
                    }
        }
        require (true, "non-finite audio/parameter recovery");
        std::cout << "Bypass, mono, sample ceiling and non-finite input checks complete\n";
    }

    bool waitForDescription (SpectralCarverAudioProcessor& p, const juce::String& description)
    {
        for (int attempt = 0; attempt < 500; ++attempt)
        {
            if (p.getMaskSourceDescription().contains (description)) return true;
            juce::Thread::sleep (10);
        }
        return false;
    }

    void engravingModes()
    {
        auto p = std::make_unique<SpectralCarverAudioProcessor>();
        p->setTextMask ("");
        require (waitForDescription (*p, "Text: "), "blank effect test mask ready");
        for (auto* id : { ParamIds::transparency, ParamIds::adaptive, ParamIds::transientProtection,
                          ParamIds::tonalProtection, ParamIds::lowProtection, ParamIds::safeMode,
                          ParamIds::frequencySmoothing, ParamIds::timeSmoothing, ParamIds::threshold })
            set (*p, id, 0.0f);
        for (auto* id : { ParamIds::invertMask, ParamIds::intensity, ParamIds::mix })
            set (*p, id, 1.0f);
        set (*p, ParamIds::maximumCutDb, 12.0f);
        set (*p, ParamIds::lowerFrequency, 10.0f);
        set (*p, ParamIds::upperFrequency, 20000.0f);
        juce::AudioBuffer<float> block (2, 512);
        juce::MidiBuffer midi;
        const auto run = [&] (int audition)
        {
            set (*p, ParamIds::audition, (float) audition);
            prepare (*p, 48000.0);
            double product = 0.0, energy = 0.0;
            float difference = 0.0f;
            for (int frame = 0; frame < 96; ++frame)
            {
                for (int sample = 0; sample < 512; ++sample)
                {
                    const auto value = (float) (0.2 * std::sin (6.283185307179586 * 1000.0 * (frame * 512 + sample) / 48000.0));
                    block.setSample (0, sample, value);
                    block.setSample (1, sample, value);
                }
                p->processBlock (block, midi);
                if (frame < 48) continue;
                for (int sample = 0; sample < 512; ++sample)
                {
                    const auto reference = (float) (0.2 * std::sin (6.283185307179586 * 1000.0
                        * (frame * 512 + sample - p->getLatencySamples()) / 48000.0));
                    product += block.getSample (0, sample) * reference;
                    energy += reference * reference;
                    difference = juce::jmax (difference, std::abs (block.getSample (0, sample) - block.getSample (1, sample)));
                }
            }
            require (difference < 1.0e-6f, "linked processing preserves centred mono image");
            return product / energy;
        };
        const auto cut = run (0), mask = run (1), removed = run (2), added = run (3), delta = run (4);
        require (std::abs (cut - std::pow (10.0, -12.0 / 20.0)) < 0.002, "Cut produces requested in-band attenuation");
        require (std::abs (mask - 1.0) < 0.002, "Mask audition exposes masked original spectrum");
        require (std::abs (removed - (1.0 - cut)) < 0.002, "Removed audition equals removed Cut contribution");
        require (std::abs (added) < 1.0e-5, "Added audition is silent for Cut");
        require (std::abs (delta + removed) < 0.002, "Delta audition has signed wet-minus-dry contribution");
        set (*p, ParamIds::mix, 0.2f);
        require (std::abs (run (4) - delta * 0.2) < 0.002, "Delta solo scales with wet mix without leaking the dry signal");
        set (*p, ParamIds::mix, 1.0f);
        set (*p, ParamIds::mode, 2.0f);
        (void) run (0);
        prepare (*p, 48000.0);
        float silencePeak = 0.0f;
        for (int frame = 0; frame < 48; ++frame)
        {
            block.clear();
            p->processBlock (block, midi);
            silencePeak = juce::jmax (silencePeak, block.getMagnitude (0, 512));
        }
        require (silencePeak == 0.0f, "Noise-Fill does not generate a carrier during digital silence");
        const auto beforeReset = p->getEngravingCyclePosition();
        set (*p, ParamIds::resetCycle, 1.0f);
        p->processBlock (block, midi);
        require (p->getEngravingCyclePosition() < beforeReset, "Reset Cycle rising edge restarts phase");
        for (int frame = 0; frame < 8; ++frame) p->processBlock (block, midi);
        const auto beforeSecondReset = p->getEngravingCyclePosition();
        set (*p, ParamIds::resetCycle, 0.0f);
        p->processBlock (block, midi);
        require (p->getEngravingCyclePosition() < beforeSecondReset, "Reset Cycle can be used repeatedly on either toggle edge");
        std::cout << "Cut gain=" << cut << " mask=" << mask << " removed=" << removed << " added=" << added << " delta=" << delta << '\n';
    }

    void stateAndMasks()
    {
        auto p = std::make_unique<SpectralCarverAudioProcessor>();
        auto restored = std::make_unique<SpectralCarverAudioProcessor>();
        const auto unicode = juce::String::fromUTF8 ("D\xe1\xba\xa5u \xe1\xba\xa4n / \xe9\xa2\x91\xe8\xb0\xb1");
        p->setTextMask (unicode);
        set (*p, ParamIds::intensity, 0.678f);
        juce::MemoryBlock state;
        p->getStateInformation (state);
        restored->setStateInformation (state.getData(), (int) state.getSize());
        require (restored->getMaskText() == unicode, "Unicode text state roundtrip");
        require (std::abs (restored->getParameters().getRawParameterValue (ParamIds::intensity)->load() - 0.678f) < 1.0e-5f, "parameter state roundtrip");
        juce::Image image (juce::Image::ARGB, 32, 32, true, juce::SoftwareImageType());
        {
            juce::Graphics graphics (image);
            graphics.fillAll (juce::Colours::white);
        }
        juce::MemoryOutputStream encoded;
        require (juce::PNGImageFormat().writeImageToStream (image, encoded), "generate image fixture");
        const auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory)
            .getChildFile ("SpectralSignatureTests-" + juce::Uuid().toString() + "-" + unicode.replaceCharacter ('/', '-'));
        require (directory.createDirectory().wasOk(), "Unicode fixture directory");
        const auto file = directory.getChildFile ("mask.png");
        require (file.replaceWithData (encoded.getData(), encoded.getDataSize()), "write image fixture");
        require (p->loadImageMask (file), "accept bounded PNG image");
        require (waitForDescription (*p, "Embedded image"), "image rendered on worker");
        require (p->getMaskValue (0.5f, 0.5f) > 0.99f, "decoded image mask contains actual white pixels");
        p->getStateInformation (state);
        require (file.deleteFile(), "remove original image fixture");
        restored->setStateInformation (state.getData(), (int) state.getSize());
        require (waitForDescription (*restored, "Embedded image"), "embedded image recalled without original file");
        require (restored->getMaskValue (0.5f, 0.5f) > 0.99f, "recalled white mask content");
        require (file.replaceWithData (encoded.getData(), 24), "write valid-header corrupt-body PNG fixture");
        require (p->loadImageMask (file), "bounded corrupt image is queued for worker validation");
        p->getStateInformation (state);
        require (p->getMaskSourceDescription().contains ("decode failed"), "worker reports corrupt image decode failure");
        require (p->getMaskValue (0.5f, 0.5f) > 0.99f, "decode failure retains prior audible image");
        auto afterFailure = std::make_unique<SpectralCarverAudioProcessor>();
        afterFailure->setStateInformation (state.getData(), (int) state.getSize());
        require (waitForDescription (*afterFailure, "Embedded image"), "save after failed decode retains valid embedded source");
        require (afterFailure->getMaskValue (0.5f, 0.5f) > 0.99f, "save after failed decode reopens exact prior white image");
        require (file.deleteFile(), "remove corrupt fixture");
        p->setTextMask ("");
        p->getStateInformation (state);
        restored->setStateInformation (state.getData(), (int) state.getSize());
        require (restored->getMaskText().isEmpty(), "empty text intentionally recalled");
        require (waitForDescription (*restored, "Text: "), "text replaces old image source");
        require (restored->getMaskValue (0.5f, 0.5f) == 0.0f, "blank mask replaces embedded image");
        const auto before = restored->getParameters().getRawParameterValue (ParamIds::intensity)->load();
        constexpr char bad[] = "not a plugin state";
        restored->setStateInformation (bad, sizeof (bad));
        require (restored->getParameters().getRawParameterValue (ParamIds::intensity)->load() == before, "malformed state is ignored");
        require (! restored->loadImageMask (file), "missing image rejected");
        require (directory.deleteFile(), "remove empty fixture directory");

        auto bank = std::make_unique<SpectralMaskBank>();
        float defaultTextMaximum = 0.0f;
        for (int x = 0; x < 50; ++x)
            for (int y = 0; y < 50; ++y)
                defaultTextMaximum = juce::jmax (defaultTextMaximum, bank->sample ((float) x / 50.0f, (float) y / 50.0f));
        require (defaultTextMaximum > 0.9f, "default text raster contains visible glyphs");
        const auto held = bank->acquireReadView();
        const auto heldValue = held.sample (0.5f, 0.5f);
        std::atomic<bool> done { false };
        std::thread reader ([&]
        {
            while (! done.load (std::memory_order_relaxed))
            {
                const auto view = bank->acquireReadView();
                for (int index = 0; index < 128; ++index)
                    (void) view.sample ((float) index / 128.0f, 0.4f);
            }
        });
        for (int index = 0; index < 1000; ++index)
            bank->requestTextMask (juce::String (index));
        bank->requestTextMask ("");
        for (int attempt = 0; attempt < 500 && bank->getSourceDescription() != "Text: "; ++attempt)
            juce::Thread::sleep (10);
        done.store (true, std::memory_order_relaxed);
        reader.join();
        require (bank->getSourceDescription() == "Text: ", "1000 rapid requests coalesce to latest mask");
        require (held.sample (0.5f, 0.5f) == heldValue, "pinned mask cannot be overwritten by worker");
        require (bank->sample (0.5f, 0.5f) == 0.0f, "latest blank mask published");
        std::cout << "Unicode/empty/image state, missing files and concurrent mask publication checks complete\n";
    }

    void uiBindings()
    {
        auto processor = std::make_unique<SpectralCarverAudioProcessor>();
        prepare (*processor, 48000.0);
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor->createEditor());
        int sliderCount = 0, comboCount = 0, toggleCount = 0;
        bool controlsContained = true;
        juce::Slider* wet = nullptr;
        juce::Slider* intensity = nullptr;
        juce::ComboBox* process = nullptr;
        juce::TextEditor* text = nullptr;
        juce::Button* apply = nullptr;
        juce::Button* reset = nullptr;
        juce::ToggleButton* bypass = nullptr;
        for (auto* component : editor->getChildren())
        {
            if (component->isVisible())
                controlsContained = controlsContained && ! component->getBounds().isEmpty()
                    && editor->getLocalBounds().contains (component->getBounds());
            if (auto* slider = dynamic_cast<juce::Slider*> (component))
            {
                ++sliderCount;
                if (slider->getName() == "Wet mix") wet = slider;
                if (slider->getName() == "Intensity") intensity = slider;
            }
            if (auto* combo = dynamic_cast<juce::ComboBox*> (component))
            {
                ++comboCount;
                if (combo->getTitle() == "PROCESS") process = combo;
            }
            if (auto* toggle = dynamic_cast<juce::ToggleButton*> (component))
            {
                ++toggleCount;
                if (toggle->getButtonText() == "Bypass") bypass = toggle;
            }
            if (auto* candidate = dynamic_cast<juce::TextEditor*> (component))
                if (candidate->getTitle() == "Signature text") text = candidate;
            if (auto* button = dynamic_cast<juce::Button*> (component))
            {
                if (button->getButtonText() == "Apply text") apply = button;
                if (button->getButtonText() == "Restart cycle") reset = button;
            }
        }
        require (sliderCount == 18 && comboCount == 6 && toggleCount == 5 && reset != nullptr,
                 "editor exposes all 18 sliders, 6 selectors, 5 toggles and cycle reset");
        require (controlsContained, "visible editor controls fit within editor bounds");
        const auto found = wet != nullptr && intensity != nullptr && process != nullptr && text != nullptr
                        && apply != nullptr && reset != nullptr && bypass != nullptr;
        require (found, "named UI controls are discoverable");
        if (! found) return;
        wet->setValue (0.37, juce::sendNotificationSync);
        intensity->setValue (0.52, juce::sendNotificationSync);
        require (std::abs (processor->getParameters().getRawParameterValue (ParamIds::mix)->load() - 0.37f) < 1.0e-5f,
                 "Wet mix UI slider updates processor parameter");
        require (std::abs (processor->getParameters().getRawParameterValue (ParamIds::intensity)->load() - 0.52f) < 1.0e-5f,
                 "Intensity UI slider updates processor parameter");
        process->setSelectedId (2, juce::sendNotificationSync);
        require (processor->getParameters().getRawParameterValue (ParamIds::mode)->load() == 1.0f,
                 "Process selector selects Emboss parameter");
        bypass->setToggleState (true, juce::sendNotificationSync);
        require (processor->getParameters().getRawParameterValue (ParamIds::bypass)->load() == 1.0f,
                 "Bypass UI toggle updates bypass parameter");
        set (*processor, ParamIds::mix, 0.61f);
        text->setText ("UI SIGNATURE");
        apply->triggerClick();
        reset->triggerClick();
        juce::Timer::callAfterDelay (180, [&]
        {
            require (std::abs (wet->getValue() - 0.61) < 1.0e-5,
                     "host parameter automation updates the Wet mix slider");
            require (processor->getMaskText() == "UI SIGNATURE", "Apply text button submits editor text to the mask worker");
            require (processor->getParameters().getRawParameterValue (ParamIds::resetCycle)->load() == 1.0f,
                     "Restart cycle button updates the reset parameter");
            juce::MessageManager::getInstance()->stopDispatchLoop();
        });
        juce::MessageManager::getInstance()->runDispatchLoop();
        std::cout << "UI bindings and bounds: " << sliderCount << " sliders, " << comboCount << " selectors, "
                  << toggleCount << " toggles plus reset; bidirectional controls and text action checked\n";
    }

    void automation()
    {
        auto p = std::make_unique<SpectralCarverAudioProcessor>();
        set (*p, ParamIds::intensity, 0.0f);
        set (*p, ParamIds::safeMode, 0.0f);
        set (*p, ParamIds::mix, 1.0f);
        prepare (*p, 48000.0);
        juce::AudioBuffer<float> block (2, 257);
        juce::MidiBuffer midi;
        float worst = 0.0f;
        for (int index = 0; index < 1200; ++index)
        {
            if (index % 71 == 0)
            {
                set (*p, ParamIds::quality, (float) ((index / 71) % 5));
                set (*p, ParamIds::overlap, (float) ((index / 71) % 3));
            }
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < 257; ++sample)
                    block.setSample (channel, sample, signal (index * 257 + sample, channel, 48000.0));
            trackAllocations = true;
            p->processBlock (block, midi);
            trackAllocations = false;
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < 257; ++sample)
                    worst = juce::jmax (worst, std::abs (block.getSample (channel, sample)
                        - signal (index * 257 + sample - p->getLatencySamples(), channel, 48000.0)));
        }
        require (worst < 2.0e-5f, "automated FFT/overlap transitions preserve unity delayed signal");
        std::cout << "Quality automation worst absolute error=" << worst << '\n';
        require (audioAllocations == 0 && audioDeallocations == 0, "no C++ heap allocation/deallocation in monitored audio callbacks");
        std::cout << "Audio-thread C++ new/delete counts: " << audioAllocations << '/' << audioDeallocations << '\n';
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    if (argc == 3 && juce::String (argv[1]) == "--editor-png")
    {
        auto processor = std::make_unique<SpectralCarverAudioProcessor>();
        prepare (*processor, 48000.0);
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor->createEditor());
        juce::Timer::callAfterDelay (600, [] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
        juce::MessageManager::getInstance()->runDispatchLoop();
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto output = juce::File (juce::String::fromUTF8 (argv[2])).createOutputStream();
        const auto success = output != nullptr && juce::PNGImageFormat().writeImageToStream (image, *output);
        std::cout << "Editor snapshot: " << (success ? "saved" : "failed") << '\n';
        return success ? 0 : 1;
    }
    fftReference();
    nullMatrix();
    bypassAndSafety();
    engravingModes();
    stateAndMasks();
    automation();
    uiBindings();
    std::cout << "RESULT: " << (checks - failures) << '/' << checks << " checks passed; failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
