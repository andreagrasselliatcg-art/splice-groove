#include "PluginEditor.h"

using namespace sg;

namespace sgui
{
const char* longLaneName (int lane)
{
    static const char* n[kNumLanes] = { "BASS DRUM", "HAND CLAP", "SNARE DRUM", "CLOSED HAT",
                                        "OPEN HAT", "SHAKER", "PERCUSSION", "RIDE / TOP" };
    return n[lane];
}

const char* shortLaneName (int lane)
{
    static const char* n[kNumLanes] = { "BD", "CP", "SD", "CH", "OH", "SH", "PC", "RD" };
    return n[lane];
}

void drawLed (juce::Graphics& g, juce::Point<float> c, float r, float brightness)
{
    brightness = juce::jlimit (0.0f, 1.0f, brightness);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - r - 1.5f, c.y - r - 1.5f, (r + 1.5f) * 2, (r + 1.5f) * 2);

    const auto base = col::ledOff.interpolatedWith (col::ledOn, brightness);
    if (brightness > 0.05f)
    {
        g.setGradientFill (juce::ColourGradient (col::ledOn.withAlpha (0.45f * brightness), c,
                                                 col::ledOn.withAlpha (0.0f), c.translated (r * 3.0f, 0), true));
        g.fillEllipse (c.x - r * 3, c.y - r * 3, r * 6, r * 6);
    }
    g.setGradientFill (juce::ColourGradient (base.brighter (0.6f * brightness), c.translated (-r * 0.3f, -r * 0.3f),
                                             base.darker (0.3f), c.translated (r, r), true));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
}

