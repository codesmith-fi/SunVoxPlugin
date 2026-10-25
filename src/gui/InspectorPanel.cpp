#include "InspectorPanel.hpp"

namespace sunvox
{

ParameterRowComponent::ParameterRowComponent(int modId, const ControllerInfo& ctl, SunVoxEngine* engine)
	: modId_(modId)
	, ctlInfo_(ctl)
	, engine_(engine)
{
	nameLabel_.setText(ctl.name, juce::dontSendNotification);
	nameLabel_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
	nameLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffd1d5db));
	nameLabel_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(nameLabel_);

	valueLabel_.setText(juce::String(ctl.currentValue), juce::dontSendNotification);
	valueLabel_.setFont(juce::FontOptions(11.0f));
	valueLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
	valueLabel_.setJustificationType(juce::Justification::centredRight);
	addAndMakeVisible(valueLabel_);

	slider_.setSliderStyle(juce::Slider::LinearHorizontal);
	slider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	slider_.setRange(static_cast<double>(ctl.minValue), static_cast<double>(ctl.maxValue), 1.0);
	slider_.setValue(static_cast<double>(ctl.currentValue), juce::dontSendNotification);

	slider_.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00e5ff));
	slider_.setColour(juce::Slider::trackColourId, juce::Colour(0xff0096c7));
	slider_.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff222834));

	slider_.onValueChange = [this] {
		int val = static_cast<int>(slider_.getValue());
		valueLabel_.setText(juce::String(val), juce::dontSendNotification);
		if (engine_ != nullptr)
			engine_->setModuleCtlValue(modId_, ctlInfo_.id, val);
	};
	addAndMakeVisible(slider_);
}

void ParameterRowComponent::paint(juce::Graphics& g)
{
	// Subtle row divider line
	g.setColour(juce::Colour(0x1a3a4454));
	g.drawHorizontalLine(getHeight() - 1, 0.0f, static_cast<float>(getWidth()));
}

void ParameterRowComponent::resized()
{
	auto area = getLocalBounds();
	auto topRow = area.removeFromTop(18);

	valueLabel_.setBounds(topRow.removeFromRight(45));
	nameLabel_.setBounds(topRow);

	slider_.setBounds(area.removeFromTop(20));
}

// ==============================================================================

InspectorPanel::InspectorPanel(SunVoxEngine* engine)
	: engine_(engine)
{
	viewport_.setScrollBarsShown(true, false);
	viewport_.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, juce::Colour(0xff3a4454));
	addAndMakeVisible(viewport_);
}

void InspectorPanel::clearInspection()
{
	currentModId_ = -1;
	currentModuleName_.clear();
	currentModuleType_.clear();
	rebuildParameters();
}

void InspectorPanel::inspectModule(int modId)
{
	currentModId_ = modId;
	currentModuleName_ = "";
	currentModuleType_ = "";

	if (engine_ != nullptr && modId >= 0)
	{
		for (const auto& mod : engine_->getActiveModules())
		{
			if (mod.id == modId)
			{
				currentModuleName_ = mod.name;
				currentModuleType_ = mod.type;
				break;
			}
		}
	}

	rebuildParameters();
}

void InspectorPanel::rebuildParameters()
{
	paramRows_.clear();
	contentComponent_.reset();

	if (engine_ == nullptr || currentModId_ < 0)
	{
		viewport_.setViewedComponent(nullptr, false);
		repaint();
		return;
	}

	auto controllers = engine_->getModuleControllers(currentModId_);
	if (controllers.empty())
	{
		viewport_.setViewedComponent(nullptr, false);
		repaint();
		return;
	}

	contentComponent_ = std::make_unique<juce::Component>();

	const int rowHeight = 44;
	int y = 0;
	int width = std::max(200, viewport_.getMaximumVisibleWidth());

	for (const auto& ctl : controllers)
	{
		auto row = std::make_unique<ParameterRowComponent>(currentModId_, ctl, engine_);
		row->setBounds(0, y, width, rowHeight);
		contentComponent_->addAndMakeVisible(row.get());
		paramRows_.push_back(std::move(row));
		y += rowHeight;
	}

	contentComponent_->setSize(width, y + 8);
	viewport_.setViewedComponent(contentComponent_.get(), false);

	repaint();
}

void InspectorPanel::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	const float cornerSize = 8.0f;

	// Panel background
	g.setColour(juce::Colour(0xff161a22));
	g.fillRoundedRectangle(bounds, cornerSize);

	// Outer border
	g.setColour(juce::Colour(0xff2b323d));
	g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

	// Header background
	auto headerBounds = bounds.removeFromTop(54.0f);
	g.setColour(juce::Colour(0xff1e242f));
	g.fillRoundedRectangle(headerBounds.getX(), headerBounds.getY(), headerBounds.getWidth(), cornerSize * 2.0f, cornerSize);
	g.setColour(juce::Colour(0xff1e242f));
	g.fillRect(headerBounds.getX(), headerBounds.getY() + cornerSize, headerBounds.getWidth(), headerBounds.getHeight() - cornerSize);

	// Header bottom divider
	g.setColour(juce::Colour(0xff2b323d));
	g.drawHorizontalLine(static_cast<int>(headerBounds.getBottom()), bounds.getX(), bounds.getRight());

	// Header title and details
	if (currentModId_ >= 0)
	{
		g.setColour(juce::Colours::white);
		g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
		g.drawText(currentModuleName_, 14, 8, getWidth() - 28, 20, juce::Justification::centredLeft, true);

		g.setColour(juce::Colour(0xff00e5ff));
		g.setFont(juce::FontOptions(11.0f));
		juce::String sub = juce::String(currentModuleType_) + "  [#" + juce::String(currentModId_) + "]";
		g.drawText(sub, 14, 28, getWidth() - 28, 18, juce::Justification::centredLeft, true);
	}
	else
	{
		g.setColour(juce::Colour(0xffd1d5db));
		g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
		g.drawText("Inspector", 14, 8, getWidth() - 28, 20, juce::Justification::centredLeft, true);

		g.setColour(juce::Colour(0xff6b7280));
		g.setFont(juce::FontOptions(11.0f));
		g.drawText("Select a module to edit parameters", 14, 28, getWidth() - 28, 18, juce::Justification::centredLeft, true);
	}

	// Empty state when module is selected but has no controllers
	if (currentModId_ >= 0 && paramRows_.empty())
	{
		g.setColour(juce::Colour(0xff6b7280));
		g.setFont(juce::FontOptions(12.0f));
		g.drawText("No editable controllers for this module", getLocalBounds().withTrimmedTop(60), juce::Justification::centred, true);
	}
}

void InspectorPanel::resized()
{
	auto area = getLocalBounds();
	area.removeFromTop(56); // Space for header
	viewport_.setBounds(area.reduced(6, 4));

	if (contentComponent_ != nullptr)
	{
		int visibleWidth = viewport_.getMaximumVisibleWidth();
		contentComponent_->setSize(visibleWidth, contentComponent_->getHeight());
		for (auto& row : paramRows_)
		{
			row->setSize(visibleWidth, row->getHeight());
		}
	}
}

} // namespace sunvox
