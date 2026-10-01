#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SignatureLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SignatureLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
};

class SignatureMaskPreview final : public juce::Component
{
public:
    explicit SignatureMaskPreview (SpectralCarverAudioProcessor& p) : processor (p) {}
    void refresh();
    void paint (juce::Graphics&) override;
private:
    SpectralCarverAudioProcessor& processor;
    juce::Image maskImage { juce::Image::RGB, 256, 128, true };
};

class SignatureMeter final : public juce::Component
{
public:
    SignatureMeter (juce::String title, juce::Colour colour) : name (std::move (title)), accent (colour) {}
    void update (float);
    void paint (juce::Graphics&) override;
private:
    juce::String name;
    juce::Colour accent;
    float levelDb = -90.0f;
};

class SpectralCarverAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               private juce::Timer,
                                               public juce::FileDragAndDropTarget
{
public:
    explicit SpectralCarverAudioProcessorEditor (SpectralCarverAudioProcessor&);
    ~SpectralCarverAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    void timerCallback() override;
    void chooseImageMask();
    void importImage (const juce::File&);
    void addKnob (const char*, const juce::String&, const juce::String&, bool percent = false);
    void addCombo (const char*, const juce::String&, const juce::StringArray&, const juce::String&);
    void addToggle (const char*, const juce::String&, const juce::String&);
    void placeKnob (int, juce::Rectangle<int>);
    void placeCombo (int, juce::Rectangle<int>);
    SpectralCarverAudioProcessor& processor;
    SignatureLookAndFeel theme;
    juce::TooltipWindow tooltip { this, 650 };
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> sliderLabels;
    juce::OwnedArray<SliderAttachment> sliderAttachments;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::Label> comboLabels;
    juce::OwnedArray<ComboAttachment> comboAttachments;
    juce::OwnedArray<juce::ToggleButton> toggles;
    juce::OwnedArray<ButtonAttachment> toggleAttachments;
    juce::TextEditor textEditor;
    juce::TextButton applyText { "Apply text" }, loadImage { "Import image" };
    SignatureMaskPreview maskPreview;
    SignatureMeter inputMeter { "INPUT", juce::Colour (0xff82d8c5) };
    SignatureMeter outputMeter { "OUTPUT", juce::Colour (0xff82d8c5) };
    SignatureMeter deltaMeter { "DIFFERENCE", juce::Colour (0xffdfa96b) };
    juce::Label sourceLabel, peakLabel, statusLabel;
    juce::TextButton resetButton { "Restart cycle" };
    std::unique_ptr<ButtonAttachment> resetAttachment;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::Rectangle<int> sourcePanel, previewPanel, meterPanel;
    std::array<juce::Rectangle<int>, 3> controlPanels;
    juce::String transientStatus;
    int statusFrames = 0, frame = 0;
    bool draggingImage = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralCarverAudioProcessorEditor)
};
