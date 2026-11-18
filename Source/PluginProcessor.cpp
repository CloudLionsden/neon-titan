#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"
#include "Presets.h"

static constexpr int numVoices = 16;

NeonTitanProcessor::NeonTitanProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NeonTitan", Params::createLayout())
{
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            raw[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());

    static const char* oscFields[8] = { "_wave", "_oct", "_semi", "_fine", "_level", "_uni", "_detune", "_spread" };
    for (int o = 0; o < 2; ++o)
        for (int f = 0; f < 8; ++f)
            oscRaw[(size_t) o][(size_t) f] = raw.at (std::string (o == 0 ? "oscA" : "oscB") + oscFields[f]);

    for (auto& h : chordHeld) h.reserve (8);   // no memory allocation later on the audio thread

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice (new TitanVoice (settings));
    synth.addSound (new TitanSound());
}

bool NeonTitanProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void NeonTitanProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    keyboardState.reset();

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    chorus.prepare (spec);
    chorus.setRate (0.8f);
    chorus.setDepth (0.3f);
    chorus.setCentreDelay (8.0f);
    chorus.setFeedback (0.0f);
    reverb.prepare (spec);
    delayL.assign ((size_t) (sampleRate * 4.0) + 1, 0.0f);
    delayR.assign ((size_t) (sampleRate * 4.0) + 1, 0.0f);
    delayWrite = 0;

    masterGain.reset (sampleRate, 0.05);
    widthSmoothed.reset (sampleRate, 0.05);
    distSmoothed.reset (sampleRate, 0.05);
    masterGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get ("master")));
    widthSmoothed.setCurrentAndTargetValue (get ("fx_width"));
    distSmoothed.setCurrentAndTargetValue (get ("fx_dist"));
}

void NeonTitanProcessor::updateSettings (double bpm)
{
    const float mSize = get ("m_size"), mGrowl = get ("m_growl"), mWobble = get ("m_wobble"), mBright = get ("m_bright");

    for (int o = 0; o < 2; ++o)
    {
        const auto& r = oscRaw[(size_t) o];
        auto& os = settings.osc[o];
        os.wave   = r[0]->load();
        os.octave = (int) r[1]->load();
        os.semi   = (int) r[2]->load();
        os.fine   = r[3]->load();
        os.level  = r[4]->load();
        os.unison = (int) r[5]->load();
        os.detune = juce::jlimit (0.0f, 1.0f, r[6]->load() + mSize * 0.4f);
        os.spread = juce::jlimit (0.0f, 1.0f, r[7]->load() + mSize * 0.3f);
        if (mSize > 0.5f && os.unison < 3) os.unison = 3;   // SIZE macro thickens thin sounds
    }

    settings.subLevel   = get ("sub_level");
    settings.subOctave  = (int) get ("sub_oct");
    settings.noiseLevel = get ("noise_level");
    settings.filterType = (int) get ("flt_type");
    settings.cutoff     = juce::jlimit (20.0f, 20000.0f, get ("flt_cutoff") * std::pow (2.0f, mBright * 3.0f));
    settings.resonance  = juce::jlimit (0.0f, 1.0f, get ("flt_res") + mGrowl * 0.35f);
    settings.drive      = juce::jlimit (0.0f, 1.0f, get ("flt_drive") + mGrowl * 0.5f);
    settings.envAmount  = get ("flt_env");
    settings.keyTrack   = get ("flt_key");

    settings.ampEnv    = { get ("amp_a"),  get ("amp_d"),  get ("amp_s"),  get ("amp_r") };
    settings.filterEnv = { get ("fenv_a"), get ("fenv_d"), get ("fenv_s"), get ("fenv_r") };

    settings.lfoShape  = (int) get ("lfo_shape");
    const int sync = (int) get ("lfo_sync");
    static const double beatsPerCycle[] = { 0.0, 4.0, 2.0, 1.0, 0.5, 0.25, 2.0 / 3.0, 1.0 / 3.0 };
    settings.lfoHz     = sync == 0 ? get ("lfo_rate") : (float) (bpm / 60.0 / beatsPerCycle[juce::jlimit (1, 7, sync)]);
    settings.lfoCut    = juce::jlimit (0.0f, 1.0f, get ("lfo_cut") + mWobble);
    settings.lfoPitch  = get ("lfo_pitch");
    settings.lfoAmp    = get ("lfo_amp");
    settings.lfoRetrig = get ("lfo_retrig") > 0.5f;
    settings.glide     = get ("glide");
    settings.mono      = get ("mono") > 0.5f;
    settings.pitchEnv  = get ("pitch_env");
    settings.pitchDecay = get ("pitch_dec");
    settings.click     = get ("click");
    settings.velCut    = get ("vel_cut");
    settings.globalLfoPhase = lfoGlobalPhase;
    synth.mono = settings.mono;
}

void NeonTitanProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    double bpm = fallbackBpm;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = *hostBpm;

    updateSettings (bpm);
    lfoGlobalPhase += settings.lfoHz * numSamples / sr;
    lfoGlobalPhase -= std::floor (lfoGlobalPhase);

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    expandChords (midi);
    synth.renderNextBlock (buffer, midi, 0, numSamples);
    buffer.applyGain (0.5f);   // headroom before the FX so chords don't slam the soft clipper

    // Mono output? Copy left to right is not needed; just process channel 0.
    const int numCh = buffer.getNumChannels();
    if (numCh < 2)
    {
        buffer.applyGain (juce::Decibels::decibelsToGain (get ("master")));
        return;
    }

    // --- FX chain: distortion -> chorus -> reverb -> width -> master -> soft clip ---
    distSmoothed.setTargetValue (get ("fx_dist") + get ("m_growl") * 0.3f);
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    for (int i = 0; i < numSamples; ++i)
    {
        const float d = juce::jlimit (0.0f, 1.0f, distSmoothed.getNextValue());
        if (d > 0.001f)
        {
            const float g = 1.0f + d * 20.0f, norm = 1.0f / std::tanh (g);
            const float wl = std::tanh (L[i] * g) * norm, wr = std::tanh (R[i] * g) * norm;
            L[i] = L[i] + (wl - L[i]) * juce::jmin (1.0f, d * 1.5f);
            R[i] = R[i] + (wr - R[i]) * juce::jmin (1.0f, d * 1.5f);
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    const float chorusMix = get ("fx_chorus");
    if (chorusMix > 0.001f)
    {
        chorus.setMix (chorusMix * 0.6f);
        chorus.process (ctx);
    }

    // Ping-pong delay, locked to tempo
    const float dMix = get ("dly_mix");
    if (dMix > 0.001f && ! delayL.empty())
    {
        static const double beats[] = { 1.0, 0.5, 0.75, 0.25, 1.5, 1.0 / 3.0 };
        const int dt = juce::jlimit (0, 5, (int) get ("dly_time"));
        const int len = (int) delayL.size();
        const int dSamples = juce::jlimit (1, len - 1, (int) (beats[dt] * 60.0 / bpm * sr));
        const float fb = get ("dly_fb");
        for (int i = 0; i < numSamples; ++i)
        {
            const int r = (delayWrite - dSamples + len) % len;
            const float outL = delayL[(size_t) r], outR = delayR[(size_t) r];
            delayL[(size_t) delayWrite] = (L[i] + R[i]) * 0.5f + outR * fb;
            delayR[(size_t) delayWrite] = outL * fb;
            L[i] += outL * dMix * 0.7f;
            R[i] += outR * dMix * 0.7f;
            delayWrite = (delayWrite + 1) % len;
        }
    }

    const float revMix = juce::jlimit (0.0f, 1.0f, get ("fx_reverb") + get ("m_size") * 0.35f);
    if (revMix > 0.001f)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.0f, 1.0f, get ("fx_size") + get ("m_size") * 0.2f);
        rp.damping = 0.45f;
        rp.wetLevel = revMix * 0.5f;
        rp.dryLevel = 1.0f - revMix * 0.35f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
        reverb.process (ctx);
    }

    widthSmoothed.setTargetValue (juce::jlimit (0.0f, 2.0f, get ("fx_width") + get ("m_size") * 0.5f));
    masterGain.setTargetValue (juce::Decibels::decibelsToGain (get ("master")));
    for (int i = 0; i < numSamples; ++i)
    {
        const float w = widthSmoothed.getNextValue();
        const float mid = (L[i] + R[i]) * 0.5f, side = (L[i] - R[i]) * 0.5f * w;
        const float g = masterGain.getNextValue();
        L[i] = std::tanh ((mid + side) * g);   // gentle soft-clip: never hard-clips Maschine's input
        R[i] = std::tanh ((mid - side) * g);
    }
}

// Chord mode: one key (or Maschine pad) plays a whole chord.
void NeonTitanProcessor::expandChords (juce::MidiBuffer& midi)
{
    static const std::vector<std::vector<int>> shapes {
        { 0 }, { 0, 4, 7 }, { 0, 3, 7 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 4, 7, 10 }, { 0, 4, 7, 11, 14 },
        { 0, 3, 7, 10, 14 }, { 0, 2, 7 }, { 0, 5, 7 }, { 0, 4, 7, 14 }, { 0, 3, 7, 10, 14, 17 }, { 0, 7, 12 }, { 0, 12 } };
    const int type = settings.mono ? 0 : juce::jlimit (0, (int) shapes.size() - 1, (int) get ("chord"));
    bool anyHeld = false;
    for (const auto& h : chordHeld) if (! h.empty()) { anyHeld = true; break; }
    if (type == 0 && ! anyHeld)
        return;

    juce::MidiBuffer out;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            auto& held = chordHeld[(size_t) m.getNoteNumber()];
            for (int n : held) out.addEvent (juce::MidiMessage::noteOff (m.getChannel(), n), meta.samplePosition);
            held.clear();
            for (int iv : shapes[(size_t) type])
            {
                const int n = m.getNoteNumber() + iv;
                if (n > 127) continue;
                held.push_back (n);
                out.addEvent (juce::MidiMessage::noteOn (m.getChannel(), n, m.getVelocity()), meta.samplePosition);
            }
        }
        else if (m.isNoteOff())
        {
            auto& held = chordHeld[(size_t) m.getNoteNumber()];
            if (held.empty()) out.addEvent (m, meta.samplePosition);
            for (int n : held) out.addEvent (juce::MidiMessage::noteOff (m.getChannel(), n), meta.samplePosition);
            held.clear();
        }
        else
            out.addEvent (m, meta.samplePosition);
    }
    midi.swapWith (out);
}

// ---------------- Presets (shown as "programs" in hosts) ----------------
int NeonTitanProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String NeonTitanProcessor::getProgramName (int index)
{
    const auto& p = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? p[(size_t) index].category + " - " + p[(size_t) index].name : juce::String();
}

void NeonTitanProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;
    currentProgram = index;

    // Reset everything to default, then apply the preset's values
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

void NeonTitanProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NeonTitanProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* NeonTitanProcessor::createEditor() { return new NeonTitanEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeonTitanProcessor(); }
