#include "PluginProcessor.hpp"
#include "../gui/PluginEditor.hpp"

namespace sunvox
{

SunVoxAudioProcessor::SunVoxAudioProcessor()
	: AudioProcessor(BusesProperties()
	                     .withInput("Input", juce::AudioChannelSet::stereo(), false)
	                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

SunVoxAudioProcessor::~SunVoxAudioProcessor()
{
}

void SunVoxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::ignoreUnused(samplesPerBlock);
	sunVoxEngine_.init(static_cast<int>(sampleRate));
}

void SunVoxAudioProcessor::releaseResources()
{
	sunVoxEngine_.close();
}

bool SunVoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	return true;
}

void SunVoxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;

	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	keyboardState_.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

	for (const auto metadata : midiMessages)
	{
		const auto message = metadata.getMessage();
		const int channelTrack = std::max(0, message.getChannel() - 1);

		if (message.isNoteOn())
		{
			sunVoxEngine_.sendMidiNote(message.getNoteNumber(), message.getVelocity(), channelTrack);
		}
		else if (message.isNoteOff())
		{
			sunVoxEngine_.sendMidiNoteOff(channelTrack);
		}
		else if (message.isAllNotesOff())
		{
			sunVoxEngine_.sendAllNotesOff();
		}
		else if (message.isPitchWheel())
		{
			sunVoxEngine_.sendPitchBend(channelTrack, message.getPitchWheelValue());
		}
	}

	if (totalNumOutputChannels > 0)
	{
		auto* leftChannel = buffer.getWritePointer(0);
		auto* rightChannel = (totalNumOutputChannels > 1) ? buffer.getWritePointer(1) : leftChannel;
		sunVoxEngine_.renderAudio(leftChannel, rightChannel, buffer.getNumSamples());
	}
}

juce::AudioProcessorEditor* SunVoxAudioProcessor::createEditor()
{
	return new SunVoxAudioProcessorEditor(*this);
}

bool SunVoxAudioProcessor::acceptsMidi() const
{
	return true;
}

bool SunVoxAudioProcessor::producesMidi() const
{
	return false;
}

bool SunVoxAudioProcessor::isMidiEffect() const
{
	return false;
}

double SunVoxAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

void SunVoxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	juce::ignoreUnused(destData);
}

void SunVoxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	juce::ignoreUnused(data, sizeInBytes);
}

} // namespace sunvox

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new sunvox::SunVoxAudioProcessor();
}
