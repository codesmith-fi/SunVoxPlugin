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
	setOpaque(true);
#if JUCE_WINDOWS && ENABLE_WINE_BUILD
	if (auto* peer = getPeer())
	{
		peer->setCurrentRenderingEngine(0);
		repaint();
	}
#endif

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

void SunVoxAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
	if (!isDraggingFile_)
		return;

	auto bounds = getLocalBounds();

	// Translucent dark overlay over the whole window
	g.setColour(juce::Colour(0xcc0b0f14));
	g.fillRect(bounds);

	auto innerBounds = bounds.reduced(14);
	juce::Path borderPath;
	borderPath.addRoundedRectangle(innerBounds.toFloat(), 10.0f);

	if (isDragValid_)
	{
		// Neon cyan outer glow / dashed outline
		g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.25f));
		g.strokePath(borderPath, juce::PathStrokeType(6.0f));

		g.setColour(juce::Colour(0xff00e5ff));
		juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
		juce::Path dashedBorder;
		float dashes[] = {12.0f, 6.0f};
		stroke.createDashedStroke(dashedBorder, borderPath, dashes, 2);
		g.fillPath(dashedBorder);

		// Center banner card
		auto center = bounds.getCentre().toFloat();
		juce::Rectangle<float> card(center.x - 220.0f, center.y - 65.0f, 440.0f, 130.0f);

		g.setColour(juce::Colour(0xff181a1f));
		g.fillRoundedRectangle(card, 8.0f);

		g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.7f));
		g.drawRoundedRectangle(card, 8.0f, 1.5f);

		auto textRect = card;
		g.setColour(juce::Colour(0xff00e5ff));
		g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
		g.drawText("DROP TO IMPORT", textRect.removeFromTop(45.0f), juce::Justification::centred);

		g.setColour(juce::Colour(0xffe5e9f0));
		g.setFont(juce::FontOptions(13.5f));
		g.drawText(".sunvox (Load Project)  •  .sunsynth (Insert Module)", textRect.removeFromTop(30.0f), juce::Justification::centred);

		g.setColour(juce::Colour(0xff8a9ba8));
		g.setFont(juce::FontOptions(12.0f));
		g.drawText("Release to load into SunVox modular graph", textRect, juce::Justification::centred);
	}
	else
	{
		// Red outline for unsupported files
		g.setColour(juce::Colour(0xffe74c3c).withAlpha(0.25f));
		g.strokePath(borderPath, juce::PathStrokeType(6.0f));

		g.setColour(juce::Colour(0xffe74c3c));
		juce::PathStrokeType stroke(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
		juce::Path dashedBorder;
		float dashes[] = {8.0f, 4.0f};
		stroke.createDashedStroke(dashedBorder, borderPath, dashes, 2);
		g.fillPath(dashedBorder);

		auto center = bounds.getCentre().toFloat();
		juce::Rectangle<float> card(center.x - 180.0f, center.y - 45.0f, 360.0f, 90.0f);

		g.setColour(juce::Colour(0xff181a1f));
		g.fillRoundedRectangle(card, 8.0f);

		g.setColour(juce::Colour(0xffe74c3c).withAlpha(0.7f));
		g.drawRoundedRectangle(card, 8.0f, 1.5f);

		auto textRect = card;
		g.setColour(juce::Colour(0xffe74c3c));
		g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
		g.drawText("Unsupported File Type", textRect.removeFromTop(45.0f), juce::Justification::centred);

		g.setColour(juce::Colour(0xffabb2bf));
		g.setFont(juce::FontOptions(12.0f));
		g.drawText("Please drop .sunvox or .sunsynth files", textRect, juce::Justification::centred);
	}
}

bool SunVoxAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
	for (const auto& file : files)
	{
		juce::File f(file);
		if (f.hasFileExtension(".sunvox") || f.hasFileExtension(".sunsynth"))
			return true;
	}
	return false;
}

void SunVoxAudioProcessorEditor::fileDragEnter(const juce::StringArray& files, int x, int y)
{
	isDraggingFile_ = true;
	isDragValid_ = isInterestedInFileDrag(files);
	dragDropPos_ = {x, y};
	repaint();
}

