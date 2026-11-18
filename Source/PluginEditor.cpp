#include "PluginEditor.h"
#include "Params.h"
#include "Presets.h"

namespace Neon
{
    const juce::Colour bg      { 0xff07040f };
    const juce::Colour panel   { 0xff120c22 };
    const juce::Colour text    { 0xffe8e6f0 };
    const juce::Colour dim     { 0xff8a84a3 };
    const juce::Colour cyan    { 0xff00f0ff };
    const juce::Colour magenta { 0xffff2bd6 };
    const juce::Colour purple  { 0xffb026ff };
    const juce::Colour yellow  { 0xffffe600 };
    const juce::Colour green   { 0xff39ff88 };
    const juce::Colour orange  { 0xffff7a3d };
}

// ---------------- Look and feel ----------------
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Neon::panel.brighter (0.15f));
    setColour (juce::ComboBox::textColourId, Neon::text);
    setColour (juce::ComboBox::outlineColourId, Neon::cyan.withAlpha (0.5f));
    setColour (juce::ComboBox::arrowColourId, Neon::cyan);
    setColour (juce::PopupMenu::backgroundColourId, Neon::panel);
    setColour (juce::PopupMenu::textColourId, Neon::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Neon::magenta.withAlpha (0.5f));
    setColour (juce::TextButton::buttonColourId, Neon::panel.brighter (0.15f));
    setColour (juce::TextButton::textColourOffId, Neon::cyan);
    setColour (juce::Label::textColourId, Neon::dim);
    setColour (juce::BubbleComponent::backgroundColourId, Neon::panel);
    setColour (juce::BubbleComponent::outlineColourId, Neon::magenta);
    setColour (juce::TooltipWindow::textColourId, Neon::text);
}

void NeonLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto colour = s.findColour (juce::Slider::rotarySliderFillColourId);
    const float thick = juce::jmax (2.5f, radius * 0.16f);

    // body
    g.setColour (juce::Colour (0xff1a1230));
    g.fillEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre));
    g.setColour (juce::Colour (0xff2a2245));
    g.drawEllipse (juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre), 1.0f);

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2245));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc with glow
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (colour.withAlpha (0.25f));
    g.strokePath (arc, juce::PathStrokeType (thick * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (arc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer
    const auto tip = centre.getPointOnCircumference (radius * 0.62f, angle);
    g.setColour (Neon::text);
    g.drawLine ({ centre.getPointOnCircumference (radius * 0.2f, angle), tip }, 2.0f);
}

void NeonLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (4.0f, 8.0f);
    const bool on = b.getToggleState();
    const auto c = Neon::magenta;
    if (on)
    {
        g.setColour (c.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (2.0f), 8.0f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 6.0f);
    }
    else
    {
        g.setColour (Neon::panel.brighter (0.2f));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (c.withAlpha (highlighted ? 0.9f : 0.5f));
        g.drawRoundedRectangle (r, 6.0f, 1.2f);
    }
    g.setColour (on ? juce::Colours::black : Neon::text);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

// ---------------- Editor ----------------
static constexpr int knobW = 64, bigW = 92, choiceW = 100, toggleW = 78, rowH = 128, headerH = 58, keysH = 64, margin = 10, editorW = 1260;

NeonTitanEditor::NeonTitanEditor (NeonTitanProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    knobs.reserve (64); choices.reserve (8); toggles.reserve (4);

    auto makeOsc = [this] (const juce::String& id, const juce::String& title, juce::Colour c)
    {
        Section s { title, c, {}, {} };
        addKnob (s, id + "_wave", "WAVE");
        addKnob (s, id + "_oct", "OCT");
        addKnob (s, id + "_semi", "SEMI");
        addKnob (s, id + "_fine", "FINE");
        addKnob (s, id + "_level", "LEVEL");
        addKnob (s, id + "_uni", "UNISON");
        addKnob (s, id + "_detune", "DETUNE");
        addKnob (s, id + "_spread", "SPREAD");
        sections.push_back (std::move (s));
    };
    makeOsc ("oscA", "OSC A", Neon::cyan);
    makeOsc ("oscB", "OSC B", Neon::magenta);

    { Section s { "SUB + NOISE", Neon::purple, {}, {} };
      addKnob (s, "sub_level", "SUB"); addChoice (s, "sub_oct", "SUB OCT", Params::subOctaves); addKnob (s, "noise_level", "NOISE"); addKnob (s, "click", "CLICK");
      sections.push_back (std::move (s)); }

    { Section s { "FILTER", Neon::yellow, {}, {} };
      addChoice (s, "flt_type", "TYPE", Params::filterTypes);
      addKnob (s, "flt_cutoff", "CUTOFF"); addKnob (s, "flt_res", "RES"); addKnob (s, "flt_drive", "DRIVE");
      addKnob (s, "flt_env", "ENV AMT"); addKnob (s, "flt_key", "KEY TRK"); addKnob (s, "vel_cut", "VEL>CUT");
      sections.push_back (std::move (s)); }

    { Section s { "FILTER ENV", Neon::yellow, {}, {} };
      addKnob (s, "fenv_a", "ATTACK"); addKnob (s, "fenv_d", "DECAY"); addKnob (s, "fenv_s", "SUSTAIN"); addKnob (s, "fenv_r", "RELEASE");
      sections.push_back (std::move (s)); }

    { Section s { "AMP ENV", Neon::green, {}, {} };
      addKnob (s, "amp_a", "ATTACK"); addKnob (s, "amp_d", "DECAY"); addKnob (s, "amp_s", "SUSTAIN"); addKnob (s, "amp_r", "RELEASE");
      sections.push_back (std::move (s)); }

    { Section s { "LFO", Neon::orange, {}, {} };
      addChoice (s, "lfo_shape", "SHAPE", Params::lfoShapes); addChoice (s, "lfo_sync", "SYNC", Params::lfoSyncs);
      addKnob (s, "lfo_rate", "RATE"); addKnob (s, "lfo_cut", "> CUTOFF"); addKnob (s, "lfo_pitch", "> PITCH"); addKnob (s, "lfo_amp", "> VOLUME");
      addToggle (s, "lfo_retrig", "RETRIG");
      sections.push_back (std::move (s)); }

    { Section s { "VOICE", Neon::purple, {}, {} };
      addToggle (s, "mono", "MONO"); addChoice (s, "chord", "CHORD", Params::chordTypes); addKnob (s, "glide", "GLIDE"); addKnob (s, "pitch_env", "P.ENV"); addKnob (s, "pitch_dec", "P.DECAY");
      sections.push_back (std::move (s)); }

    { Section s { "FX", Neon::cyan, {}, {} };
      addKnob (s, "fx_dist", "DISTORT"); addKnob (s, "fx_chorus", "CHORUS");
      addKnob (s, "dly_mix", "DELAY"); addChoice (s, "dly_time", "D.TIME", Params::delayTimes); addKnob (s, "dly_fb", "FEEDBK");
      addKnob (s, "fx_reverb", "REVERB");
      addKnob (s, "fx_size", "SIZE"); addKnob (s, "fx_width", "WIDTH"); addKnob (s, "master", "MASTER");
      sections.push_back (std::move (s)); }

    { Section s { "MACROS", Neon::magenta, {}, {} };
      addKnob (s, "m_size", "SIZE", true); addKnob (s, "m_growl", "GROWL", true);
      addKnob (s, "m_wobble", "WOBBLE", true); addKnob (s, "m_bright", "BRIGHT", true);
      sections.push_back (std::move (s)); }

    // Preset browser: category, search, list, prev/next, random
    categoryBox.addItemList (getPresetCategories(), 1);
    categoryBox.onChange = [this] { searchBox.clear(); rebuildPresetList(); };
    searchBox.setTextToShowWhenEmpty ("search 6000+ sounds", Neon::dim);
    searchBox.setColour (juce::TextEditor::backgroundColourId, Neon::panel.brighter (0.15f));
    searchBox.setColour (juce::TextEditor::textColourId, Neon::text);
    searchBox.setColour (juce::TextEditor::outlineColourId, Neon::cyan.withAlpha (0.5f));
    searchBox.setColour (juce::TextEditor::focusedOutlineColourId, Neon::magenta);
    searchBox.onTextChange = [this] { rebuildPresetList(); };
    searchBox.setJustification (juce::Justification::centredLeft);
    searchBox.setIndents (8, 0);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram())
            proc.setCurrentProgram (idx);
        shownProgram = proc.getCurrentProgram();
    };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    randomButton.onClick = [this]
    {
        if (filtered.empty()) return;
        proc.setCurrentProgram (filtered[(size_t) random.nextInt ((int) filtered.size())]);
        refreshPresetBox();
    };
    randomButton.setColour (juce::TextButton::textColourOffId, Neon::magenta);
    for (auto* comp : std::initializer_list<juce::Component*> { &categoryBox, &searchBox, &presetBox, &prevButton, &nextButton, &randomButton })
        addAndMakeVisible (comp);
    refreshPresetBox();

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xff1c1530));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff05030a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff3a2f5a));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Neon::magenta.withAlpha (0.7f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Neon::cyan.withAlpha (0.3f));
    keyboard.setAvailableRange (24, 96);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);

    setSize (editorW, headerH + rowH * 4 + keysH + margin * 3);
    startTimerHz (10);
}

