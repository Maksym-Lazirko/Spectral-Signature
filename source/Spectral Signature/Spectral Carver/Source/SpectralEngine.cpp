#include "SpectralEngine.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;

    float getReal (const std::array<float, SpectralEngine::maximumFftSize * 2>& data, int bin, int) noexcept
    {
        return data[(size_t) bin * 2];
    }

    float getImaginary (const std::array<float, SpectralEngine::maximumFftSize * 2>& data, int bin, int fftSize) noexcept
    {
        return (bin == 0 || bin == fftSize / 2) ? 0.0f : data[(size_t) bin * 2 + 1];
    }

    void setComplex (std::array<float, SpectralEngine::maximumFftSize * 2>& data, int bin, int fftSize,
                     float real, float imaginary) noexcept
    {
        data[(size_t) bin * 2] = real;
        data[(size_t) bin * 2 + 1] = (bin == 0 || bin == fftSize / 2) ? 0.0f : imaginary;
    }

    float readLuminance (const juce::Image& image, float x, float y)
    {
        const auto px = juce::jlimit (0, image.getWidth() - 1, juce::roundToInt (x * (float) (image.getWidth() - 1)));
        const auto py = juce::jlimit (0, image.getHeight() - 1, juce::roundToInt (y * (float) (image.getHeight() - 1)));
        const auto colour = image.getPixelAt (px, py);
        return colour.getFloatAlpha() * (colour.getFloatRed() * 0.2126f
             + colour.getFloatGreen() * 0.7152f + colour.getFloatBlue() * 0.0722f);
    }
}

SpectralMaskBank::SpectralMaskBank() : juce::Thread ("Spectral Signature mask renderer")
{
    static_assert (std::atomic<int>::is_always_lock_free && std::atomic<Mask*>::is_always_lock_free);
    for (auto& mask : masks)
        mask = std::make_unique<Mask>();
    renderText (*masks[0], "SPECTRAL\nSIGNATURE");
    activeMask.store (masks[0].get(), std::memory_order_release);
    startThread();
}

SpectralMaskBank::~SpectralMaskBank()
{
    signalThreadShouldExit();
    notify();
    stopThread (-1); // Only the control thread waits; images and text are bounded.
}

void SpectralMaskBank::requestTextMask (const juce::String& text)
{
    {
        const juce::ScopedLock lock (requestLock);
        pendingText = text.substring (0, maximumTextLength);
        pendingImage.reset();
        pending = true;
        requestGeneration.fetch_add (1, std::memory_order_release);
    }
    notify();
}

bool SpectralMaskBank::isSupportedImage (const juce::MemoryBlock& block) noexcept
{
    const auto size = block.getSize();
    if (size < 10 || size > maximumImageBytes)
        return false;
    const auto* data = static_cast<const uint8_t*> (block.getData());
    const auto valid = [] (uint32_t width, uint32_t height)
    {
        return width > 0 && height > 0 && width <= 4096 && height <= 4096
            && (uint64_t) width * height <= 16 * 1024 * 1024;
    };
    const auto be16 = [data] (size_t offset) { return ((uint32_t) data[offset] << 8) | data[offset + 1]; };
    const auto be32 = [data] (size_t offset)
    {
        return ((uint32_t) data[offset] << 24) | ((uint32_t) data[offset + 1] << 16)
             | ((uint32_t) data[offset + 2] << 8) | data[offset + 3];
    };
    constexpr uint8_t png[] { 137, 80, 78, 71, 13, 10, 26, 10 };
    if (size >= 24 && std::memcmp (data, png, sizeof (png)) == 0)
        return be32 (8) == 13 && std::memcmp (data + 12, "IHDR", 4) == 0 && valid (be32 (16), be32 (20));
    if (std::memcmp (data, "GIF87a", 6) == 0 || std::memcmp (data, "GIF89a", 6) == 0)
        return valid ((uint32_t) data[6] | ((uint32_t) data[7] << 8), (uint32_t) data[8] | ((uint32_t) data[9] << 8));
    if (data[0] != 0xff || data[1] != 0xd8)
        return false;
    for (size_t offset = 2; offset + 4 <= size;)
    {
        if (data[offset++] != 0xff)
            return false;
        while (offset < size && data[offset] == 0xff)
            ++offset;
        if (offset >= size)
            return false;
        const auto marker = data[offset++];
        if (marker == 0xd9 || marker == 0xda)
            return false;
        if (marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7))
            continue;
        if (offset + 2 > size)
            return false;
        const auto length = be16 (offset);
        if (length < 2 || length > size - offset)
            return false;
        const auto isSof = marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc;
        if (isSof)
            return length >= 8 && valid (be16 (offset + 5), be16 (offset + 3));
        offset += length;
    }
    return false;
}

