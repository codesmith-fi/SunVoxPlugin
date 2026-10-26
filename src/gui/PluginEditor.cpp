#include "PluginEditor.hpp"
#include "../audio/PluginProcessor.hpp"

namespace sunvox
{

SunVoxAudioProcessorEditor::SunVoxAudioProcessorEditor(SunVoxAudioProcessor& owner)
	: AudioProcessorEditor(&owner),
	  processor_(owner),
	  graphCanvas_(&processor_.getSunVoxEngine()),
	  inspectorPanel_(&processor_.getSunVoxEngine()),
	  presetBrowser_(processor_.getPresetManager(), processor_.getSunVoxEngine()),
	  keyboardComponent_(processor_.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
	setResizable(true, true);
	setResizeLimits(750, 480, 2560, 1440);
	setSize(1280, 720);
	setWantsKeyboardFocus(true);

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

	// Preset Browser drawer
	addChildComponent(presetBrowser_);
	presetBrowser_.onPresetLoaded = [this] {
		refreshUI();
	};
	presetBrowser_.onCloseClicked = [this] {
		isBrowserOpen_ = false;
		presetBrowser_.setVisible(false);
		resized();
		repaint();
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

	// Header controls: Preset Browser toggle & quick audition
	presetsToggleBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	presetsToggleBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
	presetsToggleBtn_.onClick = [this] {
		isBrowserOpen_ = !isBrowserOpen_;
		presetBrowser_.setVisible(isBrowserOpen_);
		resized();
		repaint();
	};
	addAndMakeVisible(presetsToggleBtn_);

	prevPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	prevPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	prevPresetBtn_.onClick = [this] {
		presetBrowser_.selectPreviousPreset();
	};
	addAndMakeVisible(prevPresetBtn_);

	nextPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	nextPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	nextPresetBtn_.onClick = [this] {
		presetBrowser_.selectNextPreset();
	};
	addAndMakeVisible(nextPresetBtn_);

	currentPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181a1f));
	currentPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
	currentPresetBtn_.onClick = [this] {
		isBrowserOpen_ = !isBrowserOpen_;
		presetBrowser_.setVisible(isBrowserOpen_);
		resized();
		repaint();
	};
	addAndMakeVisible(currentPresetBtn_);
	updatePresetDisplay();

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
	updatePresetDisplay();
	presetBrowser_.refreshBrowser();
	repaint();
}

void SunVoxAudioProcessorEditor::updatePresetDisplay()
{
	currentPresetBtn_.setButtonText(processor_.getPresetManager().getCurrentPresetDisplayName());
	presetsToggleBtn_.setColour(juce::TextButton::buttonColourId, isBrowserOpen_ ? juce::Colour(0xff005b66) : juce::Colour(0xff2b323d));
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

	// Title & version
	auto titleArea = headerArea.removeFromLeft(230).reduced(12, 0);
	g.setColour(juce::Colours::white);
	g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
	g.drawText("SUNVOX MODULAR", titleArea.removeFromTop(27), juce::Justification::bottomLeft);

	g.setColour(juce::Colour(0xff00e5ff));
	g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
	g.drawText("SYNTH  v" JucePlugin_VersionString, titleArea, juce::Justification::topLeft);

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

	// Position Header controls
	loadButton_.setBounds(240, 13, 56, 28);
	saveButton_.setBounds(302, 13, 56, 28);

	presetsToggleBtn_.setBounds(372, 13, 70, 28);
	prevPresetBtn_.setBounds(448, 13, 26, 28);
	currentPresetBtn_.setBounds(478, 13, 220, 28);
	nextPresetBtn_.setBounds(702, 13, 26, 28);

	auto keyboardArea = bounds.removeFromBottom(84).reduced(16, 4);
	keyboardComponent_.setBounds(keyboardArea);

	bounds.removeFromBottom(6); // gap above keyboard
	auto contentArea = bounds.reduced(16, 8);

	// Drawer if open
	if (isBrowserOpen_)
	{
		const int browserWidth = 320;
		presetBrowser_.setBounds(contentArea.removeFromLeft(browserWidth));
		contentArea.removeFromLeft(10);
	}

	// Inspector panel takes 240px on the right
	const int inspectorWidth = 240;
	auto inspectorArea = contentArea.removeFromRight(inspectorWidth);
	contentArea.removeFromRight(10); // spacing between canvas and inspector

	graphCanvas_.setBounds(contentArea);
	inspectorPanel_.setBounds(inspectorArea);
}

void SunVoxAudioProcessorEditor::mouseDown(const juce::MouseEvent&)
{
	grabKeyboardFocus();
}

bool SunVoxAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
	// If search box in preset browser has focus, let it handle standard typing and cursor movement
	if (isBrowserOpen_ && presetBrowser_.isSearchBoxFocused())
	{
		if (key == juce::KeyPress::downKey || key == juce::KeyPress::returnKey)
			return presetBrowser_.keyPressed(key);

		// If shift or ctrl is pressed, allow preset changing anyway
		if (key.getModifiers().isShiftDown() || key.getModifiers().isCtrlDown())
		{
			if (key.isKeyCode(juce::KeyPress::leftKey) || key.isKeyCode(juce::KeyPress::upKey))
				return presetBrowser_.selectPreviousPreset();
			if (key.isKeyCode(juce::KeyPress::rightKey) || key.isKeyCode(juce::KeyPress::downKey))
				return presetBrowser_.selectNextPreset();
		}

		return false;
	}

	if (key.isKeyCode(juce::KeyPress::leftKey))
	{
		return presetBrowser_.selectPreviousPreset();
	}
	if (key.isKeyCode(juce::KeyPress::rightKey))
	{
		return presetBrowser_.selectNextPreset();
	}

	if (isBrowserOpen_)
	{
		if (key.isKeyCode(juce::KeyPress::upKey))
		{
			return presetBrowser_.selectPreviousPreset();
		}
		if (key.isKeyCode(juce::KeyPress::downKey))
		{
			return presetBrowser_.selectNextPreset();
		}
		if (key.isKeyCode(juce::KeyPress::escapeKey))
		{
			isBrowserOpen_ = false;
			presetBrowser_.setVisible(false);
			resized();
			repaint();
			return true;
		}
	}

	return false;
}

} // namespace sunvox
