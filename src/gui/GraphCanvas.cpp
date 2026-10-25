#include "GraphCanvas.hpp"
#include <cmath>

namespace sunvox
{

GraphCanvas::CanvasContentHolder::CanvasContentHolder(GraphCanvas& owner)
	: owner_(owner)
{
	setInterceptsMouseClicks(false, true);
}

void GraphCanvas::CanvasContentHolder::paintOverChildren(juce::Graphics& g)
{
	owner_.paintCanvasContent(g);
}

GraphCanvas::GraphCanvas(SunVoxEngine* engine)
	: engine_(engine)
{
	contentHolder_ = std::make_unique<CanvasContentHolder>(*this);
	contentHolder_->setBounds(0, 0, 10000, 10000);
	addAndMakeVisible(contentHolder_.get());
	updateTransform();
}

GraphCanvas::~GraphCanvas()
{
	contentHolder_.reset();
}

juce::Point<float> GraphCanvas::screenToWorld(juce::Point<float> screenPos) const noexcept
{
	return (screenPos - viewOffset_) / zoomFactor_;
}

juce::Point<float> GraphCanvas::worldToScreen(juce::Point<float> worldPos) const noexcept
{
	return (worldPos * zoomFactor_) + viewOffset_;
}

juce::AffineTransform GraphCanvas::getCanvasTransform() const noexcept
{
	return juce::AffineTransform::scale(zoomFactor_).translated(viewOffset_.x, viewOffset_.y);
}

void GraphCanvas::updateTransform()
{
	if (contentHolder_ != nullptr)
	{
		contentHolder_->setTransform(getCanvasTransform());
	}
}

void GraphCanvas::refreshFromEngine()
{
	if (engine_ != nullptr)
		updateFromEngine(engine_->getActiveModules());
}

void GraphCanvas::refreshFromEngine(SunVoxEngine& engine)
{
	engine_ = &engine;
	updateFromEngine(engine.getActiveModules());
}

void GraphCanvas::updateFromEngine(const std::vector<ModuleInfo>& modules)
{
	// Keep track of positions of existing nodes
	std::unordered_map<int, juce::Point<int>> existingPositions;
	for (const auto& node : nodes_)
	{
		existingPositions[node->getModuleId()] = node->getPosition();
	}

	nodes_.clear();
	nodeMap_.clear();
	if (contentHolder_ != nullptr)
		contentHolder_->removeAllChildren();

	// Default layout coordinates: Generator x=180, y=220; Output x=560, y=220
	int autoXGenerator = 180;
	int autoYGenerator = 220;
	int autoXOutput = 560;
	int autoYOutput = 220;

	for (const auto& info : modules)
	{
		auto node = std::make_unique<ModuleNodeComponent>(info);
		node->setSelected(info.id == selectedModuleId_);
		node->onSelected = [this](int modId) {
			selectModule(modId);
		};

		node->onMoved = [this, n = node.get()] {
			if (engine_ != nullptr)
				engine_->setModuleXY(n->getModuleId(), n->getX(), n->getY());
			repaint();
		};

		node->onCableDragStarted = [this](int srcId, juce::Point<float> pinPos) {
			handleCableDragStarted(srcId, pinPos);
		};

		node->onCableDragging = [this](juce::Point<float> curPos) {
			handleCableDragging(curPos);
		};

		node->onCableDragEnded = [this](juce::Point<float> relPos) {
			handleCableDragEnded(relPos);
		};

		node->onNodeRightClicked = [this](int modId, juce::Point<int> screenPos) {
			showNodeContextMenu(modId, screenPos);
		};

		node->onInputPinRightClicked = [this](int modId, juce::Point<int> screenPos) {
			showInputPinContextMenu(modId, screenPos);
		};

		// Position resolution
		juce::Point<int> pos;
		if (auto it = existingPositions.find(info.id); it != existingPositions.end())
		{
			pos = it->second;
		}
		else if (info.x != 0 || info.y != 0)
		{
			pos = { std::max(20, info.x), std::max(20, info.y) };
		}
		else
		{
			// Automatic initial layout
			if (info.type == "Output" || info.id == 0)
			{
				pos = { autoXOutput, autoYOutput };
				autoYOutput += 100;
			}
			else
			{
				pos = { autoXGenerator, autoYGenerator };
				autoYGenerator += 100;
			}
		}

		node->setTopLeftPosition(pos.x, pos.y);
		if (contentHolder_ != nullptr)
			contentHolder_->addAndMakeVisible(node.get());

		nodeMap_[info.id] = node.get();
		nodes_.push_back(std::move(node));
	}

	repaint();
}

void GraphCanvas::selectModule(int modId)
{
	selectedModuleId_ = modId;
	for (auto& node : nodes_)
	{
		node->setSelected(node->getModuleId() == modId);
	}

	if (onModuleSelected)
		onModuleSelected(modId);
}

void GraphCanvas::mouseDown(const juce::MouseEvent& e)
{
	if (e.mods.isPopupMenu())
		return;

	isPanning_ = true;
	panAnchorPos_ = e.position;
	panMouseDownPos_ = e.position;
}

void GraphCanvas::mouseDrag(const juce::MouseEvent& e)
{
	if (isPanning_)
	{
		auto delta = e.position - panAnchorPos_;
		panAnchorPos_ = e.position;
		viewOffset_ += delta;
		updateTransform();
		repaint();
	}
}

void GraphCanvas::mouseUp(const juce::MouseEvent& e)
{
	if (isPanning_)
	{
		isPanning_ = false;
		if (panMouseDownPos_.getDistanceFrom(e.position) < 4.0f && e.mods.isLeftButtonDown())
		{
			selectModule(-1);
		}
	}

	if (e.mods.isPopupMenu())
	{
		auto hitCable = findCableAt(e.position.toFloat());
		if (hitCable.has_value())
		{
			int srcId = hitCable->first;
			int dstId = hitCable->second;

			std::string srcName = "Module #" + std::to_string(srcId);
			std::string dstName = "Module #" + std::to_string(dstId);

			if (auto srcIt = nodeMap_.find(srcId); srcIt != nodeMap_.end() && srcIt->second)
				srcName = srcIt->second->getModuleInfo().name;
			if (auto dstIt = nodeMap_.find(dstId); dstIt != nodeMap_.end() && dstIt->second)
				dstName = dstIt->second->getModuleInfo().name;

			juce::PopupMenu cableMenu;
			cableMenu.addSectionHeader("Cable: " + srcName + " -> " + dstName);
			cableMenu.addItem(1, "Disconnect Cable");

			cableMenu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(e.getScreenX(), e.getScreenY(), 1, 1)),
				[this, srcId, dstId](int result) {
					if (result == 1 && engine_ != nullptr)
					{
						engine_->disconnectModules(srcId, dstId);
						refreshFromEngine();
					}
				});
			return;
		}

