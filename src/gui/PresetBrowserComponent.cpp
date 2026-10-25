#include "PresetBrowserComponent.hpp"

namespace sunvox
{

PresetBrowserComponent::PresetBrowserComponent(PresetManager& presetManager, SunVoxEngine& engine)
	: presetManager_(presetManager),
	  engine_(engine)
{
	// Header close button
	closeButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff21252b));
	closeButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffabb2bf));
	closeButton_.onClick = [this] {
		if (onCloseClicked)
			onCloseClicked();
	};
	addAndMakeVisible(closeButton_);

	// Search box
	searchBox_.setTextToShowWhenEmpty("Search presets...", juce::Colour(0xff5c6370));
	searchBox_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff181a1f));
	searchBox_.setColour(juce::TextEditor::textColourId, juce::Colour(0xffe5e9f0));
	searchBox_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d3139));
	searchBox_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00e5ff));
	searchBox_.onTextChange = [this] {
		updateFilteredList();
	};
	addAndMakeVisible(searchBox_);

	// Category ListBox
	categoryListBox_.setRowHeight(24);
	categoryListBox_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff1e2227));
	categoryListBox_.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff2d3139));
	addAndMakeVisible(categoryListBox_);

	// Preset ListBox
	presetListBox_.setRowHeight(26);
	presetListBox_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff181a1f));
	presetListBox_.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff2d3139));
	addAndMakeVisible(presetListBox_);

	// Prev / Next buttons
	prevButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	prevButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	prevButton_.onClick = [this] {
		if (presetManager_.prevPreset(engine_))
		{
			refreshBrowser();
			if (onPresetLoaded)
				onPresetLoaded();
		}
	};
	addAndMakeVisible(prevButton_);

	nextButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	nextButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e9f0));
	nextButton_.onClick = [this] {
		if (presetManager_.nextPreset(engine_))
		{
			refreshBrowser();
			if (onPresetLoaded)
				onPresetLoaded();
		}
	};
	addAndMakeVisible(nextButton_);

	// Rescan button
	rescanButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2b323d));
	rescanButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffabb2bf));
	rescanButton_.onClick = [this] {
		presetManager_.scanPresets();
		refreshBrowser();
	};
	addAndMakeVisible(rescanButton_);

	// Status label
	statusLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff5c6370));
	statusLabel_.setFont(juce::FontOptions(11.0f));
	statusLabel_.setJustificationType(juce::Justification::centredLeft);
	addAndMakeVisible(statusLabel_);

	refreshBrowser();
}

void PresetBrowserComponent::refreshBrowser()
{
	updateFilteredList();
	categoryListBox_.updateContent();
	categoryListBox_.repaint();
}

void PresetBrowserComponent::selectCategory(const juce::String& category)
{
	selectedCategory_ = category;
	updateFilteredList();
	categoryListBox_.repaint();
}

void PresetBrowserComponent::updateFilteredList()
{
	currentFilteredPresets_ = presetManager_.getFilteredPresets(selectedCategory_, searchBox_.getText());
	presetListBox_.updateContent();

	// If current loaded preset is in the list, highlight it
	int activeRow = -1;
	auto cur = presetManager_.getCurrentPreset();
	for (size_t i = 0; i < currentFilteredPresets_.size(); ++i)
	{
		if (currentFilteredPresets_[i].file == cur.file)
		{
			activeRow = static_cast<int>(i);
			break;
		}
	}
	if (activeRow >= 0)
	{
		presetListBox_.selectRow(activeRow, false, true);
	}
	else
	{
		presetListBox_.deselectAllRows();
	}

	presetListBox_.repaint();

	statusLabel_.setText(juce::String(currentFilteredPresets_.size()) + " of " +
	                     juce::String(presetManager_.getAllPresets().size()) + " presets",
	                     juce::dontSendNotification);
}

void PresetBrowserComponent::handlePresetSelected(int row)
{
	if (row >= 0 && row < static_cast<int>(currentFilteredPresets_.size()))
	{
		const auto& item = currentFilteredPresets_[static_cast<size_t>(row)];
		if (presetManager_.loadPreset(item, engine_))
		{
			if (onPresetLoaded)
				onPresetLoaded();
			presetListBox_.repaint();
		}
	}
}

void PresetBrowserComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colour(0xff1e2227));

	// Right border separating browser drawer from canvas
	g.setColour(juce::Colour(0xff2d3139));
	g.drawVerticalLine(getWidth() - 1, 0.0f, static_cast<float>(getHeight()));

	// Header background
	g.setColour(juce::Colour(0xff21252b));
	g.fillRect(0, 0, getWidth() - 1, 38);
	g.setColour(juce::Colour(0xff2d3139));
	g.drawHorizontalLine(38, 0.0f, static_cast<float>(getWidth() - 1));

	// Title
	g.setColour(juce::Colour(0xffe5e9f0));
	g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
	g.drawText("PRESET BROWSER", 12, 0, getWidth() - 40, 38, juce::Justification::centredLeft);
}

