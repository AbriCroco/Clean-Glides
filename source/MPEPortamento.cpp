/*
  ==============================================================================

  This file is part of Clean Glides.
  Copyright (C) 2026 AbriCroco

  Clean Glides is free software: you can redistribute it and/or modify it
  under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  You should have received a copy of the GNU Affero General Public License
  along with this program. If not, see https://www.gnu.org/licenses/.

  ==============================================================================
*/

#include "MPEPortamento.h"

void MPEPortamento::prepare(double sampleRate, const juce::MidiBuffer& mpeInitMessage)
{
    // We initialize our glides, capture the sample rate and define a 10 ms gate for MIDI events.
    _sampleRate = sampleRate;
    glides.fill({});
    bendStep = juce::jmax(1, juce::roundToInt(0.01 * _sampleRate));

    // We initialize our timer to reset the pitch for all channels. More on this below.
    resetCountdown.fill(-1);
    pendingResets = 0;
    graceSamples = juce::roundToInt(releaseGraceSeconds * _sampleRate);

    // We assign our MPE initializing message to a temporary buffer.
    mpeInitBuffer = mpeInitMessage;
}

void MPEPortamento::process(juce::MidiBuffer& midiBuffer, int numSamples)
{
    // As this is an internal buffer, we need to clear it before any new processing.
    mpeMidiBuffer.clear();

    // If our buffer for initializing MPE is not empty, we set the message as our first message and clear it.
    if (!mpeInitBuffer.isEmpty())
    {
        mpeMidiBuffer.addEvents(mpeInitBuffer, 0, -1, 0);
        mpeInitBuffer.clear();
    }

    // We need our own tracker for time for our pitch bend events so we define cursor and set it to 0.
    int cursor{};
    for (const juce::MidiMessageMetadata metadata : midiBuffer)
    {
        // We capture some information from the MIDI message we received that we will use later.
        const int time{metadata.samplePosition};
        renderGlides(cursor, time);
        cursor = time;

        juce::MidiMessage message{metadata.getMessage()};
        const int ch{message.getChannel()};

        /* According to MPE standards, we need a global channel. This can be set to 1 (lower) or to 16 (higher).
         * For this plugin, for now, we only will use the lower version and thus we will treat channel 1 differently.
         * This means that any message received on channel 1 will not be treated and will not have any portamento.

         * I decided to still output "note on" and "note off" messages from the first channel. This is because many DAWs
         * will default MIDI keyboards to be set on channel 1. That way, users can still play their keyboard with
         * the plugin routed and hear sound without changing the routing. However, it is possible that some synths
         * may not authorise this and will not output any sound. */

        if (ch == 1)
        {
            mpeMidiBuffer.addEvent(message, time);

            if (message.isAllNotesOff() || message.isAllSoundOff())
            {
                for (size_t i{}; i < glides.size(); ++i)
                {
                    clearChannel(i);
                }
            }

            continue;
        }

        // If the message received is on an expected channel, we send it untouched early to avoid a bad index wrapping.
        if (ch == 0)
        {
            mpeMidiBuffer.addEvent(message, time);
            continue;
        }

        const size_t index{static_cast<size_t>(ch - 2)};
        MPEPortamento::Glide& glide{glides[index]};

        // Most of this plugin's process will only occur on "note on" messages, which means that a new note is played.
        if (message.isNoteOn())
        {
            // We initialized our anchor note to -1.
            // If this is true, this means that this is the first "note on" message received for this channel.
            if (glide.anchorNote == -1)
            {
                cancelReset(index);

                // We initialize all our glide values to default.
                glide = {};
                glide.anchorNote = glide.playingNote = message.getNoteNumber();
                glide.start = glide.target = glide.current = 8192.f;

                // We reset the pitch bend and play the note untouched.
                mpeMidiBuffer.addEvent(juce::MidiMessage::pitchWheel(ch, 8192), time);
                mpeMidiBuffer.addEvent(message, time);

                // If this is true, this means that we received a "note on" from a new note and need to process it.
            } else if (glide.playingNote != message.getNoteNumber())
            {
                // We allow users to set custom bend range. If the new note is in the range, we initiate a glide.
                if (std::abs(message.getNoteNumber() - glide.anchorNote) <= params.bendRange)
                {
                    // We set our playing note to the new note.
                    glide.playingNote = message.getNoteNumber();

                    // We set our starting point for the glide to the current one.
                    // This ensure a continuous glide as we are already gliding while receiving the new note.
                    glide.start = glide.current;

                    // We calculate in semitones the new pitch we need to glide to.
                    glide.target = 8192.f + (glide.playingNote - glide.anchorNote) * semitoneUnit;
                    glide.currentSample = 0;

                    // We calculate in samples, the time that we will need to glide to the new note and enable the glide.
                    glide.totalSample = juce::jmax(1, static_cast<int>(params.portamentoTime * _sampleRate));
                    glide.active = true;

                    /* Note that we do not update the anchor note. This is for two reasons:
                     * - We later need to send a "note off" message to the synth and this needs to be the same note we sent a
                     *   "note on" message for.
                     * - We want glides to sound continuous. If we updated the anchor note, we would need to send a "note
                     *   off" and a new "note on" message, which would retrigger the enveloppe of the synth.

                     * While there are ways to work around that inside the DAW, the goal for this plugin is to minimise
                     * the work needed to achieve clean continuous glides. Thus this design was retained over others. */
                } else
                {
                    // If we hit this block, this means that the glide was outside the pitch bend range that was set.
                    // In this case, we cannot glide to the new note and need to reset the glide and the note played.
                    mpeMidiBuffer.addEvent(juce::MidiMessage::noteOff(ch, glide.anchorNote, message.getVelocity()), time);
                    cancelReset(index);

                    // We reset all our glide values to default.
                    glide = {};
                    glide.anchorNote = glide.playingNote = message.getNoteNumber();
                    glide.start = glide.target = glide.current = 8192.f;

                    // We reset the pitch bend and play the new note untouched.
                    mpeMidiBuffer.addEvent(juce::MidiMessage::pitchWheel(ch, 8192), time);
                    mpeMidiBuffer.addEvent(message, time);
                }
            }
            // If the below is true, this means that a note is not held anymore, thus should be registered as off.
            // In our case, as we do not send new "note on" messages for new notes, we will also mostly not handle these.
        } else if (message.isNoteOff())
        {
            // If the message concerns the note that is actually playing, no new note is available for gliding.
            if (glide.playingNote == message.getNoteNumber())
            {
                // We register the message and start a timer to reset the pitch on the channel.
                mpeMidiBuffer.addEvent(juce::MidiMessage::noteOff(ch, glide.anchorNote, message.getVelocity()), time);
                clearChannel(index);
            }
        } else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // If all notes are off, we register the message and start a timer to reset the pitch on the channel.
            mpeMidiBuffer.addEvent(message, time);
            clearChannel(index);
        } else
        {
            // In any other case, we just register the message untouched.
            mpeMidiBuffer.addEvent(message, time);
        }
    }
    // Below we will render our glides and check if any channel reset is needed.
    renderGlides(cursor, numSamples);
    tickResets(numSamples);

    // Once all done, we swap our buffer with the original one so our changes are sent to the synth.
    midiBuffer.swapWith(mpeMidiBuffer);
}

