#pragma once

#include "GenreEngine.h"
#include <juce_core/juce_core.h>

namespace sg
{
// Scans the local Splice folder (by default ~/Splice) for drum one-shots and
// sorts them into lanes using file and folder names.
class SampleLibrary : private juce::Thread
{
public:
    SampleLibrary();
    ~SampleLibrary() override;

    static juce::File defaultSpliceFolder();
    static bool isAudioFile (const juce::File& f);

    // Classify a single file. Returns lane index or -1 (also -1 for loops).
    static int classify (const juce::File& file);

    void startScan (const juce::File& root);
    bool isScanning() const { return isThreadRunning(); }
    int getScanGeneration() const { return generation.load(); }
    int getFilesSeen() const { return filesSeen.load(); }

    int count (int lane) const;
    int total() const;

    juce::File pickRandom (int lane, Genre genre, juce::Random& rng, const juce::File& exclude) const;

private:
    void run() override;

    struct Entry
    {
        juce::File file;
        juce::String norm;   // lowercase path, separators replaced by spaces
    };

    mutable juce::CriticalSection lock;
    std::array<std::vector<Entry>, kNumLanes> entries;
    juce::File rootToScan;
    std::atomic<int> generation { 0 };
    std::atomic<int> filesSeen { 0 };
};
} // namespace sg