void PresetBrowserComponent::resized()
{
	auto bounds = getLocalBounds();
	bounds.removeFromRight(1); // border

	// Header
	auto headerArea = bounds.removeFromTop(38);
	closeButton_.setBounds(headerArea.removeFromRight(32).reduced(4, 6));

	// Search bar
	auto searchArea = bounds.removeFromTop(34).reduced(8, 4);
	searchBox_.setBounds(searchArea);

	// Footer controls
	auto footerArea = bounds.removeFromBottom(38).reduced(8, 6);
	prevButton_.setBounds(footerArea.removeFromLeft(28));
	footerArea.removeFromLeft(4);
	nextButton_.setBounds(footerArea.removeFromLeft(28));
	footerArea.removeFromLeft(8);
	rescanButton_.setBounds(footerArea.removeFromRight(56));
	statusLabel_.setBounds(footerArea);

	bounds.removeFromBottom(4);
	auto listArea = bounds.reduced(8, 2);

	// Left: Categories (width 110px), Right: Presets
	int catWidth = 110;
	categoryListBox_.setBounds(listArea.removeFromLeft(catWidth));
	listArea.removeFromLeft(6);
	presetListBox_.setBounds(listArea);
}

// CategoryModel
int PresetBrowserComponent::CategoryModel::getNumRows()
{
	return static_cast<int>(owner_.presetManager_.getCategories().size());
}

void PresetBrowserComponent::CategoryModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
	const auto& cats = owner_.presetManager_.getCategories();
	if (rowNumber < 0 || rowNumber >= static_cast<int>(cats.size()))
		return;

	const auto& catName = cats[static_cast<size_t>(rowNumber)];
	bool isCurrent = (catName == owner_.selectedCategory_);

	if (rowIsSelected || isCurrent)
	{
		g.setColour(juce::Colour(0xff2b323d));
		g.fillRoundedRectangle(2.0f, 1.0f, static_cast<float>(width - 4), static_cast<float>(height - 2), 3.0f);
	}

	if (isCurrent)
	{
		g.setColour(juce::Colour(0xff00e5ff));
		g.fillRect(2, 4, 3, height - 8);
	}

	g.setColour((rowIsSelected || isCurrent) ? juce::Colour(0xffffffff) : juce::Colour(0xffabb2bf));
	g.setFont(juce::FontOptions(11.5f, isCurrent ? juce::Font::bold : juce::Font::plain));
	g.drawText(catName, 8, 0, width - 12, height, juce::Justification::centredLeft, true);
}

void PresetBrowserComponent::CategoryModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
	const auto& cats = owner_.presetManager_.getCategories();
	if (row >= 0 && row < static_cast<int>(cats.size()))
	{
		owner_.selectCategory(cats[static_cast<size_t>(row)]);
	}
}

// PresetListModel
int PresetBrowserComponent::PresetListModel::getNumRows()
{
	return static_cast<int>(owner_.currentFilteredPresets_.size());
}

void PresetBrowserComponent::PresetListModel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0 || rowNumber >= static_cast<int>(owner_.currentFilteredPresets_.size()))
		return;

	const auto& item = owner_.currentFilteredPresets_[static_cast<size_t>(rowNumber)];
	bool isPlaying = (item.file == owner_.presetManager_.getCurrentPreset().file);

	if (rowIsSelected || isPlaying)
	{
		g.setColour(isPlaying ? juce::Colour(0xff1b333a) : juce::Colour(0xff2b323d));
		g.fillRoundedRectangle(2.0f, 1.0f, static_cast<float>(width - 4), static_cast<float>(height - 2), 3.0f);
		if (isPlaying)
		{
			g.setColour(juce::Colour(0xff00e5ff));
			g.drawRoundedRectangle(2.0f, 1.0f, static_cast<float>(width - 4), static_cast<float>(height - 2), 3.0f, 1.0f);
		}
	}

	// Tag (SYNTH / PROJ)
	auto tagArea = juce::Rectangle<int>(width - 46, (height - 15) / 2, 42, 15);
	g.setColour(item.isSynthFile ? juce::Colour(0xff008b8b) : juce::Colour(0xff8a4f9e));
	g.fillRoundedRectangle(tagArea.toFloat(), 3.0f);
	g.setColour(juce::Colours::white);
	g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
	g.drawText(item.isSynthFile ? "SYNTH" : "PROJ", tagArea, juce::Justification::centred);

	// Preset Name
	g.setColour(isPlaying ? juce::Colour(0xff00e5ff) : (rowIsSelected ? juce::Colour(0xffffffff) : juce::Colour(0xffe5e9f0)));
	g.setFont(juce::FontOptions(12.0f, isPlaying ? juce::Font::bold : juce::Font::plain));
	g.drawText(item.name, 8, 0, width - 56, height, juce::Justification::centredLeft, true);
}

void PresetBrowserComponent::PresetListModel::listBoxItemClicked(int row, const juce::MouseEvent&)
{
	owner_.handlePresetSelected(row);
}

void PresetBrowserComponent::PresetListModel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
	owner_.handlePresetSelected(row);
}

} // namespace sunvox
