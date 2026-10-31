#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Vintage drum-machine style UI: silver panel, black knobs, 16 step keys with red LEDs.
namespace sgui
{
namespace col
{
    const juce::Colour panel      { 0xffd9d8d2 };
    const juce::Colour panelShade { 0xffc4c3bc };
    const juce::Colour dark       { 0xff262626 };
    const juce::Colour darker     { 0xff171717 };
    const juce::Colour ink        { 0xff1f1f1f };
    const juce::Colour orange     { 0xffff6b1a };
    const juce::Colour ledOn      { 0xffff2b1f };
    const juce::Colour ledOff     { 0xff4a1512 };
    const juce::Colour key        { 0xffc9c8c1 };
    const juce::Colour lcd        { 0xff1b1d1a };
    const juce::Colour lcdText    { 0xffff8a3d };
}

const char* longLaneName (int lane);
const char* shortLaneName (int lane);
void drawLed (juce::Graphics& g, juce::Point<float> centre, float radius, float brightness);
void drawSevenSegment (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Colour on);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

//==============================================================================
struct UiState
{
    int selectedLane = 0;
    int page = 0;                 // 0 = bar 1, 1 = bar 2
    std::function<void()> changed;
};

//==============================================================================
class InstrumentColumn : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    InstrumentColumn (SpliceGrooveProcessor& p, UiState& ui, int laneIndex);
    void resized() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }

private:
    SpliceGrooveProcessor& proc;
    UiState& ui;
    const int lane;
    bool dragOver = false;

    juce::Slider tune, decay, level;
    juce::TextButton sampleButton, randomButton { "RND" }, muteButton { "MUTE" }, selectButton { "SELECT" };
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SA> tuneAtt, decayAtt, levelAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtt;
};

//==============================================================================
class PatternDisplay : public juce::Component, private juce::Timer
{
public:
    PatternDisplay (SpliceGrooveProcessor& p, UiState& ui);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> matrixArea() const;
    bool cellAt (juce::Point<float>, int& lane, int& step) const;

    SpliceGrooveProcessor& proc;
    UiState& ui;
    int lastPlay = -2;
    uint8_t paintValue = 100;
    int lastLane = -1, lastStep = -1;
};

//==============================================================================
class StepKeys : public juce::Component, private juce::Timer
{
public:
    StepKeys (SpliceGrooveProcessor& p, UiState& ui);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { pressedKey = -1; repaint(); }

private:
    void timerCallback() override;
    juce::Rectangle<float> keyRect (int i) const;
    juce::Point<float> ledPos (int i) const;

    SpliceGrooveProcessor& proc;
    UiState& ui;
    int lastPlay = -2;
    int pressedKey = -1;
};

//==============================================================================
class MidiDragArea : public juce::Component, public juce::SettableTooltipClient
{
public:
    MidiDragArea (SpliceGrooveProcessor& p, std::function<int()> barsProvider);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { dragStarted = false; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    SpliceGrooveProcessor& proc;
    std::function<int()> getBars;
    bool dragStarted = false;
};

//==============================================================================
class PanelKnob : public juce::Component
{
public:
    PanelKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
               const juce::String& title, bool lightText);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;

private:
    juce::String title;
    bool lightText;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};
} // namespace sgui

//==============================================================================
class SpliceGrooveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpliceGrooveEditor (SpliceGrooveProcessor&);
    ~SpliceGrooveEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseFolder();
    void saveMidi();
    int selectedBars() const;
    void uiChanged();

    SpliceGrooveProcessor& proc;
    sgui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 700 };
    sgui::UiState ui;

    juce::ComboBox genreBox, barsBox;
    juce::TextButton generateButton { "GENERATE" }, soundsButton { "NEW SOUNDS" },
                     seqButton { "SEQ" }, startButton { "START / STOP" },
                     bar1Button { "BAR 1" }, bar2Button { "BAR 2" }, clearButton { "CLEAR" },
                     folderButton { "SPLICE FOLDER" }, rescanButton { "RESCAN" },
                     saveMidiButton { "SAVE MIDI" };
    juce::Label statusLabel;

    sgui::PanelKnob density, shuffle, humanize, master;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAtt;

    std::array<std::unique_ptr<sgui::InstrumentColumn>, sg::kNumLanes> columns;
    sgui::PatternDisplay display;
    sgui::StepKeys keys;
    sgui::MidiDragArea dragArea;

    std::unique_ptr<juce::FileChooser> chooser;
    int lastGenre = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpliceGrooveEditor)
};
