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
	if (info_.type == "Generator")
		return juce::Colour(0xff00e5ff); // Cyan
	if (info_.type == "Filter")
		return juce::Colour(0xff9d4edd); // Purple
	if (info_.type == "Amplifier")
		return juce::Colour(0xff2ecc71); // Green

	return juce::Colour(0xff4a5568); // Slate default
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
	g.setColour(juce::Colour(0xff3a4454));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), cornerSize, 1.0f);

	// Module Name Title
	g.setColour(juce::Colours::white);
	g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
	g.drawText(info_.name, 14, 12, getWidth() - 28, 20, juce::Justification::centredLeft, true);

	// Module Type and ID
	g.setColour(juce::Colour(0xff8c96a5));
	g.setFont(juce::FontOptions(11.0f));
	juce::String typeText = juce::String(info_.type) + " [#" + juce::String(info_.id) + "]";
	g.drawText(typeText, 14, 34, getWidth() - 28, 16, juce::Justification::centredLeft, true);

	// Input Pin (Left)
	const float centerY = static_cast<float>(getHeight()) * 0.5f;
	g.setColour(juce::Colour(0xff14171d));
	g.fillEllipse(0.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f);
	g.setColour(juce::Colour(0xff00e5ff));
	g.drawEllipse(0.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f, 1.5f);

	// Output Pin (Right)
	g.setColour(juce::Colour(0xff14171d));
	g.fillEllipse(static_cast<float>(getWidth()) - kPinRadius * 2.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f);
	g.setColour(juce::Colour(0xff00e5ff));
	g.drawEllipse(static_cast<float>(getWidth()) - kPinRadius * 2.0f, centerY - kPinRadius, kPinRadius * 2.0f, kPinRadius * 2.0f, 1.5f);
}

void ModuleNodeComponent::mouseDown(const juce::MouseEvent& e)
{
	dragger_.startDraggingComponent(this, e);
	toFront(true);
}

void ModuleNodeComponent::mouseDrag(const juce::MouseEvent& e)
{
	dragger_.dragComponent(this, e, nullptr);

	if (onMoved)
		onMoved();
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
