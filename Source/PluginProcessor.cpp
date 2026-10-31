#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sg;

static juce::String laneParamId (const char* prefix, int lane) { return juce::String (prefix) + juce::String (lane); }

juce::AudioProcessorValueTreeState::ParameterLayout SpliceGrooveProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "genre", 1 }, "Genre", genreNames(), 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "density", 1 }, "Density",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing",
                                                       NormalisableRange<float> (0.5f, 0.75f), 0.54f,
                                                       AudioParameterFloatAttributes().withStringFromValueFunction (
                                                           [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.3f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "master", 1 }, "Master",
                                                       NormalisableRange<float> (0.0f, 1.5f), 0.8f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Sequencer On", true));

    for (int l = 0; l < kNumLanes; ++l)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { laneParamId ("vol", l), 1 },
                                                           String (laneName (l)) + " Volume",
                                                           NormalisableRange<float> (0.0f, 1.5f), 0.9f));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { laneParamId ("mute", l), 1 },
                                                          String (laneName (l)) + " Mute", false));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { laneParamId ("tune", l), 1 },
                                                           String (laneName (l)) + " Tune",
                                                           NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                           AudioParameterFloatAttributes().withLabel ("st")));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { laneParamId ("decay", l), 1 },
                                                           String (laneName (l)) + " Decay",
                                                           NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    }
    return layout;
}

SpliceGrooveProcessor::SpliceGrooveProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GrooveX", createLayout())
{
    formatManager.registerBasicFormats();
    events.reserve (2048);
    graveyard.reserve (64);

    pDensity  = apvts.getRawParameterValue ("density");
    pSwing    = apvts.getRawParameterValue ("swing");
    pHumanize = apvts.getRawParameterValue ("humanize");
    pMaster   = apvts.getRawParameterValue ("master");
    pSeqOn    = apvts.getRawParameterValue ("seqOn");
    pGenre    = apvts.getRawParameterValue ("genre");
    for (int l = 0; l < kNumLanes; ++l)
    {
        pLaneVol[(size_t) l]  = apvts.getRawParameterValue (laneParamId ("vol", l));
        pLaneMute[(size_t) l] = apvts.getRawParameterValue (laneParamId ("mute", l));
        pLaneTune[(size_t) l] = apvts.getRawParameterValue (laneParamId ("tune", l));
        pLaneDecay[(size_t) l] = apvts.getRawParameterValue (laneParamId ("decay", l));
    }

    // A first groove so the plugin is never empty
    generatePattern();

    spliceRoot = SampleLibrary::defaultSpliceFolder();
    if (spliceRoot.isDirectory())
        library.startScan (spliceRoot);

    startTimerHz (4);
}

SpliceGrooveProcessor::~SpliceGrooveProcessor()
{
    stopTimer();
}

bool SpliceGrooveProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SpliceGrooveProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& lane : voices)
        for (auto& v : lane)
            v.active = false;
}

//==============================================================================
Genre SpliceGrooveProcessor::getGenre() const
{
    return (Genre) juce::jlimit (0, kNumGenres - 1, (int) pGenre->load());
}

void SpliceGrooveProcessor::setGenre (Genre g, bool regenerateAndPickSounds)
{
    if (auto* p = apvts.getParameter ("genre"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) (int) g));

    if (auto* s = apvts.getParameter ("swing"))
        s->setValueNotifyingHost (s->convertTo0to1 (genreInfo (g).swing));

    if (regenerateAndPickSounds)
    {
        generatePattern();
        pickNewSounds();
    }
}

void SpliceGrooveProcessor::generatePattern()
{
    GenerateOptions o;
    o.density = pDensity->load();
    o.humanize = pHumanize->load();
    o.seed = juce::Random::getSystemRandom().nextInt64();
    pattern.assign (sg::generatePattern (getGenre(), o));
}

void SpliceGrooveProcessor::pickNewSound (int lane)
{
    auto f = library.pickRandom (lane, getGenre(), rng, laneFiles[(size_t) lane]);
    if (f.existsAsFile())
        loadLaneSample (lane, f);
}

