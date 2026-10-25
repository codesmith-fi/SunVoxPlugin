#include "GraphCanvas.hpp"
#include <cmath>

namespace sunvox
{

GraphCanvas::GraphCanvas(SunVoxEngine* engine)
	: engine_(engine)
{
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
	removeAllChildren();

	// Default layout coordinates: Generator x=150, y=180; Output x=420, y=180
	int autoXGenerator = 150;
	int autoYGenerator = 180;
	int autoXOutput = 420;
	int autoYOutput = 180;

	for (const auto& info : modules)
	{
		auto node = std::make_unique<ModuleNodeComponent>(info);
		node->onMoved = [this] { repaint(); };

		// Position resolution
		juce::Point<int> pos;
		if (auto it = existingPositions.find(info.id); it != existingPositions.end())
		{
			pos = it->second;
		}
		else if (info.x != 0 || info.y != 0)
		{
			// Map SunVox coordinates if available
			pos = { std::clamp(info.x, 20, 700), std::clamp(info.y, 20, 400) };
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
		addAndMakeVisible(node.get());

		nodeMap_[info.id] = node.get();
		nodes_.push_back(std::move(node));
	}

	repaint();
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

	// Subtle grid
	g.setColour(juce::Colour(0xff1d222b));
	const int gridSize = 24;
	for (int x = static_cast<int>(bounds.getX()); x < static_cast<int>(bounds.getRight()); x += gridSize)
	{
		g.drawVerticalLine(x, bounds.getY(), bounds.getBottom());
	}
	for (int y = static_cast<int>(bounds.getY()); y < static_cast<int>(bounds.getBottom()); y += gridSize)
	{
		g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
	}
}

void GraphCanvas::paintOverChildren(juce::Graphics& g)
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
}

void GraphCanvas::resized()
{
}

} // namespace sunvox
