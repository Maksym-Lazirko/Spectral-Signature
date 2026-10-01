#include "PluginEditor.h"

namespace
{
const juce::Colour background { 0xff101619 }, panel { 0xff192226 }, border { 0xff2c3b40 };
const juce::Colour ink { 0xffedf2ed }, muted { 0xff9cabae }, mint { 0xff82d8c5 }, amber { 0xffdfa96b };
void labelStyle (juce::Label& label, float size = 11.0f)
{
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, muted);
    label.setJustificationType (juce::Justification::centredLeft);
}
void card (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& number, const juce::String& title)
{
    auto r = bounds.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 12.0f);
    g.setGradientFill (juce::ColourGradient (panel.brighter (0.025f), r.getTopLeft(), panel, r.getBottomRight(), false));
    g.fillRoundedRectangle (r, 12.0f);
    g.setColour (border);
    g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.setColour (mint.withAlpha (0.8f));
    g.drawText (number, bounds.getX() + 16, bounds.getY() + 13, 24, 18, juce::Justification::left);
    g.setColour (ink);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (title, bounds.getX() + 43, bounds.getY() + 13, bounds.getWidth() - 58, 18, juce::Justification::left);
}
juce::String dbText (float value) { return value > -89.0f ? juce::String (value, 1) : "-inf"; }
}

SignatureLookAndFeel::SignatureLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, mint.withAlpha (0.3f));
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::backgroundColourId, background);
    setColour (juce::ComboBox::outlineColourId, border);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, mint.withAlpha (0.18f));
    setColour (juce::PopupMenu::highlightedTextColourId, mint);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff27363b));
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, ink);
    setColour (juce::TextEditor::backgroundColourId, background);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::outlineColourId, border);
    setColour (juce::TextEditor::focusedOutlineColourId, mint.withAlpha (0.75f));
    setColour (juce::TextEditor::highlightColourId, mint.withAlpha (0.25f));
    setColour (juce::CaretComponent::caretColourId, mint);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xffe4ede8));
    setColour (juce::TooltipWindow::textColourId, background);
    setColour (juce::TooltipWindow::outlineColourId, mint);
}
void SignatureLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float position, float start, float end, juce::Slider& slider)
{
    const auto r = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (8.0f);
    const auto radius = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const auto centre = r.getCentre();
    const float angle = start + position * (end - start);
    const auto colour = slider.isMouseOverOrDragging() ? mint.brighter (0.18f) : mint;
    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, start, end, true);
    g.setColour (border.brighter (0.08f));
    g.strokePath (track, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, start, angle, true);
    if (slider.isMouseOverOrDragging())
    {
        g.setColour (colour.withAlpha (0.10f));
        g.strokePath (value, juce::PathStrokeType (8.0f));
    }
    g.setColour (colour);
    g.strokePath (value, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto face = juce::Rectangle<float> (radius * 1.60f, radius * 1.60f).withCentre (centre);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (face.translated (0.0f, 2.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35444a), face.getTopLeft(), juce::Colour (0xff202c31), face.getBottomRight(), false));
    g.fillEllipse (face);
    g.setColour (juce::Colour (0xff45555a));
    g.drawEllipse (face.reduced (0.5f), 0.8f);
    g.setColour (ink);
    g.drawLine ({ centre.getPointOnCircumference (radius * 0.40f, angle), centre.getPointOnCircumference (radius * 0.65f, angle) }, 2.0f);
}
void SignatureLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& colour, bool hover, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? colour.darker (0.15f) : hover ? colour.brighter (0.14f) : colour);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (hover ? mint.withAlpha (0.6f) : border.brighter (0.15f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}
void SignatureLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool hover, bool)
{
    const bool on = button.getToggleState();
    auto r = button.getLocalBounds().toFloat().reduced (1.0f, 3.0f);
    g.setColour (on ? mint.withAlpha (0.12f) : background.withAlpha (0.4f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? mint.withAlpha (0.65f) : hover ? muted : border);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (on ? mint : muted.withAlpha (0.5f));
    g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
    g.setColour (on ? ink : muted);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (button.getButtonText(), r.withTrimmedLeft (22.0f).withTrimmedRight (4.0f), juce::Justification::centredLeft);
}
void SignatureLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& combo)
{
    const auto r = juce::Rectangle<float> (static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);
    g.setColour (background);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (combo.isMouseOver() ? mint.withAlpha (0.5f) : border);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const auto x = static_cast<float> (width) - 16.0f, y = static_cast<float> (height) * 0.5f;
    arrow.startNewSubPath (x - 3.0f, y - 1.5f); arrow.lineTo (x, y + 1.5f); arrow.lineTo (x + 3.0f, y - 1.5f);
    g.setColour (muted); g.strokePath (arrow, juce::PathStrokeType (1.3f));
}
juce::Font SignatureLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::Font (juce::FontOptions (12.0f)); }
juce::Font SignatureLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::Font (juce::FontOptions (11.0f, juce::Font::bold)); }

