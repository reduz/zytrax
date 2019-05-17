#ifndef AudioFrame_H
#define AudioFrame_H

#include "typedefs.h"

struct AudioFrame {

	float l, r;

	_FORCE_INLINE_ const float &operator[](int idx) const { return idx == 0 ? l : r; }
	_FORCE_INLINE_ float &operator[](int idx) { return idx == 0 ? l : r; }
	_FORCE_INLINE_ AudioFrame operator+(const AudioFrame &p_frame) const { return AudioFrame(l + p_frame.l, r + p_frame.r); }
	_FORCE_INLINE_ AudioFrame operator-(const AudioFrame &p_frame) const { return AudioFrame(l - p_frame.l, r - p_frame.r); }
	_FORCE_INLINE_ AudioFrame operator*(const AudioFrame &p_frame) const { return AudioFrame(l * p_frame.l, r * p_frame.r); }
	_FORCE_INLINE_ AudioFrame operator/(const AudioFrame &p_frame) const { return AudioFrame(l / p_frame.l, r / p_frame.r); }

	_FORCE_INLINE_ void operator+=(const AudioFrame &p_frame) {
		l += p_frame.l;
		r += p_frame.r;
	}
	_FORCE_INLINE_ void operator-=(const AudioFrame &p_frame) {
		l -= p_frame.l;
		r -= p_frame.r;
	}
	_FORCE_INLINE_ void operator*=(const AudioFrame &p_frame) {
		l *= p_frame.l;
		r *= p_frame.r;
	}
	_FORCE_INLINE_ void operator/=(const AudioFrame &p_frame) {
		l /= p_frame.l;
		r /= p_frame.r;
	}

	_FORCE_INLINE_ AudioFrame operator+(float p_frame) const { return AudioFrame(l + p_frame, r + p_frame); }
	_FORCE_INLINE_ AudioFrame operator-(float p_frame) const { return AudioFrame(l - p_frame, r - p_frame); }
	_FORCE_INLINE_ AudioFrame operator*(float p_frame) const { return AudioFrame(l * p_frame, r * p_frame); }
	_FORCE_INLINE_ AudioFrame operator/(float p_frame) const { return AudioFrame(l / p_frame, r / p_frame); }

	_FORCE_INLINE_ void operator+=(float p_frame) {
		l += p_frame;
		r += p_frame;
	}
	_FORCE_INLINE_ void operator-=(float p_frame) {
		l -= p_frame;
		r -= p_frame;
	}
	_FORCE_INLINE_ void operator*=(float p_frame) {
		l *= p_frame;
		r *= p_frame;
	}
	_FORCE_INLINE_ void operator/=(float p_frame) {
		l /= p_frame;
		r /= p_frame;
	}

	_FORCE_INLINE_ AudioFrame(float p_l, float p_r) {
		l = p_l;
		r = p_r;
	}
	_FORCE_INLINE_ AudioFrame(const AudioFrame &p_frame) {
		l = p_frame.l;
		r = p_frame.r;
	}
	_FORCE_INLINE_ AudioFrame() {}
};

#endif // AudioFrame_H
