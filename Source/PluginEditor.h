#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Neon cyberpunk look: dark panels, glowing cyan/magenta knob arcs.
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class NeonTitanEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NeonTitanEditor (NeonTitanProcessor&);
    ~NeonTitanEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
        juce::Colour colour;
        bool big = false;
    };
    struct Choice
    {
        std::unique_ptr<juce::ComboBox> box;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };
    struct Toggle
    {
        std::unique_ptr<juce::ToggleButton> button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attach;
    };
    struct Item { int kind; int index; };           // 0 = knob, 1 = choice, 2 = toggle
    struct Section { juce::String title; juce::Colour colour; std::vector<Item> items; juce::Rectangle<int> bounds; };

    void addKnob (Section&, const juce::String& id, const juce::String& text, bool big = false);
    void addChoice (Section&, const juce::String& id, const juce::String& text, const juce::StringArray& items);
    void addToggle (Section&, const juce::String& id, const juce::String& text);
    void timerCallback() override;
    void refreshPresetBox();

    NeonTitanProcessor& proc;
    NeonLookAndFeel lnf;
    std::vector<Knob> knobs;
    std::vector<Choice> choices;
    std::vector<Toggle> toggles;
    std::vector<Section> sections;

    void rebuildPresetList();
    void stepPreset (int delta);

    juce::ComboBox categoryBox, presetBox;
    juce::TextEditor searchBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, randomButton { "RANDOM" };
    std::vector<int> filtered;   // preset indices shown in the list
    juce::Random random;
    juce::MidiKeyboardComponent keyboard;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonTitanEditor)
};