// Helper function to reassign parameters value only when they change.
void MPEPortamento::updateParameters(const MPEPortamento::Parameters& newParams)
{
    if (!juce::approximatelyEqual(newParams.portamentoTime, params.portamentoTime))
        params.portamentoTime = newParams.portamentoTime;

    if (!juce::approximatelyEqual(newParams.portamentoCurve, params.portamentoCurve))
        params.portamentoCurve = newParams.portamentoCurve;

    if (newParams.bendRange != params.bendRange)
    {
        params.bendRange = newParams.bendRange;
        semitoneUnit = 8192.f / static_cast<float>(params.bendRange);
    }
}

// Our function to make glides continuous and shape them as wished.
void MPEPortamento::renderGlides(int from, int to)
{
    // We will process each glide individually.
    for (size_t i{}; i < glides.size(); ++i)
    {
        MPEPortamento::Glide& glide{glides[i]};
        if (!glide.active)
            continue;

        // We will process each glide per sample for the duration of the process block.
        for (int time{from}; time < to; ++time)
        {
            // We advance our glide so it never equals 0 and gate our processing to be done only every 10 ms.
            ++glide.currentSample;
            if (glide.currentSample % bendStep != 0 && glide.currentSample < glide.totalSample)
                continue;

            // We calculate the x value that we will use in our f(x) function below.
            const float x = juce::jmin(1.f, static_cast<float>(glide.currentSample) / static_cast<float>(glide.totalSample));

            // To calculate the current glide, we use an exponential function.
            // x corresponds to the current time so the current glide will be set as the image of x.
            glide.current = glide.start + (glide.target - glide.start) * std::pow(x, params.portamentoCurve);

            // Once calculated, we add it to our MIDI buffer.
            mpeMidiBuffer.addEvent(juce::MidiMessage::pitchWheel(static_cast<int>(i) + 2, juce::jlimit(0, 16383, juce::roundToInt(glide.current))), time);

            if (x >= 1.f)
            {
                glide.active = false;
                break;
            }
        }
    }
}

