#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sunvox
{

struct ModuleInfo
{
	int id{-1};
	std::string name;
	std::string type;
	int x{0};
	int y{0};
	std::vector<int> outputConnections;
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
	bool createDefaultSynth();
	std::vector<ModuleInfo> getActiveModules();

	int createModule(const std::string& type, const std::string& name, int x, int y);
	bool removeModule(int modId);
	bool connectModule(int source, int destination);
	bool disconnectModule(int source, int destination);
	bool setModuleXY(int modId, int x, int y);

	std::vector<ControllerInfo> getModuleControllers(int modId);
	void setModuleCtlValue(int modId, int ctlId, int value);

private:
	bool isLibraryLoaded_{false};
	bool isInitialized_{false};
	int sampleRate_{44100};
	int slot_{0};
	int defaultSynthModule_{-1};
	std::vector<float> interleavedBuffer_;
};

} // namespace sunvox
