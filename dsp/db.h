#ifndef DB_H
#define DB_H
#include "typedefs.h"
#include <math.h>

static inline float undenormalize(float value) {
	union {
		float f;
		uint32_t u;
	} data = { value };

	// Check if the value is subnormal (exponent bits are 0 but mantissa is non-zero)
	if ((data.u & 0x7F800000) == 0 && (data.u & 0x007FFFFF) != 0) {
		return 0.0f; // Set to zero
	}

	return value; // Return unchanged if not subnormal
}


static _FORCE_INLINE_ float linear2db(float p_linear) {
	return log(p_linear) * 8.6858896380650365530225783783321;
}

static _FORCE_INLINE_ float db2linear(float p_db) {
	return exp(p_db * 0.11512925464970228420089957273422);
}

#endif // DB_H
