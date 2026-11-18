#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

// Snapshot of every setting a voice needs, refreshed once per audio block by the processor.
struct OscSettings
{
    float wave = 2.0f, level = 0.8f, detune = 0.25f, spread = 0.8f;
    int octave = 0, semi = 0, unison = 1;
    float fine = 0.0f;
};

struct VoiceSettings
{
    OscSettings osc[2];
    float subLevel = 0.0f, noiseLevel = 0.0f;
    int subOctave = 1;
    int filterType = 0;
    float cutoff = 8000.0f, resonance = 0.15f, drive = 0.1f, envAmount = 0.0f, keyTrack = 0.3f;
    juce::ADSR::Parameters ampEnv, filterEnv;
    int lfoShape = 0;
    float lfoHz = 2.0f, lfoCut = 0.0f, lfoPitch = 0.0f, lfoAmp = 0.0f;
    bool lfoRetrig = true;
    double globalLfoPhase = 0.0;
    float glide = 0.0f;
    bool mono = false;
    float pitchEnv = 0.0f, pitchDecay = 0.08f;
    float click = 0.0f, velCut = 0.0f;
};

struct TitanSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class TitanVoice : public juce::SynthesiserVoice
{
public:
    explicit TitanVoice (const VoiceSettings& s) : settings (s)
    {
        for (auto& o : phases)
            for (auto& ph : o)
                ph = random.nextDouble();
    }

    bool canPlaySound (juce::SynthesiserSound* s) override { return dynamic_cast<TitanSound*> (s) != nullptr; }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        if (newRate <= 0) return;
        sr = newRate;
        juce::dsp::ProcessSpec spec { newRate, 512, 2 };
        filter.prepare (spec);
        filter.reset();
        ampAdsr.setSampleRate (newRate);
        filterAdsr.setSampleRate (newRate);
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        note = midiNote;
        vel = velocity;
        targetFreq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
        const bool canGlide = settings.mono && settings.glide > 0.0005f && lastFreq > 0.0;
        currentFreq = canGlide ? lastFreq : targetFreq;
        const double glideSamples = juce::jmax (1.0, settings.glide * sr);
        glideCoef = canGlide ? std::exp (-1.0 / glideSamples) : 0.0;

        if (! wasStolenLegato)
        {
            pitchEnvValue = 1.0f;
            clickValue = 1.0f;      // piano hammer: short noise tick at the start of the note
        }   // 808 punch: pitch starts high and falls (not re-triggered on mono slides)

        if (! wasStolenLegato)
        {
            ampAdsr.reset();
            filterAdsr.reset();
            filter.reset();
        }
        wasStolenLegato = false;

        ampAdsr.setParameters (settings.ampEnv);
        filterAdsr.setParameters (settings.filterEnv);
        ampAdsr.noteOn();
        filterAdsr.noteOn();

