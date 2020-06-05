#include "android_codec_factory_helper.h"

#include "rtc_base/logging.h"
#include "sdk/android/native_api/codecs/wrapper.h"
#include "sdk/android/native_api/jni/class_loader.h"
#include "sdk/android/native_api/jni/scoped_java_ref.h"

namespace sora {

std::unique_ptr<webrtc::VideoEncoderFactory> CreateAndroidEncoderFactory(JNIEnv* env) {
  // int[] configPlain = EglBase.CONFIG_PLAIN;
  // EglBase eglBase = new EglBase14Impl(configPlain);
  // EglBase.Context context = eglBase.getEglBaseContext();
  // DefaultVideoEncoderFactory encoderFactory = new DefaultVideoEncoderFactory(context, true /* enableIntelVp8Encoder */, false /* enableH264HighProfile */);

  //jobject config_plain;
  //{
  //  webrtc::ScopedJavaLocalRef<jclass> eglcls =
  //      webrtc::GetClass(env, "org/webrtc/EglBase");
  //  jfieldID fieldid =
  //      env->GetStaticFieldID(eglcls.obj(), "CONFIG_PLAIN", "[I");
  //  config_plain = env->GetStaticObjectField(eglcls.obj(), fieldid);
  //}

  //jobject egl_base;
  //{
  //  webrtc::ScopedJavaLocalRef<jclass> eglcls =
  //      webrtc::GetClass(env, "org/webrtc/EglBase14Impl");
  //  jmethodID ctorid = env->GetMethodID(eglcls.obj(), "<init>",
  //                                      "(Landroid/opengl/EGLContext;[I)V");
  //  egl_base = env->NewObject(eglcls.obj(), ctorid, nullptr, config_plain);
  //}

  //jobject context;
  //{
  //  webrtc::ScopedJavaLocalRef<jclass> eglcls =
  //      webrtc::GetClass(env, "org/webrtc/EglBase");
  //  jmethodID ctxid = env->GetMethodID(eglcls.obj(), "getEglBaseContext",
  //                                     "()Lorg/webrtc/EglBase$Context;");
  //  context = env->CallObjectMethod(egl_base, ctxid);
  //}

  jobject encoder_factory;
  {
    webrtc::ScopedJavaLocalRef<jclass> faccls =
        webrtc::GetClass(env, "org/webrtc/DefaultVideoEncoderFactory");
    jmethodID ctorid = env->GetMethodID(faccls.obj(), "<init>",
                                        "(Lorg/webrtc/EglBase$Context;ZZ)V");
    encoder_factory =
        env->NewObject(faccls.obj(), ctorid, nullptr, true, false);
  }

  return webrtc::JavaToNativeVideoEncoderFactory(env, encoder_factory);
}
std::unique_ptr<webrtc::VideoDecoderFactory> CreateAndroidDecoderFactory(
    JNIEnv* env) {
  // EglBase eglBase = EglBase.create();
  // EglBase.Context context = context.getEglBaseContext();
  // DefaultVideoDecoderFactory decoderFactory = new DefaultVideoDecoderFactory(context);

  webrtc::ScopedJavaLocalRef<jclass> faccls =
      webrtc::GetClass(env, "org/webrtc/DefaultVideoDecoderFactory");
  jmethodID ctorid = env->GetMethodID(faccls.obj(), "<init>",
                                      "(Lorg/webrtc/EglBase$Context;)V");
  jobject decoder_factory = env->NewObject(faccls.obj(), ctorid, nullptr);
  return webrtc::JavaToNativeVideoDecoderFactory(env, decoder_factory);
}

}  // namespace sora
