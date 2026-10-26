#include "ModuleNodeComponent.hpp"

namespace sunvox
{

ModuleNodeComponent::ModuleNodeComponent(const ModuleInfo& info)
	: info_(info)
{
	setSize(kDefaultWidth, kDefaultHeight);
}

juce::Colour ModuleNodeComponent::getHeaderColor() const
{
	if (info_.type == "Output")
		return juce::Colour(0xfff77f00); // Warm orange
	if (info_.type == "Generator" || info_.type == "Analog Generator" || info_.type == "FM" || info_.type == "SpectraVoice")
		return juce::Colour(0xff00e5ff); // Cyan / Synth
	if (info_.type == "Filter" || info_.type == "Vocal Filter")
		return juce::Colour(0xff9d4edd); // Purple / Filter
	if (info_.type == "Delay" || info_.type == "Reverb" || info_.type == "Flanger")
		return juce::Colour(0xff06d6a0); // Teal / Time-based FX
	if (info_.type == "Distortion")
		return juce::Colour(0xffef476f); // Red / Distortion
	if (info_.type == "Amplifier" || info_.type == "Compressor" || info_.type == "Feedback" || info_.type == "Kicker")
		return juce::Colour(0xff2ecc71); // Green / Dynamics & Tools

	return juce::Colour(0xff4a5568); // Slate default
}

void ModuleNodeComponent::setSelected(bool selected)
{
	if (isSelected_ != selected)
	{
		isSelected_ = selected;
		repaint();
	}
}

void ModuleNodeComponent::setTargetMidiModule(bool target)
{
	if (isTargetMidiModule_ != target)
	{
		isTargetMidiModule_ = target;
		repaint();
	}
}

void ModuleNodeComponent::setInputPinHighlighted(bool highlighted)
{
	if (isInputPinHighlighted_ != highlighted)
	{
		isInputPinHighlighted_ = highlighted;
		repaint();
	}
}

ModuleNodeComponent::PinType ModuleNodeComponent::getPinAt(juce::Point<int> localPos) const
{
	auto p = localPos.toFloat();
	float centerY = static_cast<float>(getHeight()) * 0.5f;

	juce::Point<float> inputPinCenter(kPinRadius, centerY);
	if (p.getDistanceFrom(inputPinCenter) <= kPinHitRadius)
		return PinType::Input;

	juce::Point<float> outputPinCenter(static_cast<float>(getWidth()) - kPinRadius, centerY);
	if (p.getDistanceFrom(outputPinCenter) <= kPinHitRadius)
		return PinType::Output;

	return PinType::None;
}

void ModuleNodeComponent::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	const float cornerSize = 6.0f;

	// Background
	g.setColour(juce::Colour(0xff1a1f26));
	g.fillRoundedRectangle(bounds, cornerSize);

	// Header accent bar
	auto headerArea = bounds.removeFromTop(6.0f);
	g.setColour(getHeaderColor());
	g.fillRoundedRectangle(headerArea.getX(), headerArea.getY(), headerArea.getWidth(), cornerSize * 2.0f, cornerSize);
	// Clip bottom of header accent
	g.setColour(juce::Colour(0xff1a1f26));
	g.fillRect(headerArea.getX(), headerArea.getY() + 4.0f, headerArea.getWidth(), cornerSize * 2.0f);