        lfoPhase = settings.lfoRetrig ? 0.0 : settings.globalLfoPhase;
        sampleAndHold = random.nextFloat() * 2.0f - 1.0f;
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampAdsr.noteOff();
            filterAdsr.noteOff();
        }
        else
        {
            // In mono mode a new note "steals" this voice: keep the envelope running so there's no click.
            wasStolenLegato = settings.mono;
            if (! wasStolenLegato)
                ampAdsr.reset();
            clearCurrentNote();
        }
    }

    using juce::SynthesiserVoice::renderNextBlock;

    void pitchWheelMoved (int value) override { pitchBend = (value - 8192) / 8192.0f * 2.0f; }
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int start, int num) override
    {
        if (! isVoiceActive())
            return;

        ampAdsr.setParameters (settings.ampEnv);
        filterAdsr.setParameters (settings.filterEnv);
        filter.setType (settings.filterType == 0 ? juce::dsp::StateVariableTPTFilterType::lowpass
                       : settings.filterType == 1 ? juce::dsp::StateVariableTPTFilterType::bandpass
                                                  : juce::dsp::StateVariableTPTFilterType::highpass);
        filter.setResonance (0.5f + settings.resonance * 9.5f);

        // Per-block unison setup
        for (int o = 0; o < 2; ++o)
        {
            const auto& os = settings.osc[o];
            const int n = juce::jlimit (1, maxUnison, os.unison);
            uniCount[(size_t) o] = n;
            const float baseSemis = (float) os.octave * 12.0f + (float) os.semi + os.fine / 100.0f;
            for (int u = 0; u < n; ++u)
            {
                const float pos = n == 1 ? 0.0f : (float) u / (float) (n - 1) * 2.0f - 1.0f;
                const float cents = pos * os.detune * 50.0f;
                uniRatio[(size_t) o][(size_t) u] = std::pow (2.0f, (baseSemis + cents / 100.0f) / 12.0f);
                const float pan = pos * os.spread;
                const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
                uniGainL[(size_t) o][(size_t) u] = std::cos (angle);
                uniGainR[(size_t) o][(size_t) u] = std::sin (angle);
            }
            uniNorm[(size_t) o] = 1.0f / std::sqrt ((float) n);
        }

        const float pitchEnvCoef = std::exp (-1.0f / (juce::jmax (0.001f, settings.pitchDecay) * (float) sr * 0.25f));
        const float driveGain = 1.0f + settings.drive * 12.0f;
        const float driveNorm = 1.0f / std::tanh (driveGain * 0.5f + 0.5f);
        const float subRatio = settings.subOctave == 0 ? 0.5f : 0.25f;
        const float keyOct = settings.keyTrack * (float) (note - 60) / 12.0f
                           + settings.velCut * (vel - 0.85f) * 4.0f;   // harder hits = brighter
        const float clickCoef = std::exp (-1.0f / (0.006f * (float) sr));

        for (int i = 0; i < num; ++i)
        {
            // Glide toward the target pitch
            if (glideCoef > 0.0)
            {
                currentFreq = targetFreq + (currentFreq - targetFreq) * glideCoef;
                if (std::abs (currentFreq - targetFreq) < 0.01) { currentFreq = targetFreq; glideCoef = 0.0; }
            }

            // LFO
            lfoPhase += settings.lfoHz / sr;
            if (lfoPhase >= 1.0) { lfoPhase -= 1.0; sampleAndHold = random.nextFloat() * 2.0f - 1.0f; }
            const float lfo = lfoValue();

            const float penv = settings.pitchEnv * pitchEnvValue;
            pitchEnvValue *= pitchEnvCoef;
            const float pitchMod = std::pow (2.0f, (pitchBend + lfo * settings.lfoPitch * 2.0f + penv) / 12.0f);
            const double freq = currentFreq * pitchMod;

            float l = 0.0f, r = 0.0f;
            for (int o = 0; o < 2; ++o)
            {
                const auto& os = settings.osc[o];
                if (os.level <= 0.0001f) continue;
                float ol = 0.0f, orr = 0.0f;
                const int n = uniCount[(size_t) o];
                for (int u = 0; u < n; ++u)
                {
                    const double inc = freq * uniRatio[(size_t) o][(size_t) u] / sr;
                    auto& ph = phases[(size_t) o][(size_t) u];
                    const float s = morph ((float) ph, (float) inc, os.wave);
                    ph += inc;
                    ph -= std::floor (ph);
                    ol += s * uniGainL[(size_t) o][(size_t) u];
                    orr += s * uniGainR[(size_t) o][(size_t) u];
                }
                const float g = os.level * uniNorm[(size_t) o];
                l += ol * g;
                r += orr * g;
            }

            if (settings.subLevel > 0.0001f)
            {
                const float sub = std::sin ((float) subPhase * juce::MathConstants<float>::twoPi) * settings.subLevel;
                subPhase += freq * subRatio / sr;
                subPhase -= std::floor (subPhase);
                l += sub * 0.7071f;
                r += sub * 0.7071f;
            }

            if (settings.noiseLevel > 0.0001f)
            {
                l += (random.nextFloat() * 2.0f - 1.0f) * settings.noiseLevel * 0.3f;
                r += (random.nextFloat() * 2.0f - 1.0f) * settings.noiseLevel * 0.3f;
            }

            // Drive into the filter
            l = std::tanh (l * driveGain * 0.5f) * driveNorm;
            r = std::tanh (r * driveGain * 0.5f) * driveNorm;

            const float fenv = filterAdsr.getNextSample();
            if ((controlCounter++ & 15) == 0)
            {
                const float octaves = settings.envAmount * 5.0f * fenv + lfo * settings.lfoCut * 4.0f + keyOct;
                const float cutoff = juce::jlimit (20.0f, (float) (sr * 0.45), settings.cutoff * std::pow (2.0f, octaves));
                filter.setCutoffFrequency (cutoff);
            }
            l = filter.processSample (0, l);
            r = filter.processSample (1, r);

            if (settings.click > 0.001f && clickValue > 0.0005f)
            {
                const float c = (random.nextFloat() * 2.0f - 1.0f) * clickValue * settings.click * vel * 0.6f;
                clickValue *= clickCoef;
                l += c;
                r += c;
            }

            const float tremolo = 1.0f - settings.lfoAmp * (0.5f + 0.5f * lfo);
            const float amp = ampAdsr.getNextSample() * vel * tremolo;

            out.addSample (0, start + i, l * amp);
            if (out.getNumChannels() > 1)
                out.addSample (1, start + i, r * amp);

            if (! ampAdsr.isActive())
            {
                lastFreq = currentFreq;
                clearCurrentNote();
                break;
            }
        }
        lastFreq = currentFreq;
    }