void SignatureMaskPreview::refresh()
{
    // This is the source mask, not a measured audio spectrogram. Drawing stays on the UI thread.
    juce::Image::BitmapData bitmap (maskImage, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < maskImage.getHeight(); ++y)
        for (int x = 0; x < maskImage.getWidth(); ++x)
        {
            const float value = juce::jlimit (0.0f, 1.0f, processor.getMaskValue (static_cast<float> (x) / 255.0f, 1.0f - static_cast<float> (y) / 127.0f));
            const auto colour = background.interpolatedWith (mint, juce::jlimit (0.0f, 1.0f, value * 1.6f))
                                          .interpolatedWith (juce::Colour (0xfff1d6a5), juce::jmax (0.0f, value - 0.62f) * 2.0f);
            bitmap.setPixelColour (x, y, colour);
        }
    repaint();
}
void SignatureMaskPreview::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (background); g.fillRoundedRectangle (r, 6.0f);
    auto plot = r.reduced (12.0f, 8.0f).withTrimmedBottom (18.0f);
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    g.drawImage (maskImage, plot, juce::RectanglePlacement::stretchToFit);
    g.setColour (juce::Colour (0xff8cb6ad).withAlpha (0.07f));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine (juce::roundToInt (plot.getX() + plot.getWidth() * static_cast<float> (i) / 8.0f), plot.getY(), plot.getBottom());
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (plot.getY() + plot.getHeight() * static_cast<float> (i) / 4.0f), plot.getX(), plot.getRight());
    const float scanCursor = plot.getX() + plot.getWidth() * juce::jlimit (0.0f, 1.0f, processor.getEngravingCyclePosition());
    g.setGradientFill (juce::ColourGradient (mint.withAlpha (0.0f), scanCursor - 25.0f, plot.getY(), mint.withAlpha (0.14f), scanCursor, plot.getY(), false));
    g.fillRect (juce::Rectangle<float> (scanCursor - 25.0f, plot.getY(), 25.0f, plot.getHeight()).getIntersection (plot));
    g.setColour (mint.withAlpha (0.8f)); g.drawLine (scanCursor, plot.getY(), scanCursor, plot.getBottom(), 1.0f);
    g.setColour (muted); g.setFont (juce::FontOptions (9.0f));
    g.drawText ("0", 12, getHeight() - 22, 20, 16, juce::Justification::left);
    g.drawText ("ONE ENGRAVING CYCLE", 35, getHeight() - 22, getWidth() - 70, 16, juce::Justification::centred);
    g.drawText ("1", getWidth() - 32, getHeight() - 22, 20, 16, juce::Justification::right);
}
void SignatureMeter::update (float linear)
{
    const auto target = juce::Decibels::gainToDecibels (juce::jmax (0.0f, linear), -90.0f);
    levelDb = target > levelDb ? target : juce::jmax (target, levelDb - 1.5f);
    repaint();
}
void SignatureMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold)); g.setColour (muted);
    g.drawText (name, r.removeFromTop (18), juce::Justification::left);
    auto valueRow = r.removeFromTop (27);
    g.setColour (ink); g.setFont (juce::FontOptions (23.0f));
    g.drawText (dbText (levelDb), valueRow.withTrimmedRight (35), juce::Justification::left);
    g.setFont (juce::FontOptions (10.0f)); g.setColour (muted);
    g.drawText ("dBFS", valueRow, juce::Justification::right);
    auto bar = r.removeFromTop (6).toFloat();
    g.setColour (border); g.fillRoundedRectangle (bar, 2.0f);
    auto fill = bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, (levelDb + 60.0f) / 60.0f));
    if (fill.getWidth() > 0.0f)
    {
        g.setGradientFill (juce::ColourGradient (accent.darker (0.25f), bar.getTopLeft(), accent, bar.getTopRight(), false));
        g.fillRoundedRectangle (fill, 2.0f);
        if (levelDb > -1.0f) { g.setColour (amber); g.fillRect (bar.withTrimmedLeft (bar.getWidth() - 3.0f)); }
    }
}

