#ifndef SORA_UNITY_SDK_ANDROID_HELPER_ANDROID_CONTEXT_H_
#define SORA_UNITY_SDK_ANDROID_HELPER_ANDROID_CONTEXT_H_

#include <sdk/android/native_api/jni/scoped_java_ref.h>

namespace sora_unity_sdk {

webrtc::ScopedJavaLocalRef<jobject> GetAndroidApplicationContext(JNIEnv* env);

}

#endif
