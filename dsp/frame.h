#ifndef AudioFrame_H
#define AudioFrame_H

#include "typedefs.h"
typedef float AudioFrame;

struct Frame {

	float l,r;


	_FORCE_INLINE_ const AudioFrame& operator[](int idx) const { return idx==0?l:r; }
	_FORCE_INLINE_ AudioFrame& operator[](int idx) { return idx==0?l:r; }
	_FORCE_INLINE_ Frame operator+(const Frame& p_Frame) const { return Frame(l+p_Frame.l,r+p_Frame.r); }
	_FORCE_INLINE_ Frame operator-(const Frame& p_Frame) const { return Frame(l-p_Frame.l,r-p_Frame.r); }
	_FORCE_INLINE_ Frame operator*(const Frame& p_Frame) const { return Frame(l*p_Frame.l,r*p_Frame.r); }
	_FORCE_INLINE_ Frame operator/(const Frame& p_Frame) const { return Frame(l/p_Frame.l,r/p_Frame.r); }

	_FORCE_INLINE_ void operator+=(const Frame& p_Frame) { l+=p_Frame.l;r+=p_Frame.r; }
	_FORCE_INLINE_ void operator-=(const Frame& p_Frame) { l-=p_Frame.l;r-=p_Frame.r; }
	_FORCE_INLINE_ void operator*=(const Frame& p_Frame) { l*=p_Frame.l;r*=p_Frame.r; }
	_FORCE_INLINE_ void operator/=(const Frame& p_Frame) { l/=p_Frame.l;r/=p_Frame.r; }

	_FORCE_INLINE_ Frame operator+(const AudioFrame& p_AudioFrame) const { return Frame(l+p_AudioFrame,r+p_AudioFrame); }
	_FORCE_INLINE_ Frame operator-(const AudioFrame& p_AudioFrame) const { return Frame(l-p_AudioFrame,r-p_AudioFrame); }
	_FORCE_INLINE_ Frame operator*(const AudioFrame& p_AudioFrame) const { return Frame(l*p_AudioFrame,r*p_AudioFrame); }
	_FORCE_INLINE_ Frame operator/(const AudioFrame& p_AudioFrame) const { return Frame(l/p_AudioFrame,r/p_AudioFrame); }

	_FORCE_INLINE_ void operator+=(const AudioFrame& p_AudioFrame) { l+=p_AudioFrame; r+=p_AudioFrame; }
	_FORCE_INLINE_ void operator-=(const AudioFrame& p_AudioFrame) { l-=p_AudioFrame; r-=p_AudioFrame; }
	_FORCE_INLINE_ void operator*=(const AudioFrame& p_AudioFrame) { l*=p_AudioFrame; r*=p_AudioFrame; }
	_FORCE_INLINE_ void operator/=(const AudioFrame& p_AudioFrame) { l/=p_AudioFrame; r/=p_AudioFrame; }

	_FORCE_INLINE_ Frame(AudioFrame p_l, AudioFrame p_r) {l=p_l; r=p_r;}
	_FORCE_INLINE_ Frame(AudioFrame p_AudioFrame) {l=p_AudioFrame; r=p_AudioFrame;}
	_FORCE_INLINE_ Frame() {}

} _FORCE_ALIGN_;

typedef Frame StereoAudioFrame;

enum AudioFrameType {

	AUDIO_FRAME_MONO,
	AUDIO_FRAME_STEREO
};




#endif // AudioFrame_H