/* Below are four functions that solve one issue. All of them serve as a sample timer that will check our grace period
 * in samples and will decide whether all channels pitch bend need to be reset or not.
 * For a lack of a better solution due to DAWs limitations, this was the best I could come up with to solve an edge case.

 * To explain further: the current process implementation  only holds a "lazy reset". Which is that we will reset the pitch
 * on a channel only if a new note is registered while no previous note is playing. This is because if we reset on a
 * "note off" message, the pitch reset will be audible in the case the decay of the instrument is set to some value
 * greater than just a few milliseconds. This way to reset works in all cases when the routing is left untouched.
 * However, in the case where the plugin is bypassed, or the routing changed and the MIDI doesn't pass through the plugin
 * anymore, no pitch reset will be sent and the synth's pitch will stay with the last offset unless the synth is reset.

 * To solve this, the timer was the best solution that I could come up with is a timer that will work universally.
 * With this, if  no new note is played in a certain amount of time (5 seconds for now), the pitch will automatically
 * be reset on all channels. Avoiding the pitch to stay with the last set offset. I intend to implement this as an option
 * with more control hoping it can make up for the far from ideal solution that this is. */

void MPEPortamento::scheduleReset(size_t i)
{
    if (resetCountdown[i] < 0)
        ++pendingResets;

    resetCountdown[i] = graceSamples;
}

void MPEPortamento::tickResets(int numSamples)
{
    if (pendingResets == 0)
        return;

    for (size_t i{}; i < resetCountdown.size(); ++i)
    {
        if (resetCountdown[i] < 0)
            continue;

        resetCountdown[i] -= numSamples;
        if (resetCountdown[i] > 0)
            continue;

        mpeMidiBuffer.addEvent(juce::MidiMessage::pitchWheel(static_cast<int>(i) + 2, 8192), juce::jmax(0, numSamples - 1));
        resetCountdown[i] = -1;
        --pendingResets;
    }
}

void MPEPortamento::cancelReset(size_t i)
{
    if (resetCountdown[i] >= 0)
    {
        resetCountdown[i] = -1;
        --pendingResets;
    }
}

void MPEPortamento::clearChannel(size_t i)
{
    if (!juce::approximatelyEqual(glides[i].current, 8192.f))
        scheduleReset(i);

    glides[i] = {};
}
