#include "PluginProcessor.hpp"
#include "../gui/PluginEditor.hpp"

namespace sunvox
{

SunVoxAudioProcessor::SunVoxAudioProcessor()
	: AudioProcessor(BusesProperties()
	                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
	sunVoxEngine_.init(44100);
}

SunVoxAudioProcessor::~SunVoxAudioProcessor()
{
}

void SunVoxAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::ignoreUnused(samplesPerBlock);
	if (!sunVoxEngine_.isInitialized() || sunVoxEngine_.getSampleRate() != static_cast<int>(sampleRate))
	{
		sunVoxEngine_.init(static_cast<int>(sampleRate));
	}
}

void SunVoxAudioProcessor::releaseResources()
{
	sunVoxEngine_.close();
}

bool SunVoxAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
		return false;

	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
	 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	return true;
}

void SunVoxAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;

	buffer.clear();

	keyboardState_.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

	for (const auto metadata : midiMessages)
	{
		const auto message = metadata.getMessage();
		const int channelTrack = std::clamp(message.getChannel() - 1, 0, 15);

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

	auto totalNumOutputChannels = getTotalNumOutputChannels();
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

static constexpr uint32_t kPluginStateMagic = 0x5356504c; // "SVPL" in ASCII
static constexpr uint32_t kPluginStateVersion = 1;

void SunVoxAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	auto sunvoxBytes = sunVoxEngine_.saveToMemory();
	if (sunvoxBytes.empty())
		return;

	destData.reset();
	juce::MemoryOutputStream stream(destData, false);
	stream.writeInt(static_cast<int>(kPluginStateMagic));
	stream.writeInt(static_cast<int>(kPluginStateVersion));
	stream.writeInt(static_cast<int>(sunvoxBytes.size()));
	stream.write(sunvoxBytes.data(), sunvoxBytes.size());
}

void SunVoxAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	if (data == nullptr || sizeInBytes < 12)
		return;

	juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
	uint32_t magic = static_cast<uint32_t>(stream.readInt());
	uint32_t version = static_cast<uint32_t>(stream.readInt());
	int dataSize = stream.readInt();

	if (magic != kPluginStateMagic || version != kPluginStateVersion || dataSize <= 0)
		return;

	if (stream.getNumBytesRemaining() < dataSize)
		return;

	std::vector<uint8_t> buffer(static_cast<size_t>(dataSize));
	if (stream.read(buffer.data(), static_cast<size_t>(dataSize)) == dataSize)
	{
		sunVoxEngine_.loadFromMemory(buffer.data(), buffer.size());

		if (onStateChanged)
			onStateChanged();
	}
}

} // namespace sunvox

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new sunvox::SunVoxAudioProcessor();
}
