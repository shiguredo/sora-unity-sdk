#ifndef ANDROID_CONTEXT_H_
#define ANDROID_CONTEXT_H_

#include "sdk/android/native_api/jni/scoped_java_ref.h"

namespace sora {

webrtc::ScopedJavaLocalRef<jobject> GetAndroidApplicationContext(JNIEnv* env);

}

#endif  // ANDROID_CONTEXT_H_
