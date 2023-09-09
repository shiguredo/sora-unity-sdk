#include "android_context.h"

#include <sdk/android/native_api/jni/class_loader.h>
#include <sdk/android/native_api/jni/jvm.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>

namespace sora_unity_sdk {

webrtc::ScopedJavaLocalRef<jobject> GetAndroidApplicationContext(JNIEnv* env) {
  // Context context = UnityPlayer.currentActivity.getApplicationContext()
  // を頑張って C++ から呼んでるだけ
  webrtc::ScopedJavaLocalRef<jclass> upcls =
      webrtc::GetClass(env, "com/unity3d/player/UnityPlayer");
  jfieldID actid = env->GetStaticFieldID(upcls.obj(), "currentActivity",
                                         "Landroid/app/Activity;");
  webrtc::ScopedJavaLocalRef<jobject> activity(
      env, env->GetStaticObjectField(upcls.obj(), actid));

  webrtc::ScopedJavaLocalRef<jclass> actcls(
      env, env->GetObjectClass(activity.obj()));
  jmethodID ctxid = env->GetMethodID(actcls.obj(), "getApplicationContext",
                                     "()Landroid/content/Context;");
  webrtc::ScopedJavaLocalRef<jobject> context(
      env, env->CallObjectMethod(activity.obj(), ctxid));

  // org.webrtc.ContextUtils.initialize(context)
  // を呼ぶ。これをしないと ADM が生成できない
  webrtc::ScopedJavaLocalRef<jclass> cucls =
      webrtc::GetClass(env, "org/webrtc/ContextUtils");
  jmethodID initid = env->GetStaticMethodID(cucls.obj(), "initialize",
                                            "(Landroid/content/Context;)V");
  env->CallStaticObjectMethod(cucls.obj(), initid, context.obj());

  return context;
}

}  // namespace sora_unity_sdk
