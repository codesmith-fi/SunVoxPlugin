#include "SunVoxEngine.hpp"

#include <algorithm>
#include <iostream>
#include <filesystem>

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

	// Potential relative paths depending on executable location
	candidatePaths.push_back("sunvox.so");
	candidatePaths.push_back("./sunvox.so");
	candidatePaths.push_back("sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../../../sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("sunvox_lib/sunvox_lib/linux/x86_64/sunvox.so");
	candidatePaths.push_back("../sunvox_lib/sunvox_lib/linux/x86_64/sunvox.so");

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
	if (isInitialized_)
		close();

	if (!isLibraryLoaded_ && !loadLibrary())
		return false;

	sampleRate_ = sampleRate > 0 ? sampleRate : 44100;

	uint32_t flags = SV_INIT_FLAG_USER_AUDIO_CALLBACK |
	                 SV_INIT_FLAG_AUDIO_FLOAT32 |
	                 SV_INIT_FLAG_ONE_THREAD;

	int res = sv_init(nullptr, sampleRate_, 2, flags);
	if (res < 0)
	{
		std::cerr << "[SunVoxEngine] Failed to initialize SunVox, code: " << res << std::endl;
		return false;
	}

	sv_open_slot(slot_);
	isInitialized_ = true;

	createDefaultSynth();
	return true;
}

bool SunVoxEngine::createDefaultSynth()
{
	if (!isInitialized_)
		return false;

	sv_lock_slot(slot_);
	defaultSynthModule_ = sv_new_module(slot_, "Generator", "Default Lead", 0, 0, 0);
	if (defaultSynthModule_ >= 0)
	{
		// Module 0 is always the master Output module in SunVox
		sv_connect_module(slot_, defaultSynthModule_, 0);
		std::cout << "[SunVoxEngine] Created Generator module (" << defaultSynthModule_
		          << ") and connected to Output (0)" << std::endl;
	}
	else
	{
		std::cerr << "[SunVoxEngine] Failed to create Generator module" << std::endl;
	}
	sv_unlock_slot(slot_);

	return defaultSynthModule_ >= 0;
}

bool SunVoxEngine::loadProject(const std::string& filePath)
{
	if (!isInitialized_)
		return false;

	sv_lock_slot(slot_);
	int res = sv_load(slot_, filePath.c_str());
	sv_unlock_slot(slot_);

	if (res == 0)
	{
		defaultSynthModule_ = -1; // Project has its own routing
		return true;
	}
	return false;
}

void SunVoxEngine::close()
{
	if (isInitialized_)
	{
		sv_stop(slot_);
		sv_close_slot(slot_);
		sv_deinit();
		isInitialized_ = false;
	}

	if (isLibraryLoaded_)
	{
		sv_unload_dll();
		isLibraryLoaded_ = false;
	}
}

void SunVoxEngine::renderAudio(float* leftOutput, float* rightOutput, int numFrames)
{
	if (!isInitialized_ || numFrames <= 0)
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
	sv_audio_callback(interleavedBuffer_.data(), numFrames, 0, ticks);

	for (int i = 0; i < numFrames; ++i)
	{
		leftOutput[i] = interleavedBuffer_[static_cast<size_t>(i) * 2];
		rightOutput[i] = interleavedBuffer_[static_cast<size_t>(i) * 2 + 1];
	}
}

void SunVoxEngine::sendMidiNote(int note, int velocity, int track)
{
	if (!isInitialized_ || defaultSynthModule_ < 0)
		return;

	// SunVox-nuotit ovat 1..127 (note + 1).
	// Huom: SunVox C API:ssa sv_send_event:in module-parametri on 1-pohjainen:
	// sunvox.h rivi 289: "module: 0 (empty) or module number + 1 (1..65535);"
	// Siksi moduulille 1 (Generator) välitetään (defaultSynthModule_ + 1) eli 2.
	int svNote = std::clamp(note + 1, 1, 127);
	int svVel = (velocity > 0) ? std::clamp(velocity, 1, 129) : 129;
	int targetModule = defaultSynthModule_ + 1;

	sv_send_event(slot_, track, svNote, svVel, targetModule, 0, 0);
}

void SunVoxEngine::sendMidiNoteOff(int track)
{
	if (!isInitialized_ || defaultSynthModule_ < 0)
		return;

	int targetModule = defaultSynthModule_ + 1;
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
	if (!isInitialized_)
		return;

	sv_send_event(slot_, 0, NOTECMD_ALL_NOTES_OFF, 0, 0, 0, 0);
}

void SunVoxEngine::sendPitchBend(int track, int pitchVal)
{
	if (!isInitialized_ || defaultSynthModule_ < 0)
		return;

	int targetModule = defaultSynthModule_ + 1;
	sv_send_event(slot_, track, NOTECMD_SET_PITCH, 0, targetModule, 0, pitchVal);
}

std::vector<ModuleInfo> SunVoxEngine::getActiveModules()
{
	std::vector<ModuleInfo> modules;
	if (!isInitialized_)
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

} // namespace sunvox
