#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "GraphCanvas.hpp"
#include "InspectorPanel.hpp"

namespace sunvox
{

class SunVoxAudioProcessor;

class SunVoxAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	explicit SunVoxAudioProcessorEditor(SunVoxAudioProcessor& owner);
	~SunVoxAudioProcessorEditor() override;

	void paint(juce::Graphics& g) override;
	void resized() override;

	void refreshUI();

private:
	SunVoxAudioProcessor& processor_;
	GraphCanvas graphCanvas_;
	InspectorPanel inspectorPanel_;
	juce::MidiKeyboardComponent keyboardComponent_;

	juce::TextButton loadButton_{"Load"};
	juce::TextButton saveButton_{"Save"};
	std::unique_ptr<juce::FileChooser> fileChooser_;

	void handleLoadClicked();
	void handleSaveClicked();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunVoxAudioProcessorEditor)
};

} // namespace sunvox