		auto worldPos = screenToWorld(e.position.toFloat()).roundToInt();
		showContextMenu(e.getScreenPosition(), worldPos);
	}
}

void GraphCanvas::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
	float delta = wheel.deltaY;
	if (std::abs(delta) < 0.0001f)
		return;

	auto mousePos = e.position;
	auto worldPos = screenToWorld(mousePos);

	float zoomSpeed = 0.15f;
	float newZoom = juce::jlimit(0.35f, 2.0f, zoomFactor_ + delta * zoomSpeed);

	if (std::abs(newZoom - zoomFactor_) > 0.001f)
	{
		zoomFactor_ = newZoom;
		viewOffset_ = mousePos - (worldPos * zoomFactor_);
		updateTransform();
		repaint();
	}
}

void GraphCanvas::handleCableDragStarted(int sourceId, juce::Point<float> startPos)
{
	isDraggingCable_ = true;
	dragSourceNodeId_ = sourceId;
	dragSourcePinPos_ = startPos;
	dragCurrentPos_ = startPos;
	dragHoverTargetNodeId_ = -1;
	repaint();
}

void GraphCanvas::handleCableDragging(juce::Point<float> currentPos)
{
	dragCurrentPos_ = currentPos;
	int newHoverTarget = -1;

	for (const auto& node : nodes_)
	{
		if (node->getModuleId() == dragSourceNodeId_)
			continue;

		auto inputPinPos = node->getInputPinPosition();
		bool nearInputPin = (currentPos.getDistanceFrom(inputPinPos) <= 24.0f);
		bool insideNode = node->getBounds().toFloat().contains(currentPos);

		if (nearInputPin || insideNode)
		{
			newHoverTarget = node->getModuleId();
			break;
		}
	}

	if (dragHoverTargetNodeId_ != newHoverTarget)
	{
		if (auto it = nodeMap_.find(dragHoverTargetNodeId_); it != nodeMap_.end() && it->second)
			it->second->setInputPinHighlighted(false);

		dragHoverTargetNodeId_ = newHoverTarget;

		if (auto it = nodeMap_.find(dragHoverTargetNodeId_); it != nodeMap_.end() && it->second)
			it->second->setInputPinHighlighted(true);
	}

	repaint();
}