private:
    static constexpr int maxUnison = 7;

    // PolyBLEP smooths the sharp edges of saw and square so they don't alias (sound harsh/digital).
    static float polyBlep (float t, float dt)
    {
        if (dt <= 0.0f) return 0.0f;
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    static float wave (int w, float t, float dt)
    {
        switch (w)
        {
            case 0:  return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1:  return 1.0f - 4.0f * std::abs (t - 0.5f);
            case 2:  return (2.0f * t - 1.0f) - polyBlep (t, dt);
            default:
            {
                float sq = t < 0.5f ? 1.0f : -1.0f;
                float t2 = t + 0.5f; t2 -= std::floor (t2);
                return sq + polyBlep (t, dt) - polyBlep (t2, dt);
            }
        }
    }

    // Wave knob morphs smoothly: 0 = sine, 1 = triangle, 2 = saw, 3 = square
    static float morph (float t, float dt, float w)
    {
        w = juce::jlimit (0.0f, 3.0f, w);
        const int a = juce::jmin (2, (int) w);
        const float f = w - (float) a;
        if (f < 0.001f) return wave (a, t, dt);
        if (f > 0.999f) return wave (a + 1, t, dt);
        return wave (a, t, dt) * (1.0f - f) + wave (a + 1, t, dt) * f;
    }

    float lfoValue() const
    {
        const float t = (float) lfoPhase;
        switch (settings.lfoShape)
        {
            case 0:  return std::sin (t * juce::MathConstants<float>::twoPi);
            case 1:  return 1.0f - 4.0f * std::abs (t - 0.5f);
            case 2:  return 1.0f - 2.0f * t;      // falling saw: classic wobble
            case 3:  return t < 0.5f ? 1.0f : -1.0f;
            default: return sampleAndHold;
        }
    }

    const VoiceSettings& settings;
    juce::Random random;
    double sr = 44100.0;
    int note = 60;
    float vel = 1.0f, pitchBend = 0.0f;
    double targetFreq = 440.0, currentFreq = 440.0, lastFreq = 0.0, glideCoef = 0.0;
    bool wasStolenLegato = false;

    std::array<std::array<double, maxUnison>, 2> phases {};
    std::array<std::array<float, maxUnison>, 2> uniRatio {}, uniGainL {}, uniGainR {};
    std::array<int, 2> uniCount { 1, 1 };
    std::array<float, 2> uniNorm { 1.0f, 1.0f };
    double subPhase = 0.0, lfoPhase = 0.0;
    float sampleAndHold = 0.0f;
    float pitchEnvValue = 0.0f, clickValue = 0.0f;
    unsigned int controlCounter = 0;

    juce::ADSR ampAdsr, filterAdsr;
    juce::dsp::StateVariableTPTFilter<float> filter;
};

// Synthesiser that supports a mono/glide mode (one voice that slides between notes, great for 808s).
class TitanSynth : public juce::Synthesiser
{
public:
    bool mono = false;

    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (! mono || getNumVoices() == 0)
        {
            juce::Synthesiser::noteOn (midiChannel, midiNoteNumber, velocity);
            return;
        }

        const juce::ScopedLock sl (lock);
        for (auto* sound : sounds)
        {
            if (sound->appliesToNote (midiNoteNumber) && sound->appliesToChannel (midiChannel))
            {
                startVoice (getVoice (0), sound, midiChannel, midiNoteNumber, velocity);
                return;
            }
        }
    }
};