bool SpectralMaskBank::requestImageMask (const juce::MemoryBlock& image)
{
    if (! isSupportedImage (image))
        return false;
    {
        const juce::ScopedLock lock (requestLock);
        pendingImage = image;
        pendingText.clear();
        pending = true;
        requestGeneration.fetch_add (1, std::memory_order_release);
    }
    notify();
    return true;
}

void SpectralMaskBank::run()
{
    while (! threadShouldExit())
    {
        juce::String text;
        juce::MemoryBlock image;
        uint64_t generation = 0;
        bool haveRequest = false;
        {
            const juce::ScopedLock lock (requestLock);
            if (pending)
            {
                text = std::move (pendingText);
                image = std::move (pendingImage);
                generation = requestGeneration.load (std::memory_order_acquire);
                pending = false;
                haveRequest = true;
            }
        }
        if (! haveRequest)
        {
            wait (200);
            continue;
        }
        Mask* destination = nullptr;
        while (! threadShouldExit() && (destination = acquireWritableMask()) == nullptr)
            wait (1);
        if (destination == nullptr)
            break;
        bool success = true;
        if (image.getSize() > 0)
            success = renderImage (*destination, image);
        else
            renderText (*destination, text);
        destination->readers.store (0, std::memory_order_release);
        if (generation == requestGeneration.load (std::memory_order_acquire))
        {
            if (success)
                publish (destination, image.getSize() > 0 ? "Embedded image mask" : "Text: " + text.substring (0, 48), text, image);
            else
            {
                const juce::ScopedLock lock (sourceLock);
                sourceDescription = "Image decode failed; previous mask retained";
            }
        }
        completedGeneration.store (generation, std::memory_order_release);
        renderCompleted.signal();
    }
}

SpectralMaskBank::Mask* SpectralMaskBank::acquireWritableMask() noexcept
{
    const auto* active = activeMask.load (std::memory_order_acquire);
    for (auto& mask : masks)
    {
        int expected = 0;
        if (mask.get() != active && mask->readers.compare_exchange_strong (expected, -1, std::memory_order_acq_rel))
            return mask.get();
    }
    return nullptr;
}

SpectralMaskBank::ReadView SpectralMaskBank::acquireReadView() const noexcept
{
    // Bounded attempts: the audio thread never waits for a render or another reader.
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        auto* mask = activeMask.load (std::memory_order_acquire);
        if (mask == nullptr)
            break;
        auto readers = mask->readers.load (std::memory_order_acquire);
        if (readers >= 0 && readers < 1024
            && mask->readers.compare_exchange_strong (readers, readers + 1, std::memory_order_acq_rel))
            return ReadView (mask);
    }
    return {};
}

SpectralMaskBank::ReadView::~ReadView()
{
    if (mask != nullptr)
        mask->readers.fetch_sub (1, std::memory_order_release);
}

void SpectralMaskBank::publish (Mask* mask, const juce::String& description, const juce::String& text, juce::MemoryBlock& image)
{
    const juce::ScopedLock lock (sourceLock);
    currentText = text;
    currentImage = std::move (image);
    sourceDescription = description;
    activeMask.store (mask, std::memory_order_release);
}

void SpectralMaskBank::getStateSource (juce::String& text, juce::MemoryBlock& image)
{
    // Preset serialization already runs off the audio callback. Wait for a queued
    // decode before choosing its source; failed decodes preserve the previous one.
    const auto requested = requestGeneration.load (std::memory_order_acquire);
    const auto started = juce::Time::getMillisecondCounter();
    while (completedGeneration.load (std::memory_order_acquire) < requested
           && juce::Time::getMillisecondCounter() - started < 5000u)
        renderCompleted.wait (10);
    const juce::ScopedLock lock (sourceLock);
    text = currentText;
    image = currentImage;
}

