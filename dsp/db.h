#ifndef DB_H
#define DB_H
#include "typedefs.h"
#include <math.h>

static _FORCE_INLINE_ float linear2db(float p_linear) {
	return log(p_linear) * 8.6858896380650365530225783783321;
}

static _FORCE_INLINE_ float db2linear(float p_db) {
	return exp(p_db * 0.11512925464970228420089957273422);
}

#endif // DB_H
