#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

// Factory presets. Any parameter not listed is reset to its default.
// Values are in real units (Hz, seconds, 0-1, etc.).
#include "PresetGenerator.h"
#include "PresetGains.h"

struct TitanPreset
{
    juce::String name, category, subtype;
    std::vector<std::pair<const char*, float>> values;
};

struct ClassicPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline std::vector<TitanPreset> buildPresetLibrary();

inline const std::vector<TitanPreset>& getFactoryPresets()
{
    static const std::vector<TitanPreset> library = buildPresetLibrary();
    return library;
}

inline const juce::StringArray& getPresetCategories()
{
    static const juce::StringArray cats { "Classic", "808", "Bass", "Lead", "Piano", "Chords", "Keys", "Pluck", "Pad", "FX" };
    return cats;
}

inline std::vector<ClassicPreset> getClassicPresets()
{
    return {
        { "Init", {} },

        { "Titan Reese", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 5 }, { "oscA_detune", 0.35f }, { "oscA_level", 0.8f },
            { "oscB_wave", 2.0f }, { "oscB_oct", -1 }, { "oscB_uni", 3 }, { "oscB_detune", 0.5f }, { "oscB_level", 0.6f },
            { "sub_level", 0.5f }, { "flt_cutoff", 900.0f }, { "flt_res", 0.25f }, { "flt_drive", 0.45f }, { "flt_key", 0.2f },
            { "lfo_shape", 0 }, { "lfo_rate", 0.25f }, { "lfo_cut", 0.15f }, { "lfo_retrig", 0 },
            { "amp_a", 0.005f }, { "amp_s", 1.0f }, { "amp_r", 0.2f }, { "mono", 1 }, { "glide", 0.06f },
            { "fx_dist", 0.25f }, { "fx_chorus", 0.2f }, { "fx_reverb", 0.08f } } },

        { "Neon Wobble 1/8", {
            { "oscA_wave", 2.3f }, { "oscA_uni", 3 }, { "oscA_detune", 0.2f }, { "oscB_wave", 3.0f }, { "oscB_oct", -1 }, { "oscB_level", 0.5f },
            { "sub_level", 0.6f }, { "flt_cutoff", 250.0f }, { "flt_res", 0.55f }, { "flt_drive", 0.6f },
            { "lfo_shape", 0 }, { "lfo_sync", 4 }, { "lfo_cut", 0.85f }, { "lfo_retrig", 1 },
            { "amp_s", 1.0f }, { "amp_r", 0.15f }, { "mono", 1 }, { "glide", 0.03f },
            { "fx_dist", 0.45f }, { "fx_reverb", 0.05f }, { "m_wobble", 0.0f } } },

        { "Cyber Lead", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 7 }, { "oscA_detune", 0.28f }, { "oscA_spread", 1.0f },
            { "oscB_wave", 3.0f }, { "oscB_oct", 1 }, { "oscB_level", 0.25f }, { "oscB_uni", 3 },
            { "flt_cutoff", 3500.0f }, { "flt_res", 0.2f }, { "flt_env", 0.35f }, { "fenv_d", 0.6f }, { "fenv_s", 0.2f },
            { "lfo_pitch", 0.08f }, { "lfo_rate", 5.5f }, { "lfo_retrig", 0 },
            { "amp_a", 0.01f }, { "amp_s", 0.85f }, { "amp_r", 0.35f }, { "glide", 0.05f },
            { "fx_chorus", 0.25f }, { "fx_reverb", 0.3f }, { "fx_size", 0.75f }, { "fx_width", 1.3f } } },

        { "Sub 808 Glide", {
            { "oscA_wave", 0.0f }, { "oscA_level", 1.0f }, { "oscA_oct", -1 },
            { "oscB_wave", 1.0f }, { "oscB_oct", -1 }, { "oscB_level", 0.15f },
            { "flt_cutoff", 2500.0f }, { "flt_drive", 0.55f }, { "flt_key", 0.0f },
            { "amp_a", 0.002f }, { "amp_d", 1.6f }, { "amp_s", 0.0f }, { "amp_r", 0.6f },
            { "mono", 1 }, { "glide", 0.09f }, { "fx_dist", 0.3f }, { "fx_reverb", 0.0f }, { "fx_width", 0.0f } } },

        { "Growl Bass", {
            { "oscA_wave", 2.6f }, { "oscA_uni", 3 }, { "oscA_detune", 0.15f }, { "oscB_wave", 1.4f }, { "oscB_semi", 7 }, { "oscB_level", 0.4f },
            { "sub_level", 0.55f }, { "flt_type", 1 }, { "flt_cutoff", 700.0f }, { "flt_res", 0.7f }, { "flt_drive", 0.8f },
            { "lfo_shape", 1 }, { "lfo_sync", 5 }, { "lfo_cut", 0.6f }, { "lfo_retrig", 1 },
            { "amp_s", 1.0f }, { "amp_r", 0.1f }, { "mono", 1 }, { "fx_dist", 0.65f }, { "fx_reverb", 0.0f } } },

        { "Supersaw Anthem", {
            { "oscA_wave", 2.0f }, { "oscA_uni", 7 }, { "oscA_detune", 0.42f }, { "oscA_spread", 1.0f },
            { "oscB_wave", 2.0f }, { "oscB_oct", -1 }, { "oscB_uni", 7 }, { "oscB_detune", 0.38f }, { "oscB_level", 0.55f },
            { "flt_cutoff", 6000.0f }, { "flt_res", 0.1f }, { "amp_a", 0.02f }, { "amp_s", 0.9f }, { "amp_r", 0.6f },
            { "fx_chorus", 0.3f }, { "fx_reverb", 0.35f }, { "fx_size", 0.85f }, { "fx_width", 1.5f }, { "master", -9.0f } } },

        { "Night Drive Pad", {
            { "oscA_wave", 1.6f }, { "oscA_uni", 5 }, { "oscA_detune", 0.3f }, { "oscB_wave", 0.5f }, { "oscB_oct", 1 }, { "oscB_level", 0.35f },
            { "noise_level", 0.06f }, { "flt_cutoff", 1600.0f }, { "flt_res", 0.2f }, { "lfo_shape", 0 }, { "lfo_rate", 0.18f }, { "lfo_cut", 0.3f }, { "lfo_retrig", 0 },
            { "amp_a", 1.2f }, { "amp_d", 1.0f }, { "amp_s", 0.85f }, { "amp_r", 2.8f },
            { "fx_chorus", 0.45f }, { "fx_reverb", 0.6f }, { "fx_size", 0.92f }, { "fx_width", 1.6f } } },

        { "Pluck City", {
            { "oscA_wave", 2.2f }, { "oscA_uni", 3 }, { "oscA_detune", 0.18f }, { "oscB_wave", 3.0f }, { "oscB_oct", 1 }, { "oscB_level", 0.2f },
            { "flt_cutoff", 500.0f }, { "flt_res", 0.3f }, { "flt_env", 0.65f }, { "fenv_a", 0.001f }, { "fenv_d", 0.25f }, { "fenv_s", 0.0f },
            { "amp_a", 0.001f }, { "amp_d", 0.45f }, { "amp_s", 0.0f }, { "amp_r", 0.35f },
            { "fx_reverb", 0.35f }, { "fx_size", 0.7f }, { "fx_width", 1.3f } } },

        { "Acid Line", {
            { "oscA_wave", 2.0f }, { "oscB_level", 0.0f }, { "flt_cutoff", 300.0f }, { "flt_res", 0.85f }, { "flt_drive", 0.5f },
            { "flt_env", 0.7f }, { "fenv_a", 0.001f }, { "fenv_d", 0.22f }, { "fenv_s", 0.0f },
            { "amp_a", 0.001f }, { "amp_s", 0.8f }, { "amp_r", 0.08f }, { "mono", 1 }, { "glide", 0.07f }, { "fx_dist", 0.35f }, { "fx_reverb", 0.1f } } },

        { "Hyper Stab", {
            { "oscA_wave", 3.0f }, { "oscA_uni", 5 }, { "oscA_detune", 0.25f }, { "oscB_wave", 2.0f }, { "oscB_semi", 7 }, { "oscB_uni", 5 }, { "oscB_level", 0.6f },
            { "flt_cutoff", 1800.0f }, { "flt_env", 0.5f }, { "fenv_d", 0.18f }, { "fenv_s", 0.1f },
            { "amp_a", 0.001f }, { "amp_d", 0.3f }, { "amp_s", 0.0f }, { "amp_r", 0.25f },
            { "fx_dist", 0.2f }, { "fx_reverb", 0.4f }, { "fx_size", 0.8f }, { "fx_width", 1.4f } } },

        { "Riser Noise", {
            { "oscA_level", 0.2f }, { "noise_level", 0.9f }, { "flt_type", 1 }, { "flt_cutoff", 400.0f }, { "flt_res", 0.6f },
            { "flt_env", 0.9f }, { "fenv_a", 4.0f }, { "fenv_s", 1.0f },
            { "amp_a", 2.5f }, { "amp_s", 1.0f }, { "amp_r", 1.5f }, { "fx_reverb", 0.55f }, { "fx_size", 0.9f }, { "fx_width", 1.7f } } }
    };
}

