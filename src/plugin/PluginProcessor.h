#pragma once

#include "../engine/EngineHost.h"
#include "AuditionSynth.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace morph
{

/** Lock-free SPSC queue for UI-injected note events (on-screen keyboard). */
class UiNoteQueue
{
public:
    struct Event { int pitch = -1; bool on = false; float velocity = 1.0f; };

    bool push (Event e)
    {
        const auto w = writeIndex.load (std::memory_order_relaxed);
        const auto next = (w + 1) % capacity;
        if (next == readIndex.load (std::memory_order_acquire))
            return false; // full
        buffer[(size_t) w] = e;
        writeIndex.store (next, std::memory_order_release);
        return true;
    }

    bool pop (Event& e)
    {
        const auto r = readIndex.load (std::memory_order_relaxed);
        if (r == writeIndex.load (std::memory_order_acquire))
            return false;
        e = buffer[(size_t) r];
        readIndex.store ((r + 1) % capacity, std::memory_order_release);
        return true;
    }

private:
    static constexpr int capacity = 64;
    std::array<Event, capacity> buffer {};
    std::atomic<int> writeIndex { 0 };
    std::atomic<int> readIndex { 0 };
};

class MorphAudioProcessor : public juce::AudioProcessor
{
public:
    MorphAudioProcessor();
    ~MorphAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MORPH"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    EngineHost engine;
    juce::AudioProcessorValueTreeState apvts;
    UiNoteQueue uiNoteQueue;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    AuditionSynth auditionSynth; // standalone audition only
    double lastTempoBpm = 80.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MorphAudioProcessor)
};

} // namespace morph
