#include "SampleLibrary.h"

namespace sg
{
namespace
{
juce::StringArray tokenize (const juce::String& text)
{
    juce::StringArray tokens;
    juce::String current;
    int lastKind = 0; // 1 letter, 2 digit

    auto flush = [&]
    {
        if (current.isNotEmpty())
            tokens.add (current);
        current.clear();
    };

    const juce::String lowerText = text.toLowerCase();
    auto p = lowerText.getCharPointer();
    while (! p.isEmpty())
    {
        const auto c = p.getAndAdvance();
        const int kind = juce::CharacterFunctions::isLetter (c) ? 1
                       : juce::CharacterFunctions::isDigit (c) ? 2 : 0;

        if (kind == 0) { flush(); lastKind = 0; continue; }
        if (kind != lastKind) flush();
        current += juce::String::charToString (c);
        lastKind = kind;
    }
    flush();
    return tokens;
}

int classifyText (const juce::String& text)
{
    const auto tokens = tokenize (text);
    const auto lower = text.toLowerCase();

    auto has = [&] (std::initializer_list<const char*> words)
    {
        for (auto* w : words)
            if (tokens.contains (w))
                return true;
        return false;
    };
    auto sub = [&] (std::initializer_list<const char*> words)
    {
        for (auto* w : words)
            if (lower.contains (w))
                return true;
        return false;
    };

    if (has ({ "kick", "kicks", "kck", "bd", "bassdrum" }) || sub ({ "kick" }))
        return Kick;
    if (has ({ "clap", "claps", "clp", "snap", "snaps" }) || sub ({ "clap" }))
        return Clap;
    if (has ({ "snare", "snares", "snr", "sd" }) || sub ({ "snare" }))
        return Snare;

    const bool isHat = has ({ "hat", "hats", "hh", "hihat", "hihats", "hht", "chh", "ohh", "ch", "oh" })
                    || sub ({ "hihat", "hi hat", "hi_hat", "openhat", "closedhat", "ophat" });
    if (isHat)
    {
        const bool open = has ({ "open", "opn", "oh", "ohh", "ophat", "ohat" })
                       || sub ({ "openhat", "open hat", "open_hat", "ophat" });
        return open ? OpenHat : ClosedHat;
    }

    if (has ({ "shaker", "shakers", "shk", "shake", "tamb", "tambourine", "tambo", "maraca", "maracas", "cabasa" })
        || sub ({ "shaker", "tambourine" }))
        return Shaker;

    if (has ({ "ride", "rides", "crash", "crashes", "cymbal", "cymbals", "cym", "top", "tops", "splash", "china" })
        || sub ({ "cymbal" }))
        return Top;

    if (has ({ "perc", "percs", "percussion", "conga", "congas", "bongo", "bongos", "tom", "toms", "cowbell",
               "clave", "claves", "rim", "rims", "rimshot", "block", "wood", "djembe", "timbale", "timbales",
               "agogo", "guiro", "triangle", "click", "tabla" })
        || sub ({ "perc", "conga", "bongo", "cowbell", "rimshot", "djembe" }))
        return Perc;

    return -1;
}

bool tokensContainLoop (const juce::StringArray& t)
{
    return t.contains ("loop") || t.contains ("loops") || t.contains ("fill") || t.contains ("fills")
        || t.contains ("drumloop") || t.contains ("groove") || t.contains ("grooves");
}

bool tokensContainOneShot (const juce::StringArray& t)
{
    if (t.contains ("oneshot") || t.contains ("oneshots"))
        return true;
    const int i = t.indexOf ("one");
    return i >= 0 && i + 1 < t.size() && (t[i + 1] == "shot" || t[i + 1] == "shots");
}
} // namespace

SampleLibrary::SampleLibrary() : juce::Thread ("Splice scanner") {}

SampleLibrary::~SampleLibrary()
{
    stopThread (4000);
}

juce::File SampleLibrary::defaultSpliceFolder()
{
    auto home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    auto splice = home.getChildFile ("Splice");
    if (splice.getChildFile ("sounds").isDirectory())
        return splice.getChildFile ("sounds");
    return splice;
}

bool SampleLibrary::isAudioFile (const juce::File& f)
{
    return f.hasFileExtension ("wav;aif;aiff;flac;mp3;m4a;caf;ogg");
}

int SampleLibrary::classify (const juce::File& file)
{
    const auto stem = file.getFileNameWithoutExtension();
    const auto nameTokens = tokenize (stem);

    if (tokensContainLoop (nameTokens))
        return -1;

    // folder names (pack structure)
    // Walk up from the file: the nearest folder that says "loop" or "one shot" decides.
    // (pack names like "Tech House Loops & One Shots" are usually 3+ levels up and are ignored)
    auto dir = file.getParentDirectory();
    for (int depth = 0; depth < 3; ++depth)
    {
        const auto t = tokenize (dir.getFileName());
        if (tokensContainOneShot (t))
            break;
        if (tokensContainLoop (t))
            return -1;
        dir = dir.getParentDirectory();
    }

    const int byName = classifyText (stem);
    if (byName >= 0)
        return byName;

    // fall back on the two closest folders, e.g. ".../drum_one_shots/kicks/xyz_01.wav"
    auto parent = file.getParentDirectory();
    for (int depth = 0; depth < 2; ++depth)
    {
        const int byFolder = classifyText (parent.getFileName());
        if (byFolder >= 0)
            return byFolder;
        parent = parent.getParentDirectory();
    }
    return -1;
}

void SampleLibrary::startScan (const juce::File& root)
{
    stopThread (4000);
    rootToScan = root;
    filesSeen = 0;
    startThread (juce::Thread::Priority::low);
}

void SampleLibrary::run()
{
    std::array<std::vector<Entry>, kNumLanes> found;

    if (rootToScan.isDirectory())
    {
        for (const auto& it : juce::RangedDirectoryIterator (rootToScan, true, "*", juce::File::findFiles))
        {
            if (threadShouldExit())
                return;

            const auto& f = it.getFile();
            if (! isAudioFile (f) || it.getFileSize() > 20 * 1024 * 1024)
                continue;

            ++filesSeen;
            const int lane = classify (f);
            if (lane < 0)
                continue;

            auto norm = f.getFullPathName().toLowerCase().replaceCharacters ("_-.", "   ");
            found[(size_t) lane].push_back ({ f, norm });

            if (filesSeen.load() > 300000)
                break;
        }
    }

    {
        const juce::ScopedLock sl (lock);
        entries = std::move (found);
    }
    ++generation;
}

int SampleLibrary::count (int lane) const
{
    const juce::ScopedLock sl (lock);
    return (int) entries[(size_t) lane].size();
}

int SampleLibrary::total() const
{
    const juce::ScopedLock sl (lock);
    int n = 0;
    for (auto& e : entries)
        n += (int) e.size();
    return n;
}

juce::File SampleLibrary::pickRandom (int lane, Genre genre, juce::Random& rng, const juce::File& exclude) const
{
    const juce::ScopedLock sl (lock);
    const auto& list = entries[(size_t) lane];
    if (list.empty())
        return {};

    const auto& info = genreInfo (genre);
    std::vector<int> preferred;

    for (int i = 0; i < (int) list.size(); ++i)
    {
        int score = 0;
        for (auto& kw : info.pathKeywords)
            if (list[(size_t) i].norm.contains (kw))
                score += 2;
        for (auto& kw : info.laneKeywords[lane])
            if (list[(size_t) i].norm.contains (kw))
                score += 1;
        if (score > 0)
            preferred.push_back (i);
    }

    const bool usePreferred = ! preferred.empty() && rng.nextFloat() < 0.85f;
    const int poolSize = usePreferred ? (int) preferred.size() : (int) list.size();

    juce::File choice;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const int r = rng.nextInt (poolSize);
        const int idx = usePreferred ? preferred[(size_t) r] : r;
        choice = list[(size_t) idx].file;
        if (choice != exclude)
            break;
    }
    return choice;
}
} // namespace sg