NeonTitanEditor::~NeonTitanEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void NeonTitanEditor::addKnob (Section& s, const juce::String& id, const juce::String& text, bool big)
{
    Knob k;
    k.colour = s.colour;
    k.big = big;
    k.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    k.slider->setColour (juce::Slider::rotarySliderFillColourId, s.colour);
    k.slider->setPopupDisplayEnabled (true, true, this);
    k.slider->setDoubleClickReturnValue (true, (double) proc.apvts.getParameterRange (id).convertFrom0to1 (proc.apvts.getParameter (id)->getDefaultValue()));
    k.label = std::make_unique<juce::Label> (id, text);
    k.label->setJustificationType (juce::Justification::centred);
    k.label->setFont (juce::FontOptions (big ? 13.0f : 11.0f, juce::Font::bold));
    k.label->setColour (juce::Label::textColourId, big ? s.colour : Neon::dim);
    addAndMakeVisible (*k.slider);
    addAndMakeVisible (*k.label);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.slider);
    knobs.push_back (std::move (k));
    s.items.push_back ({ 0, (int) knobs.size() - 1 });
}

void NeonTitanEditor::addChoice (Section& s, const juce::String& id, const juce::String& text, const juce::StringArray& items)
{
    Choice c;
    c.box = std::make_unique<juce::ComboBox>();
    c.box->addItemList (items, 1);
    c.label = std::make_unique<juce::Label> (id, text);
    c.label->setJustificationType (juce::Justification::centred);
    c.label->setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (*c.box);
    addAndMakeVisible (*c.label);
    c.attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, *c.box);
    choices.push_back (std::move (c));
    s.items.push_back ({ 1, (int) choices.size() - 1 });
}

void NeonTitanEditor::addToggle (Section& s, const juce::String& id, const juce::String& text)
{
    Toggle t;
    t.button = std::make_unique<juce::ToggleButton> (text);
    addAndMakeVisible (*t.button);
    t.attach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, *t.button);
    toggles.push_back (std::move (t));
    s.items.push_back ({ 2, (int) toggles.size() - 1 });
}

void NeonTitanEditor::rebuildPresetList()
{
    const auto& presets = getFactoryPresets();
    const auto query = searchBox.getText().trim().toLowerCase();
    const auto category = categoryBox.getText();
    filtered.clear();
    presetBox.clear (juce::dontSendNotification);

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        const auto& p = presets[(size_t) i];
        const bool match = query.isEmpty() ? p.category == category
                                           : (p.name + " " + p.subtype + " " + p.category).toLowerCase().contains (query);
        if (! match) continue;
        filtered.push_back (i);
        presetBox.addItem (p.name + "   (" + (query.isEmpty() ? p.subtype : p.category + " / " + p.subtype) + ")", i + 1);
        if (filtered.size() >= 1500) break;
    }
    presetBox.setTextWhenNothingSelected (juce::String ((int) filtered.size()) + " sounds");
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void NeonTitanEditor::stepPreset (int delta)
{
    if (filtered.empty()) return;
    const auto it = std::find (filtered.begin(), filtered.end(), proc.getCurrentProgram());
    int pos = it == filtered.end() ? (delta > 0 ? -1 : 0) : (int) std::distance (filtered.begin(), it);
    pos = (pos + delta + (int) filtered.size()) % (int) filtered.size();
    proc.setCurrentProgram (filtered[(size_t) pos]);
    refreshPresetBox();
}

void NeonTitanEditor::refreshPresetBox()
{
    shownProgram = proc.getCurrentProgram();
    const auto& presets = getFactoryPresets();
    if (searchBox.isEmpty() && juce::isPositiveAndBelow (shownProgram, (int) presets.size()))
        categoryBox.setSelectedId (getPresetCategories().indexOf (presets[(size_t) shownProgram].category) + 1, juce::dontSendNotification);
    rebuildPresetList();
}

void NeonTitanEditor::timerCallback()
{
    if (proc.getCurrentProgram() != shownProgram)
        refreshPresetBox();
}