void SpectralMaskBank::renderText (Mask& destination, const juce::String& text)
{
    juce::Image rendered (juce::Image::ARGB, 1024, 512, true, juce::SoftwareImageType());
    {
        juce::Graphics graphics (rendered);
        graphics.fillAll (juce::Colours::black);
        graphics.setColour (juce::Colours::white);
        graphics.setFont (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 150.0f, juce::Font::bold));
        graphics.drawFittedText (text, rendered.getBounds().reduced (32), juce::Justification::centred, 5, 0.82f);
    }
    for (int y = 0; y < frequencyResolution; ++y)
        for (int x = 0; x < timeResolution; ++x)
            destination.pixels[(size_t) y * timeResolution + x]
                = readLuminance (rendered, (float) x / (timeResolution - 1), (float) y / (frequencyResolution - 1));
}

bool SpectralMaskBank::renderImage (Mask& destination, const juce::MemoryBlock& data)
{
    const auto image = juce::ImageFileFormat::loadFrom (data.getData(), data.getSize());
    if (! image.isValid())
        return false;
    const auto resized = juce::SoftwareImageType().convert (image)
        .rescaled (timeResolution, frequencyResolution, juce::Graphics::highResamplingQuality);
    for (int y = 0; y < frequencyResolution; ++y)
        for (int x = 0; x < timeResolution; ++x)
            destination.pixels[(size_t) y * timeResolution + x]
                = readLuminance (resized, (float) x / (timeResolution - 1), (float) y / (frequencyResolution - 1));
    return true;
}

float SpectralMaskBank::ReadView::sample (float time, float frequency) const noexcept
{
    if (mask == nullptr || ! std::isfinite (time) || ! std::isfinite (frequency))
        return 0.0f;
    time -= std::floor (time);
    frequency = juce::jlimit (0.0f, 1.0f, frequency);
    const auto x = time * (float) (timeResolution - 1);
    const auto y = (1.0f - frequency) * (float) (frequencyResolution - 1);
    const auto x0 = juce::jlimit (0, timeResolution - 1, (int) x);
    const auto y0 = juce::jlimit (0, frequencyResolution - 1, (int) y);
    const auto x1 = (x0 + 1) % timeResolution;
    const auto y1 = juce::jmin (y0 + 1, frequencyResolution - 1);
    const auto dx = x - (float) x0, dy = y - (float) y0;
    const auto at = [this] (int px, int py) { return mask->pixels[(size_t) py * timeResolution + px]; };
    return juce::jlimit (0.0f, 1.0f,
        juce::jmap (dy, juce::jmap (dx, at (x0, y0), at (x1, y0)), juce::jmap (dx, at (x0, y1), at (x1, y1))));
}

float SpectralMaskBank::sample (float time, float frequency) const noexcept
{
    return acquireReadView().sample (time, frequency);
}

juce::String SpectralMaskBank::getSourceDescription() const
{
    const juce::ScopedLock lock (sourceLock);
    return sourceDescription;
}

SpectralEngine::SpectralEngine (SpectralMaskBank& maskBankIn) : maskBank (maskBankIn)
{
    for (int order = minimumFftOrder; order <= maximumFftOrder; ++order)
    {
        const auto index = (size_t) (order - minimumFftOrder);
        const auto size = 1 << order;
        ffts[index] = std::make_unique<RealtimeFFT> (order);

        for (int n = 0; n < size; ++n)
            analysisWindows[index][(size_t) n] = 0.5f - 0.5f * std::cos (2.0f * pi * (float) n / (float) size);
    }
}

void SpectralEngine::prepare (double sampleRate)
{
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    mapMode = -1;
    reset();
    resetCycle();
}

void SpectralEngine::configureFor (const SpectralSettings& settings) noexcept
{
    configure (settings);
}

