#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NoirLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class NoireqAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoireqAudioProcessorEditor(NoireqAudioProcessor&);
    ~NoireqAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    struct Control { juce::Slider slider; juce::Label label; };
    NoireqAudioProcessor& audioProcessor;
    NoirLookAndFeel noirLook;
    std::array<std::unique_ptr<Control>, 12> controls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 12> attachments;
    juce::TextButton eqButton { "EQ" }, compButton { "COMP" }, resetButton { "RESET" };
    juce::AudioProcessorValueTreeState::ButtonAttachment eqAttach, compAttach;
    juce::Rectangle<float> graphBounds;
    int draggedBand = -1;
    float inputMeter = 0.0f;
    std::unique_ptr<juce::Drawable> surfaceTexture, brandMark;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoireqAudioProcessorEditor)
};
