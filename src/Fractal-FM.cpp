#include "plugin.hpp"
const double TWO_PI = 2.0 * 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

struct Fractal_FM : Module {
	double phase1[16] = {};
	double phase2[16] = {};
	double phase3[16] = {};
	double phase4[16] = {};
	double sample_time = 0.0;
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
			double output = 0.0;
			for (int s = 0; s < num_samples; s++) { 
				double pitch = (double) params[PITCH_PARAM].getValue();
				pitch += (double) inputs[NOTE_INPUT].getPolyVoltage(c);

				double m = (double) params[MULTIPLIER_PARAM].getValue();
				if (inputs[MULTIPLIER_INPUT].isConnected()) {
					double m_input = (double) inputs[MULTIPLIER_INPUT].getPolyVoltage(c);
					double m_m = (m_input > 0.0) ? m_input / 10.0 : 0.0;
					m *= m_m;
				}
				double m_2 = m * m;
				double m_3 = m_2 * m;
				double m_0_freq = (double) dsp::FREQ_C4 * std::pow(2.0, pitch);
				double m_1_freq = m * m_0_freq;
				double m_2_freq = m * m_1_freq;
				double m_3_freq = m * m_2_freq;
				double delta_time = sample_time / (double) num_samples;
				
				phase1[c] += m_0_freq * delta_time;
				phase2[c] += m_1_freq * delta_time;
				phase3[c] += m_2_freq * delta_time;
				phase4[c] += m_3_freq * delta_time;
					
				if (phase1[c] >= 1.0) phase1[c] -= 1.0;
				if (phase2[c] >= 1.0) phase1[c] -= 1.0;
				if (phase3[c] >= 1.0) phase1[c] -= 1.0;
				if (phase4[c] >= 1.0) phase1[c] -= 1.0;

				double a = (double) params[DEPTH_PARAM].getValue();
				if (inputs[DEPTH_INPUT].isConnected()) {
					double a_input = (double) inputs[DEPTH_INPUT].getPolyVoltage(c);
					double a_m = (a_input > 0.0) ? a_input / 10.0 : 0.0;
					a *= a_m;
				}
				
				double t = (double) params[DELAY_PARAM].getValue();
				t += (double) inputs[DELAY_INPUT].getPolyVoltage(c);
				
				double x1 = TWO_PI * phase1[c];
				double x2 = TWO_PI * (phase2[c] - t) * m;
				double x3 = TWO_PI * (phase3[c] - 2.0 * t) * m_2;
				double x4 = TWO_PI * (phase4[c] - 3.0 * t) * m_3;	

				// double y1 = a * std::sin((x4 - 3.0 * t));
				double y2 = a * std::sin((x3 - 2.0 * t));//y1);
				double y3 = a * std::sin((x2 - t) + y2);
				double y4 = std::sin(x1 + y3);

				output += y4;
			}
			output *= 5.0 / (double) num_samples;
			outputs[OUTPUT_OUTPUT].setVoltage((float)output, c);
			// outputs[OUTPUT_OUTPUT].setVoltage(output, c);
		}
		outputs[OUTPUT_OUTPUT].setChannels(channels);
	}

	void onAdd() override {
		sample_time = 1.0 / APP->engine->getSampleRate();
	}

	void onSampleRateChange() override {
		sample_time = 1.0 / APP->engine->getSampleRate();
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
