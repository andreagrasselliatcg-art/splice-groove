#include "GenreEngine.h"

namespace sg
{
namespace
{
// One template = probability of a hit on each 16th of a bar, per lane.
// A probability >= 0.99 is a "core" hit that is always played.
struct Template
{
    float prob[kNumLanes][16];
    float vel[kNumLanes];        // base velocity 0..127
    float fill[kNumLanes];       // how much the lane takes part in end-of-loop fills
    float accent[16];            // accent curve for lanes that use it
    bool useAccent[kNumLanes];
};

#define FOUR_ON_FLOOR 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0
#define STD_ACCENT { 1.f,.6f,.85f,.68f, 1.f,.6f,.85f,.68f, 1.f,.6f,.85f,.68f, 1.f,.6f,.85f,.68f }
//                         Kick   Clap   Snare  CH     OH     Shaker Perc   Top
#define STD_USE_ACCENT { false, false, false, true,  true,  true,  true,  true }

const Template techHouse {
    {
        /* Kick   */ { 1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,.08f,.12f },
        /* Clap   */ { 0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,.05f },
        /* Snare  */ { 0,0,0,0, 0,0,0,.06f, 0,0,0,0, 0,0,.1f,.15f },
        /* CH     */ { .9f,.5f,.95f,.6f, .9f,.5f,.95f,.6f, .9f,.5f,.95f,.6f, .9f,.5f,.95f,.7f },
        /* OH     */ { 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0 },
        /* Shaker */ { .7f,.7f,.7f,.7f, .7f,.7f,.7f,.7f, .7f,.7f,.7f,.7f, .7f,.7f,.7f,.7f },
        /* Perc   */ { 0,0,0,.45f, 0,0,.25f,.4f, 0,0,.35f,.5f, 0,.2f,0,.4f },
        /* Top    */ { 0,0,.35f,0, 0,0,.35f,0, 0,0,.35f,0, 0,0,.35f,0 },
    },
    { 120, 110, 90, 85, 95, 70, 95, 80 },
    { .1f, .2f, .6f, .3f, .1f, 0, .5f, .1f },
    STD_ACCENT, STD_USE_ACCENT
};

const Template house {
    {
        /* Kick   */ { FOUR_ON_FLOOR },
        /* Clap   */ { 0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0 },
        /* Snare  */ { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,.05f,0,.1f },
        /* CH     */ { .3f,.5f,.2f,.5f, .3f,.5f,.2f,.5f, .3f,.5f,.2f,.5f, .3f,.5f,.2f,.5f },
        /* OH     */ { 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0 },
        /* Shaker */ { .45f,.45f,.45f,.45f, .45f,.45f,.45f,.45f, .45f,.45f,.45f,.45f, .45f,.45f,.45f,.45f },
        /* Perc   */ { 0,0,0,.2f, 0,0,.25f,0, 0,0,0,.3f, 0,0,.15f,0 },
        /* Top    */ { .2f,0,.55f,0, 0,0,.55f,0, 0,0,.55f,0, 0,0,.55f,0 },
    },
    { 120, 108, 95, 75, 95, 65, 90, 80 },
    { .1f, .3f, .7f, .2f, .1f, 0, .3f, .1f },
    STD_ACCENT, STD_USE_ACCENT
};

const Template deepHouse {
    {
        /* Kick   */ { 1,0,0,0, 1,0,0,.12f, 1,0,.15f,0, 1,0,0,0 },
        /* Clap   */ { 0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0 },
        /* Snare  */ { 0,0,0,0, 0,0,0,.1f, 0,0,0,0, 0,0,0,.12f },
        /* CH     */ { .8f,.6f,.8f,.6f, .8f,.6f,.8f,.6f, .8f,.6f,.8f,.6f, .8f,.6f,.8f,.6f },
        /* OH     */ { 0,0,.7f,0, 0,0,.5f,0, 0,0,.7f,0, 0,0,.5f,0 },
        /* Shaker */ { .85f,.85f,.85f,.85f, .85f,.85f,.85f,.85f, .85f,.85f,.85f,.85f, .85f,.85f,.85f,.85f },
        /* Perc   */ { 0,0,0,.35f, 0,0,.3f,0, 0,.2f,0,.4f, 0,0,.25f,0 },
        /* Top    */ { 0,0,.2f,0, 0,0,.2f,0, 0,0,.2f,0, 0,0,.2f,0 },
    },
    { 115, 95, 70, 70, 80, 60, 85, 65 },
    { .05f, .2f, .4f, .2f, .1f, 0, .4f, .05f },
    STD_ACCENT, STD_USE_ACCENT
};

const Template afroTech {
    {
        /* Kick   */ { 1,0,0,0, 1,0,0,0, 1,0,.1f,0, 1,0,0,0 },
        /* Clap   */ { 0,0,0,0, .9f,0,0,0, 0,0,0,0, .9f,0,0,0 },
        /* Snare  */ { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,.1f },
        /* CH     */ { .25f,.25f,.85f,.25f, .25f,.25f,.85f,.25f, .25f,.25f,.85f,.25f, .25f,.25f,.85f,.25f },
        /* OH     */ { 0,0,.4f,0, 0,0,0,0, 0,0,.4f,0, 0,0,0,0 },
        /* Shaker */ { .9f,.9f,.9f,.9f, .9f,.9f,.9f,.9f, .9f,.9f,.9f,.9f, .9f,.9f,.9f,.9f },
        /* Perc   */ { .3f,0,0,.85f, 0,0,.85f,0, .2f,0,.6f,.8f, 0,0,.7f,.3f },   // tresillo congas/toms
        /* Top    */ { 0,.3f,0,0, 0,.4f,0,.5f, 0,.3f,0,0, 0,.45f,0,.5f },
    },
    { 120, 100, 85, 80, 85, 75, 100, 85 },
    { .1f, .2f, .3f, .2f, .1f, .2f, .8f, .3f },
    // 3-3-2 accent (afro / tribal feel)
    { 1.f,.55f,.6f,1.f, .55f,.6f,1.f,.55f, 1.f,.55f,.6f,1.f, .55f,.6f,1.f,.55f },
    STD_USE_ACCENT
};

const Template indieTech {
    {
        /* Kick   */ { 1,0,0,0, 1,0,0,.15f, 1,0,.2f,0, 1,0,0,0 },
        /* Clap   */ { 0,0,0,0, .35f,0,0,0, 0,0,0,0, .35f,0,0,0 },
        /* Snare  */ { 0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,.15f },
        /* CH     */ { 1,.3f,1,.3f, 1,.3f,1,.3f, 1,.3f,1,.3f, 1,.3f,1,.3f },
        /* OH     */ { 0,0,.5f,0, 0,0,.5f,0, 0,0,.5f,0, 0,0,.5f,0 },
        /* Shaker */ { .3f,.5f,.6f,.5f, .3f,.5f,.6f,.5f, .3f,.5f,.6f,.5f, .3f,.5f,.6f,.5f },
        /* Perc   */ { 0,0,0,.2f, 0,0,0,0, 0,0,.25f,0, 0,.2f,0,0 },
        /* Top    */ { .3f,0,.4f,0, 0,0,.4f,0, 0,0,.4f,0, 0,0,.4f,0 },
    },
    { 120, 90, 115, 85, 85, 70, 85, 80 },
    { .1f, .2f, .8f, .2f, .1f, 0, .3f, .1f },
    STD_ACCENT, STD_USE_ACCENT
};

const Template& templateFor (Genre g)
{
    switch (g)
    {
        case Genre::TechHouse: return techHouse;
        case Genre::House:     return house;
        case Genre::DeepHouse: return deepHouse;
        case Genre::AfroTech:  return afroTech;
        case Genre::IndieTech: return indieTech;
    }
    return techHouse;
}
} // namespace

const GenreInfo& genreInfo (Genre g)
{
    static const std::array<GenreInfo, kNumGenres> table = []
    {
        GenreInfo th { "Tech House", 126.0, 0.54f, { "tech house", "techhouse", "minimal" }, {} };
        th.laneKeywords[Perc] = juce::StringArray { "rim", "bongo", "conga", "perc" };
        th.laneKeywords[Top]  = juce::StringArray { "ride", "top" };

        GenreInfo h { "House", 124.0, 0.56f, { "house", "disco", "classic", "garage" }, {} };
        h.laneKeywords[Top] = juce::StringArray { "ride" };

        GenreInfo dh { "Deep House", 120.0, 0.60f, { "deep house", "deephouse", "deep", "lofi house", "lo fi house" }, {} };
        dh.laneKeywords[Clap] = juce::StringArray { "snap", "finger" };
        dh.laneKeywords[Perc] = juce::StringArray { "rim", "wood", "click" };

        GenreInfo at { "Afro Tech", 122.0, 0.52f, { "afro", "tribal", "african", "organic", "latin", "amapiano" }, {} };
        at.laneKeywords[Perc]   = juce::StringArray { "conga", "bongo", "tom", "djembe", "hand", "talking" };
        at.laneKeywords[Shaker] = juce::StringArray { "shaker", "cabasa", "maraca" };

        GenreInfo it { "Indie Tech", 120.0, 0.50f, { "indie", "nu disco", "nudisco", "indie dance", "dance punk", "live drums" }, {} };
        it.laneKeywords[Snare]  = juce::StringArray { "live", "acoustic", "real", "break", "vinyl" };
        it.laneKeywords[Shaker] = juce::StringArray { "tamb" };
        it.laneKeywords[Top]    = juce::StringArray { "crash", "ride" };

        return std::array<GenreInfo, kNumGenres> { th, h, dh, at, it };
    }();

    return table[(size_t) juce::jlimit (0, kNumGenres - 1, (int) g)];
}

juce::StringArray genreNames()
{
    juce::StringArray names;
    for (int i = 0; i < kNumGenres; ++i)
        names.add (genreInfo ((Genre) i).name);
    return names;
}

PatternData generatePattern (Genre g, const GenerateOptions& o)
{
    const auto& t = templateFor (g);
    juce::Random rng (o.seed);
    PatternData d;

    const float densityScale = 0.25f + 1.5f * juce::jlimit (0.0f, 1.0f, o.density);

    auto hitProb = [&] (int lane, int s16)
    {
        const float p = t.prob[lane][s16];
        if (p >= 0.99f) return 1.0f;
        return juce::jlimit (0.0f, 1.0f, p * densityScale);
    };

    auto makeVel = [&] (int lane, int s16, bool ghost) -> uint8_t
    {
        float v = t.vel[lane];
        if (t.useAccent[lane])
            v *= t.accent[s16];
        if (ghost)
            v *= 0.75f;
        v += (rng.nextFloat() * 2.0f - 1.0f) * o.humanize * 18.0f;
        return (uint8_t) juce::jlimit (1, 127, juce::roundToInt (v));
    };

    bool hits[kNumLanes][kNumSteps] {};

    // Bar 1: roll every step
    for (int l = 0; l < kNumLanes; ++l)
        for (int s = 0; s < 16; ++s)
            hits[l][s] = rng.nextFloat() < hitProb (l, s);

    // Bar 2: mostly repeat bar 1 (keeps the groove), re-roll some optional hits
    for (int l = 0; l < kNumLanes; ++l)
        for (int s = 0; s < 16; ++s)
        {
            if (t.prob[l][s] >= 0.99f || rng.nextFloat() < 0.65f)
                hits[l][16 + s] = hits[l][s];
            else
                hits[l][16 + s] = rng.nextFloat() < hitProb (l, s);
        }

    // Fill at the end of the loop (last beat, or last 2 beats when very dense)
    const int fillStart = o.density > 0.75f ? 24 : 28;
    for (int l = 0; l < kNumLanes; ++l)
    {
        if (t.fill[l] <= 0.0f || l == Kick)
            continue;

        const float p = t.fill[l] * o.density * 0.9f;
        for (int s = fillStart; s < kNumSteps; ++s)
            if (rng.nextFloat() < p)
                hits[l][s] = true;
    }

    // Choke logic: no closed hat exactly on an open hat
    for (int s = 0; s < kNumSteps; ++s)
        if (hits[OpenHat][s])
            hits[ClosedHat][s] = false;

    for (int l = 0; l < kNumLanes; ++l)
        for (int s = 0; s < kNumSteps; ++s)
        {
            if (! hits[l][s])
                continue;

            const int s16 = s % 16;
            const bool ghost = t.prob[l][s16] < 0.99f && (l == Kick || l == Clap || l == Snare);
            auto v = makeVel (l, s16, ghost);

            // crescendo on snare/clap rolls
            if ((l == Snare || l == Clap) && s >= fillStart && t.prob[l][s16] < 0.99f)
                v = (uint8_t) juce::jlimit (1, 127, (int) v - (kNumSteps - 1 - s) * 6);

            d.vel[l][s] = v;
        }

    return d;
}
} // namespace sg