void drawSevenSegment (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Colour on)
{
    static const uint8_t map[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };
    const float h = area.getHeight();
    const float w = h * 0.58f;
    const float t = h * 0.13f;
    const float gap = h * 0.24f;

    int digits = 0;
    for (auto c : text) if (c != '.') ++digits;
    float x = area.getRight() - digits * (w + gap) + gap;
    const float y = area.getY();

    for (int i = 0; i < text.length(); ++i)
    {
        const auto c = text[i];
        if (c == '.')
            continue;

        uint8_t bits = 0;
        if (c >= '0' && c <= '9') bits = map[c - '0'];
        else if (c == '-') bits = 0x40;

        const juce::Rectangle<float> seg[7] = {
            { x + t, y, w - 2 * t, t },                               // a
            { x + w - t, y + t, t, h / 2 - 1.5f * t },                // b
            { x + w - t, y + h / 2 + 0.5f * t, t, h / 2 - 1.5f * t }, // c
            { x + t, y + h - t, w - 2 * t, t },                       // d
            { x, y + h / 2 + 0.5f * t, t, h / 2 - 1.5f * t },         // e
            { x, y + t, t, h / 2 - 1.5f * t },                        // f
            { x + t, y + h / 2 - t / 2, w - 2 * t, t },               // g
        };
        for (int s = 0; s < 7; ++s)
        {
            g.setColour ((bits >> s) & 1 ? on : on.withAlpha (0.07f));
            g.fillRoundedRectangle (seg[s].reduced (0.6f), t * 0.4f);
        }

        if (i + 1 < text.length() && text[i + 1] == '.')
        {
            g.setColour (on);
            g.fillEllipse (x + w + gap * 0.2f, y + h - t, t, t);
        }
        x += w + gap;
    }
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (juce::TextButton::buttonColourId, col::key);
    setColour (juce::TextButton::buttonOnColourId, col::key);
    setColour (juce::TextButton::textColourOffId, col::ink);
    setColour (juce::TextButton::textColourOnId, col::ink);
    setColour (juce::ComboBox::backgroundColourId, col::lcd);
    setColour (juce::ComboBox::textColourId, col::lcdText);
    setColour (juce::ComboBox::arrowColourId, col::lcdText);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::PopupMenu::backgroundColourId, col::dark);
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::orange);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::Label::textColourId, col::ink);
    setColour (juce::Slider::textBoxTextColourId, col::ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, col::dark);
    setColour (juce::TooltipWindow::textColourId, juce::Colours::white);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float start, float end, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    const auto c = bounds.getCentre();
    const bool lightPanel = ! s.getProperties().getWithDefault ("darkPanel", false);

    // scale ticks printed on the panel
    g.setColour (lightPanel ? col::ink.withAlpha (0.7f) : juce::Colours::white.withAlpha (0.6f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        const auto p1 = c.getPointOnCircumference (radius, a);
        const auto p2 = c.getPointOnCircumference (radius - (i % 5 == 0 ? 4.5f : 2.5f), a);
        g.drawLine ({ p1, p2 }, i % 5 == 0 ? 1.4f : 0.9f);
    }

    // knob body: grey skirt + black cap
    const float skirt = radius - 5.0f;
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - skirt + 1.5f, c.y - skirt + 2.5f, skirt * 2, skirt * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4a), c.translated (0, -skirt),
                                             juce::Colour (0xff121212), c.translated (0, skirt), false));
    g.fillEllipse (c.x - skirt, c.y - skirt, skirt * 2, skirt * 2);

    const float cap = skirt * 0.72f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3a), c.translated (-cap * 0.4f, -cap * 0.6f),
                                             juce::Colour (0xff0d0d0d), c.translated (cap * 0.5f, cap * 0.7f), true));
    g.fillEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2);

    // white pointer
    const float angle = start + pos * (end - start);
    g.setColour (juce::Colours::white);
    g.drawLine ({ c.getPointOnCircumference (skirt * 0.25f, angle), c.getPointOnCircumference (skirt - 1.0f, angle) }, 2.2f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& colour, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool pressed = down || (b.getToggleState() && b.getClickingTogglesState() && false);

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (r.translated (0, pressed ? 0.5f : 2.0f), 3.0f);

    auto body = r.withTrimmedBottom (pressed ? 0.5f : 2.0f).translated (0, pressed ? 1.5f : 0.0f);
    auto c = colour;
    if (over) c = c.brighter (0.06f);
    g.setGradientFill (juce::ColourGradient (c.brighter (0.12f), body.getTopLeft(), c.darker (0.12f), body.getBottomLeft(), false));
    g.fillRoundedRectangle (body, 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (body, 3.0f, 0.8f);

    // LED for toggle keys
    if (b.getClickingTogglesState() || b.getProperties().contains ("led"))
    {
        const bool lit = b.getToggleState();
        drawLed (g, { body.getX() + 9.0f, body.getCentreY() }, 3.5f, lit ? 1.0f : 0.0f);
    }
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    auto r = b.getLocalBounds().toFloat().withTrimmedBottom (2.0f).translated (0, down ? 1.5f : 0.0f);
    if (b.getClickingTogglesState() || b.getProperties().contains ("led"))
        r.removeFromLeft (14.0f);

    const float size = juce::jmin (13.0f, b.getHeight() * 0.42f);
    g.setFont (juce::FontOptions (size, juce::Font::bold));
    g.setColour (b.findColour (juce::TextButton::textColourOffId));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r.reduced (2.0f), 2.0f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return juce::Font (juce::FontOptions (juce::jmin (16.0f, box.getHeight() * 0.5f), juce::Font::bold));
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

//==============================================================================
PanelKnob::PanelKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                      const juce::String& t, bool light)
    : title (t), lightText (light)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.getProperties().set ("darkPanel", light);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    slider.setPopupDisplayEnabled (true, true, nullptr);
}

void PanelKnob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (16);
    slider.setBounds (r);
}

void PanelKnob::paint (juce::Graphics& g)
{
    g.setColour (lightText ? juce::Colours::white : col::ink);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (title, getLocalBounds().removeFromTop (16), juce::Justification::centred);
}

//==============================================================================
static void setupColumnKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setPopupDisplayEnabled (true, true, nullptr);
}

