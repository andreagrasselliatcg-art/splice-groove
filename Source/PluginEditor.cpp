#include "PluginEditor.h"

using namespace sg;

namespace
{
const juce::Colour bgDark   { 0xff121318 };
const juce::Colour bgPanel  { 0xff1b1d24 };
const juce::Colour bgCell   { 0xff262933 };
const juce::Colour bgCellB  { 0xff2e3240 };
const juce::Colour textDim  { 0xff8a90a2 };
const juce::Colour accent   { 0xffff7a30 };
} // namespace

namespace sgui
{
juce::Colour laneColour (int lane)
{
    static const juce::uint32 cols[kNumLanes] = { 0xffff7a30, 0xffff4f8b, 0xffe8434a, 0xfff5d142,
                                                  0xffa3e048, 0xff3fd1c0, 0xff4f9dff, 0xffa875ff };
    return juce::Colour (cols[lane]);
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, bgDark);
    setColour (juce::TextButton::buttonColourId, bgCellB);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::ComboBox::backgroundColourId, bgCellB);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, bgPanel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, bgCellB);
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, textDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float start, float end, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const float stroke = juce::jmax (2.0f, radius * 0.18f);

    juce::Path track, value;
    track.addCentredArc (centre.x, centre.y, radius - stroke, radius - stroke, 0.0f, start, end, true);
    value.addCentredArc (centre.x, centre.y, radius - stroke, radius - stroke, 0.0f, start, angle, true);

    g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto tip = centre.getPointOnCircumference (radius - stroke * 2.2f, angle);
    g.setColour (juce::Colours::white);
    g.drawLine ({ centre, tip }, juce::jmax (1.5f, stroke * 0.6f));
}

//==============================================================================
StepGrid::StepGrid (SpliceGrooveProcessor& p) : proc (p)
{
    startTimerHz (30);
}

void StepGrid::timerCallback()
{
    const int s = proc.playStep.load();
    if (s != lastPlayStep)
    {
        lastPlayStep = s;
        repaint();
    }
}

bool StepGrid::cellAt (juce::Point<float> pos, int& lane, int& step) const
{
    const float cw = (float) getWidth() / (float) kNumSteps;
    lane = (int) (pos.y / rowHeight());
    step = (int) (pos.x / cw);
    return lane >= 0 && lane < kNumLanes && step >= 0 && step < kNumSteps;
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    int lane, step;
    if (! cellAt (e.position, lane, step))
        return;

    const auto current = proc.pattern.get (lane, step);

    if (e.mods.isPopupMenu())
    {
        // right click / ctrl-click: cycle velocity of an active step
        if (current > 0)
        {
            const uint8_t next = current > 110 ? 70 : current > 80 ? 40 : 127;
            proc.pattern.set (lane, step, next);
        }
        paintValue = 255; // no painting on drag
    }
    else
    {
        paintValue = current > 0 ? 0 : (e.mods.isShiftDown() ? 127 : 100);
        proc.pattern.set (lane, step, paintValue);
        if (paintValue > 0)
            proc.audition (lane);
    }

    lastLane = lane;
    lastStep = step;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (paintValue == 255)
        return;

    int lane, step;
    if (! cellAt (e.position, lane, step) || (lane == lastLane && step == lastStep))
        return;

    proc.pattern.set (lane, step, paintValue);
    lastLane = lane;
    lastStep = step;
    repaint();
}

void StepGrid::paint (juce::Graphics& g)
{
    const float cw = (float) getWidth() / (float) kNumSteps;
    const float rh = rowHeight();
    const int play = proc.playStep.load();

    for (int l = 0; l < kNumLanes; ++l)
    {
        const auto col = laneColour (l);
        for (int s = 0; s < kNumSteps; ++s)
        {
            auto r = juce::Rectangle<float> (s * cw, l * rh, cw, rh).reduced (2.0f, 4.0f);
            const bool beatShade = ((s / 4) % 2) == 0;
            g.setColour (beatShade ? bgCellB : bgCell);
            g.fillRoundedRectangle (r, 3.0f);

            const auto v = proc.pattern.get (l, s);
            if (v > 0)
            {
                const float a = 0.35f + 0.65f * (v / 127.0f);
                g.setColour (col.withAlpha (a));
                g.fillRoundedRectangle (r, 3.0f);

                // velocity bar
                const float vh = (r.getHeight() - 6.0f) * (v / 127.0f);
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRect (r.getX() + 3.0f, r.getBottom() - 3.0f - vh, 3.0f, vh);
            }

            if (s == play)
            {
                g.setColour (juce::Colours::white.withAlpha (0.25f));
                g.fillRoundedRectangle (r, 3.0f);
            }
        }
    }

    // bar separator
    g.setColour (accent.withAlpha (0.6f));
    g.fillRect (16.0f * cw - 1.0f, 0.0f, 2.0f, (float) getHeight());
}