	// Outer border
	if (isTargetMidiModule_ && isSelected_)
	{
		// Both target MIDI and selected: double bright glow
		g.setColour(juce::Colour(0x6600e5ff));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), cornerSize, 3.0f);
		g.setColour(juce::Colour(0xff00ffcc));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), cornerSize, 2.0f);
	}
	else if (isTargetMidiModule_)
	{
		// Target MIDI module border
		g.setColour(juce::Colour(0x5500e5ff));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), cornerSize, 2.0f);
		g.setColour(juce::Colour(0xff00e5ff));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), cornerSize, 1.5f);
	}
	else if (isSelected_)
	{
		// Glow effect
		g.setColour(juce::Colour(0x5500e5ff));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), cornerSize, 3.0f);
		g.setColour(juce::Colour(0xff00e5ff));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), cornerSize, 1.5f);
	}
	else
	{
		g.setColour(juce::Colour(0xff3a4454));
		g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), cornerSize, 1.0f);
	}

	// Module Name Title
	g.setColour(juce::Colours::white);
	g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
	g.drawText(info_.name, 14, 12, getWidth() - 28, 20, juce::Justification::centredLeft, true);

	// Module Type and ID
	g.setColour(isSelected_ ? juce::Colour(0xffa5d8ff) : juce::Colour(0xff8c96a5));
	g.setFont(juce::FontOptions(11.0f));
	juce::String typeText = juce::String(info_.type) + " [#" + juce::String(info_.id) + "]";
	g.drawText(typeText, 14, 34, getWidth() - 28, 16, juce::Justification::centredLeft, true);

	// MIDI Target Badge
	if (isTargetMidiModule_)
	{
		auto badgeRect = juce::Rectangle<float>(static_cast<float>(getWidth() - 48), static_cast<float>(getHeight() - 20), 38.0f, 13.0f);
		g.setColour(juce::Colour(0xff00e5ff));
		g.fillRoundedRectangle(badgeRect, 3.0f);
		g.setColour(juce::Colour(0xff14171d));
		g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
		g.drawText("MIDI", badgeRect, juce::Justification::centred, false);
	}

	// Input Pin (Left)
	const float centerY = static_cast<float>(getHeight()) * 0.5f;

	if (isInputPinHighlighted_)
	{
		// Glowing hover ring when connecting
		g.setColour(juce::Colour(0x6600ffcc));
		g.fillEllipse(-4.0f, centerY - (kPinRadius + 4.0f), (kPinRadius + 4.0f) * 2.0f, (kPinRadius + 4.0f) * 2.0f);
		g.setColour(juce::Colour(0xff00ffcc));
		g.drawEllipse(-4.0f, centerY - (kPinRadius + 4.0f), (kPinRadius + 4.0f) * 2.0f, (kPinRadius + 4.0f) * 2.0f, 2.0f);
	}

	g.setColour(isInputPinHighlighted_ ? juce::Colour(0xff00ffcc) : juce::Colour(0xff14171d));
	g.fillEllipse(0.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f);
	g.setColour(isInputPinHighlighted_ ? juce::Colours::white : juce::Colour(0xff00e5ff));
	g.drawEllipse(0.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f, 1.5f);

	// Output Pin (Right)
	g.setColour(juce::Colour(0xff14171d));
	g.fillEllipse(static_cast<float>(getWidth()) - kPinRadius * 2.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f);
	g.setColour(juce::Colour(0xff00e5ff));
	g.drawEllipse(static_cast<float>(getWidth()) - kPinRadius * 2.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f, 1.5f);
}

void ModuleNodeComponent::mouseDown(const juce::MouseEvent& e)
{
	if (e.mods.isPopupMenu())
	{
		auto pin = getPinAt(e.getPosition());
		if (pin == PinType::Input && onInputPinRightClicked)
		{
			onInputPinRightClicked(info_.id, e.getScreenPosition());
		}
		else if (onNodeRightClicked)
		{
			onNodeRightClicked(info_.id, e.getScreenPosition());
		}
		return;
	}

	auto pin = getPinAt(e.getPosition());
	if (pin == PinType::Output)
	{
		isDraggingCable_ = true;
		if (onCableDragStarted)
			onCableDragStarted(info_.id, getOutputPinPosition());
		return;
	}

	if (e.mods.isLeftButtonDown())
	{
		if (onSelected)
			onSelected(info_.id);
	}

	isDraggingCable_ = false;
	if (auto* parent = getParentComponent())
	{
		dragStartComponentPos_ = getPosition();
		dragStartMousePos_ = e.getEventRelativeTo(parent).position;
	}
	toFront(true);
}

void ModuleNodeComponent::mouseDrag(const juce::MouseEvent& e)
{
	if (isDraggingCable_)
	{
		if (auto* parent = getParentComponent())
		{
			auto canvasPos = e.getEventRelativeTo(parent).position;
			if (onCableDragging)
				onCableDragging(canvasPos);
		}
		return;
	}

	if (auto* parent = getParentComponent())
	{
		auto curMouse = e.getEventRelativeTo(parent).position;
		auto delta = (curMouse - dragStartMousePos_).roundToInt();
		setTopLeftPosition(dragStartComponentPos_ + delta);
	}

	if (onMoved)
		onMoved();
}

void ModuleNodeComponent::mouseUp(const juce::MouseEvent& e)
{
	if (isDraggingCable_)
	{
		isDraggingCable_ = false;
		if (auto* parent = getParentComponent())
		{
			auto canvasPos = e.getEventRelativeTo(parent).position;
			if (onCableDragEnded)
				onCableDragEnded(canvasPos);
		}
	}
}

juce::Point<float> ModuleNodeComponent::getInputPinPosition() const
{
	return getPosition().toFloat() + juce::Point<float>(kPinRadius, static_cast<float>(getHeight()) * 0.5f);
}

juce::Point<float> ModuleNodeComponent::getOutputPinPosition() const
{
	return getPosition().toFloat() + juce::Point<float>(static_cast<float>(getWidth()) - kPinRadius, static_cast<float>(getHeight()) * 0.5f);
}

} // namespace sunvox

