#pragma once
#include <vector>
#include <jansson.h>
#include "widgets.hpp"


namespace rack {


struct Module;
struct Wire;

struct RackWidget;
struct ParamWidget;
struct Port;
struct Scene;

////////////////////
// module
////////////////////

// A 1U module should be 15x380. Thus the width of a module should be a factor of 15.
struct Model;
struct ModuleWidget : OpaqueWidget {
	Model *model = NULL;
	/** Owns the module pointer */
	Module *module = NULL;

	std::vector<Port*> inputs;
	std::vector<Port*> outputs;
	std::vector<ParamWidget*> params;

	~ModuleWidget();
	void setModule(Module *module);
	// Convenience functions for adding special widgets (calls addChild())
	void addInput(Port *input);
	void addOutput(Port *output);
	void addParam(ParamWidget *param);

	json_t *toJson();
	void fromJson(json_t *root);
	void disconnectPorts();
	void resetParams();
	void cloneParams(ModuleWidget *source);

	void draw(NVGcontext *vg);

	bool requested = false;
	Vec requestedPos;
	Vec dragPos;
	void onDragStart();
	void onDragMove(Vec mouseRel);
	void onDragEnd();
	void onMouseDown(int button);
};

struct WireWidget : OpaqueWidget {
	Port *inputPort = NULL;
	Port *outputPort = NULL;
	Port *hoveredInputPort = NULL;
	Port *hoveredOutputPort = NULL;
	Wire *wire = NULL;
	NVGcolor color;

	WireWidget();
	~WireWidget();
	void updateWire();
	void draw(NVGcontext *vg);
	void drawOutputPlug(NVGcontext *vg);
	void drawInputPlug(NVGcontext *vg);
};

struct RackWidget : OpaqueWidget {
	// Only put ModuleWidgets in here
	Widget *moduleContainer;
	// Only put WireWidgets in here
	Widget *wireContainer;
	WireWidget *activeWire = NULL;
	std::shared_ptr<Image> railsImage;

	RackWidget();
	~RackWidget();
	void clear();
	void savePatch(std::string filename);
	void loadPatch(std::string filename);
	json_t *toJson();
	void fromJson(json_t *root);

	void repositionModule(ModuleWidget *module);
	void step();
	void draw(NVGcontext *vg);

	void onMouseDown(int button);
};

struct Panel : TransparentWidget {
	NVGcolor backgroundColor;
	NVGcolor borderColor;
	std::shared_ptr<Image> backgroundImage;
	void draw(NVGcontext *vg);
};

////////////////////
// params
////////////////////

struct CircularShadow : TransparentWidget {
	float blur = 0.0;
	void draw(NVGcontext *vg);
};

struct Light : TransparentWidget {
	NVGcolor color;
	void draw(NVGcontext *vg);
};

struct ParamWidget : OpaqueWidget, QuantityWidget {
	Module *module = NULL;
	int paramId;

	json_t *toJson();
	void fromJson(json_t *root);
	void onMouseDown(int button);
	void onChange();
};

/** Implements vertical dragging behavior for ParamWidgets */
struct Knob : ParamWidget {
	void onDragStart();
	void onDragMove(Vec mouseRel);
	void onDragEnd();
};

struct SpriteKnob : Knob, SpriteWidget {
	int minIndex, maxIndex, spriteCount;
	void step();
};

/** A knob which rotates an SVG and caches it in a framebuffer */
struct SVGKnob : Knob, FramebufferWidget {
	/** Angles in radians */
	float minAngle, maxAngle;
	/** Not owned */
	TransformWidget *tw;
	SVGWidget *sw;
	CircularShadow *shadow;

	SVGKnob();
	void setSVG(std::shared_ptr<SVG> svg);
	void step();
	void onChange();
};

struct SVGSlider : Knob, FramebufferWidget {
	/** Intermediate positions will be interpolated between these positions */
	Vec minHandlePos, maxHandlePos;
	/** Not owned */
	SVGWidget *background;
	SVGWidget *handle;

	SVGSlider();
	void step();
	void onChange();
};

struct Switch : ParamWidget {
	virtual void setIndex(int index) {}
};

struct SVGSwitch : virtual Switch, FramebufferWidget {
	std::vector<std::shared_ptr<SVG>> frames;
	/** Not owned */
	SVGWidget *sw;

	SVGSwitch();
	/** Adds an SVG file to represent the next switch position */
	void addFrame(std::shared_ptr<SVG> svg);
	void setIndex(int index);
	void step();
};

/** A switch that cycles through each mechanical position */
struct ToggleSwitch : virtual Switch {
	void onDragStart() {
		// Cycle through values
		// e.g. a range of [0.0, 3.0] would have modes 0, 1, 2, and 3.
		float v = value + 1.0;
		setValue(v <= maxValue ? v : minValue);
	}
	void onChange() {
		int index = (int)roundf(value);
		setIndex(index);
		ParamWidget::onChange();
	}
};

/** FIXME I don't think this should exist. The audio engine should read from a MomentarySwitch and increment its internal state, instead of relying on the knob to do that logic. */
struct ModeSwitch : virtual Switch {
	void onDragStart() {
		setIndex(1);
	}
	void onDragEnd() {
		setIndex(0);
	}
	void onDragDrop(Widget *origin) {
		if (origin != this)
			return;
		// Cycle through values
		// e.g. a range of [0.0, 3.0] would have modes 0, 1, 2, and 3.
		float v = value + 1.0;
		setValue(v <= maxValue ? v : minValue);
	}
};

/** A switch that is turned on when held */
struct MomentarySwitch : virtual Switch {
	void onDragStart() {
		setValue(maxValue);
		setIndex(1);
	}
	void onDragEnd() {
		setValue(minValue);
		setIndex(0);
	}
};

////////////////////
// ports
////////////////////

struct Port : OpaqueWidget {
	enum PortType {
		DEFAULT,
		INPUT,
		OUTPUT
	};

	Module *module = NULL;
	WireWidget *connectedWire = NULL;
	PortType type = DEFAULT;
	int portId;

	Port();
	~Port();
	void disconnect();

	void draw(NVGcontext *vg);
	void onMouseDown(int button);
	void onDragEnd();
	void onDragStart();
	void onDragDrop(Widget *origin);
	void onDragEnter(Widget *origin);
	void onDragLeave(Widget *origin);
};

struct SVGPort : Port, FramebufferWidget {
	SVGWidget *background;

	SVGPort();
	void draw(NVGcontext *vg);
};

/** If you don't add these to your ModuleWidget, they will fall out of the rack... */
struct SVGScrew : FramebufferWidget {
	SVGWidget *sw;

	SVGScrew();
};

////////////////////
// scene
////////////////////

struct Toolbar : OpaqueWidget {
	Slider *wireOpacitySlider;
	Slider *wireTensionSlider;
	RadioButton *cpuUsageButton;

	Toolbar();
	void draw(NVGcontext *vg);
};

struct PluginManagerWidget : Widget {
	Widget *loginWidget;
	Widget *manageWidget;
	Widget *downloadWidget;
	PluginManagerWidget();
	void step();
};

struct RackScene : Scene {
	Toolbar *toolbar;
	ScrollWidget *scrollWidget;

	RackScene();
	void step();
	void draw(NVGcontext *vg);
};

////////////////////
// globals
////////////////////

extern std::string gApplicationName;
extern std::string gApplicationVersion;

extern Scene *gScene;
extern RackWidget *gRackWidget;

void sceneInit();
void sceneDestroy();

} // namespace rack
