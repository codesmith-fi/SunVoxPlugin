#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "ModuleNodeComponent.hpp"
#include "../audio/SunVoxEngine.hpp"

namespace sunvox
{

class GraphCanvas : public juce::Component
{
public:
	explicit GraphCanvas(SunVoxEngine* engine = nullptr);
	~GraphCanvas() override = default;

	void setEngine(SunVoxEngine* engine) noexcept { engine_ = engine; }
	void refreshFromEngine();
	void refreshFromEngine(SunVoxEngine& engine);
	void updateFromEngine(const std::vector<ModuleInfo>& modules);

	void selectModule(int modId);
	int getSelectedModuleId() const noexcept { return selectedModuleId_; }

	void paint(juce::Graphics& g) override;
	void paintOverChildren(juce::Graphics& g) override;
	void resized() override;

	void mouseDown(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;

	std::function<void(int)> onModuleSelected;

private:
	SunVoxEngine* engine_{nullptr};
	int selectedModuleId_{-1};
	std::vector<std::unique_ptr<ModuleNodeComponent>> nodes_;
	std::unordered_map<int, ModuleNodeComponent*> nodeMap_;

	// Cable drag state
	bool isDraggingCable_{false};
	int dragSourceNodeId_{-1};
	juce::Point<float> dragSourcePinPos_;
	juce::Point<float> dragCurrentPos_;
	int dragHoverTargetNodeId_{-1};

	void handleCableDragStarted(int sourceId, juce::Point<float> startPos);
	void handleCableDragging(juce::Point<float> currentPos);
	void handleCableDragEnded(juce::Point<float> releasePos);

	void showContextMenu(juce::Point<int> screenPos, juce::Point<int> canvasPos);
	void showNodeContextMenu(int modId, juce::Point<int> screenPos);
	void showInputPinContextMenu(int modId, juce::Point<int> screenPos);

	std::optional<std::pair<int, int>> findCableAt(juce::Point<float> pos) const;
	void drawCable(juce::Graphics& g, juce::Point<float> p0, juce::Point<float> p3);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphCanvas)
};

} // namespace sunvox
