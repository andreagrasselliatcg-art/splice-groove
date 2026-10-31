#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <string>

namespace sg
{
constexpr int kNumLanes = 8;
constexpr int kNumSteps = 32;               // 2 bars of 16ths
constexpr double kStepPpq = 0.25;           // a 16th note in quarter notes
constexpr double kLoopPpq = kNumSteps * kStepPpq;

enum LaneId { Kick = 0, Clap, Snare, ClosedHat, OpenHat, Shaker, Perc, Top };

inline const char* laneName (int lane)
{
    static const char* names[kNumLanes] = { "Kick", "Clap", "Snare", "Closed Hat",
                                            "Open Hat", "Shaker", "Perc", "Top/Ride" };
    return names[lane];
}

// GM-style drum notes (C1 = 36 = Kick in Logic)
inline int laneMidiNote (int lane)
{
    static const int notes[kNumLanes] = { 36, 39, 38, 42, 46, 70, 62, 51 };
    return notes[lane];
}

inline int laneForMidiNote (int note)
{
    for (int l = 0; l < kNumLanes; ++l)
        if (laneMidiNote (l) == note)
            return l;

    switch (note)
    {
        case 35: return Kick;
        case 40: return Snare;
        case 37: return Perc;
        case 44: return ClosedHat;
        case 49: case 57: case 59: return Top;
        case 54: case 69: return Shaker;
        case 60: case 61: case 63: case 64: return Perc;
        default: return -1;
    }
}

// Position of a step in quarter notes inside the loop, with swing (0.5 = straight, 0.75 = hard swing)
inline double stepToPpq (int step, double swing)
{
    const int pair = step / 2;
    return pair * 0.5 + ((step & 1) ? swing * 0.5 : 0.0);
}

struct PatternData
{
    uint8_t vel[kNumLanes][kNumSteps] {};
};

// Lock-free pattern storage shared between GUI and audio thread.
class Pattern
{
public:
    Pattern() { clear(); }

    uint8_t get (int lane, int step) const noexcept { return cells[index (lane, step)].load (std::memory_order_relaxed); }
    void set (int lane, int step, uint8_t v) noexcept { cells[index (lane, step)].store (v, std::memory_order_relaxed); }

    void clear() noexcept
    {
        for (auto& c : cells)
            c.store (0, std::memory_order_relaxed);
    }

    PatternData snapshot() const noexcept
    {
        PatternData d;
        for (int l = 0; l < kNumLanes; ++l)
            for (int s = 0; s < kNumSteps; ++s)
                d.vel[l][s] = get (l, s);
        return d;
    }

    void assign (const PatternData& d) noexcept
    {
        for (int l = 0; l < kNumLanes; ++l)
            for (int s = 0; s < kNumSteps; ++s)
                set (l, s, d.vel[l][s]);
    }

    std::string toHex() const
    {
        static const char* hex = "0123456789abcdef";
        std::string out;
        out.reserve (cells.size() * 2);
        for (auto& c : cells)
        {
            const auto v = c.load();
            out.push_back (hex[v >> 4]);
            out.push_back (hex[v & 15]);
        }
        return out;
    }

    void fromHex (const std::string& s)
    {
        auto nib = [] (char ch) -> int
        {
            if (ch >= '0' && ch <= '9') return ch - '0';
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            return 0;
        };

        if (s.size() != cells.size() * 2)
            return;

        for (size_t i = 0; i < cells.size(); ++i)
            cells[i].store ((uint8_t) ((nib (s[i * 2]) << 4) | nib (s[i * 2 + 1])));
    }

private:
    static size_t index (int lane, int step) noexcept { return (size_t) (lane * kNumSteps + step); }
    std::array<std::atomic<uint8_t>, (size_t) (kNumLanes * kNumSteps)> cells;
};
} // namespace sg