InstrumentColumn::InstrumentColumn (SpliceGrooveProcessor& p, UiState& u, int laneIndex)
    : proc (p), ui (u), lane (laneIndex)
{
    for (auto* s : { &tune, &decay, &level })
    {
        setupColumnKnob (*s);
        addAndMakeVisible (*s);
    }
    const auto id = juce::String (lane);
    tuneAtt  = std::make_unique<SA> (proc.apvts, "tune" + id, tune);
    decayAtt = std::make_unique<SA> (proc.apvts, "decay" + id, decay);
    levelAtt = std::make_unique<SA> (proc.apvts, "vol" + id, level);
    tune.setDoubleClickReturnValue (true, 0.0);
    decay.setDoubleClickReturnValue (true, 1.0);
    tune.textFromValueFunction = [] (double v) { return (v > 0 ? "+" : "") + juce::String (v, 1) + " st"; };
    decay.textFromValueFunction = [] (double v) { return v >= 0.99 ? juce::String ("FULL") : juce::String (juce::roundToInt (v * 100)) + "%"; };
    level.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100)) + "%"; };

    sampleButton.setColour (juce::TextButton::buttonColourId, col::lcd);
    sampleButton.setColour (juce::TextButton::textColourOffId, col::lcdText);
    sampleButton.setTooltip ("Click to audition. Drag a sample here from Finder or the Splice app.");
    sampleButton.onClick = [this] { proc.audition (lane); };
    addAndMakeVisible (sampleButton);

    randomButton.setTooltip ("Another sound from your Splice library");
    randomButton.onClick = [this]
    {
        proc.pickNewSound (lane);
        proc.audition (lane);
        refresh();
    };
    addAndMakeVisible (randomButton);

    muteButton.setClickingTogglesState (true);
    muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "mute" + id, muteButton);
    addAndMakeVisible (muteButton);

    selectButton.getProperties().set ("led", true);
    selectButton.setColour (juce::TextButton::buttonColourId, col::key);
    selectButton.setColour (juce::TextButton::buttonOnColourId, col::orange);
    selectButton.setTooltip ("Edit this instrument with the 16 step keys");
    selectButton.onClick = [this]
    {
        ui.selectedLane = lane;
        proc.audition (lane);
        if (ui.changed) ui.changed();
    };
    addAndMakeVisible (selectButton);

    refresh();
}

void InstrumentColumn::refresh()
{
    const auto f = proc.getLaneFile (lane);
    const auto text = f.existsAsFile() ? f.getFileNameWithoutExtension() : juce::String ("-- NO SAMPLE --");
    if (sampleButton.getButtonText() != text)
        sampleButton.setButtonText (text);
    selectButton.setToggleState (ui.selectedLane == lane, juce::dontSendNotification);
}

void InstrumentColumn::mouseDown (const juce::MouseEvent&)
{
    ui.selectedLane = lane;
    if (ui.changed) ui.changed();
}

void InstrumentColumn::resized()
{
    auto r = getLocalBounds().reduced (6, 0);
    r.removeFromTop (30);   // name strip
    r.removeFromTop (16);   // knob labels
    auto knobs = r.removeFromTop (46);
    const int kw = knobs.getWidth() / 3;
    tune.setBounds (knobs.removeFromLeft (kw));
    decay.setBounds (knobs.removeFromLeft (kw));
    level.setBounds (knobs);
    r.removeFromTop (8);
    sampleButton.setBounds (r.removeFromTop (24));
    r.removeFromTop (6);
    auto row = r.removeFromTop (24);
    randomButton.setBounds (row.removeFromLeft (row.getWidth() / 2 - 2));
    row.removeFromLeft (4);
    muteButton.setBounds (row);
    r.removeFromTop (8);
    selectButton.setBounds (r.removeFromTop (30));
}

void InstrumentColumn::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // name strip (white letters on dark band)
    auto strip = r.removeFromTop (24.0f).reduced (2.0f, 0.0f);
    g.setColour (col::dark);
    g.fillRect (strip);
    g.setColour (ui.selectedLane == lane ? col::orange : juce::Colours::white);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (longLaneName (lane), strip, juce::Justification::centred);

    // knob captions
    r.removeFromTop (6.0f);
    auto caps = r.removeFromTop (16.0f).reduced (6.0f, 0.0f);
    g.setColour (col::ink);
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    const float cw = caps.getWidth() / 3.0f;
    const char* names[3] = { "TUNE", "DECAY", "LEVEL" };
    for (int i = 0; i < 3; ++i)
        g.drawText (names[i], caps.withX (caps.getX() + i * cw).withWidth (cw), juce::Justification::centred);

    // separator line between columns
    g.setColour (col::panelShade.darker (0.15f));
    g.drawLine ((float) getWidth() - 0.5f, 30.0f, (float) getWidth() - 0.5f, (float) getHeight() - 4.0f, 1.0f);

    if (dragOver)
    {
        g.setColour (col::orange);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 4.0f, 2.5f);
    }
}

bool InstrumentColumn::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (SampleLibrary::isAudioFile (juce::File (f)))
            return true;
    return false;
}