//==============================================================================
LaneStrip::LaneStrip (SpliceGrooveProcessor& p, int laneIndex) : proc (p), lane (laneIndex)
{
    addAndMakeVisible (sampleButton);
    addAndMakeVisible (randomButton);
    addAndMakeVisible (muteButton);
    addAndMakeVisible (volume);

    sampleButton.setTooltip ("Click to audition. Drag a sample here from Finder or the Splice app to replace it.");
    sampleButton.onClick = [this] { proc.audition (lane); };
    sampleButton.setColour (juce::TextButton::buttonColourId, bgCell);
    sampleButton.setColour (juce::TextButton::textColourOffId, textDim);

    randomButton.setTooltip ("Pick another sound from your Splice library");
    randomButton.onClick = [this]
    {
        proc.pickNewSound (lane);
        proc.audition (lane);
        refresh();
    };

    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffe8434a));
    muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, "mute" + juce::String (lane), muteButton);

    volume.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    volume.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    volume.setColour (juce::Slider::rotarySliderFillColourId, laneColour (lane));
    volAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, "vol" + juce::String (lane), volume);

    refresh();
}

void LaneStrip::refresh()
{
    const auto f = proc.getLaneFile (lane);
    sampleButton.setButtonText (f.existsAsFile() ? f.getFileNameWithoutExtension() : juce::String ("(no sample)"));
}

void LaneStrip::resized()
{
    auto r = getLocalBounds().reduced (0, 4);
    r.removeFromLeft (78); // name painted
    volume.setBounds (r.removeFromRight (34));
    r.removeFromRight (4);
    muteButton.setBounds (r.removeFromRight (26));
    r.removeFromRight (4);
    randomButton.setBounds (r.removeFromRight (40));
    r.removeFromRight (4);
    sampleButton.setBounds (r);
}

void LaneStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (0, 4).toFloat();
    g.setColour (laneColour (lane));
    g.fillRoundedRectangle (r.getX(), r.getY(), 4.0f, r.getHeight(), 2.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (laneName (lane), r.withX (10.0f).withWidth (68.0f), juce::Justification::centredLeft);

    if (dragOver)
    {
        g.setColour (accent);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }
}

bool LaneStrip::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (SampleLibrary::isAudioFile (juce::File (f)))
            return true;
    return false;
}

void LaneStrip::filesDropped (const juce::StringArray& files, int, int)
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
MidiDragArea::MidiDragArea (SpliceGrooveProcessor& p, std::function<int()> barsProvider)
    : proc (p), getBars (std::move (barsProvider))
{
    setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    setTooltip ("Drag this onto a Logic track to create a MIDI region");
}

void MidiDragArea::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (isMouseOver() ? accent : accent.withAlpha (0.8f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("DRAG MIDI TO LOGIC", r, juce::Justification::centred);
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

//==============================================================================
LabeledKnob::LabeledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& title)
{
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
    label.setText (title, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, textDim);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);
    slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    slider.updateText();
}

void LabeledKnob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}
} // namespace sgui

