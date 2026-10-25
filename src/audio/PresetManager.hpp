#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include "SunVoxEngine.hpp"

namespace sunvox
{

struct PresetItem
{
	juce::String name;
	juce::File file;
	juce::String category;
	bool isSynthFile{true}; // true: .sunsynth, false: .sunvox
};

class PresetManager
{
public:
	PresetManager();
	~PresetManager() = default;

	void scanPresets(const juce::File& explicitDir = {});

	const std::vector<PresetItem>& getAllPresets() const noexcept { return allPresets_; }
	const std::vector<juce::String>& getCategories() const noexcept { return categories_; }

	std::vector<PresetItem> getFilteredPresets(const juce::String& category, const juce::String& searchText) const;

	bool loadPreset(const PresetItem& item, SunVoxEngine& engine);
	bool loadPreset(const juce::File& file, SunVoxEngine& engine);
	bool loadPresetByIndex(int index, SunVoxEngine& engine);

	bool nextPreset(SunVoxEngine& engine);
	bool prevPreset(SunVoxEngine& engine);

	int getCurrentIndex() const noexcept { return currentIndex_; }
	PresetItem getCurrentPreset() const { return currentPreset_; }
	juce::String getCurrentPresetDisplayName() const;

	juce::File getPresetsDirectory() const noexcept { return baseDir_; }

private:
	std::vector<PresetItem> allPresets_;
	std::vector<juce::String> categories_;
	PresetItem currentPreset_;
	int currentIndex_{-1};
	juce::File baseDir_;

	juce::File findPresetsDirectory(const juce::File& explicitDir);
	void scanDirectoryRecursive(const juce::File& dir, const juce::String& categoryPrefix, bool isProjectFolder);
};

} // namespace sunvox
