#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "GraphCanvas.hpp"
#include "InspectorPanel.hpp"
#include "PresetBrowserComponent.hpp"

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
	bool keyPressed(const juce::KeyPress& key) override;
	void mouseDown(const juce::MouseEvent& e) override;

	void refreshUI();

	void parentHierarchyChanged() override;
	void visibilityChanged() override;

private:
	SunVoxAudioProcessor& processor_;
	GraphCanvas graphCanvas_;
	InspectorPanel inspectorPanel_;
	PresetBrowserComponent presetBrowser_;
	juce::MidiKeyboardComponent keyboardComponent_;

	juce::TextButton loadButton_{"Load"};
	juce::TextButton saveButton_{"Save"};

	juce::TextButton presetsToggleBtn_{"Presets"};
	juce::TextButton prevPresetBtn_{"<"};
	juce::TextButton nextPresetBtn_{">"};
	juce::TextButton currentPresetBtn_{"Default Lead"};

	bool isBrowserOpen_{false};
	std::unique_ptr<juce::FileChooser> fileChooser_;

	void handleLoadClicked();
	void handleSaveClicked();
	void updatePresetDisplay();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SunVoxAudioProcessorEditor)
};

} // namespace sunvox
