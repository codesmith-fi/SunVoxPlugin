#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "../audio/SunVoxEngine.hpp"

namespace sunvox
{

class ParameterRowComponent : public juce::Component
{
public:
	ParameterRowComponent(int modId, const ControllerInfo& ctl, SunVoxEngine* engine);
	~ParameterRowComponent() override = default;

	void resized() override;
	void paint(juce::Graphics& g) override;

	int getControllerId() const noexcept { return ctlInfo_.id; }

private:
	int modId_;
	ControllerInfo ctlInfo_;
	SunVoxEngine* engine_;

	juce::Label nameLabel_;
	juce::Label valueLabel_;
	juce::Slider slider_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterRowComponent)
};

class InspectorPanel : public juce::Component
{
public:
	explicit InspectorPanel(SunVoxEngine* engine = nullptr);
	~InspectorPanel() override = default;

	void setEngine(SunVoxEngine* engine) noexcept { engine_ = engine; }
	void inspectModule(int modId);
	void clearInspection();

	void paint(juce::Graphics& g) override;
	void resized() override;

	int getCurrentModuleId() const noexcept { return currentModId_; }

private:
	SunVoxEngine* engine_{nullptr};
	int currentModId_{-1};
	std::string currentModuleName_;
	std::string currentModuleType_;

	juce::Viewport viewport_;
	std::unique_ptr<juce::Component> contentComponent_;
	std::vector<std::unique_ptr<ParameterRowComponent>> paramRows_;

	void rebuildParameters();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorPanel)
};

} // namespace sunvox