void SpliceGrooveProcessor::pickNewSounds()
{
    for (int l = 0; l < kNumLanes; ++l)
        pickNewSound (l);
}

bool SpliceGrooveProcessor::loadLaneSample (int lane, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0)
        return false;

    // One-shots: cap to 4 seconds
    const auto maxLen = (juce::int64) (reader->sampleRate * 4.0);
    const int len = (int) juce::jmin (reader->lengthInSamples, maxLen);

    auto data = std::make_shared<SampleData>();
    data->sampleRate = reader->sampleRate;
    data->file = file;
    data->buffer.setSize (2, len);
    data->buffer.clear();
    reader->read (&data->buffer, 0, len, 0, true, true);
    if (reader->numChannels == 1)
        data->buffer.copyFrom (1, 0, data->buffer, 0, 0, len);

    // short fade out so truncated tails don't click
    const int fade = juce::jmin (len, 256);
    data->buffer.applyGainRamp (len - fade, fade, 1.0f, 0.0f);

    SamplePtr old;
    {
        const juce::SpinLock::ScopedLockType sl (sampleLock);
        old = std::move (laneSamples[(size_t) lane]);
        laneSamples[(size_t) lane] = data;
    }
    if (old != nullptr)
        graveyard.push_back (std::move (old));   // freed later on the message thread

    laneFiles[(size_t) lane] = file;
    return true;
}

void SpliceGrooveProcessor::setSpliceRoot (const juce::File& folder)
{
    spliceRoot = folder;
    library.startScan (folder);
}

void SpliceGrooveProcessor::timerCallback()
{
    // free samples no longer used by the audio thread
    graveyard.erase (std::remove_if (graveyard.begin(), graveyard.end(),
                                     [] (const SamplePtr& p) { return p.use_count() <= 1; }),
                     graveyard.end());

    // first scan finished: fill empty lanes automatically
    const int gen = library.getScanGeneration();
    if (gen != lastLibraryGeneration)
    {
        lastLibraryGeneration = gen;
        for (int l = 0; l < kNumLanes; ++l)
            if (! laneFiles[(size_t) l].existsAsFile())
                pickNewSound (l);
    }
}

double SpliceGrooveProcessor::getEffectiveBpm() const
{
    const double b = hostBpm.load();
    return b > 1.0 ? b : genreInfo (getGenre()).bpm;
}

//==============================================================================
void SpliceGrooveProcessor::startVoice (int lane, float velocity)
{
    SamplePtr s;
    {
        const juce::SpinLock::ScopedTryLockType tl (sampleLock);
        if (! tl.isLocked())
            return;
        s = laneSamples[(size_t) lane];
    }
    if (s == nullptr)
        return;

    // closed hat chokes open hat
    if (lane == ClosedHat)
        for (auto& v : voices[OpenHat])
            if (v.active && v.release < 0)
                v.release = v.releaseLen = kReleaseSamples * 4;

    auto& lv = voices[(size_t) lane];
    for (auto& v : lv)
        if (v.active && v.release < 0)
            v.release = v.releaseLen = kReleaseSamples;

    Voice* target = nullptr;
    for (auto& v : lv)
        if (! v.active) { target = &v; break; }
    if (target == nullptr)
        target = lv[0].release < lv[1].release ? &lv[0] : &lv[1];

    const float vol = pLaneVol[(size_t) lane]->load();
    target->sample = std::move (s);
    target->pos = 0.0;
    const float semis = pLaneTune[(size_t) lane]->load();
    target->inc = target->sample->sampleRate / currentSampleRate * std::pow (2.0, semis / 12.0);

    // DECAY: 1.0 = whole sample, lower = shorter (30 ms .. 3 s)
    const float decay = pLaneDecay[(size_t) lane]->load();
    target->envPos = 0;
    target->envLen = decay >= 0.99f ? 0 : (int) (currentSampleRate * 0.03 * std::pow (100.0, (double) decay));
    target->gain = std::pow (juce::jlimit (0.0f, 1.0f, velocity), 1.4f) * vol;
    target->release = -1;
    target->active = true;
}

