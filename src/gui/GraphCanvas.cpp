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
		node->setSelected(info.id == selectedModuleId_);
		node->onSelected = [this](int modId) {
			selectModule(modId);
		};

		node->onMoved = [this, n = node.get()] {
			if (engine_ != nullptr)
				engine_->setModuleXY(n->getModuleId(), n->getX(), n->getY());
			repaint();
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
		addAndMakeVisible(node.get());

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
	if (e.mods.isLeftButtonDown())
	{
		selectModule(-1);
	}
}

void GraphCanvas::mouseUp(const juce::MouseEvent& e)
{
	if (e.mods.isPopupMenu())
	{
		showContextMenu(e.getScreenPosition(), e.getPosition());
	}
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
