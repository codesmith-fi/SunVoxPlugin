#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/PresetManager.hpp"
#include "../audio/SunVoxEngine.hpp"

namespace sunvox
{

class PresetBrowserComponent : public juce::Component
{
public:
	PresetBrowserComponent(PresetManager& presetManager, SunVoxEngine& engine);
	~PresetBrowserComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

	void refreshBrowser();
	void selectCategory(const juce::String& category);

	std::function<void()> onPresetLoaded;
	std::function<void()> onCloseClicked;

private:
	PresetManager& presetManager_;
	SunVoxEngine& engine_;

	juce::TextButton closeButton_{"✕"};
	juce::TextEditor searchBox_;
	juce::TextButton prevButton_{"<"};
	juce::TextButton nextButton_{">"};
	juce::TextButton rescanButton_{"Rescan"};
	juce::Label statusLabel_;

	juce::String selectedCategory_{"All"};
	std::vector<PresetItem> currentFilteredPresets_;

	// List models
	class CategoryModel : public juce::ListBoxModel
	{
	public:
		explicit CategoryModel(PresetBrowserComponent& owner) : owner_(owner) {}
		int getNumRows() override;
		void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
		void listBoxItemClicked(int row, const juce::MouseEvent&) override;

	private:
		PresetBrowserComponent& owner_;
	};

	class PresetListModel : public juce::ListBoxModel
	{
	public:
		explicit PresetListModel(PresetBrowserComponent& owner) : owner_(owner) {}
		int getNumRows() override;
		void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
		void listBoxItemClicked(int row, const juce::MouseEvent&) override;
		void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

	private:
		PresetBrowserComponent& owner_;
	};

	CategoryModel categoryModel_{*this};
	PresetListModel presetListModel_{*this};

	juce::ListBox categoryListBox_{"Categories", &categoryModel_};
	juce::ListBox presetListBox_{"Presets", &presetListModel_};

	void updateFilteredList();
	void handlePresetSelected(int row);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserComponent)
};

} // namespace sunvox
