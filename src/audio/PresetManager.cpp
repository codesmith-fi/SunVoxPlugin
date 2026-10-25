#include "PresetManager.hpp"
#include <algorithm>
#include <set>

namespace sunvox
{

PresetManager::PresetManager()
{
	scanPresets();
}

juce::File PresetManager::findPresetsDirectory(const juce::File& explicitDir)
{
	if (explicitDir.isDirectory())
		return explicitDir;

	// 1. Environment variable override
	auto envDir = juce::SystemStats::getEnvironmentVariable("SUNVOX_ASSETS_DIR", {});
	if (envDir.isNotEmpty())
	{
		auto envFile = juce::File(envDir);
		if (envFile.getChildFile("instruments").isDirectory())
			return envFile.getChildFile("instruments");
		if (envFile.isDirectory())
			return envFile;
	}

	// 2. Standard XDG data directory (~/.local/share/SunVoxPlugin)
	auto userLocalShare = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
	                          .getChildFile(".local/share/SunVoxPlugin");
	if (userLocalShare.getChildFile("assets/instruments").isDirectory())
		return userLocalShare.getChildFile("assets/instruments");
	if (userLocalShare.getChildFile("instruments").isDirectory())
		return userLocalShare.getChildFile("instruments");

	// 3. VST3 bundle resources directory
	auto appFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
	if (appFile.exists())
	{
		auto bundleResources = appFile.getParentDirectory().getParentDirectory().getChildFile("Resources");
		if (bundleResources.getChildFile("assets/instruments").isDirectory())
			return bundleResources.getChildFile("assets/instruments");
		if (bundleResources.getChildFile("instruments").isDirectory())
			return bundleResources.getChildFile("instruments");
	}

	// 4. Standard ~/.vst3 bundle location
	auto homeVst3 = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
	                    .getChildFile(".vst3/SunVoxPlugin.vst3/Contents/Resources");
	if (homeVst3.getChildFile("assets/instruments").isDirectory())
		return homeVst3.getChildFile("assets/instruments");

	// 5. Relative search from CWD, executable, application
	std::vector<juce::File> searchRoots = {
		juce::File::getCurrentWorkingDirectory(),
		juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory(),
		appFile.getParentDirectory()
	};

	for (const auto& root : searchRoots)
	{
		juce::File curr = root;
		for (int level = 0; level < 6; ++level)
		{
			auto candidate = curr.getChildFile("assets").getChildFile("instruments");
			if (candidate.isDirectory())
				return candidate;

			auto cand2 = curr.getChildFile("instruments");
			if (cand2.isDirectory())
				return cand2;

			auto parent = curr.getParentDirectory();
			if (parent == curr)
				break;
			curr = parent;
		}
	}

	return {};
}

void PresetManager::scanPresets(const juce::File& explicitDir)
{
	allPresets_.clear();
	categories_.clear();
	currentIndex_ = -1;
	currentPreset_ = {};

	baseDir_ = findPresetsDirectory(explicitDir);
	if (!baseDir_.isDirectory())
	{
		categories_.push_back("All");
		return;
	}

	// 1. Scan instruments folder
	scanDirectoryRecursive(baseDir_, "", false);

	// 2. Also check if sibling "projects" folder exists
	auto projectsDir = baseDir_.getParentDirectory().getChildFile("projects");
	if (projectsDir.isDirectory())
	{
		scanDirectoryRecursive(projectsDir, "Projects", true);
	}

	// 3. Build unique sorted categories list
	std::set<juce::String> catSet;
	for (const auto& p : allPresets_)
	{
		if (p.category.isNotEmpty())
			catSet.insert(p.category);
	}

	categories_.push_back("All");
	for (const auto& cat : catSet)
	{
		categories_.push_back(cat);
	}

	// Sort presets by category then name
	std::sort(allPresets_.begin(), allPresets_.end(), [](const PresetItem& a, const PresetItem& b) {
		if (a.category != b.category)
			return a.category < b.category;
		return a.name.compareIgnoreCase(b.name) < 0;
	});
}

void PresetManager::scanDirectoryRecursive(const juce::File& dir, const juce::String& categoryPrefix, bool isProjectFolder)
{
	auto files = dir.findChildFiles(juce::File::findFilesAndDirectories, false);
	for (const auto& f : files)
	{
		if (f.isDirectory())
		{
			juce::String subCat = categoryPrefix.isEmpty() ? f.getFileName() : (categoryPrefix + "/" + f.getFileName());
			scanDirectoryRecursive(f, subCat, isProjectFolder);
		}
		else if (f.existsAsFile())
		{
			bool isSynth = f.hasFileExtension(".sunsynth");
			bool isProject = f.hasFileExtension(".sunvox");

			if (isSynth || isProject)
			{
				PresetItem item;
				item.file = f;
				item.name = f.getFileNameWithoutExtension();
				item.isSynthFile = isSynth;
				item.category = categoryPrefix.isEmpty() ? (isSynth ? "Instruments" : "Projects") : categoryPrefix;
				allPresets_.push_back(std::move(item));
			}
		}
	}
}

std::vector<PresetItem> PresetManager::getFilteredPresets(const juce::String& category, const juce::String& searchText) const
{
	std::vector<PresetItem> result;
	juce::String lowerSearch = searchText.trim().toLowerCase();

	for (const auto& p : allPresets_)
	{
		if (category.isNotEmpty() && category != "All")
		{
			if (p.category != category && !p.category.startsWith(category + "/"))
				continue;
		}

		if (lowerSearch.isNotEmpty())
		{
			if (!p.name.toLowerCase().contains(lowerSearch) && !p.category.toLowerCase().contains(lowerSearch))
				continue;
		}

		result.push_back(p);
	}

	return result;
}

bool PresetManager::loadPreset(const PresetItem& item, SunVoxEngine& engine)
{
	if (!item.file.existsAsFile())
		return false;

	bool success = false;
	if (item.isSynthFile)
	{
		// Clear previous synths to keep the canvas clean during auditioning
		engine.clearNonOutputModules();

		int modId = engine.loadModule(item.file.getFullPathName().toStdString(), 180, 180);
		if (modId >= 0)
		{
			engine.connectModules(modId, 0);
			engine.setDefaultSynthModule(modId);
			engine.setActiveMidiModule(modId);
			success = true;
		}
	}
	else
	{
		success = engine.loadProject(item.file.getFullPathName().toStdString());
	}

	if (success)
	{
		currentPreset_ = item;
		for (size_t i = 0; i < allPresets_.size(); ++i)
		{
			if (allPresets_[i].file == item.file)
			{
				currentIndex_ = static_cast<int>(i);
				break;
			}
		}
	}

	return success;
}

bool PresetManager::loadPreset(const juce::File& file, SunVoxEngine& engine)
{
	for (const auto& p : allPresets_)
	{
		if (p.file == file)
			return loadPreset(p, engine);
	}

	// Unlisted file
	PresetItem item;
	item.file = file;
	item.name = file.getFileNameWithoutExtension();
	item.isSynthFile = file.hasFileExtension(".sunsynth");
	item.category = "External";
	return loadPreset(item, engine);
}

bool PresetManager::loadPresetByIndex(int index, SunVoxEngine& engine)
{
	if (index < 0 || index >= static_cast<int>(allPresets_.size()))
		return false;

	return loadPreset(allPresets_[static_cast<size_t>(index)], engine);
}

bool PresetManager::nextPreset(SunVoxEngine& engine)
{
	if (allPresets_.empty())
		return false;

	int nextIdx = (currentIndex_ + 1) % static_cast<int>(allPresets_.size());
	return loadPresetByIndex(nextIdx, engine);
}

bool PresetManager::prevPreset(SunVoxEngine& engine)
{
	if (allPresets_.empty())
		return false;

	int prevIdx = currentIndex_ - 1;
	if (prevIdx < 0)
		prevIdx = static_cast<int>(allPresets_.size()) - 1;

	return loadPresetByIndex(prevIdx, engine);
}

juce::String PresetManager::getCurrentPresetDisplayName() const
{
	if (currentPreset_.name.isEmpty())
		return "Default Lead";

	if (currentPreset_.category.isNotEmpty())
		return "[" + currentPreset_.category + "] " + currentPreset_.name;

	return currentPreset_.name;
}

} // namespace sunvox
