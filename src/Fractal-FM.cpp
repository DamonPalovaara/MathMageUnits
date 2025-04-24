#include "plugin.hpp"

const double TWO_PI = 2.0 * 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

struct Fractal_FM : Module {
	double phase1[16] = {};
	double phase2[16] = {};
	double phase3[16] = {};
	double phase4[16] = {};
	double sample_time = 0.0;
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
			double pitch = (double) params[PITCH_PARAM].getValue();
			pitch += (double) inputs[NOTE_INPUT].getPolyVoltage(c);
			double freq = (double) dsp::FREQ_C4 * std::pow(2.0, pitch);

			double m = (double) params[MULTIPLIER_PARAM].getValue();
			if (inputs[MULTIPLIER_INPUT].isConnected()) {
				double m_input = (double) inputs[MULTIPLIER_INPUT].getPolyVoltage(c);
				double m_m = (m_input > 0.0) ? m_input / 10.0 : 0.0;
				m *= m_m;
			}
			
			double phase_offset = freq * sample_time;

			phase1[c] += phase_offset;
			phase2[c] += phase_offset;
			phase3[c] += phase_offset;
			phase4[c] += phase_offset;

			double m_2 = std::pow(m, 2.0);
			double m_3 = std::pow(m, 3.0);
			double phase2_cycle = 1.0 / m;
			double phase3_cycle = 1.0 / m_2;
			double phase4_cycle = 1.0 / m_3;
			
			if (phase1[c] >= 1.0) 
				phase1[c] -= 1.0;
			if (phase2[c] >= phase2_cycle) 
				phase2[c] -= phase2_cycle;
			if (phase3[c] >= phase3_cycle) 
				phase3[c] -= phase3_cycle;
			if (phase4[c] >= phase4_cycle) 
				phase4[c] -= phase4_cycle;

			double a = (double) params[DEPTH_PARAM].getValue();
			if (inputs[DEPTH_INPUT].isConnected()) {
				double a_input = (double) inputs[DEPTH_INPUT].getPolyVoltage(c);
				double a_m = (a_input > 0.0) ? a_input / 10.0 : 0.0;
				a *= a_m;
			}
			
			double t = (double) params[DELAY_PARAM].getValue();
			t += (double) inputs[DELAY_INPUT].getPolyVoltage(c);
			
			double x1 = TWO_PI * phase1[c];
			double x2 = TWO_PI * phase2[c];
			double x3 = TWO_PI * phase3[c];
			double x4 = TWO_PI * phase4[c];	

			double y1 = a * std::sin(m_3 * (x4 - 3.0 * t));
			double y2 = a * std::sin(m_2 * (x3 - 2.0 * t) + y1);
			double y3 = a * std::sin(m * (x2 - t) + y2);
			double y4 = std::sin(x1 + y3);

			outputs[OUTPUT_OUTPUT].setVoltage((float)(5.0 * y4), c);		
		}

		outputs[OUTPUT_OUTPUT].setChannels(channels);
	}

	void onAdd() override {
		sample_time = 1.0 / APP->engine->getSampleRate();
	}

	void onSampleRateChange() override {
		sample_time = 1.0 / APP->engine->getSampleRate();
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
};


Model* modelFractal_FM = createModel<Fractal_FM, Fractal_FMWidget>("Fractal-FM");
