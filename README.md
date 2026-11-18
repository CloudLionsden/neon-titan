# NEON TITAN

An original, big-sounding synth plugin (VST3) for Maschine 2 and any other plugin host. Built with JUCE.

**Sound engine**
- 2 oscillators that morph sine → triangle → saw → square, each with up to 7 unison voices, detune and stereo spread
- Sub oscillator (-1 or -2 octaves) and noise
- Driven filter (low pass / band pass / high pass) with its own envelope and key tracking
- LFO (sine, triangle, saw, square, random) in Hz or locked to Maschine's tempo (1/1 to 1/16, triplets), routed to cutoff, pitch and volume
- Mono mode with glide for 808s and basslines; 8-voice polyphony otherwise
- FX: distortion, chorus, reverb, stereo width, master with a soft clipper
- 4 macro knobs (SIZE, GROWL, WOBBLE, BRIGHT) that each move several settings, ideal for Maschine's hardware knobs
- Pitch envelope (the punchy drop at the start of an 808) and a tempo-synced ping-pong delay
- Hammer click and velocity-to-brightness for crisp pianos
- **Chord mode:** one key or Maschine pad plays a full chord (Major, Minor, Maj7, Min7, Dom7, Maj9, Min9, Sus2, Sus4, Add9, Min11, Power, Octave)
- **6,212 presets** in a browser with categories, search, previous/next and RANDOM:

| Category | Count | Styles |
|---|---|---|
| Classic | 12 | The original hand-made presets |
| 808 | 900 | Trap, Drill Slide, Distorted, Clean Sub, Punch |
| Bass | 900 | Reese, Wobble, Growl, Pluggnb Sub, Acid, Phonk/Hyperpop |
| Lead | 800 | Rage Supersaw, Hyperpop, Pluggnb, Cyber Mono, Synthwave |
| Piano | 600 | Crisp Digital, Bright Pop, Lofi, Felt Soft, Bell Piano, House Piano |
| Chords | 600 | Neo-Soul EP, Pluggnb, House Stab, Trance Stab, Lofi, Pad Chords (one key = full chord) |
| Pluck | 700 | Trap Pluck, Pluggnb Bell, Glass, Phonk Cowbell, Mallet |
| Keys | 400 | Hyper Stab, Soft Keys, Organ |
| Pad | 900 | Ambient, Dark Drone, Shimmer, Lo-fi |
| FX | 400 | Riser, Downlifter, Sweep |

### How the generated presets were made
Each style is a hand-designed recipe with safe ranges for every knob (`Source/PresetGenerator.h`). The library generates variations within those ranges using a fixed random seed, so it's identical on every computer and takes almost no disk space. Every preset was rendered and checked automatically: none are silent, glitchy or clipping, and each one's volume is matched to its category (`Source/PresetGains.h`, made by `Tools/make_gains.py`). Variations within a style are related, so expect families of similar sounds rather than 5,000 completely different ones.

## Putting it on another computer
The plug-in is a single folder, and all 6,212 presets are built into it, so there's nothing else to copy.

**Another Windows PC:** copy `NEON TITAN.vst3` (from your download, or from `C:\Program Files\Common Files\VST3\` on the first PC) to a USB stick or cloud drive, then paste it into `C:\Program Files\Common Files\VST3\` on the new PC. Open Maschine (updated) and rescan plug-ins.

**A Mac:** the GitHub build also makes a Mac version. Download **NEON-TITAN-Mac** from the same Actions run and follow `INSTALL_MAC.txt`.

**Your own sounds:** sounds you tweak and save in Maschine live in Maschine's User Content folder (Documents > Native Instruments > User Content) and inside your Maschine projects. Copy those too, and they'll load on the new computer once NEON TITAN is installed there.

## Maschine compatibility (read first)
Older Maschine 2 versions only load third-party plugins in the old VST2 format, which can no longer be built. Third-party **VST3** support came in later Maschine 2 updates. Update Maschine 2 to the latest version in Native Access (or use Maschine 3) before installing.

---

## Route 1: Let GitHub build it for you (no coding tools)
1. Create a free account at github.com.
2. Click **+ → New repository**, name it `neon-titan`, and create it.
3. Click **uploading an existing file**, drag in everything from this folder **including the hidden `.github` folder**, and commit.
   (If the `.github` folder doesn't upload by dragging, create the file `.github/workflows/build.yml` with **Add file → Create new file** and paste its contents.)
4. Open the **Actions** tab. The "Build NEON TITAN" workflow starts automatically and takes about 10–15 minutes.
5. When it shows a green check, open the run and download **NEON-TITAN-Windows** at the bottom.
6. Unzip it and follow `INSTALL_WINDOWS.txt`.

Every time you change the code and commit, GitHub builds a fresh copy.

## Route 2: Compile it yourself on Windows
Install once:
- **Visual Studio 2022 Community** with the "Desktop development with C++" workload
- **CMake** (cmake.org, tick "Add to PATH")
- **Git** (git-scm.com)

Then in a terminal, inside this folder:
```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
The first run downloads JUCE automatically. The plugin appears in:
`build\NeonTitan_artefacts\Release\VST3\NEON TITAN.vst3`

Copy that folder to `C:\Program Files\Common Files\VST3\`, then rescan plugins in Maschine.

## Route 3: Source code map
| File | What it does |
|---|---|
| `Source/Params.h` | Every knob: name, range and default |
| `Source/TitanVoice.h` | The sound engine: oscillators, unison, sub, noise, filter, envelopes, LFO, glide |
| `Source/PluginProcessor.cpp` | Reads the knobs, macros, tempo sync, FX chain, presets, saving/loading |
| `Source/Presets.h` | The 12 classic presets and the library builder |
| `Source/PresetGenerator.h` | The recipes and name words behind the 5,000 generated presets. Add a recipe to make more |
| `Source/PresetGains.h` | Per-preset loudness matching |
| `Source/PluginEditor.cpp` | The neon interface and on-screen keyboard |
| `Tools/Render.cpp` | Test tool: checks every preset (`qc`), renders WAV previews (`demo`), saves a screenshot (`shot`) |
| `Tools/make_gains.py` | Builds the loudness-matching table from a `qc` run |
| `.github/workflows/build.yml` | Cloud build for Windows |

## Using it in Maschine
- **Presets:** pick a category, then a sound from the list, or type in the search box (try "808", "wobble", "bell", "ambient", "rage"). Use `<` `>` to step through, or RANDOM for a surprise.
- **Hardware knobs:** in Maschine's plugin parameter pages, assign the 4 macros first. They're the most fun to play live.
- **Wobble bass:** load *Neon Wobble 1/8*, play low notes, turn **WOBBLE**. Change LFO **SYNC** to 1/16 or 1/4 T for different rhythms.
- **808s:** load *Sub 808 Glide*. Overlap notes to hear the glide; turn **GLIDE** for longer slides.

## Notes
- Windows may warn that the plugin is from an unknown publisher because it isn't code-signed. That's normal for a self-built plugin.
- JUCE and the VST3 SDK have their own licenses. They're fine for personal use; check both before selling the plugin.