void SpectralEngine::reset() noexcept
{
    std::fill (inputLeft.begin(), inputLeft.end(), 0.0f);
    std::fill (inputRight.begin(), inputRight.end(), 0.0f);
    std::fill (outputRingLeft.begin(), outputRingLeft.end(), 0.0f);
    std::fill (outputRingRight.begin(), outputRingRight.end(), 0.0f);
    std::fill (smoothedMask.begin(), smoothedMask.end(), 0.0f);
    inputPosition = outputPosition = samplesSinceFrame = validInputSamples = 0;
    previousFrameEnergy = 0.0f;
}

void SpectralEngine::resetCycle() noexcept
{
    cyclePosition = 0.0f;
    std::fill (smoothedMask.begin(), smoothedMask.end(), 0.0f);
}

void SpectralEngine::configure (const SpectralSettings& settings) noexcept
{
    const auto newOrder = juce::jlimit (minimumFftOrder, maximumFftOrder, minimumFftOrder + settings.quality);
    const auto newSize = 1 << newOrder;
    const auto newHop = newSize / (4 << juce::jlimit (0, 2, settings.overlap));
    if (newOrder != fftOrder || newHop != hopSize)
    {
        fftOrder = newOrder;
        fftSize = newSize;
        hopSize = newHop;
        reset();

        // Periodic Hann squared overlap sums to 3 * overlap / 8.
        // JUCE's inverse transform already includes 1/N scaling.
        synthesisNormalisation = 8.0f / (3.0f * (float) (fftSize / hopSize));
        ++configurationGeneration;

        mapMode = -1; // force frequency map rebuild for the new fft size
    }
}

void SpectralEngine::rebuildFrequencyMap (const SpectralSettings& settings) noexcept
{
    const auto bins = fftSize / 2 + 1;
    const auto lower = juce::jlimit (10.0f, settings.sampleRate * 0.45f, settings.lowerFrequency);
    const auto upper = juce::jlimit (lower + 10.0f, settings.sampleRate * 0.49f, settings.upperFrequency);
    const auto mel = [] (float hz) { return 2595.0f * std::log10 (1.0f + hz / 700.0f); };
    const auto melLower = mel (lower), melUpper = mel (upper);
    const auto logRatio = std::log (upper / lower);

    for (int bin = 0; bin < bins; ++bin)
    {
        const auto frequencyHz = (float) bin * settings.sampleRate / (float) fftSize;
        if (frequencyHz < lower || frequencyHz > upper)
        {
            frequencyPositions[(size_t) bin] = -1.0f;
            continue;
        }

        float position;
        if (settings.frequencyMapping == 1)
            position = std::log (frequencyHz / lower) / logRatio;
        else if (settings.frequencyMapping == 2)
            position = (mel (frequencyHz) - melLower) / (melUpper - melLower);
        else
            position = (frequencyHz - lower) / (upper - lower);

        frequencyPositions[(size_t) bin] = juce::jlimit (0.0f, 1.0f, position);
    }

    mapLowerHz = settings.lowerFrequency;
    mapUpperHz = settings.upperFrequency;
    mapMode = settings.frequencyMapping;
    mapSampleRate = (int) settings.sampleRate;
}

float SpectralEngine::randomBipolar (uint32_t& state) noexcept
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return ((float) (state & 0xffffu) / 32767.5f) - 1.0f;
}

float SpectralEngine::transformedMask (int bin, const SpectralSettings& settings, const SpectralMaskBank::ReadView& mask) noexcept
{
    const auto position = frequencyPositions[(size_t) bin];
    if (position < 0.0f)
        return 0.0f;

    auto value = mask.sample (cyclePosition, position);
    if (settings.invertMask)
        value = 1.0f - value;

    const auto threshold = juce::jlimit (0.0f, 0.98f, settings.threshold);
    value = juce::jlimit (0.0f, 1.0f, (value - threshold) / (1.0f - threshold));
    const auto exponent = juce::jmap (settings.maskContrast, 0.20f, 2.80f);
    value = std::pow (value, exponent);
    return value;
}