void InstrumentColumn::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (auto& path : files)
    {
        juce::File f (path);
        if (SampleLibrary::isAudioFile (f) && proc.loadLaneSample (lane, f))
        {
            proc.audition (lane);
            break;
        }
    }
    refresh();
    repaint();
}

//==============================================================================
PatternDisplay::PatternDisplay (SpliceGrooveProcessor& p, UiState& u) : proc (p), ui (u)
{
    startTimerHz (30);
}

void PatternDisplay::timerCallback()
{
    const int s = proc.playStep.load();
    if (s != lastPlay) { lastPlay = s; repaint(); }
}

juce::Rectangle<float> PatternDisplay::matrixArea() const
{
    return getLocalBounds().toFloat().reduced (8.0f).withTrimmedLeft (26.0f).withTrimmedTop (14.0f);
}

bool PatternDisplay::cellAt (juce::Point<float> pos, int& lane, int& step) const
{
    const auto m = matrixArea();
    if (! m.contains (pos)) return false;
    lane = (int) ((pos.y - m.getY()) / (m.getHeight() / kNumLanes));
    step = (int) ((pos.x - m.getX()) / (m.getWidth() / kNumSteps));
    return lane >= 0 && lane < kNumLanes && step >= 0 && step < kNumSteps;
}

void PatternDisplay::mouseDown (const juce::MouseEvent& e)
{
    int lane, step;
    if (! cellAt (e.position, lane, step))
        return;

    ui.selectedLane = lane;
    ui.page = step / 16;

    const auto cur = proc.pattern.get (lane, step);
    if (e.mods.isPopupMenu())
    {
        if (cur > 0) proc.pattern.set (lane, step, cur > 110 ? 70 : cur > 80 ? 40 : 127);
        paintValue = 255;
    }
    else
    {
        paintValue = cur > 0 ? 0 : (e.mods.isShiftDown() ? 127 : 100);
        proc.pattern.set (lane, step, paintValue);
    }
    lastLane = lane; lastStep = step;
    if (ui.changed) ui.changed();
}

void PatternDisplay::mouseDrag (const juce::MouseEvent& e)
{
    int lane, step;
    if (paintValue == 255 || ! cellAt (e.position, lane, step) || (lane == lastLane && step == lastStep))
        return;
    proc.pattern.set (lane, step, paintValue);
    lastLane = lane; lastStep = step;
    repaint();
}

void PatternDisplay::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (col::lcd);
    g.fillRoundedRectangle (b.reduced (3.0f), 4.0f);

    const auto m = matrixArea();
    const float cw = m.getWidth() / kNumSteps, rh = m.getHeight() / kNumLanes;
    const int play = proc.playStep.load();

    // header: bar labels
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    for (int bar = 0; bar < 2; ++bar)
    {
        g.setColour (ui.page == bar ? col::lcdText : col::lcdText.withAlpha (0.35f));
        g.drawText ("BAR " + juce::String (bar + 1), juce::Rectangle<float> (m.getX() + bar * 16 * cw, b.getY() + 5.0f, 16 * cw, 12.0f),
                    juce::Justification::centred);
    }

    // page frame
    g.setColour (col::lcdText.withAlpha (0.25f));
    g.drawRoundedRectangle (juce::Rectangle<float> (m.getX() + ui.page * 16 * cw - 2, m.getY() - 2, 16 * cw + 4, m.getHeight() + 4), 3.0f, 1.0f);

    for (int l = 0; l < kNumLanes; ++l)
    {
        const bool sel = l == ui.selectedLane;
        g.setColour (sel ? col::lcdText : col::lcdText.withAlpha (0.45f));
        g.drawText (shortLaneName (l), juce::Rectangle<float> (b.getX() + 8, m.getY() + l * rh, 24, rh), juce::Justification::centredLeft);

        if (sel)
        {
            g.setColour (col::lcdText.withAlpha (0.07f));
            g.fillRect (m.getX(), m.getY() + l * rh, m.getWidth(), rh);
        }

        for (int s = 0; s < kNumSteps; ++s)
        {
            auto cell = juce::Rectangle<float> (m.getX() + s * cw, m.getY() + l * rh, cw, rh).reduced (1.6f, 2.2f);
            const auto v = proc.pattern.get (l, s);
            juce::Colour c = (s / 4) % 2 == 0 ? juce::Colour (0xff2c2f2b) : juce::Colour (0xff252824);
            if (v > 0)
                c = col::ledOn.withAlpha (0.45f + 0.55f * v / 127.0f);
            if (s == play)
                c = v > 0 ? juce::Colour (0xffffd08a) : c.brighter (0.35f);
            g.setColour (c);
            g.fillRoundedRectangle (cell, 1.5f);
        }
    }
}

