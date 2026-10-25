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
	~GraphCanvas() override;

	void setEngine(SunVoxEngine* engine) noexcept { engine_ = engine; }
	void refreshFromEngine();
	void refreshFromEngine(SunVoxEngine& engine);
	void updateFromEngine(const std::vector<ModuleInfo>& modules);

	void selectModule(int modId);
	int getSelectedModuleId() const noexcept { return selectedModuleId_; }

	juce::Point<float> screenToWorld(juce::Point<float> screenPos) const noexcept;
	juce::Point<float> worldToScreen(juce::Point<float> worldPos) const noexcept;
	juce::AffineTransform getCanvasTransform() const noexcept;

	float getZoomFactor() const noexcept { return zoomFactor_; }
	juce::Point<float> getViewOffset() const noexcept { return viewOffset_; }

	void paint(juce::Graphics& g) override;
	void paintOverChildren(juce::Graphics& g) override;
	void resized() override;

	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

	void paintCanvasContent(juce::Graphics& g);

	std::function<void(int)> onModuleSelected;

private:
	class CanvasContentHolder : public juce::Component
	{
	public:
		explicit CanvasContentHolder(GraphCanvas& owner);
		~CanvasContentHolder() override = default;
		void paintOverChildren(juce::Graphics& g) override;
	private:
		GraphCanvas& owner_;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CanvasContentHolder)
	};

	SunVoxEngine* engine_{nullptr};
	int selectedModuleId_{-1};
	std::unique_ptr<CanvasContentHolder> contentHolder_;
	std::vector<std::unique_ptr<ModuleNodeComponent>> nodes_;
	std::unordered_map<int, ModuleNodeComponent*> nodeMap_;

	// Camera: Pan & Zoom
	juce::Point<float> viewOffset_{0.0f, 0.0f};
	float zoomFactor_{1.0f};
	bool isPanning_{false};
	juce::Point<float> panAnchorPos_;
	juce::Point<float> panMouseDownPos_;

	void updateTransform();

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

	std::optional<std::pair<int, int>> findCableAt(juce::Point<float> screenPos) const;
	void drawCable(juce::Graphics& g, juce::Point<float> p0, juce::Point<float> p3);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphCanvas)
};

} // namespace sunvox
