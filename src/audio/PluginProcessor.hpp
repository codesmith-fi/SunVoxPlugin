#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SunVoxEngine.hpp"

namespace sunvox
{

class SunVoxAudioProcessor : public juce::AudioProcessor
{
public:
	SunVoxAudioProcessor();
	~SunVoxAudioProcessor() override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

	using AudioProcessor::processBlock;
	void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override { return true; }

	const juce::String getName() const override { return "SunVox Plugin"; }

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	int getNumPrograms() override { return 1; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
	const juce::String getProgramName(int index) override { juce::ignoreUnused(index); return {}; }
	void changeProgramName(int index, const juce::String& newName) override { juce::ignoreUnused(index, newName); }

	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;

	SunVoxEngine& getSunVoxEngine() noexcept { return sunVoxEngine_; }
	const SunVoxEngine& getSunVoxEngine() const noexcept { return sunVoxEngine_; }

	juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState_; }
	const juce::MidiKeyboardState& getKeyboardState() const noexcept { return keyboardState_; }

private:
	SunVoxEngine sunVoxEngine_;
	juce::MidiKeyboardState keyboardState_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunVoxAudioProcessor)
};

} // namespace sunvox
