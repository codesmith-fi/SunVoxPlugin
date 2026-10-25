#include "PluginEditor.hpp"
#include "../audio/PluginProcessor.hpp"

namespace sunvox
{

SunVoxAudioProcessorEditor::SunVoxAudioProcessorEditor(SunVoxAudioProcessor& owner)
	: AudioProcessorEditor(&owner),
	  processor_(owner),
	  graphCanvas_(&processor_.getSunVoxEngine()),
	  inspectorPanel_(&processor_.getSunVoxEngine()),
	  keyboardComponent_(processor_.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
	setSize(850, 620);
	setResizable(true, true);
	setResizeLimits(650, 480, 1920, 1200);

	// Style virtual keyboard
	keyboardComponent_.setKeyWidth(18.0f);
	keyboardComponent_.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe5e9f0));
	keyboardComponent_.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff21252b));
	keyboardComponent_.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xff2ecc71).withAlpha(0.6f));
	addAndMakeVisible(keyboardComponent_);

	// Graph Canvas for modular nodes and cables
	addAndMakeVisible(graphCanvas_);
	graphCanvas_.refreshFromEngine();

	// Inspector panel for selected module parameters
	addAndMakeVisible(inspectorPanel_);
	graphCanvas_.onModuleSelected = [this](int modId) {
		if (modId > 0)
			processor_.getSunVoxEngine().setActiveMidiModule(modId);
		inspectorPanel_.inspectModule(modId);
	};

	// Header buttons: Load and Save
	loadButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	loadButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	loadButton_.onClick = [this] { handleLoadClicked(); };
	addAndMakeVisible(loadButton_);

	saveButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	saveButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	saveButton_.onClick = [this] { handleSaveClicked(); };
	addAndMakeVisible(saveButton_);

	// Listen for DAW state restoration
	processor_.onStateChanged = [this] {
		juce::MessageManager::callAsync([this] {
			refreshUI();
		});
	};

	// Select default synth module initially
	int defaultMod = processor_.getSunVoxEngine().getDefaultSynthModule();
	if (defaultMod >= 0)
	{
		graphCanvas_.selectModule(defaultMod);
		inspectorPanel_.inspectModule(defaultMod);
	}
}

SunVoxAudioProcessorEditor::~SunVoxAudioProcessorEditor()
{
	processor_.onStateChanged = nullptr;
}

void SunVoxAudioProcessorEditor::refreshUI()
{
	graphCanvas_.refreshFromEngine();
	int modId = processor_.getSunVoxEngine().getActiveMidiModule();
	if (modId < 0)
		modId = processor_.getSunVoxEngine().getDefaultSynthModule();

	if (modId >= 0)
	{
		graphCanvas_.selectModule(modId);
		inspectorPanel_.inspectModule(modId);
	}
	else
	{
		inspectorPanel_.clearInspection();
	}
	repaint();
}

void SunVoxAudioProcessorEditor::handleLoadClicked()
{
	juce::PopupMenu menu;
	menu.addItem(1, "Load SunVox Project (.sunvox)");
	menu.addItem(2, "Load SunVox Module (.sunsynth)");

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&loadButton_),
		[this](int result) {
			if (result == 1) // Project
			{
				fileChooser_ = std::make_unique<juce::FileChooser>(
					"Load SunVox Project",
					juce::File::getSpecialLocation(juce::File::userHomeDirectory),
					"*.sunvox");

				fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
					[this](const juce::FileChooser& fc) {
						auto file = fc.getResult();
						if (file.existsAsFile())
						{
							processor_.getSunVoxEngine().loadProject(file.getFullPathName().toStdString());
							refreshUI();
						}
					});
			}
			else if (result == 2) // Module
			{
				fileChooser_ = std::make_unique<juce::FileChooser>(
					"Load SunVox Module",
					juce::File::getSpecialLocation(juce::File::userHomeDirectory),
					"*.sunsynth");

				fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
					[this](const juce::FileChooser& fc) {
						auto file = fc.getResult();
						if (file.existsAsFile())
						{
							processor_.getSunVoxEngine().loadModule(file.getFullPathName().toStdString(), 200, 200);
							refreshUI();
						}
					});
			}
		});
}

