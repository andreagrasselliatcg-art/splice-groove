#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace sgui
{
juce::Colour laneColour (int lane);

//==============================================================================
class StepGrid : public juce::Component, private juce::Timer
{
public:
    explicit StepGrid (SpliceGrooveProcessor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    float rowHeight() const { return (float) getHeight() / (float) sg::kNumLanes; }

private:
    void timerCallback() override;
    bool cellAt (juce::Point<float> pos, int& lane, int& step) const;

    SpliceGrooveProcessor& proc;
    int lastPlayStep = -2;
    uint8_t paintValue = 100;
    int lastLane = -1, lastStep = -1;
};

//==============================================================================
class LaneStrip : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    LaneStrip (SpliceGrooveProcessor& p, int laneIndex);
    void resized() override;
    void paint (juce::Graphics&) override;
    void refresh();

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }

private:
    SpliceGrooveProcessor& proc;
    const int lane;
    bool dragOver = false;

    juce::TextButton sampleButton, randomButton { "Rnd" }, muteButton { "M" };
    juce::Slider volume;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volAtt;
};

//==============================================================================
class MidiDragArea : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MidiDragArea (SpliceGrooveProcessor& p, std::function<int()> barsProvider);
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
class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& title);
    void resized() override;
    juce::Slider slider;

private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
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

    SpliceGrooveProcessor& proc;
    sgui::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 700 };

    juce::ComboBox genreBox, barsBox;
    juce::TextButton generateButton { "GENERATE" }, soundsButton { "NEW SOUNDS" },
                     seqButton { "SEQ ON" }, previewButton { "PREVIEW" },
                     folderButton { "Splice folder..." }, rescanButton { "Rescan" },
                     saveMidiButton { "Save MIDI..." };
    juce::Label bpmLabel, folderLabel, statusLabel;

    sgui::LabeledKnob density, swing, humanize, master;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqAtt;

    std::array<std::unique_ptr<sgui::LaneStrip>, sg::kNumLanes> strips;
    sgui::StepGrid grid;
    sgui::MidiDragArea dragArea;

    std::unique_ptr<juce::FileChooser> chooser;
    int lastGenre = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpliceGrooveEditor)
};
