#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float  kMasterGain = 0.7f;
constexpr double kMaxSampleSeconds = 30.0;
const juce::Identifier kStateTag { "AlphaDrumState" };
}

AlphaDrumAudioProcessor::AlphaDrumAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.registerBasicFormats();

    const auto preset = alpha::makeDefaultPreset();
    kit = preset.kit;
    sequencer.setPattern (preset.pattern);
    sequencer.setBpm (preset.bpm);

    sampler.loadBuiltInKit();
    sampler.applyKit (kit);
}

void AlphaDrumAudioProcessor::prepareToPlay (double sampleRate, int)
{
    const juce::SpinLock::ScopedLockType lock (samplerLock);
    currentSampleRate = sampleRate;
    sampler.prepare (sampleRate);
    sequencer.resetPlayhead();
    running = false;
}

void AlphaDrumAudioProcessor::releaseResources()
{
    const juce::SpinLock::ScopedLockType lock (samplerLock);
    sampler.reset();
}

bool AlphaDrumAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void AlphaDrumAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return;

    // Un chargement de sample est en cours : on rend un bloc de silence plutôt que d'attendre.
    const juce::SpinLock::ScopedTryLockType lock (samplerLock);
    if (! lock.isLocked())
        return;

    // 1. Transport de l'hôte
    bool hostIsPlaying = false;
    juce::Optional<double> hostPpq;

    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            hostIsPlaying = position->getIsPlaying();
            hostPpq = position->getPpqPosition();

            if (const auto bpm = position->getBpm())
                sequencer.setBpm (*bpm);
        }
    }

    hostPlaying.store (hostIsPlaying, std::memory_order_relaxed);

    // 2. Pré-écoutes demandées par l'UI
    if (const auto previews = pendingPreviews.exchange (0, std::memory_order_relaxed))
        for (int t = 0; t < alpha::kNumTracks; ++t)
            if ((previews & (1u << t)) != 0)
                sampler.trigger (t);

    // 3. Notes MIDI entrantes
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isNoteOn())
            sampler.trigger (kit.trackForMidiNote (message.getNoteNumber()),
                             message.getFloatVelocity(),
                             juce::jlimit (0, numSamples - 1, metadata.samplePosition));
    }
    midi.clear();

    // 4. Séquenceur : suit l'hôte s'il joue, sinon le bouton Play interne
    const bool shouldRun = hostIsPlaying || sequencer.isPlaying();
    const bool wasRunning = running.exchange (shouldRun, std::memory_order_relaxed);

    if (shouldRun)
    {
        if (hostIsPlaying && hostPpq.hasValue())
            sequencer.syncToPpq (*hostPpq, ! wasRunning);

        sequencer.process (numSamples, currentSampleRate,
                           [this] (int track, int offset) { sampler.trigger (track, 1.0f, offset); });
    }
    else if (wasRunning)
    {
        sequencer.resetPlayhead();
    }

    // 5. Rendu audio
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    sampler.render (left, right, numSamples);
    buffer.applyGain (kMasterGain);
}

juce::AudioProcessorEditor* AlphaDrumAudioProcessor::createEditor()
{
    return new AlphaDrumAudioProcessorEditor (*this);
}

//==============================================================================
void AlphaDrumAudioProcessor::previewTrack (int track) noexcept
{
    if (track >= 0 && track < alpha::kNumTracks)
        pendingPreviews.fetch_or (1u << track, std::memory_order_relaxed);
}

bool AlphaDrumAudioProcessor::loadSampleFile (int track, const juce::File& file)
{
    if (track < 0 || track >= alpha::kNumTracks)
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
        return false;

    const auto length = (int) juce::jmin<juce::int64> (reader->lengthInSamples,
                                                       (juce::int64) (kMaxSampleSeconds * reader->sampleRate));
    const int channels = juce::jlimit (1, 2, (int) reader->numChannels);

    juce::AudioBuffer<float> temp (channels, length);
    if (! reader->read (&temp, 0, length, 0, true, true))
        return false;

    alpha::SampleData data;
    data.sampleRate = reader->sampleRate;
    data.left.assign (temp.getReadPointer (0), temp.getReadPointer (0) + length);
    if (channels > 1)
        data.right.assign (temp.getReadPointer (1), temp.getReadPointer (1) + length);

    {
        const juce::SpinLock::ScopedLockType lock (samplerLock);
        sampler.setSample (track, std::move (data));
    }

    auto& t = kit.tracks[(size_t) track];
    t.name = file.getFileNameWithoutExtension().toStdString();
    t.samplePath = file.getFullPathName().toStdString();
    return true;
}

void AlphaDrumAudioProcessor::resetTrackSample (int track)
{
    if (track < 0 || track >= alpha::kNumTracks)
        return;

    {
        const juce::SpinLock::ScopedLockType lock (samplerLock);
        sampler.loadBuiltInSample (track);
    }

    auto& t = kit.tracks[(size_t) track];
    t.name = alpha::makeDefaultKit().tracks[(size_t) track].name;
    t.samplePath.clear();
}

//==============================================================================
void AlphaDrumAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml (kStateTag);
    xml.setAttribute ("version", 1);
    xml.setAttribute ("bpm", sequencer.getBpm());
    xml.setAttribute ("pattern", juce::String (sequencer.getPattern().toString()));
    xml.setAttribute ("kitName", juce::String (kit.name));

    for (int i = 0; i < alpha::kNumTracks; ++i)
    {
        const auto& t = kit.tracks[(size_t) i];
        auto* e = xml.createNewChildElement ("Track");
        e->setAttribute ("index", i);
        e->setAttribute ("name", juce::String (t.name));
        e->setAttribute ("sample", juce::String (t.samplePath));
        e->setAttribute ("gain", (double) t.gain);
        e->setAttribute ("pan", (double) t.pan);
        e->setAttribute ("pitch", (double) t.pitchSemitones);
        e->setAttribute ("mute", t.mute);
    }

    copyXmlToBinary (xml, destData);
}

void AlphaDrumAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (kStateTag))
        return;

    sequencer.setBpm (xml->getDoubleAttribute ("bpm", sequencer.getBpm()));
    if (xml->hasAttribute ("pattern"))
        sequencer.setPattern (alpha::Pattern::fromString (xml->getStringAttribute ("pattern").toStdString()));
    kit.name = xml->getStringAttribute ("kitName", juce::String (kit.name)).toStdString();

    for (auto* e : xml->getChildWithTagNameIterator ("Track"))
    {
        const int i = e->getIntAttribute ("index", -1);
        if (i < 0 || i >= alpha::kNumTracks)
            continue;

        auto& t = kit.tracks[(size_t) i];
        t.gain           = (float) e->getDoubleAttribute ("gain", 1.0);
        t.pan            = (float) e->getDoubleAttribute ("pan", 0.0);
        t.pitchSemitones = (float) e->getDoubleAttribute ("pitch", 0.0);
        t.mute           = e->getBoolAttribute ("mute", false);

        // Sample introuvable (projet déplacé, fichier supprimé) -> son par défaut.
        const auto path = e->getStringAttribute ("sample");
        if (path.isEmpty() || ! loadSampleFile (i, juce::File (path)))
            resetTrackSample (i);
        else
            t.name = e->getStringAttribute ("name", juce::String (t.name)).toStdString();
    }

    sampler.applyKit (kit);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AlphaDrumAudioProcessor();
}
