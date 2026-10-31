#pragma once

#include "Pattern.h"
#include <juce_core/juce_core.h>

namespace sg
{
enum class Genre { TechHouse = 0, House, DeepHouse, AfroTech, IndieTech };
constexpr int kNumGenres = 5;

struct GenreInfo
{
    juce::String name;
    double bpm;               // suggested tempo
    float swing;              // 0.5 straight .. 0.75 heavy
    juce::StringArray pathKeywords;              // words in Splice pack/folder names that match the genre
    juce::StringArray laneKeywords[kNumLanes];   // extra sound preferences per lane
};

const GenreInfo& genreInfo (Genre g);
juce::StringArray genreNames();

struct GenerateOptions
{
    float density = 0.5f;     // 0..1 how busy
    float humanize = 0.3f;    // 0..1 velocity variation
    juce::int64 seed = 0;
};

PatternData generatePattern (Genre g, const GenerateOptions& options);
} // namespace sg