void NeonTitanEditor::paint (juce::Graphics& g)
{
    // Background: deep purple-black with a faint neon grid
    juce::ColourGradient bgGrad (juce::Colour (0xff0b0518), 0, 0, juce::Colour (0xff04020a), 0, (float) getHeight(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();
    g.setColour (Neon::purple.withAlpha (0.05f));
    for (int x = 0; x < getWidth(); x += 24) g.drawVerticalLine (x, 0.0f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    // Title
    auto title = juce::Rectangle<int> (margin + 4, 8, 360, 40);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    juce::ColourGradient tg (Neon::magenta, (float) title.getX(), 0, Neon::cyan, (float) title.getRight() - 140, 0, false);
    g.setGradientFill (tg);
    g.drawText ("NEON TITAN", title, juce::Justification::centredLeft);
    g.setColour (Neon::dim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("massive synth  //  v1.2", title.withX (title.getX() + 200).withWidth (160), juce::Justification::centredLeft);

    // Section panels
    for (const auto& s : sections)
    {
        const auto r = s.bounds.toFloat();
        g.setColour (Neon::panel.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (s.colour.withAlpha (0.12f));
        g.drawRoundedRectangle (r.expanded (1.5f), 11.0f, 3.0f);
        g.setColour (s.colour.withAlpha (0.75f));
        g.drawRoundedRectangle (r, 10.0f, 1.2f);
        g.setColour (s.colour);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (s.title, s.bounds.reduced (12, 6).removeFromTop (16), juce::Justification::centredLeft);
    }
}

void NeonTitanEditor::resized()
{
    // Header
    auto header = getLocalBounds().removeFromTop (headerH).reduced (margin, 12);
    auto area = header.removeFromRight (780);
    randomButton.setBounds (area.removeFromRight (74));
    area.removeFromRight (6);
    nextButton.setBounds (area.removeFromRight (34));
    area.removeFromRight (6);
    categoryBox.setBounds (area.removeFromLeft (110));
    area.removeFromLeft (6);
    searchBox.setBounds (area.removeFromLeft (170));
    area.removeFromLeft (6);
    prevButton.setBounds (area.removeFromLeft (34));
    area.removeFromLeft (6);
    presetBox.setBounds (area);

    keyboard.setBounds (getLocalBounds().removeFromBottom (keysH + margin).reduced (margin, 0).withTrimmedBottom (margin));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 43.0f);   // 6 octaves (C1-C7) = 43 white keys
    keyboard.setLowestVisibleKey (24);

    auto itemWidth = [] (const Item& it, bool big) { return it.kind == 0 ? (big ? bigW : knobW) : it.kind == 1 ? choiceW : toggleW; };
    const std::vector<std::vector<int>> rows { { 0, 1 }, { 2, 3, 4 }, { 5, 6, 7 }, { 8, 9 } };
    const int available = getWidth() - margin * 2;

    int y = headerH;
    for (const auto& row : rows)
    {
        std::vector<int> natural;
        int total = 0;
        for (int si : row)
        {
            int w = 20;
            for (const auto& it : sections[(size_t) si].items)
                w += itemWidth (it, it.kind == 0 && knobs[(size_t) it.index].big);
            natural.push_back (w);
            total += w;
        }
        const int gaps = margin * ((int) row.size() - 1);
        const float scale = (float) (available - gaps) / (float) total;

        int x = margin;
        for (size_t k = 0; k < row.size(); ++k)
        {
            auto& s = sections[(size_t) row[k]];
            const int w = (k == row.size() - 1) ? (margin + available - x) : (int) std::round (natural[k] * scale);
            s.bounds = { x, y, w, rowH - margin };
            x += w + margin;

            // Spread items evenly inside the panel
            auto inner = s.bounds.reduced (10, 0).withTrimmedTop (24).withTrimmedBottom (6);
            int itemsW = 0;
            for (const auto& it : s.items) itemsW += itemWidth (it, it.kind == 0 && knobs[(size_t) it.index].big);
            const float extra = (float) (inner.getWidth() - itemsW) / (float) s.items.size();
            float ix = (float) inner.getX();
            for (const auto& it : s.items)
            {
                const bool big = it.kind == 0 && knobs[(size_t) it.index].big;
                const int iw = itemWidth (it, big);
                auto cell = juce::Rectangle<int> ((int) (ix + extra * 0.5f), inner.getY(), iw, inner.getHeight());
                ix += (float) iw + extra;
                if (it.kind == 0)
                {
                    auto& kn = knobs[(size_t) it.index];
                    kn.label->setBounds (cell.removeFromBottom (16));
                    kn.slider->setBounds (cell);
                }
                else if (it.kind == 1)
                {
                    auto& ch = choices[(size_t) it.index];
                    ch.label->setBounds (cell.removeFromBottom (16));
                    ch.box->setBounds (cell.withSizeKeepingCentre (iw - 6, 26));
                }
                else
                {
                    toggles[(size_t) it.index].button->setBounds (cell.withSizeKeepingCentre (iw - 4, 40));
                }
            }
        }
        y += rowH;
    }
}
