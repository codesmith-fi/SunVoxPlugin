#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

namespace sunvox
{

class SlotManager
{
public:
	static int acquireSlot()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (size_t i = 0; i < usedSlots_.size(); ++i)
		{
			if (!usedSlots_[i])
			{
				usedSlots_[i] = true;
				return static_cast<int>(i);
			}
		}
		return -1; // All 16 slots full
	}

	static void releaseSlot(int slot)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (slot >= 0 && static_cast<size_t>(slot) < usedSlots_.size())
		{
			usedSlots_[static_cast<size_t>(slot)] = false;
		}
	}

	static int getActiveSlotCount()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		int count = 0;
		for (bool used : usedSlots_)
		{
			if (used) ++count;
		}
		return count;
	}

private:
	static inline std::mutex mutex_;
	static inline std::array<bool, 16> usedSlots_{false};
};

struct ModuleInfo
{
	int id{-1};
	std::string name;
	std::string type;
	int x{0};
	int y{0};
	std::vector<int> outputConnections;
};

struct PlayableModuleInfo
{
	int id{-1};
	std::string name;
	std::string type;
};

struct ControllerInfo
{
	int id{0};
	std::string name;
	int minValue{0};
	int maxValue{100};
	int currentValue{0};
	int type{0};
};

class SunVoxEngine
{
public:
	SunVoxEngine();
	~SunVoxEngine();

	bool loadLibrary(const std::string& libraryPath = "");
	bool init(int sampleRate = 44100);
	bool initialize(int sampleRate = 44100) { return init(sampleRate); }
	void close();

	void renderAudio(float* leftOutput, float* rightOutput, int numFrames);

	void sendMidiNote(int note, int velocity, int track = 0);
	void sendMidiNoteOff(int track = 0);
	void sendMidiNoteOn(int track, int noteNumber, float velocity);
	void sendMidiNoteOff(int track, int noteNumber);
	void sendAllNotesOff();
	void sendPitchBend(int track, int pitchVal);

	bool isInitialized() const noexcept { return isInitialized_; }
	bool isLibraryLoaded() const noexcept { return isLibraryLoaded_; }
	int getSampleRate() const noexcept { return sampleRate_; }
	int getSlot() const noexcept { return slot_; }
	int getDefaultSynthModule() const noexcept { return defaultSynthModule_; }
	int getDefaultModule() const noexcept { return defaultSynthModule_; }

	bool loadProject(const std::string& filePath);
	bool saveProject(const std::string& filePath);
	int loadModule(const std::string& filePath, int x = 0, int y = 0);
	std::vector<uint8_t> saveToMemory();
	bool loadFromMemory(const void* data, size_t size);

	bool createDefaultSynth();
	void clearNonOutputModules();
	void setDefaultSynthModule(int modId) noexcept { defaultSynthModule_ = modId; }
	std::vector<ModuleInfo> getActiveModules();

	int createModule(const std::string& type, const std::string& name, int x, int y);
	bool removeModule(int modId);
	bool connectModule(int source, int destination);
	bool disconnectModule(int source, int destination);
	bool connectModules(int sourceId, int destinationId) { return connectModule(sourceId, destinationId); }
	bool disconnectModules(int sourceId, int destinationId) { return disconnectModule(sourceId, destinationId); }
	bool disconnectAllInputs(int modId);
	bool disconnectAllOutputs(int modId);
	bool setModuleXY(int modId, int x, int y);

	std::vector<ControllerInfo> getModuleControllers(int modId);
	void setModuleCtlValue(int modId, int ctlId, int value);

	std::vector<PlayableModuleInfo> getPlayableModules();
	void autoSelectDefaultPlayableModule();

	void setActiveMidiModule(int modId) noexcept { targetMidiModuleId_.store(modId); }
	int getActiveMidiModule() const noexcept { return targetMidiModuleId_.load(); }

private:
	bool isLibraryLoaded_{false};
	bool isInitialized_{false};
	int sampleRate_{44100};
	int slot_{-1};
	int defaultSynthModule_{-1};
	std::atomic<int> targetMidiModuleId_{-1};
	std::vector<float> interleavedBuffer_;
	mutable std::recursive_mutex engineMutex_;

	static inline std::mutex globalInitMutex_;
	static inline std::mutex globalAudioMutex_;
	static inline int globalInstanceCount_{0};

	void updateDefaultModuleAfterLoad();
};

} // namespace sunvox