void SpliceGrooveProcessor::renderVoices (juce::AudioBuffer<float>& out, int start, int num, float master)
{
    const int numCh = out.getNumChannels();
    float* outL = out.getWritePointer (0);
    float* outR = numCh > 1 ? out.getWritePointer (1) : nullptr;

    for (auto& lane : voices)
        for (auto& v : lane)
        {
            if (! v.active)
                continue;

            const auto& b = v.sample->buffer;
            const int len = b.getNumSamples();
            const float* L = b.getReadPointer (0);
            const float* R = b.getReadPointer (1);

            for (int i = 0; i < num; ++i)
            {
                const int ip = (int) v.pos;
                if (ip + 1 >= len) { v.active = false; break; }

                const float frac = (float) (v.pos - ip);
                float g = v.gain * master;
                if (v.envLen > 0)
                {
                    if (v.envPos >= v.envLen) { v.active = false; break; }
                    const float e = 1.0f - (float) v.envPos / (float) v.envLen;
                    g *= e * e;
                    ++v.envPos;
                }
                if (v.release >= 0)
                {
                    if (v.release == 0) { v.active = false; break; }
                    g *= (float) v.release / (float) v.releaseLen;
                    --v.release;
                }

                const float l = (L[ip] + frac * (L[ip + 1] - L[ip])) * g;
                const float r = (R[ip] + frac * (R[ip + 1] - R[ip])) * g;

                if (outR != nullptr) { outL[start + i] += l; outR[start + i] += r; }
                else                 { outL[start + i] += 0.5f * (l + r); }

                v.pos += v.inc;
            }

            if (! v.active)
                v.sample.reset();
        }
}

void SpliceGrooveProcessor::scheduleSteps (double ppqStart, double ppqEnd, double samplesPerPpq, int numSamples)
{
    const double swing = pSwing->load();

    for (int step = 0; step < kNumSteps; ++step)
    {
        const double t = stepToPpq (step, swing);
        double occ = t + std::ceil ((ppqStart - t) / kLoopPpq) * kLoopPpq;

        for (; occ < ppqEnd; occ += kLoopPpq)
        {
            const int offset = juce::jlimit (0, numSamples - 1, (int) ((occ - ppqStart) * samplesPerPpq));
            for (int l = 0; l < kNumLanes; ++l)
            {
                const auto vel = pattern.get (l, step);
                if (vel > 0 && pLaneMute[(size_t) l]->load() < 0.5f)
                    events.push_back ({ offset, l, vel / 127.0f });
            }
        }
    }
}

void SpliceGrooveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    events.clear();

    // Incoming MIDI (e.g. a region dragged into Logic) plays the lanes
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            const int lane = laneForMidiNote (msg.getNoteNumber());
            if (lane >= 0)
                events.push_back ({ meta.samplePosition, lane, msg.getFloatVelocity() });
        }
    }
    midi.clear();

    const int aud = pendingAudition.exchange (-1);
    if (aud >= 0)
        events.push_back ({ 0, aud, 0.85f });

    // Transport
    bool playing = false;
    double ppq = 0.0, bpm = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition()) { ppq = *p; playing = true; }
        }

    hostBpm.store (bpm);
    hostPlaying.store (playing);
    if (bpm <= 1.0)
        bpm = genreInfo (getGenre()).bpm;

    const bool previewNow = preview.load();
    if (previewNow && ! wasPreviewing)
        previewPpq = 0.0;
    wasPreviewing = previewNow;

    const double samplesPerPpq = currentSampleRate * 60.0 / bpm;
    const double blockPpq = numSamples / samplesPerPpq;

    bool running = playing;
    if (! playing && previewNow)
    {
        running = true;
        ppq = previewPpq;
        previewPpq += blockPpq;
    }

    if (running)
    {
        if (pSeqOn->load() > 0.5f)
            scheduleSteps (ppq, ppq + blockPpq, samplesPerPpq, numSamples);

        const double inLoop = std::fmod (std::max (0.0, ppq), kLoopPpq);
        playStep.store (juce::jlimit (0, kNumSteps - 1, (int) (inLoop / kStepPpq)));
    }
    else
    {
        playStep.store (-1);
    }

    std::sort (events.begin(), events.end(),
               [] (const TriggerEvent& a, const TriggerEvent& b) { return a.offset < b.offset; });

    const float master = pMaster->load();
    int pos = 0;
    for (size_t e = 0; e <= events.size(); ++e)
    {
        const int next = e < events.size() ? juce::jlimit (0, numSamples, events[e].offset) : numSamples;
        if (next > pos)
        {
            renderVoices (buffer, pos, next - pos, master);
            pos = next;
        }
        if (e < events.size())
            startVoice (events[e].lane, events[e].velocity);
    }
}

