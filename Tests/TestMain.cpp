// Headless test harness: fake Splice library, simulated host transport, MIDI export, state, GUI snapshot.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace sg;

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::cout << "FAIL: " << msg << "\n"; } else std::cout << "ok:   " << msg << "\n"; } while (0)

static void writeWav (const juce::File& f, double freq, double seconds, bool noise)
{
    f.getParentDirectory().createDirectory();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 44100, 1, 16, {}, 0));
    if (w == nullptr) return;
    os.release();
    const int n = (int) (44100 * seconds);
    juce::AudioBuffer<float> b (1, n);
    juce::Random r (42);
    for (int i = 0; i < n; ++i)
    {
        const float env = std::exp (-6.0f * (float) i / (float) n);
        const float s = noise ? r.nextFloat() * 2 - 1 : (float) std::sin (2 * juce::MathConstants<double>::pi * freq * i / 44100.0);
        b.setSample (0, i, 0.8f * env * s);
    }
    w->writeFromAudioSampleBuffer (b, 0, n);
}

struct FakePlayHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 125;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm);
        p.setPpqPosition (ppq);
        p.setIsPlaying (true);
        return p;
    }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    // --- 1. fake Splice folder ------------------------------------------------
    auto root = juce::File ("/tmp/FakeSplice/sounds");
    root.deleteRecursively();
    auto packs = root.getChildFile ("packs");
    auto afro = packs.getChildFile ("Afro Tech Rituals").getChildFile ("AT_samples").getChildFile ("drum_one_shots");
    auto th   = packs.getChildFile ("Tech House Loops & One Shots").getChildFile ("TH_samples");

    struct F { juce::File f; int expected; double freq; bool noise; };
    std::vector<F> files = {
        { afro.getChildFile ("kicks/AT_kick_deep_01.wav"), Kick, 55, false },
        { afro.getChildFile ("claps/AT_Clap_Wide.wav"), Clap, 0, true },
        { afro.getChildFile ("percussion/AT_conga_hi_03.wav"), Perc, 300, false },
        { afro.getChildFile ("percussion/AT_bongo_slap.wav"), Perc, 450, false },
        { afro.getChildFile ("shakers/AT_shaker_02.wav"), Shaker, 0, true },
        { th.getChildFile ("drum_one_shots/TH_Kick_Punchy_02.wav"), Kick, 50, false },
        { th.getChildFile ("drum_one_shots/TH_ClosedHat_05.wav"), ClosedHat, 0, true },
        { th.getChildFile ("drum_one_shots/TH_open_hat_02.wav"), OpenHat, 0, true },
        { th.getChildFile ("drum_one_shots/TH_hihat_closed_tight.wav"), ClosedHat, 0, true },
        { th.getChildFile ("drum_one_shots/snares/Snare_Crack.wav"), Snare, 200, true },
        { th.getChildFile ("drum_one_shots/rides/TH_Ride_01.wav"), Top, 0, true },
        { th.getChildFile ("drum_one_shots/TH_Rim_03.wav"), Perc, 800, false },
        { th.getChildFile ("drum_one_shots/kicks/thud_01.wav"), Kick, 45, false },          // by folder
        { th.getChildFile ("drum_loops/TH_125_drum_loop_full.wav"), -1, 0, true },          // loop: excluded
        { th.getChildFile ("top_loops/TH_125_top_shaker.wav"), -1, 0, true },               // loop folder: excluded
        { th.getChildFile ("drum_one_shots/TH_Tamb_01.wav"), Shaker, 0, true },
        { th.getChildFile ("synth_one_shots/TH_stab_Cm.wav"), -1, 400, false },             // not drums
    };

    for (auto& f : files)
        writeWav (f.f, f.freq, 0.3, f.noise);

    for (auto& f : files)
        CHECK (SampleLibrary::classify (f.f) == f.expected,
               "classify " + f.f.getFileName() + " -> " + juce::String (SampleLibrary::classify (f.f)));

    // --- 2. processor + scan ----------------------------------------------------
    auto proc = std::make_unique<SpliceGrooveProcessor>();
    proc->setSpliceRoot (root);
    for (int i = 0; i < 200 && proc->library.isScanning(); ++i)
        juce::Thread::sleep (20);
    CHECK (! proc->library.isScanning(), "scan finished");
    CHECK (proc->library.total() == 14, "library total = " + juce::String (proc->library.total()));

    proc->setGenre (Genre::AfroTech, true);
    // afro genre should prefer the afro pack (85%) -> perc lane must be conga/bongo/rim
    std::cout << "Afro kit:\n";
    for (int l = 0; l < kNumLanes; ++l)
        std::cout << "   " << laneName (l) << ": " << proc->getLaneFile (l).getFileName() << "\n";
    CHECK (proc->getLaneFile (Kick).existsAsFile(), "kick loaded");
    CHECK (proc->getLaneFile (OpenHat).existsAsFile(), "open hat loaded");

    // --- 3. patterns per genre ----------------------------------------------------
    for (int g = 0; g < kNumGenres; ++g)
    {
        GenerateOptions o; o.seed = 1234 + g; o.density = 0.6f;
        auto d = generatePattern ((Genre) g, o);
        std::cout << "\n== " << genreInfo ((Genre) g).name << " (" << genreInfo ((Genre) g).bpm << " BPM, swing "
                  << genreInfo ((Genre) g).swing << ")\n";
        for (int l = 0; l < kNumLanes; ++l)
        {
            juce::String row = juce::String (laneName (l)).paddedRight (' ', 11);
            for (int s = 0; s < kNumSteps; ++s)
            {
                if (s == 16) row << "| ";
                const auto v = d.vel[l][s];
                row << (v == 0 ? "." : v > 105 ? "X" : v > 75 ? "x" : "o");
            }
            std::cout << row << "\n";
        }
        int kicks = 0;
        for (int s = 0; s < kNumSteps; s += 4) kicks += d.vel[Kick][s] > 0;
        CHECK (kicks == 8, "four on the floor kick");
    }

    // --- 4. render with simulated Logic transport --------------------------------
    proc->setGenre (Genre::TechHouse, false);
    proc->generatePattern();
    proc->pickNewSounds();
    FakePlayHead ph;
    proc->setPlayHead (&ph);
    proc->setPlayConfigDetails (0, 2, 48000.0, 512);
    proc->prepareToPlay (48000.0, 512);

    const int block = 512;
    const int total = (int) (48000 * 60.0 / 125.0 * 8); // 2 bars
    juce::AudioBuffer<float> out (2, total + block);
    out.clear();
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total; pos += block)
    {
        ph.ppq = pos / (48000.0 * 60.0 / 125.0);
        proc->processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, pos, buf, c, 0, block);
    }
    const float peak = out.getMagnitude (0, total);
    CHECK (peak > 0.05f, "audio rendered, peak " + juce::String (peak));

    // kick onset at ppq 0 (sample 0) and at beat 2 (sample 23040)
    auto rmsAround = [&] (int s) { return out.getRMSLevel (0, s, 400); };
    std::cout << "rms at beat1=" << rmsAround (0) << " beat2=" << rmsAround (23040) << "\n";
    CHECK (rmsAround (23040) > 0.01f, "kick on beat 2");

    {
        juce::WavAudioFormat wav;
        auto f = juce::File ("/tmp/sg_render.wav");
        f.deleteFile();
        std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), 48000, 2, 24, {}, 0));
        os.release();
        w->writeFromAudioSampleBuffer (out, 0, total);
    }

    // incoming MIDI note 36 with sequencer off should play the kick
    proc->apvts.getParameter ("seqOn")->setValueNotifyingHost (0.0f);
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 120), 10);
    ph.ppq = 100.37;
    juce::AudioBuffer<float> buf2 (2, block);
    proc->processBlock (buf2, midi);
    CHECK (buf2.getMagnitude (0, block) > 0.01f, "MIDI note 36 triggers kick with sequencer off");
    proc->apvts.getParameter ("seqOn")->setValueNotifyingHost (1.0f);

    // --- 5. MIDI export ---------------------------------------------------------
    auto mf = proc->createMidiFile (8);
    int notes = 0;
    for (auto* ev : *mf.getTrack (0))
        if (ev->message.isNoteOn()) ++notes;
    int expected = 0;
    for (int l = 0; l < kNumLanes; ++l)
        for (int s = 0; s < kNumSteps; ++s)
            expected += proc->pattern.get (l, s) > 0;
    CHECK (notes == expected * 4, "MIDI 8 bars notes=" + juce::String (notes) + " expected=" + juce::String (expected * 4));
    auto tmp = proc->writeMidiToTempFile (8);
    CHECK (tmp.existsAsFile() && tmp.getSize() > 100, "temp midi " + tmp.getFullPathName());

    // --- 6. state round trip -----------------------------------------------------
    juce::MemoryBlock state;
    proc->getStateInformation (state);
    auto proc2 = std::make_unique<SpliceGrooveProcessor>();
    proc2->setStateInformation (state.getData(), (int) state.getSize());
    CHECK (proc2->pattern.toHex() == proc->pattern.toHex(), "pattern restored");
    CHECK (proc2->getLaneFile (Kick) == proc->getLaneFile (Kick), "samples restored");
    CHECK (proc2->getGenre() == Genre::TechHouse, "genre restored");

    // --- 7. GUI snapshot ---------------------------------------------------------
    {
        proc->playStep = 9;
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditorIfNeeded());
        ed->setVisible (true);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File png ("/tmp/sg_gui.png");
        png.deleteFile();
        juce::FileOutputStream os (png);
        juce::PNGImageFormat().writeImageToStream (img, os);
        CHECK (png.getSize() > 1000, "gui snapshot");
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURES") << "\n";
    return failures == 0 ? 0 : 1;
}