//==============================================================================
StepKeys::StepKeys (SpliceGrooveProcessor& p, UiState& u) : proc (p), ui (u)
{
    startTimerHz (30);
}

void StepKeys::timerCallback()
{
    const int s = proc.playStep.load();
    if (s != lastPlay) { lastPlay = s; repaint(); }
}

juce::Rectangle<float> StepKeys::keyRect (int i) const
{
    const float w = (float) getWidth() / 16.0f;
    return { i * w + 4.0f, 52.0f, w - 8.0f, (float) getHeight() - 74.0f };
}

juce::Point<float> StepKeys::ledPos (int i) const
{
    const auto k = keyRect (i);
    return { k.getCentreX(), 40.0f };
}

void StepKeys::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < 16; ++i)
    {
        if (! keyRect (i).expanded (3.0f, 30.0f).contains (e.position))
            continue;

        const int step = ui.page * 16 + i;
        const auto cur = proc.pattern.get (ui.selectedLane, step);
        if (e.mods.isPopupMenu())
        {
            if (cur > 0) proc.pattern.set (ui.selectedLane, step, cur > 110 ? 70 : cur > 80 ? 40 : 127);
        }
        else
        {
            const uint8_t v = cur > 0 ? 0 : (e.mods.isShiftDown() ? 127 : 100);
            proc.pattern.set (ui.selectedLane, step, v);
            if (v > 0) proc.audition (ui.selectedLane);
        }
        pressedKey = i;
        if (ui.changed) ui.changed();
        return;
    }
}

void StepKeys::paint (juce::Graphics& g)
{
    // dark band with step numbers (like the printed strip above the keys)
    auto band = juce::Rectangle<float> (0, 0, (float) getWidth(), 22.0f);
    g.setColour (col::dark);
    g.fillRect (band);

    const float w = (float) getWidth() / 16.0f;
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    for (int i = 0; i < 16; ++i)
    {
        g.setColour (juce::Colours::white);
        g.drawText (juce::String (i + 1), juce::Rectangle<float> (i * w, 0, w, 22.0f), juce::Justification::centred);
    }
    // group markers every 4 steps
    g.setColour (col::orange);
    for (int grp = 0; grp < 4; ++grp)
        g.fillRect (grp * 4 * w + 6.0f, 22.0f, 4 * w - 12.0f, 3.0f);

    const int play = proc.playStep.load();
    for (int i = 0; i < 16; ++i)
    {
        const int step = ui.page * 16 + i;
        const auto v = proc.pattern.get (ui.selectedLane, step);
        const bool isPlay = (play == step);
        float bright = v > 0 ? (0.55f + 0.45f * v / 127.0f) : 0.0f;
        if (isPlay) bright = v > 0 ? 0.6f : 1.0f;   // running light inverts like on hardware
        drawLed (g, ledPos (i), 5.0f, bright);

        auto k = keyRect (i);
        const bool down = pressedKey == i;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (k.translated (0, down ? 1.0f : 3.0f), 4.0f);
        auto body = k.translated (0, down ? 2.0f : 0.0f);
        const auto kc = (i / 4) % 2 == 0 ? col::key : col::key.darker (0.07f);
        g.setGradientFill (juce::ColourGradient (kc.brighter (0.15f), body.getTopLeft(), kc.darker (0.15f), body.getBottomLeft(), false));
        g.fillRoundedRectangle (body, 4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawRoundedRectangle (body, 4.0f, 1.0f);
        // accent mark on the key
        if (v > 110)
        {
            g.setColour (col::orange);
            g.fillRoundedRectangle (body.withTrimmedTop (body.getHeight() - 8.0f).reduced (8.0f, 2.0f), 2.0f);
        }
    }

    g.setColour (col::ink);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::String ("EDITING:  ") + longLaneName (ui.selectedLane) + "   |   BAR " + juce::String (ui.page + 1)
                    + "        click = on/off   shift+click = accent   right-click = velocity",
                juce::Rectangle<float> (4.0f, (float) getHeight() - 18.0f, (float) getWidth() - 8.0f, 16.0f),
                juce::Justification::centredLeft);
}