void SpectralCarverAudioProcessorEditor::addKnob (const char* id, const juce::String& name,
                                                 const juce::String& help, bool percent)
{
    auto* slider = sliders.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
    slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 20);
    slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider->setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider->setColour (juce::Slider::textBoxTextColourId, ink);
    slider->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider->setTooltip (help); slider->setName (name); slider->setTitle (name);
    slider->setScrollWheelEnabled (false); slider->setNumDecimalPlacesToDisplay (2);
    addAndMakeVisible (slider);
    auto* label = sliderLabels.add (new juce::Label ({}, name));
    labelStyle (*label); label->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
    sliderAttachments.add (new SliderAttachment (processor.getParameters(), id, *slider));
    if (percent)
    {
        slider->textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + " %"; };
        slider->valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() / 100.0; };
        slider->updateText();
    }
    else if (juce::String (id).containsIgnoreCase ("Frequency")) slider->setTextValueSuffix (" Hz");
    else if (juce::String (id).endsWith ("Db")) slider->setTextValueSuffix (" dB");
    else if (juce::String (id) == ParamIds::loopSeconds) slider->setTextValueSuffix (" s");
}
void SpectralCarverAudioProcessorEditor::addCombo (const char* id, const juce::String& name,
                                                  const juce::StringArray& items, const juce::String& help)
{
    auto* combo = combos.add (new juce::ComboBox());
    combo->addItemList (items, 1); combo->setTooltip (help); combo->setTitle (name);
    addAndMakeVisible (combo);
    auto* label = comboLabels.add (new juce::Label ({}, name));
    labelStyle (*label, 10.0f); addAndMakeVisible (label);
    comboAttachments.add (new ComboAttachment (processor.getParameters(), id, *combo));
}
void SpectralCarverAudioProcessorEditor::addToggle (const char* id, const juce::String& name, const juce::String& help)
{
    auto* toggle = toggles.add (new juce::ToggleButton (name));
    toggle->setTooltip (help); addAndMakeVisible (toggle);
    toggleAttachments.add (new ButtonAttachment (processor.getParameters(), id, *toggle));
}
SpectralCarverAudioProcessorEditor::SpectralCarverAudioProcessorEditor (SpectralCarverAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), maskPreview (p)
{
    setLookAndFeel (&theme);
    addCombo (ParamIds::mode, "PROCESS", { "Cut", "Emboss", "Noise-fill" }, "Cut attenuates. Emboss changes local contrast. Noise-fill adds sound and can be more audible.");
    addCombo (ParamIds::stereoMode, "PLACEMENT", { "Linked stereo", "Mid only", "Side only", "Dual mono" }, "Linked stereo applies matching spectral gain to both channels.");
    addCombo (ParamIds::frequencyMapping, "MAPPING", { "Linear", "Logarithmic", "Mel-like" }, "Maps image height into the selected frequency range.");
    addCombo (ParamIds::audition, "LISTEN", { "Normal", "Mask", "Removed", "Added", "Difference", "Mid", "Side" }, "Audition processing contributions or the processed Mid/Side signal. Return to Normal for export.");
    addCombo (ParamIds::quality, "FFT SIZE", { "1024", "2048", "4096", "8192", "16384" }, "Larger transforms improve frequency resolution and increase CPU use. Host delay remains 16384 samples.");
    addCombo (ParamIds::overlap, "OVERLAP", { "4x", "8x", "16x" }, "Higher overlap increases processing load.");

    addKnob (ParamIds::intensity, "Intensity", "Overall engraving strength.", true);
    addKnob (ParamIds::mix, "Wet mix", "Balance processed and delayed original audio. Start around 10 to 25 percent.", true);
    addKnob (ParamIds::loopSeconds, "Cycle length", "Time taken to read one complete mask from left to right.");
    addKnob (ParamIds::detail, "Detail", "Controls the amount of fine mask detail.", true);
    addKnob (ParamIds::transparency, "Transparency", "Reduces engraving strength. This control does not guarantee inaudibility.", true);
    addKnob (ParamIds::adaptive, "Adaptive", "Reduces changes according to local signal energy.", true);
    addKnob (ParamIds::lowerFrequency, "Lower bound", "Lowest engraving frequency.");
    addKnob (ParamIds::upperFrequency, "Upper bound", "Highest engraving frequency, bounded by the sample rate.");
    addKnob (ParamIds::maximumCutDb, "Maximum cut", "Maximum per-bin attenuation before adaptive scaling.");
    addKnob (ParamIds::maximumBoostDb, "Maximum boost", "Maximum per-bin boost for Emboss mode.");
    addKnob (ParamIds::frequencySmoothing, "Frequency smooth", "Softens changes between neighboring frequency bins.", true);
    addKnob (ParamIds::timeSmoothing, "Time smooth", "Softens changes between successive spectral frames.", true);
    addKnob (ParamIds::transientProtection, "Transient protect", "Reduces engraving during sudden changes in signal energy.", true);
    addKnob (ParamIds::tonalProtection, "Tonal protect", "Reduces changes near prominent tonal components.", true);
    addKnob (ParamIds::maskContrast, "Mask contrast", "Controls grayscale mask contrast.", true);
    addKnob (ParamIds::threshold, "Mask threshold", "Suppresses mask values below the threshold.", true);
    addKnob (ParamIds::inputGainDb, "Input gain", "Level entering spectral processing.");
    addKnob (ParamIds::outputGainDb, "Output gain", "Final output level. Use an external true-peak meter for mastering.");

    addToggle (ParamIds::invertMask, "Invert mask", "Invert the engraving mask.");
    addToggle (ParamIds::lowProtection, "Protect lows", "Reduce engraving in low-frequency content.");
    addToggle (ParamIds::highProtection, "Protect highs", "Reduce engraving near the upper frequency range.");
    addToggle (ParamIds::safeMode, "Safe mode", "Use conservative per-bin processing limits and sample-peak safety. Not a true-peak limiter.");
    addToggle (ParamIds::bypass, "Bypass", "Fade to delayed dry audio while preserving host timing.");

    textEditor.setMultiLine (true, true); textEditor.setReturnKeyStartsNewLine (true);
    textEditor.setFont (juce::FontOptions (21.0f)); textEditor.setText (processor.getMaskText(), false);
    textEditor.setIndents (12, 10); textEditor.setTitle ("Signature text");
    textEditor.setTextToShowWhenEmpty ("Your signature", muted); addAndMakeVisible (textEditor);
    applyText.setColour (juce::TextButton::buttonColourId, mint);
    applyText.setColour (juce::TextButton::textColourOffId, background);
    applyText.onClick = [this] { processor.setTextMask (textEditor.getText()); transientStatus = "Rendering text mask..."; statusFrames = 45; };
    loadImage.onClick = [this] { chooseImageMask(); };
    resetButton.setClickingTogglesState (true);
    resetAttachment = std::make_unique<ButtonAttachment> (processor.getParameters(), ParamIds::resetCycle, resetButton);
    resetButton.setTooltip ("Restart the mask at the beginning of its cycle.");
    const std::array<juce::Component*, 10> components { &applyText, &loadImage, &resetButton, &maskPreview,
        &inputMeter, &outputMeter, &deltaMeter, &sourceLabel, &peakLabel, &statusLabel };
    for (auto* component : components) addAndMakeVisible (component);
    labelStyle (sourceLabel, 10.0f); labelStyle (peakLabel, 10.0f); labelStyle (statusLabel, 10.0f);
    setSize (1180, 800);
    maskPreview.refresh(); startTimerHz (30);
}
SpectralCarverAudioProcessorEditor::~SpectralCarverAudioProcessorEditor()
{
    stopTimer(); fileChooser.reset(); setLookAndFeel (nullptr);
}
void SpectralCarverAudioProcessorEditor::timerCallback()
{
    inputMeter.update (processor.getInputMeter()); outputMeter.update (processor.getOutputMeter()); deltaMeter.update (processor.getDeltaMeter());
    maskPreview.repaint();
    peakLabel.setText ("Peak hold  " + dbText (juce::Decibels::gainToDecibels (processor.getTruePeakMeter(), -90.0f)) + " dBFS", juce::dontSendNotification);
    if (++frame % 5 == 0)
    {
        maskPreview.refresh(); sourceLabel.setText (processor.getMaskSourceDescription(), juce::dontSendNotification);
        const bool additive = combos[0]->getSelectedItemIndex() == 2;
        auto note = additive ? "Noise-fill adds sound. Check the difference before export." : "Spectral engraving is program-dependent. Audition the difference and check the rendered master.";
        statusLabel.setText (statusFrames > 0 ? transientStatus : note, juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, additive || statusFrames > 0 ? amber : muted);
    }
    if (statusFrames > 0) --statusFrames;
}
void SpectralCarverAudioProcessorEditor::chooseImageMask()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Import signature image", juce::File(), "*.png;*.jpg;*.jpeg;*.gif");
    juce::Component::SafePointer<SpectralCarverAudioProcessorEditor> safe (this);
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe] (const juce::FileChooser& chooser) { if (safe != nullptr && chooser.getResult().existsAsFile()) safe->importImage (chooser.getResult()); });
}
void SpectralCarverAudioProcessorEditor::importImage (const juce::File& file)
{
    const bool accepted = processor.loadImageMask (file);
    transientStatus = accepted ? "Rendering image mask..." : "Import failed. Use PNG, JPEG or GIF up to 8 MiB and 4096 x 4096 pixels.";
    statusFrames = accepted ? 60 : 240; statusLabel.setText (transientStatus, juce::dontSendNotification);
}
bool SpectralCarverAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && juce::File (files[0]).hasFileExtension ("png;jpg;jpeg;gif");
}
void SpectralCarverAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    draggingImage = false; if (isInterestedInFileDrag (files)) importImage (juce::File (files[0])); repaint();
}
void SpectralCarverAudioProcessorEditor::fileDragEnter (const juce::StringArray&, int, int) { draggingImage = true; repaint(); }
void SpectralCarverAudioProcessorEditor::fileDragExit (const juce::StringArray&) { draggingImage = false; repaint(); }

void SpectralCarverAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a262a), 0.0f, 0.0f, background, 0.0f, 260.0f, false));
    g.fillAll();
    g.setColour (mint);
    for (int i = 0; i < 7; ++i)
    {
        const float h = 12.0f + static_cast<float> ((i * 7) % 5) * 5.0f;
        g.fillRoundedRectangle (23.0f + static_cast<float> (i) * 5.0f, 46.0f - h * 0.5f, 2.0f, h, 1.0f);
    }
    g.setColour (ink); g.setFont (juce::FontOptions (25.0f, juce::Font::bold));
    g.drawText ("Spectral Signature", 75, 19, 400, 34, juce::Justification::left);
    g.setColour (muted); g.setFont (juce::FontOptions (10.0f));
    g.drawText ("LAZIRKO RECORDS  /  SPECTRAL ENGRAVING", 77, 52, 430, 18, juce::Justification::left);
    g.setColour (border); g.drawHorizontalLine (83, 20.0f, static_cast<float> (getWidth() - 20));
    card (g, sourcePanel, "01", "SOURCE"); card (g, previewPanel, "02", "ENGRAVING MASK"); card (g, meterPanel, "03", "SIGNAL");
    card (g, controlPanels[0], "A", "CHARACTER"); card (g, controlPanels[1], "B", "SPECTRUM"); card (g, controlPanels[2], "C", "PROTECTION & OUTPUT");
    g.setFont (juce::FontOptions (10.0f)); g.setColour (muted);
    g.drawText ("Text or image. Brighter pixels engrave more.", sourcePanel.getX() + 16, sourcePanel.getY() + 38, sourcePanel.getWidth() - 32, 20, juce::Justification::left);
    g.drawText ("Drop a PNG, JPEG or GIF anywhere", sourcePanel.getX() + 16, sourcePanel.getBottom() - 33, sourcePanel.getWidth() - 32, 20, juce::Justification::left);
    g.drawText ("MASK PREVIEW  /  HIGH FREQUENCIES AT TOP", previewPanel.getX() + 16, previewPanel.getY() + 38, previewPanel.getWidth() - 32, 17, juce::Justification::left);
    g.drawText ("SAMPLE PEAK METERS", meterPanel.getX() + 16, meterPanel.getY() + 36, meterPanel.getWidth() - 32, 17, juce::Justification::left);
    if (draggingImage)
    {
        g.setColour (mint.withAlpha (0.7f)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (7.0f), 14.0f, 2.0f);
    }
}
void SpectralCarverAudioProcessorEditor::placeKnob (int index, juce::Rectangle<int> cell)
{
    sliderLabels[index]->setBounds (cell.removeFromTop (19)); sliders[index]->setBounds (cell.reduced (5, 0));
}
void SpectralCarverAudioProcessorEditor::placeCombo (int index, juce::Rectangle<int> cell)
{
    comboLabels[index]->setBounds (cell.removeFromTop (17)); combos[index]->setBounds (cell.reduced (1, 0));
}
void SpectralCarverAudioProcessorEditor::resized()
{
    sourcePanel = { 20, 98, 270, 270 }; previewPanel = { 304, 98, 620, 270 }; meterPanel = { 938, 98, 222, 270 };
    textEditor.setBounds (sourcePanel.getX() + 16, sourcePanel.getY() + 68, sourcePanel.getWidth() - 32, 115);
    applyText.setBounds (sourcePanel.getX() + 16, sourcePanel.getY() + 197, 114, 30);
    loadImage.setBounds (sourcePanel.getX() + 139, sourcePanel.getY() + 197, 114, 30);
    maskPreview.setBounds (previewPanel.getX() + 16, previewPanel.getY() + 61, previewPanel.getWidth() - 32, 160);
    sourceLabel.setBounds (previewPanel.getX() + 16, previewPanel.getBottom() - 36, previewPanel.getWidth() - 160, 22);
    resetButton.setBounds (previewPanel.getRight() - 134, previewPanel.getBottom() - 40, 118, 27);
    inputMeter.setBounds (meterPanel.getX() + 16, meterPanel.getY() + 62, meterPanel.getWidth() - 32, 57);
    outputMeter.setBounds (meterPanel.getX() + 16, meterPanel.getY() + 123, meterPanel.getWidth() - 32, 57);
    deltaMeter.setBounds (meterPanel.getX() + 16, meterPanel.getY() + 184, meterPanel.getWidth() - 32, 57);
    peakLabel.setBounds (meterPanel.getX() + 16, meterPanel.getBottom() - 26, meterPanel.getWidth() - 32, 20);
    for (int group = 0; group < 3; ++group)
    {
        auto& area = controlPanels[static_cast<size_t> (group)]; area = { 20 + group * 385, 384, 370, 330 };
        auto inside = area.reduced (14, 0).withTrimmedTop (42);
        auto selections = inside.removeFromTop (47);
        placeCombo (group * 2, selections.removeFromLeft (166)); selections.removeFromLeft (10); placeCombo (group * 2 + 1, selections);
        inside.removeFromTop (12);
        for (int item = 0; item < 6; ++item)
            placeKnob (group * 6 + item, { inside.getX() + (item % 3) * 114, inside.getY() + (item / 3) * 109, 114, 103 });
    }
    toggles[0]->setBounds (20, 724, 122, 34); toggles[1]->setBounds (151, 724, 130, 34); toggles[2]->setBounds (290, 724, 130, 34);
    toggles[3]->setBounds (936, 29, 108, 34); toggles[4]->setBounds (1055, 29, 105, 34);
    statusLabel.setBounds (18, 766, 1138, 23);
}
