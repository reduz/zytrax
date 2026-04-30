#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include <math.h>
#include "globals/typedefs.h"

enum OscillatorType {
	OSC_TYPE_SINE,
	OSC_TYPE_TRIANGLE,
	OSC_TYPE_SQUARE,
	OSC_TYPE_SAW,
	OSC_TYPE_MAX,
};

template<OscillatorType osc_type>
_FORCE_INLINE_ static float oscillator_eval(float p_value,float p_shape) {
	switch(osc_type) {
		case OSC_TYPE_SINE: {
			// Shape turns into notch.
			float x = sinf(p_value * M_PI * 2.0);
			float s = x < 0.0f ? -1.0f : 1.0f;
			x*=s;

			float ex = 1.0 + p_shape * 10.0;
			ex = ex * ex;
			return powf(x,ex) * s;
		} break;
		case OSC_TYPE_TRIANGLE: {
			// Shape turns into saw.		
			float pivot_a = (1.0 - p_shape) * 0.25;
			float pivot_b = 1.0 - pivot_a;
			if (p_value < pivot_a) {
				return p_value / pivot_a;
			} else if (p_value < pivot_b) {
				return 1.0 - (p_value - pivot_a) / (pivot_b - pivot_a) * 2.0;
			} else {
				return (1.0 - p_value) / -pivot_a;
			}
		} break;
		case OSC_TYPE_SQUARE: {
			// Shape changes proportion between left and right.
			float pivot = 0.5 - p_shape * 0.48;
			if (p_value < pivot) {
				return -1.0;
			} else {
				return 1.0;
			}
		} break;
		case OSC_TYPE_SAW: {
			// Shape turns into arc
			float ex = 1.0 + p_shape * 7;
			ex = ex * ex;
			float v = powf(p_value,ex);
			return v * 2.0 - 1.0;
		} break;
		default: {
			return 0;
		}
	}
}

#endif // OSCILLATOR_H