void GraphCanvas::handleCableDragEnded(juce::Point<float> releasePos)
{
	isDraggingCable_ = false;

	if (dragHoverTargetNodeId_ >= 0)
	{
		if (auto it = nodeMap_.find(dragHoverTargetNodeId_); it != nodeMap_.end() && it->second)
			it->second->setInputPinHighlighted(false);
	}

	int targetId = dragHoverTargetNodeId_;
	if (targetId < 0)
	{
		for (const auto& node : nodes_)
		{
			if (node->getModuleId() == dragSourceNodeId_)
				continue;

			auto inputPinPos = node->getInputPinPosition();
			if (releasePos.getDistanceFrom(inputPinPos) <= 24.0f || node->getBounds().toFloat().contains(releasePos))
			{
				targetId = node->getModuleId();
				break;
			}
		}
	}

	int sourceId = dragSourceNodeId_;
	dragHoverTargetNodeId_ = -1;
	dragSourceNodeId_ = -1;

	if (targetId >= 0 && targetId != sourceId && engine_ != nullptr)
	{
		engine_->connectModules(sourceId, targetId);
		refreshFromEngine();
	}

	repaint();
}

std::optional<std::pair<int, int>> GraphCanvas::findCableAt(juce::Point<float> screenPos) const
{
	auto worldPos = screenToWorld(screenPos);
	float strokeTolerance = 14.0f / zoomFactor_;

	for (const auto& node : nodes_)
	{
		int srcId = node->getModuleId();
		for (int dstId : node->getModuleInfo().outputConnections)
		{
			auto it = nodeMap_.find(dstId);
			if (it != nodeMap_.end() && it->second != nullptr)
			{
				auto p0 = node->getOutputPinPosition();
				auto p3 = it->second->getInputPinPosition();

				float dx = std::max(50.0f, std::abs(p3.x - p0.x) * 0.5f);
				float sag = std::clamp(std::abs(p3.x - p0.x) * 0.08f, 0.0f, 30.0f);
				juce::Point<float> p1(p0.x + dx, p0.y + sag);
				juce::Point<float> p2(p3.x - dx, p3.y + sag);

				juce::Path cable;
				cable.startNewSubPath(p0);
				cable.cubicTo(p1, p2, p3);

				juce::Path stroked;
				juce::PathStrokeType(strokeTolerance).createStrokedPath(stroked, cable);
				if (stroked.contains(worldPos))
				{
					return std::make_pair(srcId, dstId);
				}
			}
		}
	}
	return std::nullopt;
}