//==============================================================================
MidiDragArea::MidiDragArea (SpliceGrooveProcessor& p, std::function<int()> barsProvider)
    : proc (p), getBars (std::move (barsProvider))
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setTooltip ("Drag onto a Logic track to create a MIDI region");
}

void MidiDragArea::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 2.0f), 4.0f);
    auto body = r.withTrimmedBottom (2.0f);
    const auto c = isMouseOver() ? col::orange.brighter (0.1f) : col::orange;
    g.setGradientFill (juce::ColourGradient (c.brighter (0.15f), body.getTopLeft(), c.darker (0.15f), body.getBottomLeft(), false));
    g.fillRoundedRectangle (body, 4.0f);
    g.setColour (col::ink);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("DRAG MIDI TO LOGIC", body, juce::Justification::centred);
}

void MidiDragArea::mouseDrag (const juce::MouseEvent& e)
{
    if (dragStarted || e.getDistanceFromDragStart() < 6)
        return;
    dragStarted = true;
    const auto file = proc.writeMidiToTempFile (getBars());
    if (file.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
}
} // namespace sgui

//==============================================================================
namespace
{
constexpr int kW = 1240, kH = 760;
constexpr int kHeaderH = 70;
constexpr int kInstrTop = kHeaderH, kInstrH = 236;
constexpr int kMidTop = kInstrTop + kInstrH, kMidH = 176;
constexpr int kKeysTop = kMidTop + kMidH, kKeysH = 222;
constexpr int kFooterTop = kKeysTop + kKeysH;
namespace col = sgui::col;
}

SpliceGrooveEditor::SpliceGrooveEditor (SpliceGrooveProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      density (p.apvts, "density", "DENSITY", true),
      shuffle (p.apvts, "swing", "SHUFFLE", true),
      humanize (p.apvts, "humanize", "HUMANIZE", true),
      master (p.apvts, "master", "VOLUME", true),
      display (p, ui),
      keys (p, ui),
      dragArea (p, [this] { return selectedBars(); })
{
    setLookAndFeel (&lnf);
    ui.changed = [this] { uiChanged(); };

    shuffle.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };

    genreBox.addItemList (genreNames(), 1);
    genreBox.setSelectedItemIndex ((int) proc.getGenre(), juce::dontSendNotification);
    lastGenre = (int) proc.getGenre();
    genreBox.onChange = [this]
    {
        const int idx = genreBox.getSelectedItemIndex();
        if (idx < 0 || idx == lastGenre) return;
        lastGenre = idx;
        proc.setGenre ((Genre) idx, true);
        uiChanged();
    };
    addAndMakeVisible (genreBox);

    generateButton.setColour (juce::TextButton::buttonColourId, sgui::col::orange);
    generateButton.setTooltip ("New groove for the selected genre (keeps the sounds)");
    generateButton.onClick = [this] { proc.generatePattern(); uiChanged(); };

    soundsButton.setTooltip ("New kit from your Splice library, matched to the genre");
    soundsButton.onClick = [this] { proc.pickNewSounds(); uiChanged(); };

    clearButton.setTooltip ("Clear the selected instrument");
    clearButton.onClick = [this]
    {
        for (int s = 0; s < kNumSteps; ++s) proc.pattern.set (ui.selectedLane, s, 0);
        uiChanged();
    };

    seqButton.setClickingTogglesState (true);
    seqButton.setTooltip ("On: plays the pattern in sync with Logic. Off: plays only incoming MIDI notes.");
    seqAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "seqOn", seqButton);

    startButton.setClickingTogglesState (true);
    startButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffd8462b));
    startButton.setTooltip ("Play the pattern while Logic is stopped");
    startButton.setToggleState (proc.preview.load(), juce::dontSendNotification);
    startButton.onClick = [this] { proc.preview.store (startButton.getToggleState()); };

    for (auto* b : { &bar1Button, &bar2Button })
        b->getProperties().set ("led", true);
    bar1Button.onClick = [this] { ui.page = 0; uiChanged(); };
    bar2Button.onClick = [this] { ui.page = 1; uiChanged(); };

    folderButton.onClick = [this] { chooseFolder(); };
    rescanButton.onClick = [this] { proc.setSpliceRoot (proc.getSpliceRoot()); };
    saveMidiButton.onClick = [this] { saveMidi(); };

    for (auto* b : { &generateButton, &soundsButton, &clearButton, &seqButton, &startButton, &bar1Button,
                     &bar2Button, &folderButton, &rescanButton, &saveMidiButton })
        addAndMakeVisible (*b);

    for (auto* k : { &density, &shuffle, &humanize, &master })
        addAndMakeVisible (*k);

    for (int l = 0; l < kNumLanes; ++l)
    {
        columns[(size_t) l] = std::make_unique<sgui::InstrumentColumn> (proc, ui, l);
        addAndMakeVisible (*columns[(size_t) l]);
    }

    addAndMakeVisible (display);
    addAndMakeVisible (keys);
    addAndMakeVisible (dragArea);

    for (auto bars : { 2, 4, 8, 16 })
        barsBox.addItem (juce::String (bars) + " BARS", bars);
    barsBox.setSelectedId (8, juce::dontSendNotification);
    addAndMakeVisible (barsBox);

    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb5b5ad));
    statusLabel.setFont (juce::FontOptions (11.5f));
    addAndMakeVisible (statusLabel);

    setSize (kW, kH);
    uiChanged();
    startTimerHz (8);
    timerCallback();
}