inline std::vector<TitanPreset> buildPresetLibrary()
{
    std::vector<TitanPreset> out;
    for (auto& c : getClassicPresets())
        out.push_back ({ c.name, "Classic", "Classic", c.values });

    std::map<juce::String, int> usedNames;   // across ALL categories, so every name is unique
    const auto recipes = getRecipes();
    const auto& words = generatorWords();

    for (size_t ri = 0; ri < recipes.size(); ++ri)
    {
        const auto& rc = recipes[ri];
        const auto& w = words.at (rc.category);
        for (int i = 0; i < rc.count; ++i)
        {
            juce::Random rnd ((juce::int64) (ri + 1) * 1000003 + i * 7919 + 424242);
            TitanPreset p;
            p.category = rc.category;
            p.subtype = rc.subtype;
            p.values = rc.base;
            for (const auto& [id, lo, hi] : rc.ranges)
                p.values.push_back ({ id, lo + rnd.nextFloat() * (hi - lo) });
            for (const auto& [id, options] : rc.picks)
                p.values.push_back ({ id, options[(size_t) rnd.nextInt ((int) options.size())] });

            juce::String name = juce::String (w.adj[(size_t) rnd.nextInt ((int) w.adj.size())]) + " "
                              + w.noun[(size_t) rnd.nextInt ((int) w.noun.size())];
            for (auto& c : out) if (c.category == "Classic") usedNames[c.name] = 1;
            int n = ++usedNames[name];
            juce::String unique = name;
            while (n > 1 && usedNames.count (unique = name + " " + juce::String (n)) > 0) ++n;
            if (n > 1) usedNames[unique] = 1;
            name = unique;
            p.name = name;

            // Loudness match: a measured gain offset per preset (see Tools/Render.cpp)
            const size_t gi = out.size();
            if (gi < (size_t) kPresetGainCount)
                for (auto& [id, v] : p.values)
                    if (juce::String (id) == "master")
                        v = juce::jlimit (-24.0f, 6.0f, v + kPresetGainDb[gi]);

            out.push_back (std::move (p));
        }
    }
    return out;
}