//==============================================================================
SpliceGrooveEditor::SpliceGrooveEditor (SpliceGrooveProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      density (p.apvts, "density", "DENSITY"),
      swing (p.apvts, "swing", "SWING"),
      humanize (p.apvts, "humanize", "HUMANIZE"),
      master (p.apvts, "master", "MASTER"),
      grid (p),
      dragArea (p, [this] { return selectedBars(); })
{
    setLookAndFeel (&lnf);

    genreBox.addItemList (genreNames(), 1);
    genreBox.setSelectedItemIndex ((int) proc.getGenre(), juce::dontSendNotification);
    lastGenre = (int) proc.getGenre();
    genreBox.onChange = [this]
    {
        const int idx = genreBox.getSelectedItemIndex();
        if (idx < 0 || idx == lastGenre)
            return;
        lastGenre = idx;
        proc.setGenre ((Genre) idx, true);
        for (auto& s : strips) s->refresh();
        grid.repaint();
    };
    addAndMakeVisible (genreBox);

    generateButton.setColour (juce::TextButton::buttonColourId, accent);
    generateButton.setColour (juce::TextButton::textColourOffId, juce::Colours::black);
    generateButton.setTooltip ("New groove for the selected genre (keeps the sounds)");
    generateButton.onClick = [this] { proc.generatePattern(); grid.repaint(); };
    addAndMakeVisible (generateButton);

    soundsButton.setTooltip ("Pick a new kit from your Splice library, matched to the genre");
    soundsButton.onClick = [this] { proc.pickNewSounds(); for (auto& s : strips) s->refresh(); };
    addAndMakeVisible (soundsButton);

    seqButton.setClickingTogglesState (true);
    seqButton.setTooltip ("On: the plugin plays its pattern in sync with Logic. Off: it only plays incoming MIDI notes.");
    seqAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "seqOn", seqButton);
    addAndMakeVisible (seqButton);

    previewButton.setClickingTogglesState (true);
    previewButton.setTooltip ("Play the pattern while Logic is stopped");
    previewButton.setToggleState (proc.preview.load(), juce::dontSendNotification);
    previewButton.onClick = [this] { proc.preview.store (previewButton.getToggleState()); };
    addAndMakeVisible (previewButton);

    bpmLabel.setJustificationType (juce::Justification::centredRight);
    bpmLabel.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (bpmLabel);

    for (auto* k : { &density, &swing, &humanize, &master })
        addAndMakeVisible (*k);

    folderButton.onClick = [this] { chooseFolder(); };
    rescanButton.onClick = [this] { proc.setSpliceRoot (proc.getSpliceRoot()); };
    addAndMakeVisible (folderButton);
    addAndMakeVisible (rescanButton);

    folderLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    folderLabel.setFont (juce::FontOptions (13.0f));
    statusLabel.setColour (juce::Label::textColourId, textDim);
    statusLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (folderLabel);
    addAndMakeVisible (statusLabel);

    for (int l = 0; l < kNumLanes; ++l)
    {
        strips[(size_t) l] = std::make_unique<sgui::LaneStrip> (proc, l);
        addAndMakeVisible (*strips[(size_t) l]);
    }
    addAndMakeVisible (grid);
    addAndMakeVisible (dragArea);

    for (auto bars : { 2, 4, 8, 16 })
        barsBox.addItem (juce::String (bars) + " bars", bars);
    barsBox.setSelectedId (8, juce::dontSendNotification);
    addAndMakeVisible (barsBox);

    saveMidiButton.onClick = [this] { saveMidi(); };
    addAndMakeVisible (saveMidiButton);

    setSize (1180, 590);
    startTimerHz (8);
    timerCallback();
}

SpliceGrooveEditor::~SpliceGrooveEditor()
{
    setLookAndFeel (nullptr);
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
                              if (f.isDirectory())
                                  proc.setSpliceRoot (f);
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
                              if (f == juce::File())
                                  return;
                              f = f.withFileExtension ("mid");
                              f.deleteFile();
                              juce::FileOutputStream out (f);
                              if (out.openedOk())
                                  proc.createMidiFile (bars).writeTo (out);
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

    const auto& info = genreInfo ((Genre) g);
    const double host = proc.hostBpm.load();
    juce::String bpmText;
    if (host > 1.0)
        bpmText << "Logic: " << juce::String (host, 1) << " BPM";
    else
        bpmText << "Preview: " << juce::String (info.bpm, 0) << " BPM";
    bpmText << "   |   " << info.name << " sweet spot: " << juce::String (info.bpm, 0) << " BPM";
    bpmLabel.setText (bpmText, juce::dontSendNotification);

    folderLabel.setText (proc.getSpliceRoot().getFullPathName(), juce::dontSendNotification);

    juce::String status;
    if (proc.library.isScanning())
        status << "Scanning Splice library... " << proc.library.getFilesSeen() << " files";
    else if (! proc.getSpliceRoot().isDirectory())
        status << "Splice folder not found - click \"Splice folder...\" (Splice app > Preferences > Folder location)";
    else
    {
        status << proc.library.total() << " drum one-shots:  ";
        for (int l = 0; l < kNumLanes; ++l)
            status << laneName (l) << " " << proc.library.count (l) << (l < kNumLanes - 1 ? "  |  " : "");
    }
    statusLabel.setText (status, juce::dontSendNotification);

    for (auto& s : strips)
        s->refresh();
}

void SpliceGrooveEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgDark);

    auto header = getLocalBounds().removeFromTop (56);
    g.setColour (bgPanel);
    g.fillRect (header);

    g.setColour (accent);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("SPLICE", 16, 0, 90, 56, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawText ("GROOVE", 98, 0, 110, 56, juce::Justification::centredLeft);

    g.setColour (bgPanel);
    g.fillRoundedRectangle (juce::Rectangle<float> (12.0f, 146.0f, (float) getWidth() - 24.0f, 392.0f), 8.0f);

    // step numbers
    g.setColour (textDim);
    g.setFont (juce::FontOptions (10.0f));
    const auto gb = grid.getBounds();
    const float cw = (float) gb.getWidth() / (float) kNumSteps;
    for (int s = 0; s < kNumSteps; s += 4)
        g.drawText (juce::String (s / 16 + 1) + "." + juce::String ((s % 16) / 4 + 1),
                    juce::Rectangle<float> (gb.getX() + s * cw + 2.0f, (float) gb.getY() - 14.0f, cw * 4.0f, 12.0f),
                    juce::Justification::centredLeft);

    g.setColour (textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Click = on/off   |   Shift+click = accent   |   Right-click = velocity   |   Drag samples from Splice onto a lane",
                getLocalBounds().removeFromBottom (54).withTrimmedLeft (640).withTrimmedRight (16),
                juce::Justification::centredRight);
}

void SpliceGrooveEditor::resized()
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (56).reduced (12, 10);
    header.removeFromLeft (200);
    genreBox.setBounds (header.removeFromLeft (170));
    header.removeFromLeft (10);
    generateButton.setBounds (header.removeFromLeft (110));
    header.removeFromLeft (8);
    soundsButton.setBounds (header.removeFromLeft (120));
    header.removeFromLeft (8);
    seqButton.setBounds (header.removeFromLeft (80));
    header.removeFromLeft (8);
    previewButton.setBounds (header.removeFromLeft (90));
    bpmLabel.setBounds (header);

    auto row2 = r.removeFromTop (78).reduced (16, 6);
    for (auto* k : { &density, &swing, &humanize, &master })
    {
        k->setBounds (row2.removeFromLeft (78));
        row2.removeFromLeft (6);
    }
    row2.removeFromLeft (20);
    auto folderRow = row2.removeFromTop (30);
    folderButton.setBounds (folderRow.removeFromLeft (130));
    folderRow.removeFromLeft (6);
    rescanButton.setBounds (folderRow.removeFromLeft (70));
    folderRow.removeFromLeft (10);
    folderLabel.setBounds (folderRow);
    row2.removeFromTop (6);
    statusLabel.setBounds (row2);

    auto gridArea = juce::Rectangle<int> (16, 166, getWidth() - 32, 8 * 46);
    auto stripArea = gridArea.removeFromLeft (330);
    gridArea.removeFromLeft (10);
    for (int l = 0; l < kNumLanes; ++l)
        strips[(size_t) l]->setBounds (stripArea.removeFromTop (46));
    grid.setBounds (gridArea);

    auto bottom = getLocalBounds().removeFromBottom (54).reduced (16, 9);
    dragArea.setBounds (bottom.removeFromLeft (240));
    bottom.removeFromLeft (10);
    barsBox.setBounds (bottom.removeFromLeft (100));
    bottom.removeFromLeft (10);
    saveMidiButton.setBounds (bottom.removeFromLeft (120));
}
