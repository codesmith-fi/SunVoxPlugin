#include "SunVoxEngine.hpp"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <juce_core/juce_core.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define SUNVOX_MAIN
#include "../../sunvox_lib/sunvox.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace sunvox
{

SunVoxEngine::SunVoxEngine()
{
}

SunVoxEngine::~SunVoxEngine()
{
	close();
}

bool SunVoxEngine::loadLibrary(const std::string& libraryPath)
{
	if (isLibraryLoaded_)
		return true;

	std::vector<std::string> candidatePaths;
	if (!libraryPath.empty())
	{
		candidatePaths.push_back(libraryPath);
	}

#if defined(_WIN32) || defined(_WIN64)
	const std::string libName = "sunvox.dll";
#elif defined(__APPLE__)
	const std::string libName = "sunvox.dylib";
#else
	const std::string libName = "sunvox.so";
#endif

	// Potential relative paths depending on executable location
	candidatePaths.push_back(libName);
	candidatePaths.push_back("./" + libName);

#if defined(_WIN32) || defined(_WIN64)
	candidatePaths.push_back("sunvox_lib/sunvox_lib/windows/lib_x86_64/sunvox.dll");
	candidatePaths.push_back("../sunvox_lib/sunvox_lib/windows/lib_x86_64/sunvox.dll");
	candidatePaths.push_back("../../sunvox_lib/sunvox_lib/windows/lib_x86_64/sunvox.dll");
#else
	candidatePaths.push_back("sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../../../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("sunvox_lib/sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../sunvox_lib/sunvox_lib/linux/x86_64/sunvox.so");
#endif

	// Check relative to current application / plugin bundle (VST3 / CLAP / Standalone)
	auto appFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
	if (appFile.exists())
	{
		candidatePaths.push_back(appFile.getParentDirectory().getChildFile(libName).getFullPathName().toStdString());
		candidatePaths.push_back(appFile.getParentDirectory().getParentDirectory().getChildFile("Resources").getChildFile(libName).getFullPathName().toStdString());
	}

	auto execFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
	if (execFile.exists())
	{
		candidatePaths.push_back(execFile.getParentDirectory().getChildFile(libName).getFullPathName().toStdString());
	}

	auto userHome = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
	candidatePaths.push_back(userHome.getChildFile(".local/share/SunVoxPlugin/" + libName).getFullPathName().toStdString());
	candidatePaths.push_back(userHome.getChildFile(".vst3/SunVoxPlugin.vst3/Contents/x86_64-linux/sunvox.so").getFullPathName().toStdString());
	candidatePaths.push_back(userHome.getChildFile(".clap/" + libName).getFullPathName().toStdString());
	candidatePaths.push_back("/usr/local/lib/sunvox.so");

	for (const auto& path : candidatePaths)
	{
		if (std::filesystem::exists(path))
		{
			if (sv_load_dll2(path.c_str()) == 0)
			{
				isLibraryLoaded_ = true;
				std::cout << "[SunVoxEngine] Loaded library from: " << path << std::endl;
				return true;
			}
		}
	}

	// Fallback to default system loader
	if (sv_load_dll() == 0)
	{
		isLibraryLoaded_ = true;
		std::cout << "[SunVoxEngine] Loaded library using default loader" << std::endl;
		return true;
	}

	std::cerr << "[SunVoxEngine] Error: Could not find or load sunvox.so" << std::endl;
	return false;
}

bool SunVoxEngine::init(int sampleRate)
{
	std::lock_guard<std::recursive_mutex> lock(engineMutex_);

	if (isInitialized_ && sampleRate_ == sampleRate && slot_ >= 0)
		return true;

	if (isInitialized_)
		close();

	std::lock_guard<std::mutex> globalLock(globalInitMutex_);

	if (!isLibraryLoaded_ && !loadLibrary())
		return false;

	sampleRate_ = sampleRate > 0 ? sampleRate : 44100;

	// Global sv_init only once across all instances in the process
	if (globalInstanceCount_ == 0)
	{
		uint32_t flags = SV_INIT_FLAG_USER_AUDIO_CALLBACK |
		                 SV_INIT_FLAG_AUDIO_FLOAT32 |
		                 SV_INIT_FLAG_ONE_THREAD;

		int res = sv_init(nullptr, sampleRate_, 2, flags);
		if (res < 0)
		{
			std::cerr << "[SunVoxEngine] Failed to initialize global SunVox sound system, code: " << res << std::endl;
			return false;
		}
	}

	// Acquire a dynamic slot (0..15)
	slot_ = SlotManager::acquireSlot();
	if (slot_ < 0)
	{
		std::cerr << "[SunVoxEngine] Error: All SunVox slots (0-15) are occupied!" << std::endl;
		if (globalInstanceCount_ == 0)
		{
			sv_deinit();
		}
		return false;
	}

	sv_open_slot(slot_);
	sv_pause(slot_); // Keep paused so it won't bleed into other instances until rendered
	sv_volume(slot_, 256);

	globalInstanceCount_++;
	isInitialized_ = true;

	std::cout << "[SunVoxEngine] Instance initialized on Slot " << slot_
	          << " (active instances: " << globalInstanceCount_ << ")" << std::endl;

	createDefaultSynth();
	return true;
}

bool SunVoxEngine::createDefaultSynth()
{
	if (!isInitialized_ || slot_ < 0)
		return false;

	sv_lock_slot(slot_);
	defaultSynthModule_ = sv_new_module(slot_, "Generator", "Default Lead", 0, 0, 0);
	if (defaultSynthModule_ >= 0)
	{
		// Module 0 is always the master Output module in SunVox
		sv_connect_module(slot_, defaultSynthModule_, 0);
		targetMidiModuleId_.store(defaultSynthModule_);
		std::cout << "[SunVoxEngine] Created Generator module (" << defaultSynthModule_
		          << ") on Slot " << slot_ << " and connected to Output (0)" << std::endl;
	}
	else
	{
		std::cerr << "[SunVoxEngine] Failed to create Generator module on Slot " << slot_ << std::endl;
	}
	sv_unlock_slot(slot_);

	return defaultSynthModule_ >= 0;
}

void SunVoxEngine::clearNonOutputModules()
{
	if (!isInitialized_ || slot_ < 0)
		return;

	sv_lock_slot(slot_);
	int numModules = sv_get_number_of_modules(slot_);
	for (int i = numModules - 1; i >= 1; --i)
	{
		uint32_t flags = sv_get_module_flags(slot_, i);
		if ((flags & SV_MODULE_FLAG_EXISTS) != 0)
		{
			sv_remove_module(slot_, i);
		}
	}
	defaultSynthModule_ = -1;
	targetMidiModuleId_.store(-1);
	sv_unlock_slot(slot_);
}

bool SunVoxEngine::loadProject(const std::string& filePath)
{
	if (!isInitialized_ || slot_ < 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_load(slot_, filePath.c_str());
	sv_unlock_slot(slot_);

	if (res == 0)
	{
		sv_pause(slot_); // keep slot paused so it won't bleed into other instances
		sv_volume(slot_, 256);
		updateDefaultModuleAfterLoad();
		return true;
	}
	return false;
}

bool SunVoxEngine::saveProject(const std::string& filePath)
{
	if (!isInitialized_ || slot_ < 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_save(slot_, filePath.c_str());
	sv_unlock_slot(slot_);
	return res == 0;
}

int SunVoxEngine::loadModule(const std::string& filePath, int x, int y)
{
	if (!isInitialized_ || slot_ < 0)
		return -1;

	sv_lock_slot(slot_);
	int modId = sv_load_module(slot_, filePath.c_str(), x, y, 0);
	sv_unlock_slot(slot_);
	return modId;
}

std::vector<uint8_t> SunVoxEngine::saveToMemory()
{
	std::vector<uint8_t> buffer;
	if (!isInitialized_ || slot_ < 0)
		return buffer;

	sv_lock_slot(slot_);
	size_t size = 0;
	void* data = sv_save_to_memory(slot_, &size);
	sv_unlock_slot(slot_);

	if (data != nullptr && size > 0)
	{
		buffer.resize(size);
		std::memcpy(buffer.data(), data, size);
		free(data);
	}
	return buffer;
}

bool SunVoxEngine::loadFromMemory(const void* data, size_t size)
{
	if (!isInitialized_ || slot_ < 0 || data == nullptr || size == 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_load_from_memory(slot_, const_cast<void*>(data), static_cast<uint32_t>(size));
	sv_unlock_slot(slot_);

	if (res == 0)
	{
		sv_pause(slot_); // keep slot paused so it won't bleed into other instances
		sv_volume(slot_, 256);
		updateDefaultModuleAfterLoad();
		return true;
	}
	return false;
}

std::vector<PlayableModuleInfo> SunVoxEngine::getPlayableModules()
{
	std::vector<PlayableModuleInfo> playables;
	if (!isInitialized_ || slot_ < 0)
		return playables;

	sv_lock_slot(slot_);
	int numModules = sv_get_number_of_modules(slot_);
	for (int i = 0; i < numModules; ++i)
	{
		uint32_t flags = sv_get_module_flags(slot_, i);
		if ((flags & SV_MODULE_FLAG_EXISTS) == 0)
			continue;

		const char* type = sv_get_module_type(slot_, i);
		std::string typeStr = type ? type : "";
		if (typeStr == "Output")
			continue;

		bool isGenerator = (flags & SV_MODULE_FLAG_GENERATOR) != 0;
		bool isSynthOrSampler = (typeStr == "MultiSynth" || typeStr == "MetaModule" ||
		                         typeStr == "Generator" || typeStr == "Analog Generator" ||
		                         typeStr == "FM" || typeStr == "SpectraVoice" ||
		                         typeStr == "Sampler" || typeStr == "DrumSynth" ||
		                         typeStr == "Kicker" || typeStr == "VPlayer");

		if (isGenerator || isSynthOrSampler)
		{
			const char* name = sv_get_module_name(slot_, i);
			std::string nameStr = name ? name : ("Module " + std::to_string(i));
			playables.push_back({ i, nameStr, typeStr });
		}
	}
	sv_unlock_slot(slot_);

	return playables;
}

void SunVoxEngine::autoSelectDefaultPlayableModule()
{
	auto playables = getPlayableModules();
	if (playables.empty())
	{
		defaultSynthModule_ = -1;
		targetMidiModuleId_.store(-1);
		return;
	}

	// 1. If currently selected target is still valid in playables, keep it
	int currentTarget = targetMidiModuleId_.load();
	if (currentTarget >= 0)
	{
		for (const auto& p : playables)
		{
			if (p.id == currentTarget)
			{
				defaultSynthModule_ = currentTarget;
				return;
			}
		}
	}

	// 2. Prioritize MultiSynth if present
	for (const auto& p : playables)
	{
		if (p.type == "MultiSynth")
		{
			defaultSynthModule_ = p.id;
			targetMidiModuleId_.store(p.id);
			return;
		}
	}

	// 3. Otherwise pick the first playable module
	defaultSynthModule_ = playables.front().id;
	targetMidiModuleId_.store(playables.front().id);
}

void SunVoxEngine::updateDefaultModuleAfterLoad()
{
	defaultSynthModule_ = -1;
	if (slot_ < 0)
		return;

	autoSelectDefaultPlayableModule();
}

void SunVoxEngine::close()
{
	std::lock_guard<std::recursive_mutex> lock(engineMutex_);
	if (isInitialized_ && slot_ >= 0)
	{
		sv_stop(slot_);
		sv_close_slot(slot_);
		SlotManager::releaseSlot(slot_);
		std::cout << "[SunVoxEngine] Closed Slot " << slot_ << std::endl;
		slot_ = -1;
		isInitialized_ = false;

		std::lock_guard<std::mutex> globalLock(globalInitMutex_);
		globalInstanceCount_--;
		if (globalInstanceCount_ <= 0)
		{
			globalInstanceCount_ = 0;
			sv_deinit();
			if (isLibraryLoaded_)
			{
				sv_unload_dll();
				isLibraryLoaded_ = false;
			}
			std::cout << "[SunVoxEngine] All instances closed. Global deinit complete." << std::endl;
		}
	}
}

void SunVoxEngine::renderAudio(float* leftOutput, float* rightOutput, int numFrames)
{
	std::lock_guard<std::recursive_mutex> lock(engineMutex_);
	if (!isInitialized_ || slot_ < 0 || numFrames <= 0)
	{
		std::fill_n(leftOutput, numFrames, 0.0f);
		std::fill_n(rightOutput, numFrames, 0.0f);
		return;
	}

	size_t totalSamples = static_cast<size_t>(numFrames) * 2;
	if (interleavedBuffer_.size() < totalSamples)
	{
		interleavedBuffer_.resize(totalSamples, 0.0f);
	}

	uint32_t ticks = sv_get_ticks();

	// Render this slot exclusively to prevent bleed between instances
	{
		std::lock_guard<std::mutex> audioLock(globalAudioMutex_);
		sv_resume(slot_);
		sv_audio_callback(interleavedBuffer_.data(), numFrames, 0, ticks);
		sv_pause(slot_);
	}

	for (int i = 0; i < numFrames; ++i)
	{
		leftOutput[i] = interleavedBuffer_[static_cast<size_t>(i) * 2];
		rightOutput[i] = interleavedBuffer_[static_cast<size_t>(i) * 2 + 1];
	}
}

void SunVoxEngine::sendMidiNote(int note, int velocity, int track)
{
	int targetMod = targetMidiModuleId_.load();
	if (targetMod < 0) targetMod = defaultSynthModule_;
	if (!isInitialized_ || slot_ < 0 || targetMod < 0)
		return;

	int svNote = std::clamp(note + 1, 1, 127);
	int svVel = (velocity > 0) ? std::clamp(velocity, 1, 129) : 129;
	int targetModule = targetMod + 1;

	sv_send_event(slot_, track, svNote, svVel, targetModule, 0, 0);
}

void SunVoxEngine::sendMidiNoteOff(int track)
{
	int targetMod = targetMidiModuleId_.load();
	if (targetMod < 0) targetMod = defaultSynthModule_;
	if (!isInitialized_ || slot_ < 0 || targetMod < 0)
		return;

	int targetModule = targetMod + 1;
	sv_send_event(slot_, track, NOTECMD_NOTE_OFF, 0, targetModule, 0, 0);
}

void SunVoxEngine::sendMidiNoteOff(int track, int noteNumber)
{
	(void)noteNumber;
	sendMidiNoteOff(track);
}

void SunVoxEngine::sendMidiNoteOn(int track, int noteNumber, float velocity)
{
	int vel = std::clamp(static_cast<int>(velocity * 128.0f) + 1, 1, 129);
	sendMidiNote(noteNumber, vel, track);
}

void SunVoxEngine::sendAllNotesOff()
{
	if (!isInitialized_ || slot_ < 0)
		return;

	sv_send_event(slot_, 0, NOTECMD_ALL_NOTES_OFF, 0, 0, 0, 0);
}

void SunVoxEngine::sendPitchBend(int track, int pitchVal)
{
	int targetMod = targetMidiModuleId_.load();
	if (targetMod < 0) targetMod = defaultSynthModule_;
	if (!isInitialized_ || slot_ < 0 || targetMod < 0)
		return;

	int targetModule = targetMod + 1;
	sv_send_event(slot_, track, NOTECMD_SET_PITCH, 0, targetModule, 0, pitchVal);
}

std::vector<ModuleInfo> SunVoxEngine::getActiveModules()
{
	std::vector<ModuleInfo> modules;
	if (!isInitialized_ || slot_ < 0)
		return modules;

	sv_lock_slot(slot_);
	int numModules = sv_get_number_of_modules(slot_);
	for (int i = 0; i < numModules; ++i)
	{
		uint32_t flags = sv_get_module_flags(slot_, i);
		if ((flags & SV_MODULE_FLAG_EXISTS) == 0)
			continue;

		ModuleInfo info;
		info.id = i;

		const char* name = sv_get_module_name(slot_, i);
		info.name = name ? name : ("Module " + std::to_string(i));

		const char* type = sv_get_module_type(slot_, i);
		info.type = type ? type : "";

		uint32_t xy = sv_get_module_xy(slot_, i);
		int modX = 0, modY = 0;
		SV_GET_MODULE_XY(xy, modX, modY);
		info.x = modX;
		info.y = modY;

		int numOutputs = static_cast<int>((flags & SV_MODULE_OUTPUTS_MASK) >> SV_MODULE_OUTPUTS_OFF);
		int* outputs = sv_get_module_outputs(slot_, i);
		if (outputs != nullptr && numOutputs > 0)
		{
			for (int j = 0; j < numOutputs; ++j)
			{
				if (outputs[j] >= 0)
					info.outputConnections.push_back(outputs[j]);
			}
		}

		modules.push_back(std::move(info));
	}
	sv_unlock_slot(slot_);

	return modules;
}

int SunVoxEngine::createModule(const std::string& type, const std::string& name, int x, int y)
{
	if (!isInitialized_ || slot_ < 0)
		return -1;

	sv_lock_slot(slot_);
	int modId = sv_new_module(slot_, type.c_str(), name.c_str(), x, y, 0);
	sv_unlock_slot(slot_);
	return modId;
}

bool SunVoxEngine::removeModule(int modId)
{
	if (!isInitialized_ || slot_ < 0 || modId < 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_remove_module(slot_, modId);
	sv_unlock_slot(slot_);
	return res == 0;
}

bool SunVoxEngine::connectModule(int source, int destination)
{
	if (!isInitialized_ || slot_ < 0 || source < 0 || destination < 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_connect_module(slot_, source, destination);
	sv_unlock_slot(slot_);
	return res == 0;
}

bool SunVoxEngine::disconnectModule(int source, int destination)
{
	if (!isInitialized_ || slot_ < 0 || source < 0 || destination < 0)
		return false;

	sv_lock_slot(slot_);
	int res = sv_disconnect_module(slot_, source, destination);
	sv_unlock_slot(slot_);
	return res == 0;
}

bool SunVoxEngine::disconnectAllInputs(int modId)
{
	if (!isInitialized_ || slot_ < 0 || modId < 0)
		return false;

	auto modules = getActiveModules();
	bool anyDisconnected = false;
	for (const auto& mod : modules)
	{
		for (int dest : mod.outputConnections)
		{
			if (dest == modId)
			{
				if (disconnectModule(mod.id, modId))
					anyDisconnected = true;
			}
		}
	}
	return anyDisconnected;
}

bool SunVoxEngine::disconnectAllOutputs(int modId)
{
	if (!isInitialized_ || slot_ < 0 || modId < 0)
		return false;

	auto modules = getActiveModules();
	for (const auto& mod : modules)
	{
		if (mod.id == modId)
		{
			for (int dest : mod.outputConnections)
			{
				disconnectModule(modId, dest);
			}
			return true;
		}
	}
	return false;
}

bool SunVoxEngine::setModuleXY(int modId, int x, int y)
{
	if (!isInitialized_ || slot_ < 0 || modId < 0)
		return false;

	return sv_set_module_xy(slot_, modId, x, y) == 0;
}

std::vector<ControllerInfo> SunVoxEngine::getModuleControllers(int modId)
{
	std::vector<ControllerInfo> controllers;
	if (!isInitialized_ || slot_ < 0 || modId < 0)
		return controllers;

	int numCtls = sv_get_number_of_module_ctls(slot_, modId);
	if (numCtls <= 0)
		return controllers;

	controllers.reserve(static_cast<size_t>(numCtls));
	for (int i = 0; i < numCtls; ++i)
	{
		ControllerInfo ctl;
		ctl.id = i;
		const char* name = sv_get_module_ctl_name(slot_, modId, i);
		ctl.name = name ? name : ("Ctl " + std::to_string(i));
		ctl.minValue = sv_get_module_ctl_min(slot_, modId, i, 0);
		ctl.maxValue = sv_get_module_ctl_max(slot_, modId, i, 0);
		ctl.currentValue = sv_get_module_ctl_value(slot_, modId, i, 0);
		ctl.type = sv_get_module_ctl_type(slot_, modId, i);
		controllers.push_back(std::move(ctl));
	}

	return controllers;
}

void SunVoxEngine::setModuleCtlValue(int modId, int ctlId, int value)
{
	if (!isInitialized_ || slot_ < 0 || modId < 0 || ctlId < 0)
		return;

	sv_set_module_ctl_value(slot_, modId, ctlId, value, 0);
}

} // namespace sunvox