void SunVoxAudioProcessorEditor::fileDragMove(const juce::StringArray& files, int x, int y)
{
	isDraggingFile_ = true;
	isDragValid_ = isInterestedInFileDrag(files);
	dragDropPos_ = {x, y};
	repaint();
}

void SunVoxAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
	isDraggingFile_ = false;
	repaint();
}

void SunVoxAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
	isDraggingFile_ = false;
	repaint();

	std::vector<juce::File> sunvoxFiles;
	std::vector<juce::File> sunsynthFiles;

	for (const auto& filePath : files)
	{
		juce::File file(filePath);
		if (file.hasFileExtension(".sunvox"))
			sunvoxFiles.push_back(file);
		else if (file.hasFileExtension(".sunsynth"))
			sunsynthFiles.push_back(file);
	}

	// 1. If any .sunvox project files are dropped, load the first project file
	if (!sunvoxFiles.empty())
	{
		const auto& projectFile = sunvoxFiles.front();
		if (projectFile.existsAsFile())
		{
			processor_.getPresetManager().loadPreset(projectFile, processor_.getSunVoxEngine());
			refreshUI();
		}
	}

	// 2. If any .sunsynth module files are dropped
	if (!sunsynthFiles.empty())
	{
		// Check if dropped specifically onto Preset Browser drawer
		if (isBrowserOpen_ && presetBrowser_.getBounds().contains(x, y))
		{
			// Load as auditioned preset
			processor_.getPresetManager().loadPreset(sunsynthFiles.front(), processor_.getSunVoxEngine());
			refreshUI();
			return;
		}

		// Otherwise insert module(s) onto the canvas at drop position
		juce::Point<int> dropScreenPoint(x, y);
		juce::Point<int> localCanvasPoint = graphCanvas_.getLocalPoint(this, dropScreenPoint);

		juce::Point<float> worldDropPoint;
		if (graphCanvas_.getLocalBounds().contains(localCanvasPoint))
		{
			worldDropPoint = graphCanvas_.screenToWorld(localCanvasPoint.toFloat());
		}
		else
		{
			// Fallback to center of canvas
			worldDropPoint = graphCanvas_.screenToWorld(graphCanvas_.getLocalBounds().getCentre().toFloat());
		}

		int lastLoadedId = -1;
		int offsetX = 0;
		int offsetY = 0;

		for (const auto& synthFile : sunsynthFiles)
		{
			if (!synthFile.existsAsFile())
				continue;

			int modX = static_cast<int>(std::round(worldDropPoint.x)) + offsetX;
			int modY = static_cast<int>(std::round(worldDropPoint.y)) + offsetY;

			int modId = processor_.getSunVoxEngine().loadModule(synthFile.getFullPathName().toStdString(), modX, modY);
			if (modId >= 0)
			{
				// Connect newly added module to Output (module 0) and activate for MIDI input
				processor_.getSunVoxEngine().connectModules(modId, 0);
				processor_.getSunVoxEngine().setActiveMidiModule(modId);
				lastLoadedId = modId;

				offsetX += 40;
				offsetY += 40;
			}
		}

		if (lastLoadedId >= 0)
		{
			refreshUI();
			graphCanvas_.selectModule(lastLoadedId);
			inspectorPanel_.inspectModule(lastLoadedId);
		}
	}
}

void SunVoxAudioProcessorEditor::parentHierarchyChanged()
{
	juce::AudioProcessorEditor::parentHierarchyChanged();
	setOpaque(true);
#if JUCE_WINDOWS
	if (auto* peer = getPeer())
	{
		peer->setCurrentRenderingEngine(0);
		repaint();
	}
#endif
}

void SunVoxAudioProcessorEditor::visibilityChanged()
{
	juce::AudioProcessorEditor::visibilityChanged();
	setOpaque(true);
#if JUCE_WINDOWS
	if (auto* peer = getPeer())
	{
		peer->setCurrentRenderingEngine(0);
		repaint();
	}
#endif
}

} // namespace sunvox