void SpectralEngine::applyEngraving (float& real, float& imaginary, float magnitude, float engraving,
                                     float localContrast, const SpectralSettings& settings, uint32_t& noiseState) noexcept
{
    const auto transparencyScale = 1.0f - 0.82f * settings.transparency;
    const auto amount = engraving * settings.intensity * transparencyScale;
    if ((settings.mode == 1 ? std::abs (localContrast) * settings.intensity * transparencyScale : amount) <= 0.000001f)
        return;

    if (settings.mode == 0)
    {
        const auto gain = juce::Decibels::decibelsToGain (-settings.maximumCutDb * amount);
        real *= gain;
        imaginary *= gain;
    }
    else if (settings.mode == 1)
    {
        const auto range = localContrast >= 0.0f ? settings.maximumBoostDb : settings.maximumCutDb;
        const auto db = juce::jlimit (-settings.maximumCutDb, settings.maximumBoostDb,
                                      localContrast * range * 4.0f * settings.intensity * transparencyScale);
        const auto gain = juce::Decibels::decibelsToGain (db);
        real *= gain;
        imaginary *= gain;
    }
    else
    {
        // Intentionally conservative: shaped carrier remains far below the local bin.
        const auto carrier = magnitude * 0.00365f * amount;
        real += randomBipolar (noiseState) * carrier;
        imaginary += randomBipolar (noiseState) * carrier;
    }
}

