#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "TitanVoice.h"
#include <unordered_map>
#include <string>

class NeonTitanProcessor : public juce::AudioProcessor
{
public:
    NeonTitanProcessor();
    ~NeonTitanProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NEON TITAN"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;   // lets the on-screen keyboard play notes

    // Used by the render tool to fake a host tempo
    double fallbackBpm = 120.0;

private:
    // Direct pointers to every parameter, looked up once (no memory allocation on the audio thread)
    std::unordered_map<std::string, std::atomic<float>*> raw;
    std::array<std::array<std::atomic<float>*, 8>, 2> oscRaw {};
    float get (const std::string& id) const { return raw.at (id)->load(); }
    void updateSettings (double bpm);
    void expandChords (juce::MidiBuffer& midi);
    std::array<std::vector<int>, 128> chordHeld;   // notes each key is currently holding in chord mode

    TitanSynth synth;
    VoiceSettings settings;
    int currentProgram = 0;
    double lfoGlobalPhase = 0.0;
    double sr = 44100.0;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    juce::SmoothedValue<float> masterGain, widthSmoothed, distSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonTitanProcessor)
};
