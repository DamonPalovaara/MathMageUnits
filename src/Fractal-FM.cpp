#include "plugin.hpp"
const float TWO_PI = 2.f * M_PI;

struct Fractal_FM : Module {
	float x1[16] = {};
	float x2[16] = {};
	float x3[16] = {};
	float x4[16] = {};
	int num_samples = 1;
	enum ParamId {
		PITCH_PARAM,
		MULTIPLIER_PARAM,
		DEPTH_PARAM,
		DELAY_PARAM,
		NOTE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MULTIPLIER_INPUT,
		DEPTH_INPUT,
		DELAY_INPUT,
		NOTE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Fractal_FM() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MULTIPLIER_PARAM, 0.f, 10.f, 1.f, "");
		configParam(DEPTH_PARAM, 0.f, 3.f, 0.f, "");
		configParam(DELAY_PARAM, 0.f, 2.f * M_PI, 0.f, "");
		configParam(NOTE_PARAM, 0.f, 1.f, 0.f, "");
		configInput(MULTIPLIER_INPUT, "ratio between wave features");
		configInput(DEPTH_INPUT, "amount of fm");
		configInput(DELAY_INPUT, "phase offset for internal oscillators");
		configInput(NOTE_INPUT, "CV 1V/oct");
		configOutput(OUTPUT_OUTPUT, "");
	}
	
	void process(const ProcessArgs& args) override {
		int channels = std::max(1, inputs[NOTE_INPUT].getChannels());

		for (int c = 0; c < channels; c++) {
			float output = 0.0;
			for (int s = 0; s < num_samples; s++) {
				float pitch = params[PITCH_PARAM].getValue();
				pitch += inputs[NOTE_INPUT].getPolyVoltage(c);
				
				float m = params[MULTIPLIER_PARAM].getValue();
				if (inputs[MULTIPLIER_INPUT].isConnected()) {
					float m_input = inputs[MULTIPLIER_INPUT].getPolyVoltage(c);
					m *= (m_input > 0.f) ? m_input / 10.f : 0.f;
				}				
				
				float a = params[DEPTH_PARAM].getValue();
				if (inputs[DEPTH_INPUT].isConnected()) {
					float a_input = inputs[DEPTH_INPUT].getPolyVoltage(c);
					a *= (a_input > 0.f) ? a_input / 10.f : 0.f;
				}				
				
				float op1Freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
				float op2Freq = m * op1Freq;
				float op3Freq = m * op2Freq;
				float op4Freq = m * op3Freq;
				float delta_time = args.sampleTime / (float) num_samples;
				x1[c] += op1Freq * delta_time;
				x2[c] += op2Freq * delta_time;
				x3[c] += op3Freq * delta_time;
				x4[c] += op4Freq * delta_time;
				if (x1[c] >= 1.f) x1[c] -= 1.f;
				if (x2[c] >= 1.f) x2[c] -= 1.f;
				if (x3[c] >= 1.f) x3[c] -= 1.f;
				if (x4[c] >= 1.f) x4[c] -= 1.f;
				
				float t = params[DELAY_PARAM].getValue();
				t += inputs[DELAY_INPUT].getPolyVoltage(c);
				float y1 = std::sin(TWO_PI * x4[c]);
				float y2 = std::sin(TWO_PI * x3[c] + a * y1);
				float y3 = std::sin(TWO_PI * x2[c] + a * y2);
				float y4 = std::sin(TWO_PI * x1[c] + a * y3);

				output += y4;				
			}
			output *= 5.f / num_samples;
			outputs[OUTPUT_OUTPUT].setVoltage(output, c);
		}
		
		outputs[OUTPUT_OUTPUT].setChannels(channels);
	}

	void overSample(int samples) {
		num_samples = samples;
	}
};


struct Fractal_FMWidget : ModuleWidget {
	Fractal_FMWidget(Fractal_FM* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Fractal-FM.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 23.876)), module, Fractal_FM::PITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.625, 49.001)), module, Fractal_FM::MULTIPLIER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.625, 63.302)), module, Fractal_FM::DEPTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.625, 77.604)), module, Fractal_FM::DELAY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.625, 91.906)), module, Fractal_FM::NOTE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.855, 49.001)), module, Fractal_FM::MULTIPLIER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.855, 63.302)), module, Fractal_FM::DEPTH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.855, 77.604)), module, Fractal_FM::DELAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.855, 91.906)), module, Fractal_FM::NOTE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.24, 114.726)), module, Fractal_FM::OUTPUT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		Fractal_FM* module = getModule<Fractal_FM>();

		menu->addChild(new MenuSeparator);

		menu->addChild(createMenuLabel("Engine settings"));

		menu->addChild(createSubmenuItem("Oversampling", "",
			[=](Menu* menu) {
				menu->addChild(createMenuItem("1x", "", [=]() {module->overSample(1);}));
				menu->addChild(createMenuItem("2x", "", [=]() {module->overSample(2);}));
				menu->addChild(createMenuItem("4x", "", [=]() {module->overSample(4);}));
				menu->addChild(createMenuItem("8x", "", [=]() {module->overSample(8);}));
			}
		));
	}
};


Model* modelFractal_FM = createModel<Fractal_FM, Fractal_FMWidget>("Fractal-FM");
