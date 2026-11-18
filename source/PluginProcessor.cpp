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

#include "PluginProcessor.h"

#include "PluginEditor.h"

PluginProcessor::PluginProcessor() :
    AudioProcessor(
#if JucePlugin_IsMidiEffect
        BusesProperties()
#else
        BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    zoneLayout(juce::MPEZone(juce::MPEZone::Type::lower, 15)),
    mpeInitMessage(juce::MPEMessages::setZoneLayout(zoneLayout))
{
}

const juce::String PluginProcessor::getName() const { return JucePlugin_Name; }

bool PluginProcessor::acceptsMidi() const { return true; }
bool PluginProcessor::producesMidi() const { return true; }
bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}
bool PluginProcessor::supportsMPE() const { return true; }

double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

int PluginProcessor::getNumPrograms() { return 1; }
int PluginProcessor::getCurrentProgram() { return 0; }
void PluginProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String PluginProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}
void PluginProcessor::changeProgramName(int index, const juce::String& newName) { juce::ignoreUnused(index, newName); }

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
#endif
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    mpePortamento.prepare(sampleRate, mpeInitMessage);

    storedCurveX = -1.f;
    storedCurveY = -1.f;
}

void PluginProcessor::releaseResources() {}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    params.portamentoTime = apvts.getRawParameterValue("time")->load();

    float curveX = apvts.getRawParameterValue("curveX")->load();
    float curveY = apvts.getRawParameterValue("curveY")->load();

    if (!juce::approximatelyEqual(storedCurveX, curveX) || !juce::approximatelyEqual(storedCurveY, curveY))
    {
        params.portamentoCurve = CurveUtils::calculateExponent(curveX, curveY);

        storedCurveX = curveX;
        storedCurveY = curveY;
    }
    params.bendRange = static_cast<int>(apvts.getRawParameterValue("range")->load());

    mpePortamento.updateParameters(params);
    mpePortamento.process(midiMessages, buffer.getNumSamples());
}

bool PluginProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}
void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ValueTree tree{juce::ValueTree::readFromData(data, sizeInBytes)};
    if (tree.isValid())
    {
        apvts.replaceState(tree);
    }
    if (juce::AudioProcessorEditor * editor{getActiveEditor()})
    {
        const int width{getEditorWidth()};
        const int height{getEditorHeight()};

        juce::MessageManager::callAsync(
            [safe{juce::Component::SafePointer<juce::AudioProcessorEditor>(editor)}, width, height]
            {
                if (safe != nullptr)
                {
                    safe->setSize(width, height);
                }
            }
        );
    }
}

// Some helper functions to store and set the size of the window so it doesn't reset itself on creation.
int PluginProcessor::getEditorWidth()
{
    juce::ValueTree size{apvts.state.getOrCreateChildWithName("lastSize", nullptr)};
    return size.getProperty("width", 510);
}
int PluginProcessor::getEditorHeight()
{
    juce::ValueTree size{apvts.state.getOrCreateChildWithName("lastSize", nullptr)};
    return size.getProperty("height", 300);
}
void PluginProcessor::setEditorSize(int width, int height)
{
    juce::ValueTree size{apvts.state.getOrCreateChildWithName("lastSize", nullptr)};
    size.setProperty("width", width, nullptr);
    size.setProperty("height", height, nullptr);
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto FormatMillisecondsAndSeconds = [](float value, int)
    {
        if (value < 1.f)
        {
            return juce::String(value * 1000.f, 2) + " ms";
        }

        return juce::String(value, 1) + " s";
    };

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"time", 1},
        "Portamento Time",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.00001f, 0.3f),
        1.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(FormatMillisecondsAndSeconds)
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"curveX", 1},
        "Portamento curveX",
        juce::NormalisableRange<float>(.01f, .99f),
        .5f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"curveY", 1},
        "Portamento curveY",
        juce::NormalisableRange<float>(.01f, .99f),
        .5f
    ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"range", 1},
        "Bend range",
        2,
        96,
        48
    ));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