void GraphCanvas::showNodeContextMenu(int modId, juce::Point<int> screenPos)
{
	if (engine_ == nullptr)
		return;

	auto it = nodeMap_.find(modId);
	if (it == nodeMap_.end() || it->second == nullptr)
		return;

	const auto& info = it->second->getModuleInfo();

	juce::PopupMenu menu;
	menu.addSectionHeader(juce::String(info.name) + " [#" + juce::String(info.id) + "]");

	menu.addItem(1, "Disconnect All Inputs");
	menu.addItem(2, "Disconnect All Outputs");

	if (!info.outputConnections.empty())
	{
		juce::PopupMenu outSubMenu;
		int subId = 100;
		for (int destId : info.outputConnections)
		{
			std::string destName = "Module #" + std::to_string(destId);
			if (auto destIt = nodeMap_.find(destId); destIt != nodeMap_.end() && destIt->second)
				destName = destIt->second->getModuleInfo().name;

			outSubMenu.addItem(subId++, "Disconnect to " + destName);
		}
		menu.addSubMenu("Disconnect Output...", outSubMenu);
	}

	if (modId != 0)
	{
		menu.addSeparator();
		menu.addItem(10, "Remove Module");
	}

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
		[this, modId, info](int result) {
			if (result == 0 || engine_ == nullptr)
				return;

			if (result == 1)
			{
				engine_->disconnectAllInputs(modId);
				refreshFromEngine();
			}
			else if (result == 2)
			{
				engine_->disconnectAllOutputs(modId);
				refreshFromEngine();
			}
			else if (result == 10)
			{
				engine_->disconnectAllInputs(modId);
				engine_->disconnectAllOutputs(modId);
				engine_->removeModule(modId);
				if (selectedModuleId_ == modId)
					selectModule(-1);
				refreshFromEngine();
			}
			else if (result >= 100)
			{
				size_t idx = static_cast<size_t>(result - 100);
				if (idx < info.outputConnections.size())
				{
					int destId = info.outputConnections[idx];
					engine_->disconnectModules(modId, destId);
					refreshFromEngine();
				}
			}
		});
}

void GraphCanvas::showInputPinContextMenu(int modId, juce::Point<int> screenPos)
{
	if (engine_ == nullptr)
		return;

	juce::PopupMenu menu;
	menu.addSectionHeader("Input Connections");
	menu.addItem(1, "Disconnect All Inputs");

	std::vector<int> incomingSources;
	for (const auto& node : nodes_)
	{
		for (int destId : node->getModuleInfo().outputConnections)
		{
			if (destId == modId)
				incomingSources.push_back(node->getModuleId());
		}
	}

	if (!incomingSources.empty())
	{
		juce::PopupMenu subMenu;
		int subId = 100;
		for (int srcId : incomingSources)
		{
			std::string srcName = "Module #" + std::to_string(srcId);
			if (auto srcIt = nodeMap_.find(srcId); srcIt != nodeMap_.end() && srcIt->second)
				srcName = srcIt->second->getModuleInfo().name;

			subMenu.addItem(subId++, "Disconnect from " + srcName);
		}
		menu.addSubMenu("Disconnect Source...", subMenu);
	}

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
		[this, modId, incomingSources](int result) {
			if (result == 0 || engine_ == nullptr)
				return;

			if (result == 1)
			{
				engine_->disconnectAllInputs(modId);
				refreshFromEngine();
			}
			else if (result >= 100)
			{
				size_t idx = static_cast<size_t>(result - 100);
				if (idx < incomingSources.size())
				{
					engine_->disconnectModules(incomingSources[idx], modId);
					refreshFromEngine();
				}
			}
		});
}

