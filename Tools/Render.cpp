// NEON TITAN test tool (not part of the plugin).
//   TitanRender qc <start> <end>          render presets [start, end) and print CSV: index,category,subtype,name,peak,rms,ok
//   TitanRender demo <outDir> <i,j,k...>  write WAV previews of the listed presets
//   TitanRender shot <outDir>             save a PNG of the plugin window
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"

namespace
{
    constexpr double sr = 44100.0, bpm = 140.0;
    constexpr int block = 256;

    struct N { double start, len; int note; };

    juce::AudioBuffer<float> renderPreset (int index)
    {
        const double beat = 60.0 / bpm;
        NeonTitanProcessor proc;
        proc.fallbackBpm = bpm;
        proc.setPlayConfigDetails (0, 2, sr, block);
        proc.setCurrentProgram (index);
        proc.prepareToPlay (sr, block);

        const auto& p = getFactoryPresets()[(size_t) index];
        const bool mono = proc.apvts.getRawParameterValue ("mono")->load() > 0.5f;
        std::vector<N> riff;
        if (mono)
            riff = { { 0, 1.5, 36 }, { 1.5, 0.5, 36 }, { 2, 1, 39 }, { 3, 1, 34 }, { 4, 2, 36 }, { 6, 1, 43 }, { 7, 1, 41 } };
        else if (p.category == "Chords" || proc.apvts.getRawParameterValue ("chord")->load() > 0.5f)
            riff = { { 0, 1.5, 60 }, { 1.5, 0.5, 60 }, { 2, 2, 57 }, { 4, 1.5, 53 }, { 5.5, 0.5, 53 }, { 6, 2, 55 } };   // single keys: chord mode adds the rest
        else if (p.category == "Pad" || p.category == "FX")
            riff = { { 0, 4, 57 }, { 0, 4, 60 }, { 0, 4, 64 }, { 0, 4, 67 }, { 4, 4, 53 }, { 4, 4, 57 }, { 4, 4, 60 }, { 4, 4, 64 } };
        else if (p.category == "Piano")
            riff = { { 0, 1, 48 }, { 0, 2, 60 }, { 0, 2, 64 }, { 0, 2, 67 }, { 1, 0.5, 72 }, { 1.5, 0.5, 76 }, { 2, 2, 45 }, { 2, 2, 60 }, { 2, 2, 64 }, { 2, 2, 69 },
                     { 4, 2, 41 }, { 4, 2, 60 }, { 4, 2, 65 }, { 4, 2, 69 }, { 5, 0.5, 72 }, { 5.5, 0.5, 74 }, { 6, 2, 43 }, { 6, 2, 59 }, { 6, 2, 62 }, { 6, 2, 67 } };
        else if (p.category == "Pluck" || p.category == "Lead")
            riff = { { 0, 0.5, 72 }, { 0.5, 0.5, 75 }, { 1, 0.5, 79 }, { 1.5, 0.5, 82 }, { 2, 1, 80 }, { 3, 1, 79 },
                     { 4, 0.5, 72 }, { 4.5, 0.5, 75 }, { 5, 0.5, 77 }, { 5.5, 0.5, 79 }, { 6, 2, 75 } };
        else
            riff = { { 0, 2, 60 }, { 0, 2, 63 }, { 0, 2, 67 }, { 2, 2, 58 }, { 2, 2, 62 }, { 2, 2, 65 },
                     { 4, 2, 56 }, { 4, 2, 60 }, { 4, 2, 63 }, { 6, 2, 55 }, { 6, 2, 58 }, { 6, 2, 62 } };

        const int total = (int) (sr * beat * 8.0 + sr * 2.0);
        juce::AudioBuffer<float> out (2, total), buf (2, block);
        out.clear();
        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            juce::MidiBuffer midi;
            for (const auto& note : riff)
            {
                const int on = (int) (note.start * beat * sr), off = (int) ((note.start + note.len) * beat * sr) - 1;
                if (on >= pos && on < pos + n)   midi.addEvent (juce::MidiMessage::noteOn (1, note.note, (juce::uint8) 110), on - pos);
                if (off >= pos && off < pos + n) midi.addEvent (juce::MidiMessage::noteOff (1, note.note), off - pos);
            }
            buf.setSize (2, n, false, false, true);
            proc.processBlock (buf, midi);
            for (int c = 0; c < 2; ++c)
                out.copyFrom (c, pos, buf, c, 0, n);
        }
        return out;
    }

    void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
    {
        f.deleteFile();
        auto stream = std::unique_ptr<juce::OutputStream> (f.createOutputStream().release());
        juce::WavAudioFormat wav;
        auto opts = juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (16);
        if (auto writer = wav.createWriterFor (stream, opts))
            writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? argv[1] : "qc";
    const auto& presets = getFactoryPresets();

    if (mode == "qc")
    {
        const int start = argc > 2 ? juce::String (argv[2]).getIntValue() : 0;
        const int end = argc > 3 ? juce::jmin ((int) presets.size(), juce::String (argv[3]).getIntValue()) : (int) presets.size();
        for (int i = start; i < end; ++i)
        {
            auto out = renderPreset (i);
            float peak = 0.0f; bool finite = true;
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < out.getNumSamples(); ++s)
                {
                    const float v = out.getSample (c, s);
                    if (! std::isfinite (v)) finite = false;
                    peak = juce::jmax (peak, std::abs (v));
                }
            const float rms = (out.getRMSLevel (0, 0, out.getNumSamples()) + out.getRMSLevel (1, 0, out.getNumSamples())) * 0.5f;
            const bool ok = finite && peak > 0.02f && peak <= 1.0f;
            const auto& p = presets[(size_t) i];
            std::printf ("%d,%s,%s,%s,%.4f,%.5f,%d\n", i, p.category.toRawUTF8(), p.subtype.toRawUTF8(),
                         p.name.replaceCharacter (',', ' ').toRawUTF8(), peak, rms, ok ? 1 : 0);
            std::fflush (stdout);
        }
        return 0;
    }

    const juce::File outDir (argc > 2 ? juce::String (argv[2]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    if (mode == "demo")
    {
        auto ids = juce::StringArray::fromTokens (argc > 3 ? argv[3] : "0", ",", "");
        int n = 0;
        for (auto& s : ids)
        {
            const int i = s.getIntValue();
            const auto& p = presets[(size_t) i];
            auto name = juce::String (++n).paddedLeft ('0', 2) + "_" + p.category + "_" + p.name;
            writeWav (renderPreset (i), outDir.getChildFile (name.replaceCharacters (" /", "__") + ".wav"));
            std::printf ("%s\n", name.toRawUTF8());
        }
        return 0;
    }

    if (mode == "shot")
    {
        NeonTitanProcessor proc;
        const int idx = argc > 3 ? juce::String (argv[3]).getIntValue() : 2;
        proc.setCurrentProgram (idx);
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        auto file = outDir.getChildFile ("NeonTitan_UI.png");
        file.deleteFile();
        juce::FileOutputStream fos (file);
        juce::PNGImageFormat().writeImageToStream (img, fos);
        std::printf ("UI snapshot %dx%d\n", img.getWidth(), img.getHeight());
        return 0;
    }
    return 1;
}
