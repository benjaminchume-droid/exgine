#include "exgine/android_audio.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#if defined(__ANDROID__)
#include <aaudio/AAudio.h>
#endif
namespace exgine { namespace {
#if defined(__ANDROID__)
std::atomic<float> g_gain{0.0f};
std::atomic<float> g_pan{0.0f};
std::atomic<float> g_phase{0.0f};
aaudio_data_callback_result_t audio_callback(AAudioStream*,void*,void* audio_data,int32_t num_frames){auto*s=static_cast<float*>(audio_data);const float gain=g_gain.load(std::memory_order_relaxed),pan=std::clamp(g_pan.load(std::memory_order_relaxed),-1.0f,1.0f);float phase=g_phase.load(std::memory_order_relaxed);constexpr float sr=48000.0f,freq=196.0f,tp=6.2831853071795864769f;const float l=gain*(.5f-.25f*pan),r=gain*(.5f+.25f*pan);for(int32_t i=0;i<num_frames;++i){const float tone=std::sin(phase)*.035f,rain=std::sin(phase*7.31f)*.008f;s[i*2]=(tone+rain)*l;s[i*2+1]=(tone-rain)*r;phase+=tp*freq/sr;if(phase>=tp)phase-=tp;}g_phase.store(phase,std::memory_order_relaxed);return AAUDIO_CALLBACK_RESULT_CONTINUE;}
#endif
}
AndroidAudioBackend::~AndroidAudioBackend(){stop();}
bool AndroidAudioBackend::start(std::uint32_t sample_rate,std::uint32_t channels)noexcept{
#if defined(__ANDROID__)
if(running_)return true;if(!sample_rate||channels!=2)return false;AAudioStreamBuilder*b=nullptr;if(AAudio_createStreamBuilder(&b)!=AAUDIO_OK||!b)return false;AAudioStreamBuilder_setDirection(b,AAUDIO_DIRECTION_OUTPUT);AAudioStreamBuilder_setSampleRate(b,static_cast<int32_t>(sample_rate));AAudioStreamBuilder_setChannelCount(b,static_cast<int32_t>(channels));AAudioStreamBuilder_setFormat(b,AAUDIO_FORMAT_PCM_FLOAT);AAudioStreamBuilder_setPerformanceMode(b,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);AAudioStreamBuilder_setSharingMode(b,AAUDIO_SHARING_MODE_SHARED);AAudioStreamBuilder_setDataCallback(b,audio_callback,this);AAudioStream*s=nullptr;const auto rc=AAudioStreamBuilder_openStream(b,&s);AAudioStreamBuilder_delete(b);if(rc!=AAUDIO_OK||!s)return false;if(AAudioStream_requestStart(s)!=AAUDIO_OK){AAudioStream_close(s);return false;}stream_=static_cast<void*>(s);running_=true;return true;
#else
(void)sample_rate;(void)channels;running_=false;return false;
#endif
}
void AndroidAudioBackend::stop()noexcept{
#if defined(__ANDROID__)
if(stream_){auto*s=static_cast<AAudioStream*>(stream_);(void)AAudioStream_requestStop(s);(void)AAudioStream_close(s);stream_=nullptr;}
#endif
running_=false;gain_=0.0f;
}
void AndroidAudioBackend::set_mix(AudioMixResult mix)noexcept{gain_=std::max(0.0f,mix.gain);pan_=std::clamp(mix.pan,-1.0f,1.0f);
#if defined(__ANDROID__)
g_gain.store(gain_,std::memory_order_relaxed);g_pan.store(pan_,std::memory_order_relaxed);
#endif
}
} // namespace exgine