SpliceGrooveEditor::~SpliceGrooveEditor()
{
    setLookAndFeel (nullptr);
}

void SpliceGrooveEditor::uiChanged()
{
    bar1Button.setToggleState (ui.page == 0, juce::dontSendNotification);
    bar2Button.setToggleState (ui.page == 1, juce::dontSendNotification);
    for (auto& c : columns) { c->refresh(); c->repaint(); }
    display.repaint();
    keys.repaint();
}

int SpliceGrooveEditor::selectedBars() const
{
    return juce::jmax (2, barsBox.getSelectedId());
}

void SpliceGrooveEditor::chooseFolder()
{
    auto start = proc.getSpliceRoot().isDirectory() ? proc.getSpliceRoot() : SampleLibrary::defaultSpliceFolder();
    chooser = std::make_unique<juce::FileChooser> ("Select your Splice samples folder", start);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.isDirectory()) proc.setSpliceRoot (f);
                          });
}

void SpliceGrooveEditor::saveMidi()
{
    const int bars = selectedBars();
    auto start = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                     .getChildFile (proc.suggestedMidiName (bars) + ".mid");
    chooser = std::make_unique<juce::FileChooser> ("Save MIDI pattern", start, "*.mid");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, bars] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension ("mid");
                              f.deleteFile();
                              juce::FileOutputStream out (f);
                              if (out.openedOk()) proc.createMidiFile (bars).writeTo (out);
                          });
}

void SpliceGrooveEditor::timerCallback()
{
    const int g = (int) proc.getGenre();
    if (g != genreBox.getSelectedItemIndex())
    {
        lastGenre = g;
        genreBox.setSelectedItemIndex (g, juce::dontSendNotification);
    }

    juce::String status;
    if (proc.library.isScanning())
        status << "SCANNING SPLICE LIBRARY...  " << proc.library.getFilesSeen() << " files";
    else if (! proc.getSpliceRoot().isDirectory())
        status << "Splice folder not found - click SPLICE FOLDER (Splice app > Preferences > Folder location)";
    else
    {
        status << proc.getSpliceRoot().getFullPathName() << "   |   " << proc.library.total() << " one-shots: ";
        for (int l = 0; l < kNumLanes; ++l)
            status << sgui::shortLaneName (l) << " " << proc.library.count (l) << (l < kNumLanes - 1 ? "  " : "");
    }
    statusLabel.setText (status, juce::dontSendNotification);

    for (auto& c : columns) c->refresh();
    repaint (juce::Rectangle<int> (kW - 520, 8, 300, kHeaderH - 12)); // tempo display
}