void SunVoxAudioProcessorEditor::handleSaveClicked()
{
	fileChooser_ = std::make_unique<juce::FileChooser>(
		"Save SunVox Project",
		juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("patch.sunvox"),
		"*.sunvox");

	fileChooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
		[this](const juce::FileChooser& fc) {
			auto file = fc.getResult();
			if (file != juce::File{})
			{
				juce::File destFile = file.hasFileExtension(".sunvox") ? file : file.withFileExtension(".sunvox");
				processor_.getSunVoxEngine().saveProject(destFile.getFullPathName().toStdString());
			}
		});
}

void SunVoxAudioProcessorEditor::paint(juce::Graphics& g)
{
	// Background
	g.fillAll(juce::Colour(0xff181a1f));

	auto bounds = getLocalBounds();

	// Header bar
	auto headerArea = bounds.removeFromTop(54);
	g.setColour(juce::Colour(0xff21252b));
	g.fillRect(headerArea);

	g.setColour(juce::Colour(0xff2d3139));
	g.drawHorizontalLine(headerArea.getBottom(), 0.0f, static_cast<float>(getWidth()));

	// Title
	g.setColour(juce::Colours::white);
	g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
	g.drawText("SUNVOX MODULAR SYNTH", headerArea.removeFromLeft(260).reduced(16, 0), juce::Justification::centredLeft);

	// Status badge
	const auto& engine = processor_.getSunVoxEngine();
	bool isOk = engine.isInitialized();

	auto statusArea = headerArea.removeFromRight(270).reduced(12, 10);
	g.setColour(isOk ? juce::Colour(0xff2ecc71) : juce::Colour(0xffe74c3c));
	g.fillEllipse(static_cast<float>(statusArea.getX()), static_cast<float>(statusArea.getCentreY() - 5), 10.0f, 10.0f);

	g.setColour(juce::Colour(0xffabb2bf));
	g.setFont(juce::FontOptions(12.5f));
	int targetMidiMod = (engine.getActiveMidiModule() >= 0) ? engine.getActiveMidiModule() : engine.getDefaultSynthModule();
	juce::String statusText = isOk ? "Engine: Running (Slot " + juce::String(engine.getSlot()) + ", MIDI -> #" + juce::String(targetMidiMod) + ")"
	                               : "Engine: Not Initialized";
	g.drawText(statusText, statusArea.withTrimmedLeft(18), juce::Justification::centredLeft);

	// Keyboard area separator background
	auto keyboardArea = bounds.removeFromBottom(90);
	g.setColour(juce::Colour(0xff21252b));
	g.fillRect(keyboardArea);
	g.setColour(juce::Colour(0xff2d3139));
	g.drawHorizontalLine(keyboardArea.getY(), 0.0f, static_cast<float>(getWidth()));
}

void SunVoxAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds();
	auto headerArea = bounds.removeFromTop(54); // header

	// Position Load and Save buttons in header
	loadButton_.setBounds(268, 13, 64, 28);
	saveButton_.setBounds(340, 13, 64, 28);

	auto keyboardArea = bounds.removeFromBottom(84).reduced(16, 4);
	keyboardComponent_.setBounds(keyboardArea);

	bounds.removeFromBottom(6); // gap above keyboard
	auto contentArea = bounds.reduced(16, 8);

	// Inspector panel takes 240px on the right
	const int inspectorWidth = 240;
	auto inspectorArea = contentArea.removeFromRight(inspectorWidth);
	contentArea.removeFromRight(10); // spacing between canvas and inspector

	graphCanvas_.setBounds(contentArea);
	inspectorPanel_.setBounds(inspectorArea);
}

} // namespace sunvox
