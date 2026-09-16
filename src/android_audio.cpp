#include "exgine/android_audio.hpp"
#include <algorithm>
#include <cmath>
namespace exgine {
#if defined(__ANDROID__)
aaudio_data_callback_result_t AndroidAudioBackend::audio_callback(AAudioStream*,void* user,void* audio_data,int32_t num_frames) noexcept {
    auto* self=static_cast<AndroidAudioBackend*>(user);auto* dst=static_cast<float*>(audio_data);const std::size_t needed=std::size_t(num_frames)*self->channels_;
    std::size_t r=self->read_.load(std::memory_order_relaxed),w=self->write_.load(std::memory_order_acquire),available=(w+ring_samples-r)%ring_samples;
    const float gain=self->gain_.load(std::memory_order_relaxed),pan=std::clamp(self->pan_.load(std::memory_order_relaxed),-1.0f,1.0f),L=std::sqrt(.5f*(1-pan))*gain,R=std::sqrt(.5f*(1+pan))*gain;
    for(std::size_t i=0;i<needed;i+=2){float l=0,rr=0;if(available>=2){l=self->ring_[r];rr=self->ring_[(r+1)%ring_samples];r=(r+2)%ring_samples;available-=2;}dst[i]=l*L;dst[i+1]=rr*R;}
    self->read_.store(r,std::memory_order_release);return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#endif
AndroidAudioBackend::~AndroidAudioBackend(){stop();}
bool AndroidAudioBackend::start(std::uint32_t sr,std::uint32_t ch) noexcept {if(running_||!sr||ch!=2)return running_;sample_rate_=sr;channels_=ch;clear();
#if defined(__ANDROID__)
AAudioStreamBuilder*b=nullptr;if(AAudio_createStreamBuilder(&b)!=AAUDIO_OK||!b)return false;AAudioStreamBuilder_setDirection(b,AAUDIO_DIRECTION_OUTPUT);AAudioStreamBuilder_setSampleRate(b,(int32_t)sr);AAudioStreamBuilder_setChannelCount(b,(int32_t)ch);AAudioStreamBuilder_setFormat(b,AAUDIO_FORMAT_PCM_FLOAT);AAudioStreamBuilder_setPerformanceMode(b,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);AAudioStreamBuilder_setSharingMode(b,AAUDIO_SHARING_MODE_SHARED);AAudioStreamBuilder_setDataCallback(b,audio_callback,this);AAudioStream*s=nullptr;auto rc=AAudioStreamBuilder_openStream(b,&s);AAudioStreamBuilder_delete(b);if(rc!=AAUDIO_OK||!s)return false;if(AAudioStream_requestStart(s)!=AAUDIO_OK){AAudioStream_close(s);return false;}stream_=s;running_=true;return true;
#else
return false;
#endif
}
void AndroidAudioBackend::stop() noexcept {
#if defined(__ANDROID__)
if(stream_){auto*s=static_cast<AAudioStream*>(stream_);(void)AAudioStream_requestStop(s);(void)AAudioStream_close(s);stream_=nullptr;}
#endif
running_=false;clear();}
void AndroidAudioBackend::set_mix(AudioMixResult m) noexcept{gain_.store(std::max(0.0f,m.gain),std::memory_order_relaxed);pan_.store(std::clamp(m.pan,-1.0f,1.0f),std::memory_order_relaxed);}
bool AndroidAudioBackend::submit(const SoundSample&s) noexcept{if(s.channels!=2||s.sample_rate!=sample_rate_||s.pcm.empty())return false;const std::size_t r=read_.load(std::memory_order_acquire),w=write_.load(std::memory_order_relaxed),free_space=ring_samples-1-((w+ring_samples-r)%ring_samples);if(s.pcm.size()>free_space)return false;for(std::size_t i=0;i<s.pcm.size();++i)ring_[(w+i)%ring_samples]=s.pcm[i];write_.store((w+s.pcm.size())%ring_samples,std::memory_order_release);return true;}
void AndroidAudioBackend::clear() noexcept{read_.store(0,std::memory_order_release);write_.store(0,std::memory_order_release);}
} // namespace exgine
