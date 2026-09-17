#include "AuditionSynth.h"

namespace morph
{

void AuditionSynth::Voice::startNote (int midiNoteNumber, float velocity,
                                      juce::SynthesiserSound*, int)
{
    pitch = midiNoteNumber;
    const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phaseDelta = freq / sampleRate;
    secondDelta = freq * 2.005 / sampleRate; // faint octave shimmer
    level = velocity * 0.18f;
    envelope = 0.0f;
    releasing = false;
}

void AuditionSynth::Voice::stopNote (float, bool allowTailOff)
{
    juce::ignoreUnused (allowTailOff);
    releasing = true;
}

void AuditionSynth::Voice::renderNextBlock (juce::AudioBuffer<float>& buffer,
                                            int startSample, int numSamples)
{
    if (phaseDelta == 0.0)
        return;

    for (int i = 0; i < numSamples; ++i)
    {
        // Soft attack, moderate release — gentle electric-piano-ish tone.
        const float target = releasing ? 0.0f : 1.0f;
        const float coeff = releasing ? 0.9992f : 0.985f;
        envelope = target + (envelope - target) * coeff;

        const float s1 = std::sin (juce::MathConstants<double>::twoPi * phase);
        const float s2 = std::sin (juce::MathConstants<double>::twoPi * secondPhase);
        const float sample = (s1 * 0.85f + s2 * 0.15f) * envelope * level;

        phase += phaseDelta;
        secondPhase += secondDelta;
        if (phase > 1.0) phase -= 1.0;
        if (secondPhase > 1.0) secondPhase -= 1.0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample (ch, startSample + i, sample);
    }

    if (releasing && envelope < 0.0005f)
        clearCurrentNote();
}

void AuditionSynth::prepare (double sampleRate)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synth.clearVoices();
    for (int i = 0; i < 12; ++i)
    {
        auto* v = new Voice();
        v->sampleRate = sampleRate;
        synth.addVoice (v);
    }
    synth.clearSounds();
    synth.addSound (new Sound());
}

void AuditionSynth::render (const juce::MidiBuffer& midi, juce::AudioBuffer<float>& audio, int numSamples)
{
    synth.renderNextBlock (audio, midi, 0, numSamples);
}

} // namespace morph
