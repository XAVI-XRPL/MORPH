#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace morph
{

juce::AudioProcessorValueTreeState::ParameterLayout MorphAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using NAP = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<API> ("keyIndex", "Key", 0, 11, 0));
    layout.add (std::make_unique<API> ("styleIndex", "Style", 0, numImplementedStyles - 1, 0));
    layout.add (std::make_unique<API> ("performanceMode", "Performance", 0, 5, 0)); // + pulse/pattern/arp (M8)
    layout.add (std::make_unique<APF> ("strumSpread", "Strum Spread", NAP (20.0f, 120.0f), 42.0f));

    layout.add (std::make_unique<API> ("togetherKind",   "Together Kind", 0, 3, 0));
    layout.add (std::make_unique<API> ("strumCurve",     "Strum Curve",   0, 3, 3)); // human
    layout.add (std::make_unique<API> ("strumVelShape",  "Strum Vel",     0, 2, 1)); // rise
    layout.add (std::make_unique<API> ("bassPolicy",     "Bass Policy",   0, 4, 0));
    layout.add (std::make_unique<API> ("topPolicy",      "Top Policy",    0, 4, 0));

    layout.add (std::make_unique<API> ("strumDirection", "Strum Dir",     0, 6, 0));
    layout.add (std::make_unique<API> ("streamRate",     "Stream Rate",   0, 2, 1));
    layout.add (std::make_unique<API> ("arpDirection",   "Arp Dir",       0, 2, 0));
    layout.add (std::make_unique<API> ("patternKind",    "Pattern",       0, 2, 0));

    layout.add (std::make_unique<APF> ("color",   "COLOR",   NAP (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<APF> ("motion",  "MOTION",  NAP (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<APF> ("morph",   "MORPH",   NAP (0.0f, 1.0f), 0.5f));
    layout.add (std::make_unique<APF> ("space",   "SPACE",   NAP (0.0f, 1.0f), 0.4f));
    layout.add (std::make_unique<APF> ("texture", "TEXTURE", NAP (0.0f, 1.0f), 0.2f));
    layout.add (std::make_unique<APF> ("output",  "OUTPUT",  NAP (0.0f, 1.0f), 0.75f));

    return layout;
}

MorphAudioProcessor::MorphAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MORPH_STATE", createParameterLayout())
{
}

void MorphAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    auditionSynth.prepare (sampleRate);
}

void MorphAudioProcessor::releaseResources()
{
}

bool MorphAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled();
}

void MorphAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    // Host tempo.
    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto bpm = pos->getBpm())
                lastTempoBpm = *bpm;
        }
    }
    engine.setTempoBpm (lastTempoBpm);

    // APVTS → engine atomics.
    auto load = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    engine.keyIndex.store ((int) load ("keyIndex"), std::memory_order_relaxed);
    engine.styleIndex.store ((int) load ("styleIndex"), std::memory_order_relaxed);
    engine.strumSpreadMs.store (load ("strumSpread"), std::memory_order_relaxed);
    engine.togetherKind.store ((int) load ("togetherKind"), std::memory_order_relaxed);
    engine.strumCurve.store ((int) load ("strumCurve"), std::memory_order_relaxed);
    engine.strumVelocityShape.store ((int) load ("strumVelShape"), std::memory_order_relaxed);
    engine.bassPolicy.store ((int) load ("bassPolicy"), std::memory_order_relaxed);
    engine.topVoicePolicy.store ((int) load ("topPolicy"), std::memory_order_relaxed);
    engine.strumDirectionSetting.store ((int) load ("strumDirection"), std::memory_order_relaxed);
    engine.streamRate.store ((int) load ("streamRate"), std::memory_order_relaxed);
    engine.arpDirectionSetting.store ((int) load ("arpDirection"), std::memory_order_relaxed);
    engine.patternKindSetting.store ((int) load ("patternKind"), std::memory_order_relaxed);
    engine.colorKnob.store (load ("color"), std::memory_order_relaxed);
    engine.motionKnob.store (load ("motion"), std::memory_order_relaxed);
    engine.morphKnob.store (load ("morph"), std::memory_order_relaxed);
    engine.spaceKnob.store (load ("space"), std::memory_order_relaxed);
    engine.textureKnob.store (load ("texture"), std::memory_order_relaxed);
    engine.outputKnob.store (load ("output"), std::memory_order_relaxed);

    // Performance mode: request path preserves the current chord identity.
    const int mode = (int) load ("performanceMode");
    if (mode != engine.performanceMode.load (std::memory_order_relaxed))
        engine.requestPerformanceMode ((PerformanceMode) mode);

    // UI-injected notes (on-screen keyboard) at block start.
    UiNoteQueue::Event uiEvent;
    while (uiNoteQueue.pop (uiEvent))
    {
        if (uiEvent.on)
            engine.noteOn (uiEvent.pitch, uiEvent.velocity, 0);
        else
            engine.noteOff (uiEvent.pitch, 0);
    }

    // Host MIDI input.
    juce::MidiBuffer output;
    for (const auto meta : midiMessages)
    {
        const auto& m = meta.getMessage();
        const auto offset = (int64_t) meta.samplePosition;

        if (m.isNoteOn())
            engine.noteOn (m.getNoteNumber(), m.getFloatVelocity(), offset);
        else if (m.isNoteOff())
            engine.noteOff (m.getNoteNumber(), offset);
        else if (m.isSustainPedalOn())
            engine.sustainPedal (true, offset);
        else if (m.isSustainPedalOff())
            engine.sustainPedal (false, offset);
    }

    engine.processBlock (output, numSamples);

#if JucePlugin_Build_Standalone
    // Standalone: audition the MIDI output through the built-in synth so the
    // chord is audible. DAW builds output pure MIDI.
    auditionSynth.render (output, buffer, numSamples);
#endif

    midiMessages.swapWith (output);
}

void MorphAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Composition state rides along with the parameter state.
    const auto prog = engine.getProgressionForUi();
    for (int i = 0; i < Progression::numSlots; ++i)
    {
        apvts.state.setProperty ("progDegree" + juce::String (i),
                                 prog.slots[(size_t) i].degree.value, nullptr);
        apvts.state.setProperty ("progLock" + juce::String (i),
                                 prog.slots[(size_t) i].locked, nullptr);
    }
    apvts.state.setProperty ("morphVariation", engine.getMorphVariation(), nullptr);

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MorphAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));

        // Restore composition state.
        if (apvts.state.hasProperty ("progDegree0"))
        {
            auto prog = engine.getProgressionForUi();
            for (int i = 0; i < Progression::numSlots; ++i)
            {
                prog.slots[(size_t) i].degree =
                    ScaleDegree { (int) apvts.state.getProperty ("progDegree" + juce::String (i), 1) };
                prog.slots[(size_t) i].locked =
                    (bool) apvts.state.getProperty ("progLock" + juce::String (i), false);
            }
            engine.setProgressionFromUi (prog);
        }
        engine.setMorphVariation ((int) apvts.state.getProperty ("morphVariation", 0));
    }
}

juce::AudioProcessorEditor* MorphAudioProcessor::createEditor()
{
    return new PluginEditor (*this);
}

} // namespace morph

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new morph::MorphAudioProcessor();
}
