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

	int getModuleId() const noexcept { return info_.id; }
	const ModuleInfo& getModuleInfo() const noexcept { return info_; }

	enum class PinType
	{
		None,
		Input,
		Output
	};

	PinType getPinAt(juce::Point<int> localPos) const;

	void setSelected(bool selected);
	bool isSelected() const noexcept { return isSelected_; }

	void setTargetMidiModule(bool isTarget);
	bool isTargetMidiModule() const noexcept { return isTargetMidiModule_; }

	void setInputPinHighlighted(bool highlighted);
	bool isInputPinHighlighted() const noexcept { return isInputPinHighlighted_; }

	juce::Point<float> getInputPinPosition() const;
	juce::Point<float> getOutputPinPosition() const;
	juce::Point<float> getInputPinGlobalPos() const { return getInputPinPosition(); }
	juce::Point<float> getOutputPinGlobalPos() const { return getOutputPinPosition(); }

	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;

	std::function<void()> onMoved;
	std::function<void(int)> onSelected;
	std::function<void(int sourceNodeId, juce::Point<float> startPinPos)> onCableDragStarted;
	std::function<void(juce::Point<float> currentMousePos)> onCableDragging;
	std::function<void(juce::Point<float> releaseMousePos)> onCableDragEnded;
	std::function<void(int modId, juce::Point<int> screenPos)> onNodeRightClicked;
	std::function<void(int modId, juce::Point<int> screenPos)> onInputPinRightClicked;

	static constexpr int kDefaultWidth = 140;
	static constexpr int kDefaultHeight = 72;
	static constexpr float kPinRadius = 5.0f;
	static constexpr float kPinHitRadius = 14.0f;

private:
	ModuleInfo info_;
	juce::Point<int> dragStartComponentPos_;
	juce::Point<float> dragStartMousePos_;
	bool isSelected_{false};
	bool isTargetMidiModule_{false};
	bool isInputPinHighlighted_{false};
	bool isDraggingCable_{false};

	juce::Colour getHeaderColor() const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleNodeComponent)
};

} // namespace sunvox