void SpliceGrooveEditor::paint (juce::Graphics& g)
{
    using namespace sgui;

    // ---- header: dark band with logo
    g.setColour (col::darker);
    g.fillRect (0, 0, kW, kHeaderH);
    g.setColour (col::orange);
    g.fillRect (0, kHeaderH - 4, kW, 4);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("Splice Groove", 20, 6, 260, 40, juce::Justification::centredLeft);
    g.setColour (col::orange);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("GROOVE COMPOSER  SG-26", 22, 42, 260, 16, juce::Justification::centredLeft);

    // tempo LCD
    auto lcd = juce::Rectangle<float> ((float) kW - 500.0f, 20.0f, 170.0f, 40.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (lcd, 4.0f);
    g.setColour (juce::Colour (0xff120808));
    g.fillRoundedRectangle (lcd.reduced (3.0f), 3.0f);
    const double host = proc.hostBpm.load();
    const double bpm = host > 1.0 ? host : genreInfo (proc.getGenre()).bpm;
    drawSevenSegment (g, juce::String (bpm, 1), lcd.reduced (16.0f, 8.0f), col::ledOn);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (host > 1.0 ? "TEMPO (LOGIC)" : "TEMPO (PREVIEW)", lcd.withY (4.0f).withHeight (14.0f), juce::Justification::centredLeft);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("GENRE", (int) lcd.getX() - 240, 4, 220, 14, juce::Justification::centredLeft);

    // ---- instrument section: silver panel
    g.setColour (col::panel);
    g.fillRect (0, kInstrTop, kW, kInstrH);

    // ---- middle: dark control slab
    g.setColour (col::dark);
    g.fillRect (0, kMidTop, kW, kMidH);
    g.setColour (col::orange);
    g.fillRect (0, kMidTop, kW, 2);

    // ---- keys section: silver panel
    g.setGradientFill (juce::ColourGradient (col::panel, 0, (float) kKeysTop, col::panelShade, 0, (float) kFooterTop, false));
    g.fillRect (0, kKeysTop, kW, kKeysH);

    // ---- footer
    g.setColour (col::darker);
    g.fillRect (0, kFooterTop, kW, kH - kFooterTop);
}

void SpliceGrooveEditor::resized()
{
    // header
    genreBox.setBounds (kW - 740, 20, 220, 38);
    master.setBounds (kW - 310, 2, 70, kHeaderH - 6);
    generateButton.setBounds (kW - 230, 14, 100, 42);
    soundsButton.setBounds (kW - 122, 14, 106, 42);

    // instrument columns
    const int colW = (kW - 24) / kNumLanes;
    for (int l = 0; l < kNumLanes; ++l)
        columns[(size_t) l]->setBounds (12 + l * colW, kInstrTop + 14, colW, kInstrH - 20);

    // middle slab
    auto mid = juce::Rectangle<int> (16, kMidTop + 12, kW - 32, kMidH - 24);
    auto knobs = mid.removeFromLeft (250);
    const int kw = knobs.getWidth() / 3;
    density.setBounds (knobs.removeFromLeft (kw).withHeight (96));
    shuffle.setBounds (knobs.removeFromLeft (kw).withHeight (96));
    humanize.setBounds (knobs.withHeight (96));
    auto under = juce::Rectangle<int> (16, kMidTop + 118, 250, 36);
    seqButton.setBounds (under.removeFromLeft (120));
    under.removeFromLeft (10);
    clearButton.setBounds (under);
    mid.removeFromLeft (20);
    display.setBounds (mid);

    // keys section
    auto left = juce::Rectangle<int> (16, kKeysTop + 30, 170, kKeysH - 50);
    startButton.setBounds (left.removeFromTop (70));
    left.removeFromTop (14);
    auto bars = left.removeFromTop (40);
    bar1Button.setBounds (bars.removeFromLeft (80));
    bars.removeFromLeft (10);
    bar2Button.setBounds (bars);
    keys.setBounds (206, kKeysTop + 10, kW - 222, kKeysH - 14);

    // footer
    auto f = juce::Rectangle<int> (16, kFooterTop + 10, kW - 32, kH - kFooterTop - 20);
    dragArea.setBounds (f.removeFromLeft (210));
    f.removeFromLeft (8);
    barsBox.setBounds (f.removeFromLeft (100));
    f.removeFromLeft (8);
    saveMidiButton.setBounds (f.removeFromLeft (110));
    f.removeFromLeft (20);
    folderButton.setBounds (f.removeFromLeft (130));
    f.removeFromLeft (8);
    rescanButton.setBounds (f.removeFromLeft (80));
    f.removeFromLeft (14);
    statusLabel.setBounds (f);
}
