#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Pattern.h"
#include "GenreEngine.h"
#include "SampleLibrary.h"

struct SampleData
{
    juce::AudioBuffer<float> buffer;   // always 2 channels
    double sampleRate = 44100.0;
    juce::File file;
};
using SamplePtr = std::shared_ptr<const SampleData>;

class SpliceGrooveProcessor : public juce::AudioProcessor,
                              private juce::Timer
{
public:
    SpliceGrooveProcessor();
    ~SpliceGrooveProcessor() override;

    //== AudioProcessor ===========================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== API used by the editor (message thread) ==================================
    juce::AudioProcessorValueTreeState apvts;
    sg::Pattern pattern;
    sg::SampleLibrary library;

    sg::Genre getGenre() const;
    void setGenre (sg::Genre g, bool regenerateAndPickSounds);
    void generatePattern();
    void pickNewSounds();
    void pickNewSound (int lane);
    bool loadLaneSample (int lane, const juce::File& file);
    juce::File getLaneFile (int lane) const { return laneFiles[(size_t) lane]; }
    void audition (int lane) { pendingAudition.store (lane); }

    void setSpliceRoot (const juce::File& folder);
    juce::File getSpliceRoot() const { return spliceRoot; }

    double getEffectiveBpm() const;
    juce::MidiFile createMidiFile (int bars) const;
    juce::File writeMidiToTempFile (int bars) const;
    juce::String suggestedMidiName (int bars) const;

    std::atomic<int> playStep { -1 };
    std::atomic<bool> preview { false };
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<bool> hostPlaying { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;

    struct Voice
    {
        SamplePtr sample;
        double pos = 0.0, inc = 1.0;
        float gain = 0.0f;
        int release = -1;
        int releaseLen = 1;
        bool active = false;
    };

    struct TriggerEvent
    {
        int offset;
        int lane;
        float velocity; // 0..1
    };

    void startVoice (int lane, float velocity);
    void renderVoices (juce::AudioBuffer<float>& out, int start, int num, float master);
    void scheduleSteps (double ppqStart, double ppqEnd, double samplesPerPpq, int numSamples);

    static constexpr int kReleaseSamples = 96;

    juce::AudioFormatManager formatManager;

    juce::SpinLock sampleLock;
    std::array<SamplePtr, sg::kNumLanes> laneSamples;
    std::array<juce::File, sg::kNumLanes> laneFiles;
    std::vector<SamplePtr> graveyard;

    std::array<std::array<Voice, 2>, sg::kNumLanes> voices;
    std::vector<TriggerEvent> events;

    double currentSampleRate = 44100.0;
    double previewPpq = 0.0;
    bool wasPreviewing = false;
    std::atomic<int> pendingAudition { -1 };

    juce::File spliceRoot;
    int lastLibraryGeneration = 0;
    juce::Random rng;

    std::atomic<float>* pDensity = nullptr;
    std::atomic<float>* pSwing = nullptr;
    std::atomic<float>* pHumanize = nullptr;
    std::atomic<float>* pMaster = nullptr;
    std::atomic<float>* pSeqOn = nullptr;
    std::atomic<float>* pGenre = nullptr;
    std::array<std::atomic<float>*, sg::kNumLanes> pLaneVol {};
    std::array<std::atomic<float>*, sg::kNumLanes> pLaneMute {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpliceGrooveProcessor)
};
