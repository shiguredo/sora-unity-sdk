#ifndef ANDROID_CODEC_FACTORY_HELPER_H_
#define ANDROID_CODEC_FACTORY_HELPER_H_

#include <memory>

#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "sdk/android/native_api/jni/jvm.h"

std::unique_ptr<webrtc::VideoEncoderFactory> CreateAndroidEncoderFactory(
    JNIEnv* env);
std::unique_ptr<webrtc::VideoDecoderFactory> CreateAndroidDecoderFactory(
    JNIEnv* env);

#endif // ANDROID_CODEC_FACTORY_HELPER_H_
