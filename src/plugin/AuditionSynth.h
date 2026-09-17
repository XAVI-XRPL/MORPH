#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace morph
{

/**
 * AuditionSynth: a small built-in instrument used ONLY in the Standalone
 * build so a chord is audible when playing. MORPH itself outputs MIDI;
 * this synth never ships sound into DAW builds.
 */
class AuditionSynth
{
public:
    void prepare (double sampleRate);
    void render (const juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio, int numSamples);

private:
    struct Sound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    struct Voice : public juce::SynthesiserVoice
    {
        bool canPlaySound (juce::SynthesiserSound*) override { return true; }
        void startNote (int midiNoteNumber, float velocity,
                        juce::SynthesiserSound*, int currentPitchWheelPosition) override;
        void stopNote (float velocity, bool allowTailOff) override;
        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}
        void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

        double sampleRate = 44100.0;
        double phase = 0.0;
        double phaseDelta = 0.0;
        double secondPhase = 0.0;
        double secondDelta = 0.0;
        float level = 0.0f;
        float envelope = 0.0f;
        bool releasing = false;
        int pitch = 60;
    };

    juce::Synthesiser synth;
};

} // namespace morph