void GraphCanvas::showContextMenu(juce::Point<int> screenPos, juce::Point<int> canvasPos)
{
	enum MenuItemId
	{
		kSynthGenerator = 101,
		kSynthAnalogGenerator,
		kSynthFM,
		kSynthSpectraVoice,

		kEffectFilter = 201,
		kEffectDelay,
		kEffectReverb,
		kEffectDistortion,
		kEffectFlanger,
		kEffectVocalFilter,

		kToolAmplifier = 301,
		kToolFeedback,
		kToolKicker,
		kToolCompressor
	};

	juce::PopupMenu menu;

	juce::PopupMenu synthMenu;
	synthMenu.addItem(kSynthGenerator, "Generator");
	synthMenu.addItem(kSynthAnalogGenerator, "Analog Generator");
	synthMenu.addItem(kSynthFM, "FM");
	synthMenu.addItem(kSynthSpectraVoice, "SpectraVoice");
	menu.addSubMenu("Synths", synthMenu);

	juce::PopupMenu fxMenu;
	fxMenu.addItem(kEffectFilter, "Filter");
	fxMenu.addItem(kEffectDelay, "Delay");
	fxMenu.addItem(kEffectReverb, "Reverb");
	fxMenu.addItem(kEffectDistortion, "Distortion");
	fxMenu.addItem(kEffectFlanger, "Flanger");
	fxMenu.addItem(kEffectVocalFilter, "Vocal Filter");
	menu.addSubMenu("Effects", fxMenu);

	juce::PopupMenu toolMenu;
	toolMenu.addItem(kToolAmplifier, "Amplifier");
	toolMenu.addItem(kToolFeedback, "Feedback");
	toolMenu.addItem(kToolKicker, "Kicker");
	toolMenu.addItem(kToolCompressor, "Compressor");
	menu.addSubMenu("Tools", toolMenu);

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(juce::Rectangle<int>(screenPos.x, screenPos.y, 1, 1)),
		[this, canvasPos](int result) {
			if (result == 0 || engine_ == nullptr)
				return;

			std::string type;
			std::string name;

			switch (result)
			{
			case kSynthGenerator: type = "Generator"; name = "Generator"; break;
			case kSynthAnalogGenerator: type = "Analog Generator"; name = "Analog Gen"; break;
			case kSynthFM: type = "FM"; name = "FM Synth"; break;
			case kSynthSpectraVoice: type = "SpectraVoice"; name = "SpectraVoice"; break;

			case kEffectFilter: type = "Filter"; name = "Filter"; break;
			case kEffectDelay: type = "Delay"; name = "Delay"; break;
			case kEffectReverb: type = "Reverb"; name = "Reverb"; break;
			case kEffectDistortion: type = "Distortion"; name = "Distortion"; break;
			case kEffectFlanger: type = "Flanger"; name = "Flanger"; break;
			case kEffectVocalFilter: type = "Vocal Filter"; name = "Vocal Filter"; break;

			case kToolAmplifier: type = "Amplifier"; name = "Amplifier"; break;
			case kToolFeedback: type = "Feedback"; name = "Feedback"; break;
			case kToolKicker: type = "Kicker"; name = "Kicker"; break;
			case kToolCompressor: type = "Compressor"; name = "Compressor"; break;
			default: return;
			}

			int newId = engine_->createModule(type, name, canvasPos.x, canvasPos.y);
			if (newId >= 0)
			{
				refreshFromEngine();
				selectModule(newId);
			}
		});
}

