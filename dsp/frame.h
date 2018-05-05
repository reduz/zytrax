#ifndef AudioFrame_H
#define AudioFrame_H

#include "typedefs.h"
typedef float AudioFrame;

struct AudioFrame2 {

	AudioFrame l,r;


	_FORCE_INLINE_ const AudioFrame& operator[](int idx) const { return idx==0?l:r; }
	_FORCE_INLINE_ AudioFrame& operator[](int idx) { return idx==0?l:r; }
	_FORCE_INLINE_ AudioFrame2 operator+(const AudioFrame2& p_AudioFrame2) const { return AudioFrame2(l+p_AudioFrame2.l,r+p_AudioFrame2.r); }
	_FORCE_INLINE_ AudioFrame2 operator-(const AudioFrame2& p_AudioFrame2) const { return AudioFrame2(l-p_AudioFrame2.l,r-p_AudioFrame2.r); }
	_FORCE_INLINE_ AudioFrame2 operator*(const AudioFrame2& p_AudioFrame2) const { return AudioFrame2(l*p_AudioFrame2.l,r*p_AudioFrame2.r); }
	_FORCE_INLINE_ AudioFrame2 operator/(const AudioFrame2& p_AudioFrame2) const { return AudioFrame2(l/p_AudioFrame2.l,r/p_AudioFrame2.r); }

	_FORCE_INLINE_ void operator+=(const AudioFrame2& p_AudioFrame2) { l+=p_AudioFrame2.l;r+=p_AudioFrame2.r; }
	_FORCE_INLINE_ void operator-=(const AudioFrame2& p_AudioFrame2) { l-=p_AudioFrame2.l;r-=p_AudioFrame2.r; }
	_FORCE_INLINE_ void operator*=(const AudioFrame2& p_AudioFrame2) { l*=p_AudioFrame2.l;r*=p_AudioFrame2.r; }
	_FORCE_INLINE_ void operator/=(const AudioFrame2& p_AudioFrame2) { l/=p_AudioFrame2.l;r/=p_AudioFrame2.r; }

	_FORCE_INLINE_ AudioFrame2 operator+(const AudioFrame& p_AudioFrame) const { return AudioFrame2(l+p_AudioFrame,r+p_AudioFrame); }
	_FORCE_INLINE_ AudioFrame2 operator-(const AudioFrame& p_AudioFrame) const { return AudioFrame2(l-p_AudioFrame,r-p_AudioFrame); }
	_FORCE_INLINE_ AudioFrame2 operator*(const AudioFrame& p_AudioFrame) const { return AudioFrame2(l*p_AudioFrame,r*p_AudioFrame); }
	_FORCE_INLINE_ AudioFrame2 operator/(const AudioFrame& p_AudioFrame) const { return AudioFrame2(l/p_AudioFrame,r/p_AudioFrame); }

	_FORCE_INLINE_ void operator+=(const AudioFrame& p_AudioFrame) { l+=p_AudioFrame; r+=p_AudioFrame; }
	_FORCE_INLINE_ void operator-=(const AudioFrame& p_AudioFrame) { l-=p_AudioFrame; r-=p_AudioFrame; }
	_FORCE_INLINE_ void operator*=(const AudioFrame& p_AudioFrame) { l*=p_AudioFrame; r*=p_AudioFrame; }
	_FORCE_INLINE_ void operator/=(const AudioFrame& p_AudioFrame) { l/=p_AudioFrame; r/=p_AudioFrame; }

	_FORCE_INLINE_ AudioFrame2(AudioFrame p_l, AudioFrame p_r) {l=p_l; r=p_r;}
	_FORCE_INLINE_ AudioFrame2(AudioFrame p_AudioFrame) {l=p_AudioFrame; r=p_AudioFrame;}
	_FORCE_INLINE_ AudioFrame2() {}

} _FORCE_ALIGN_;

typedef AudioFrame2 StereoAudioFrame;

enum AudioFrameType {

	AUDIO_FRAME_MONO,
	AUDIO_FRAME_STEREO
};




#endif // AudioFrame_H
