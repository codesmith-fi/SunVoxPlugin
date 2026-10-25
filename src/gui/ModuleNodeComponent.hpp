#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "../audio/SunVoxEngine.hpp"

namespace sunvox
{

class ModuleNodeComponent : public juce::Component
{
public:
	explicit ModuleNodeComponent(const ModuleInfo& info);
	~ModuleNodeComponent() override = default;

	void paint(juce::Graphics& g) override;

	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;

	int getModuleId() const noexcept { return info_.id; }
	const ModuleInfo& getModuleInfo() const noexcept { return info_; }

	juce::Point<float> getInputPinPosition() const;
	juce::Point<float> getOutputPinPosition() const;

	std::function<void()> onMoved;

	static constexpr int kDefaultWidth = 140;
	static constexpr int kDefaultHeight = 72;
	static constexpr float kPinRadius = 5.0f;

private:
	ModuleInfo info_;
	juce::ComponentDragger dragger_;

	juce::Colour getHeaderColor() const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleNodeComponent)
};

} // namespace sunvox
