#include "PluginEditor.hpp"
#include "../audio/PluginProcessor.hpp"

namespace sunvox
{

SunVoxAudioProcessorEditor::SunVoxAudioProcessorEditor(SunVoxAudioProcessor& owner)
	: AudioProcessorEditor(&owner),
	  processor_(owner),
	  graphCanvas_(&processor_.getSunVoxEngine()),
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
}

SunVoxAudioProcessorEditor::~SunVoxAudioProcessorEditor()
{
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
	g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
	g.drawText("SUNVOX MODULAR SYNTH", headerArea.removeFromLeft(300).reduced(16, 0), juce::Justification::centredLeft);

	// Status badge
	const auto& engine = processor_.getSunVoxEngine();
	bool isOk = engine.isInitialized();

	auto statusArea = headerArea.removeFromRight(280).reduced(12, 10);
	g.setColour(isOk ? juce::Colour(0xff2ecc71) : juce::Colour(0xffe74c3c));
	g.fillEllipse(static_cast<float>(statusArea.getX()), static_cast<float>(statusArea.getCentreY() - 5), 10.0f, 10.0f);

	g.setColour(juce::Colour(0xffabb2bf));
	g.setFont(juce::FontOptions(13.0f));
	juce::String statusText = isOk ? "Engine: Running (Slot " + juce::String(engine.getSlot()) + ", Mod " + juce::String(engine.getDefaultSynthModule()) + ": Default Lead)"
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
	bounds.removeFromTop(54); // header

	auto keyboardArea = bounds.removeFromBottom(84).reduced(16, 4);
	keyboardComponent_.setBounds(keyboardArea);

	bounds.removeFromBottom(6); // gap above keyboard
	graphCanvas_.setBounds(bounds.reduced(16, 8));
}

} // namespace sunvox