void SpectralEngine::processFrame (const SpectralSettings& settings) noexcept
{
    const auto fftIndex = (size_t) (fftOrder - minimumFftOrder);
    const auto* fft = ffts[fftIndex].get();
    const auto bins = fftSize / 2 + 1;
    const auto* window = analysisWindows[fftIndex].data();

    for (int n = 0; n < fftSize; ++n)
    {
        const auto sourceIndex = (inputPosition + n) % fftSize;
        fftLeft[(size_t) n] = inputLeft[(size_t) sourceIndex] * window[n];
        fftRight[(size_t) n] = inputRight[(size_t) sourceIndex] * window[n];
    }
    std::fill (fftLeft.begin() + fftSize, fftLeft.begin() + fftSize * 2, 0.0f);
    std::fill (fftRight.begin() + fftSize, fftRight.begin() + fftSize * 2, 0.0f);
    fft->performRealOnlyForwardTransform (fftLeft.data());
    fft->performRealOnlyForwardTransform (fftRight.data());

    if (settings.lowerFrequency != mapLowerHz || settings.upperFrequency != mapUpperHz
        || settings.frequencyMapping != mapMode || (int) settings.sampleRate != mapSampleRate)
        rebuildFrequencyMap (settings);
    const auto mask = maskBank.acquireReadView();
    for (int bin = 0; bin < bins; ++bin)
        targetMask[(size_t) bin] = transformedMask (bin, settings, mask);

    const auto timeCoefficient = juce::jlimit (0.02f, 0.95f,
        juce::jmap (settings.timeSmoothing, 0.85f, 0.08f) * juce::jmap (settings.detail, 0.45f, 1.6f));
    for (int bin = 0; bin < bins; ++bin)
    {
        const auto left = targetMask[(size_t) juce::jmax (0, bin - 1)];
        const auto centre = targetMask[(size_t) bin];
        const auto right = targetMask[(size_t) juce::jmin (bins - 1, bin + 1)];
        const auto neighbourhood = (left + centre * 2.0f + right) * 0.25f;
        const auto smoothedFrequency = juce::jmap (settings.frequencySmoothing, centre, neighbourhood);
        smoothedMask[(size_t) bin] += (smoothedFrequency - smoothedMask[(size_t) bin]) * timeCoefficient;
    }

    float averageMagnitude = 0.0f, frameEnergy = 0.0f;
    for (int bin = 1; bin < bins - 1; ++bin)
    {
        const auto lR = getReal (fftLeft, bin, fftSize), lI = getImaginary (fftLeft, bin, fftSize);
        const auto rR = getReal (fftRight, bin, fftSize), rI = getImaginary (fftRight, bin, fftSize);
        const auto magnitude = 0.5f * (std::sqrt (lR * lR + lI * lI) + std::sqrt (rR * rR + rI * rI));
        averageMagnitude += magnitude;
        frameEnergy += magnitude * magnitude;
    }
    averageMagnitude /= (float) juce::jmax (1, bins - 2);
    frameEnergy /= (float) juce::jmax (1, bins - 2);
    const auto transient = previousFrameEnergy > 1.0e-12f
                         ? juce::jlimit (0.0f, 1.0f, (frameEnergy / previousFrameEnergy - 1.0f) * 1.25f) : 0.0f;
    previousFrameEnergy = frameEnergy;

    for (int bin = 0; bin < bins; ++bin)
    {
        const auto originalLeftReal = getReal (fftLeft, bin, fftSize);
        const auto originalLeftImaginary = getImaginary (fftLeft, bin, fftSize);
        const auto originalRightReal = getReal (fftRight, bin, fftSize);
        const auto originalRightImaginary = getImaginary (fftRight, bin, fftSize);
        auto leftReal = originalLeftReal, leftImaginary = originalLeftImaginary;
        auto rightReal = originalRightReal, rightImaginary = originalRightImaginary;
        const auto leftMagnitude = std::sqrt (leftReal * leftReal + leftImaginary * leftImaginary);
        const auto rightMagnitude = std::sqrt (rightReal * rightReal + rightImaginary * rightImaginary);
        const auto meanMagnitude = 0.5f * (leftMagnitude + rightMagnitude);

        auto protection = 1.0f;
        const auto frequency = (float) bin * settings.sampleRate / (float) fftSize;
        if (settings.lowProtection && frequency < 200.0f)
            protection *= juce::jmap (frequency / 200.0f, 0.12f, 1.0f);
        if (settings.highProtection && frequency > settings.sampleRate * 0.38f)
            protection *= juce::jmap ((frequency - settings.sampleRate * 0.38f) / (settings.sampleRate * 0.12f), 1.0f, 0.45f);
        protection *= 1.0f - transient * settings.transientProtection * 0.80f;
        const auto tonalness = averageMagnitude > 1.0e-12f ? meanMagnitude / (averageMagnitude * 5.0f) : 0.0f;
        protection *= 1.0f - settings.tonalProtection * juce::jlimit (0.0f, 0.70f, tonalness - 1.0f);
        const auto contentWeight = averageMagnitude > 1.0e-12f
                                 ? juce::jlimit (0.15f, 1.0f, std::sqrt (meanMagnitude / averageMagnitude)) : 0.15f;
        protection *= juce::jmap (settings.adaptive, 1.0f, contentWeight);

        const auto previous = smoothedMask[(size_t) juce::jmax (0, bin - 1)];
        const auto next = smoothedMask[(size_t) juce::jmin (bins - 1, bin + 1)];
        const auto inBand = frequencyPositions[(size_t) bin] >= 0.0f;
        const auto localContrast = inBand ? smoothedMask[(size_t) bin] - 0.5f * (previous + next) : 0.0f;
        const auto engraving = inBand ? smoothedMask[(size_t) bin] * protection : 0.0f;

        if (settings.stereoMode == 1 || settings.stereoMode == 2)
        {
            auto midReal = 0.5f * (leftReal + rightReal), midImaginary = 0.5f * (leftImaginary + rightImaginary);
            auto sideReal = 0.5f * (leftReal - rightReal), sideImaginary = 0.5f * (leftImaginary - rightImaginary);
            auto& targetReal = settings.stereoMode == 1 ? midReal : sideReal;
            auto& targetImaginary = settings.stereoMode == 1 ? midImaginary : sideImaginary;
            const auto targetMagnitude = std::sqrt (targetReal * targetReal + targetImaginary * targetImaginary);
            applyEngraving (targetReal, targetImaginary, targetMagnitude, engraving, localContrast * protection, settings,
                            settings.stereoMode == 1 ? randomLeft : randomRight);
            leftReal = midReal + sideReal;
            leftImaginary = midImaginary + sideImaginary;
            rightReal = midReal - sideReal;
            rightImaginary = midImaginary - sideImaginary;
        }
        else
        {
            auto linkedNoise = randomLeft;
            applyEngraving (leftReal, leftImaginary, leftMagnitude, engraving, localContrast * protection, settings, randomLeft);
            applyEngraving (rightReal, rightImaginary, rightMagnitude, engraving, localContrast * protection, settings,
                            settings.stereoMode == 0 ? linkedNoise : randomRight);
        }

        switch (settings.audition)
        {
            case 1: // The original spectrum weighted by the current mask.
                leftReal = originalLeftReal * smoothedMask[(size_t) bin];
                leftImaginary = originalLeftImaginary * smoothedMask[(size_t) bin];
                rightReal = originalRightReal * smoothedMask[(size_t) bin];
                rightImaginary = originalRightImaginary * smoothedMask[(size_t) bin];
                break;
            case 3: // Positive gain contribution, or the additive noise carrier.
                if (settings.mode != 2 && leftReal * leftReal + leftImaginary * leftImaginary <= leftMagnitude * leftMagnitude)
                { leftReal = originalLeftReal; leftImaginary = originalLeftImaginary; }
                if (settings.mode != 2 && rightReal * rightReal + rightImaginary * rightImaginary <= rightMagnitude * rightMagnitude)
                { rightReal = originalRightReal; rightImaginary = originalRightImaginary; }
                [[fallthrough]];
            case 4: // Delta
                leftReal -= originalLeftReal; leftImaginary -= originalLeftImaginary;
                rightReal -= originalRightReal; rightImaginary -= originalRightImaginary;
                break;
            case 2: // Removed signal
                if (settings.mode == 2 || leftReal * leftReal + leftImaginary * leftImaginary >= leftMagnitude * leftMagnitude)
                { leftReal = originalLeftReal; leftImaginary = originalLeftImaginary; }
                if (settings.mode == 2 || rightReal * rightReal + rightImaginary * rightImaginary >= rightMagnitude * rightMagnitude)
                { rightReal = originalRightReal; rightImaginary = originalRightImaginary; }
                leftReal = originalLeftReal - leftReal; leftImaginary = originalLeftImaginary - leftImaginary;
                rightReal = originalRightReal - rightReal; rightImaginary = originalRightImaginary - rightImaginary;
                break;
            case 5: // Mid contribution
            {
                const auto mR = 0.5f * (leftReal + rightReal), mI = 0.5f * (leftImaginary + rightImaginary);
                leftReal = rightReal = mR; leftImaginary = rightImaginary = mI;
                break;
            }
            case 6: // Side contribution
            {
                const auto sR = 0.5f * (leftReal - rightReal), sI = 0.5f * (leftImaginary - rightImaginary);
                leftReal = sR; leftImaginary = sI; rightReal = -sR; rightImaginary = -sI;
                break;
            }
            default: break;
        }

        setComplex (fftLeft, bin, fftSize, leftReal, leftImaginary);
        setComplex (fftRight, bin, fftSize, rightReal, rightImaginary);
    }

    fft->performRealOnlyInverseTransform (fftLeft.data());
    fft->performRealOnlyInverseTransform (fftRight.data());

    for (int n = 0; n < fftSize; ++n)
    {
        const auto synthesis = window[n] * synthesisNormalisation;
        if (synthesis == 0.0f)
            continue;
        const auto outputIndex = (outputPosition + maximumFftSize - fftSize + n) % outputRingSize;
        outputRingLeft[(size_t) outputIndex]  += fftLeft[(size_t) n] * synthesis;
        outputRingRight[(size_t) outputIndex] += fftRight[(size_t) n] * synthesis;
    }

    const auto seconds = juce::jlimit (0.25f, 60.0f, settings.loopSeconds);
    cyclePosition += (float) hopSize / (seconds * settings.sampleRate);
    cyclePosition -= std::floor (cyclePosition);
}

void SpectralEngine::processSample (float inLeft, float inRight, const SpectralSettings& settings,
                                    float& outLeft, float& outRight) noexcept
{
    outLeft = outputRingLeft[(size_t) outputPosition];
    outRight = outputRingRight[(size_t) outputPosition];
    outputRingLeft[(size_t) outputPosition] = outputRingRight[(size_t) outputPosition] = 0.0f;
    outputPosition = (outputPosition + 1) % outputRingSize;

    inputLeft[(size_t) inputPosition] = inLeft;
    inputRight[(size_t) inputPosition] = inRight;
    inputPosition = (inputPosition + 1) % fftSize;
    validInputSamples = juce::jmin (fftSize, validInputSamples + 1);
    ++samplesSinceFrame;
    if (samplesSinceFrame >= hopSize)
    {
        samplesSinceFrame = 0;
        processFrame (settings);
    }
}
