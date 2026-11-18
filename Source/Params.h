#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// All NEON TITAN parameters in one place.
namespace Params
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    using Range  = juce::NormalisableRange<float>;

    inline const juce::StringArray filterTypes { "Low Pass", "Band Pass", "High Pass" };
    inline const juce::StringArray lfoShapes   { "Sine", "Triangle", "Saw", "Square", "Random" };
    inline const juce::StringArray lfoSyncs    { "Free", "1/1", "1/2", "1/4", "1/8", "1/16", "1/4 T", "1/8 T" };
    inline const juce::StringArray subOctaves  { "-1 Oct", "-2 Oct" };
    inline const juce::StringArray chordTypes  { "Off", "Major", "Minor", "Maj7", "Min7", "Dom7", "Maj9", "Min9", "Sus2", "Sus4", "Add9", "Min11", "Power", "Octave" };
    inline const juce::StringArray delayTimes  { "1/4", "1/8", "1/8 D", "1/16", "1/4 D", "1/8 T" };

    inline Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    inline void addOsc (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p, const juce::String& id, const juce::String& name, float level)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_wave", 1 }, name + " Wave", Range (0.0f, 3.0f), 2.0f));
        p.push_back (std::make_unique<juce::AudioParameterInt>   (juce::ParameterID { id + "_oct", 1 },  name + " Octave", -3, 3, 0));
        p.push_back (std::make_unique<juce::AudioParameterInt>   (juce::ParameterID { id + "_semi", 1 }, name + " Semitone", -12, 12, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_fine", 1 }, name + " Fine", Range (-100.0f, 100.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_level", 1 }, name + " Level", Range (0.0f, 1.0f), level));
        p.push_back (std::make_unique<juce::AudioParameterInt>   (juce::ParameterID { id + "_uni", 1 },  name + " Unison", 1, 7, 1));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_detune", 1 }, name + " Detune", Range (0.0f, 1.0f), 0.25f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_spread", 1 }, name + " Spread", Range (0.0f, 1.0f), 0.8f));
    }

    inline void addEnv (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& p, const juce::String& id, const juce::String& name,
                        float a, float d, float s, float r)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_a", 1 }, name + " Attack",  skewed (0.001f, 5.0f, 0.3f), a));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_d", 1 }, name + " Decay",   skewed (0.001f, 5.0f, 0.4f), d));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_s", 1 }, name + " Sustain", Range (0.0f, 1.0f), s));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id + "_r", 1 }, name + " Release", skewed (0.001f, 8.0f, 0.5f), r));
    }

    inline Layout createLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

        addOsc (p, "oscA", "Osc A", 0.8f);
        addOsc (p, "oscB", "Osc B", 0.0f);

        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "sub_level", 1 }, "Sub Level", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "sub_oct", 1 }, "Sub Octave", subOctaves, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "noise_level", 1 }, "Noise", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "click", 1 }, "Hammer Click", Range (0.0f, 1.0f), 0.0f));

        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "flt_type", 1 }, "Filter Type", filterTypes, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "flt_cutoff", 1 }, "Cutoff", skewed (20.0f, 20000.0f, 1200.0f), 8000.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "flt_res", 1 }, "Resonance", Range (0.0f, 1.0f), 0.15f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "flt_drive", 1 }, "Drive", Range (0.0f, 1.0f), 0.1f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "flt_env", 1 }, "Env Amount", Range (-1.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "flt_key", 1 }, "Key Track", Range (0.0f, 1.0f), 0.3f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "vel_cut", 1 }, "Velocity > Cutoff", Range (0.0f, 1.0f), 0.0f));

        addEnv (p, "fenv", "Filter Env", 0.005f, 0.4f, 0.3f, 0.4f);
        addEnv (p, "amp",  "Amp",        0.003f, 0.3f, 0.9f, 0.25f);

        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo_shape", 1 }, "LFO Shape", lfoShapes, 0));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo_sync", 1 }, "LFO Sync", lfoSyncs, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "lfo_rate", 1 }, "LFO Rate", skewed (0.05f, 20.0f, 2.0f), 2.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "lfo_cut", 1 }, "LFO > Cutoff", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "lfo_pitch", 1 }, "LFO > Pitch", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "lfo_amp", 1 }, "LFO > Volume", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { "lfo_retrig", 1 }, "LFO Retrigger", true));

        p.push_back (std::make_unique<juce::AudioParameterBool>   (juce::ParameterID { "mono", 1 }, "Mono", false));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "chord", 1 }, "Chord Mode", chordTypes, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "glide", 1 }, "Glide", skewed (0.0f, 1.0f, 0.15f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "pitch_env", 1 }, "Pitch Env", Range (-24.0f, 24.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "pitch_dec", 1 }, "Pitch Decay", skewed (0.005f, 2.0f, 0.1f), 0.08f));

        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "fx_dist", 1 }, "Distortion", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "fx_chorus", 1 }, "Chorus", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "dly_mix", 1 }, "Delay", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "dly_time", 1 }, "Delay Time", delayTimes, 1));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "dly_fb", 1 }, "Delay Feedback", Range (0.0f, 0.9f), 0.35f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "fx_reverb", 1 }, "Reverb", Range (0.0f, 1.0f), 0.15f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "fx_size", 1 }, "Room Size", Range (0.0f, 1.0f), 0.6f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "fx_width", 1 }, "Width", Range (0.0f, 2.0f), 1.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "master", 1 }, "Master", Range (-24.0f, 6.0f), -6.0f));

        // Macros: one knob moves several things at once (great on Maschine's hardware knobs)
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "m_size", 1 }, "Macro SIZE", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "m_growl", 1 }, "Macro GROWL", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "m_wobble", 1 }, "Macro WOBBLE", Range (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat>  (juce::ParameterID { "m_bright", 1 }, "Macro BRIGHT", Range (-1.0f, 1.0f), 0.0f));

        return { p.begin(), p.end() };
    }
}