//==============================================================================
juce::String SpliceGrooveProcessor::suggestedMidiName (int bars) const
{
    auto name = genreInfo (getGenre()).name.removeCharacters (" ");
    return "GrooveX_" + name + "_" + juce::String (juce::roundToInt (getEffectiveBpm())) + "bpm_"
           + juce::String (bars) + "bars";
}

juce::MidiFile SpliceGrooveProcessor::createMidiFile (int bars) const
{
    constexpr int tpq = 960;
    juce::MidiMessageSequence seq;

    seq.addEvent (juce::MidiMessage::textMetaEvent (3, suggestedMidiName (bars)), 0);
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / getEffectiveBpm())), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);

    const double swing = pSwing->load();
    const int loops = juce::jmax (1, bars / 2);

    for (int loop = 0; loop < loops; ++loop)
        for (int step = 0; step < kNumSteps; ++step)
            for (int l = 0; l < kNumLanes; ++l)
            {
                const auto vel = pattern.get (l, step);
                if (vel == 0 || pLaneMute[(size_t) l]->load() > 0.5f)
                    continue;

                const double tick = (loop * kLoopPpq + stepToPpq (step, swing)) * tpq;
                seq.addEvent (juce::MidiMessage::noteOn (1, laneMidiNote (l), (juce::uint8) vel), tick);
                seq.addEvent (juce::MidiMessage::noteOff (1, laneMidiNote (l)), tick + tpq / 8.0);
            }

    seq.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);
    return mf;
}

juce::File SpliceGrooveProcessor::writeMidiToTempFile (int bars) const
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("GrooveX");
    dir.createDirectory();
    auto file = dir.getChildFile (suggestedMidiName (bars) + ".mid");
    file.deleteFile();

    juce::FileOutputStream out (file);
    if (out.openedOk())
    {
        createMidiFile (bars).writeTo (out);
        out.flush();
    }
    return file;
}

//==============================================================================
void SpliceGrooveProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("SGExtra"), nullptr);

    juce::ValueTree extra ("SGExtra");
    extra.setProperty ("pattern", juce::String (pattern.toHex()), nullptr);
    extra.setProperty ("root", spliceRoot.getFullPathName(), nullptr);
    for (int l = 0; l < kNumLanes; ++l)
        extra.setProperty ("sample" + juce::String (l), laneFiles[(size_t) l].getFullPathName(), nullptr);
    state.appendChild (extra, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SpliceGrooveProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    auto extra = tree.getChildWithName ("SGExtra");
    apvts.replaceState (tree);

    if (! extra.isValid())
        return;

    pattern.fromHex (extra.getProperty ("pattern").toString().toStdString());

    const juce::File root (extra.getProperty ("root").toString());
    if (root.isDirectory() && root != spliceRoot)
        setSpliceRoot (root);

    for (int l = 0; l < kNumLanes; ++l)
    {
        const juce::String path = extra.getProperty ("sample" + juce::String (l)).toString();
        if (path.isNotEmpty() && juce::File::isAbsolutePath (path))
        {
            const juce::File f (path);
            if (f.existsAsFile())
                loadLaneSample (l, f);
        }
    }
}

juce::AudioProcessorEditor* SpliceGrooveProcessor::createEditor()
{
    return new SpliceGrooveEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpliceGrooveProcessor();
}