void GraphCanvas::drawCable(juce::Graphics& g, juce::Point<float> p0, juce::Point<float> p3)
{
	float dx = std::max(50.0f, std::abs(p3.x - p0.x) * 0.5f);
	float sag = std::clamp(std::abs(p3.x - p0.x) * 0.08f, 0.0f, 30.0f);

	juce::Point<float> p1(p0.x + dx, p0.y + sag);
	juce::Point<float> p2(p3.x - dx, p3.y + sag);

	juce::Path cable;
	cable.startNewSubPath(p0);
	cable.cubicTo(p1, p2, p3);

	// Outer glow shadow
	g.setColour(juce::Colour(0x3300e5ff));
	g.strokePath(cable, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	// Cable core line
	g.setColour(juce::Colour(0xff00e5ff));
	g.strokePath(cable, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

	// Connection pin dots
	g.setColour(juce::Colour(0xff00e5ff));
	g.fillEllipse(p0.x - 3.0f, p0.y - 3.0f, 6.0f, 6.0f);
	g.fillEllipse(p3.x - 3.0f, p3.y - 3.0f, 6.0f, 6.0f);
}

void GraphCanvas::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();
	const float cornerSize = 8.0f;

	// Canvas background
	g.setColour(juce::Colour(0xff14171d));
	g.fillRoundedRectangle(bounds, cornerSize);

	// Border
	g.setColour(juce::Colour(0xff2b323d));
	g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

	// Dynamic grid that scrolls and scales
	g.saveState();
	g.reduceClipRegion(bounds.toNearestInt());

	g.setColour(juce::Colour(0xff1d222b));
	float baseGridSize = 24.0f;
	float scaledGrid = baseGridSize * zoomFactor_;

	// If grid becomes too dense when zoomed out, double spacing
	while (scaledGrid < 12.0f)
		scaledGrid *= 2.0f;

	float startX = std::fmod(viewOffset_.x, scaledGrid);
	if (startX < 0.0f) startX += scaledGrid;

	float startY = std::fmod(viewOffset_.y, scaledGrid);
	if (startY < 0.0f) startY += scaledGrid;

	for (float x = bounds.getX() + startX; x < bounds.getRight(); x += scaledGrid)
	{
		g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
	}
	for (float y = bounds.getY() + startY; y < bounds.getBottom(); y += scaledGrid)
	{
		g.drawHorizontalLine(juce::roundToInt(y), bounds.getX(), bounds.getRight());
	}

	g.restoreState();

	// Zoom badge in corner
	if (std::abs(zoomFactor_ - 1.0f) > 0.01f)
	{
		g.setColour(juce::Colour(0x881e242f));
		g.fillRoundedRectangle(bounds.getRight() - 56.0f, bounds.getBottom() - 28.0f, 48.0f, 20.0f, 4.0f);
		g.setColour(juce::Colour(0xff00e5ff));
		g.setFont(juce::FontOptions(11.0f));
		juce::String zoomStr = juce::String(juce::roundToInt(zoomFactor_ * 100.0f)) + "%";
		g.drawText(zoomStr, juce::Rectangle<float>(bounds.getRight() - 56.0f, bounds.getBottom() - 28.0f, 48.0f, 20.0f), juce::Justification::centred, false);
	}
}

void GraphCanvas::paintOverChildren(juce::Graphics& g)
{
	juce::ignoreUnused(g);
}

void GraphCanvas::paintCanvasContent(juce::Graphics& g)
{
	// Draw patch cables on top of nodes between connection pins
	for (const auto& node : nodes_)
	{
		const auto& connections = node->getModuleInfo().outputConnections;
		for (int targetId : connections)
		{
			auto it = nodeMap_.find(targetId);
			if (it != nodeMap_.end() && it->second != nullptr)
			{
				auto p0 = node->getOutputPinPosition();
				auto p3 = it->second->getInputPinPosition();
				drawCable(g, p0, p3);
			}
		}
	}

	// Draw dragging cable if in progress
	if (isDraggingCable_)
	{
		juce::Point<float> p0 = dragSourcePinPos_;
		juce::Point<float> p3 = dragCurrentPos_;

		if (dragHoverTargetNodeId_ >= 0)
		{
			if (auto it = nodeMap_.find(dragHoverTargetNodeId_); it != nodeMap_.end() && it->second != nullptr)
			{
				p3 = it->second->getInputPinPosition();
			}
		}

		float dx = std::max(40.0f, std::abs(p3.x - p0.x) * 0.5f);
		float sag = std::clamp(std::abs(p3.x - p0.x) * 0.08f, 0.0f, 30.0f);
		juce::Point<float> p1(p0.x + dx, p0.y + sag);
		juce::Point<float> p2(p3.x - dx, p3.y + sag);

		juce::Path dragCable;
		dragCable.startNewSubPath(p0);
		dragCable.cubicTo(p1, p2, p3);

		// Outer glow
		g.setColour(juce::Colour(0x4400ffcc));
		g.strokePath(dragCable, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		// Glowing temporary cable line
		g.setColour(juce::Colour(0xff00ffcc));
		g.strokePath(dragCable, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

		// Pin end circles
		g.fillEllipse(p0.x - 3.5f, p0.y - 3.5f, 7.0f, 7.0f);
		g.fillEllipse(p3.x - 3.5f, p3.y - 3.5f, 7.0f, 7.0f);
	}
}

void GraphCanvas::resized()
{
	if (contentHolder_ != nullptr)
	{
		contentHolder_->setBounds(0, 0, 10000, 10000);
		updateTransform();
	}
}

} // namespace sunvox
