#ifndef ANDROID_CAPTURER_H_INCLUDED
#define ANDROID_CAPTURER_H_INCLUDED

#include "api/media_stream_interface.h"
#include "rtc_base/thread.h"
#include "sdk/android/native_api/jni/class_loader.h"
#include "sdk/android/native_api/jni/scoped_java_ref.h"
#include "sdk/android/native_api/video/video_source.h"
#include "sdk/android/src/jni/android_video_track_source.h"
#include "sdk/android/src/jni/native_capturer_observer.h"

namespace sora {

class AndroidCapturer : public webrtc::jni::AndroidVideoTrackSource {
 public:
  AndroidCapturer(JNIEnv* env, rtc::Thread* signaling_thread)
      : webrtc::jni::AndroidVideoTrackSource(signaling_thread,
                                             env,
                                             false,
                                             true) {}

  static rtc::scoped_refptr<AndroidCapturer> Create(
      JNIEnv* env,
      rtc::Thread* signaling_thread,
      size_t width,
      size_t height,
      size_t target_fps,
      std::string video_capturer_device) {
    rtc::scoped_refptr<AndroidCapturer> p(
        new rtc::RefCountedObject<AndroidCapturer>(env, signaling_thread));
    if (!p->Init(env, signaling_thread, width, height, target_fps,
                 video_capturer_device)) {
      return nullptr;
    }
    return p;
  }

 private:
  jobject getApplicationContext(JNIEnv* env) {
    // applicationContext = UnityPlayer.currentActivity.getApplicationContext();
    webrtc::ScopedJavaLocalRef<jclass> upcls =
        webrtc::GetClass(env, "com/unity3d/player/UnityPlayer");
    jfieldID actid = env->GetStaticFieldID(upcls.obj(), "currentActivity",
                                           "Landroid/app/Activity;");
    jobject activity = env->GetStaticObjectField(upcls.obj(), actid);

    jclass actcls = env->GetObjectClass(activity);
    jmethodID ctxid = env->GetMethodID(actcls, "getApplicationContext",
                                       "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(activity, ctxid);

    return context;
  }
  jobject createSurfaceTextureHelper(JNIEnv* env) {
    // videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", null);
    webrtc::ScopedJavaLocalRef<jclass> helpcls =
        webrtc::GetClass(env, "org/webrtc/SurfaceTextureHelper");
    jmethodID createid = env->GetStaticMethodID(
        helpcls.obj(), "create",
        "(Ljava/lang/String;Lorg/webrtc/EglBase$Context;)"
        "Lorg/webrtc/SurfaceTextureHelper;");
    jobject helper = env->CallStaticObjectMethod(
        helpcls.obj(), createid, env->NewStringUTF("VideoCapturerThread"),
        nullptr);

    return helper;
  }
  jobject createCamera2Enumerator(JNIEnv* env, jobject context) {
    // Camera2Enumerator enumerator = new Camera2Enumerator(applicationContext);
    webrtc::ScopedJavaLocalRef<jclass> camcls =
        webrtc::GetClass(env, "org/webrtc/Camera2Enumerator");
    jmethodID ctorid = env->GetMethodID(camcls.obj(), "<init>",
                                        "(Landroid/content/Context;)V");
    jobject enumerator = env->NewObject(camcls.obj(), ctorid, context);

    return enumerator;
  }
  std::vector<std::string> enumDevices(JNIEnv* env, jobject camera_enumerator) {
    // for (string device in enumerator.getDeviceNames()) { }
    jclass camcls = env->GetObjectClass(camera_enumerator);
    jmethodID devicesid =
        env->GetMethodID(camcls, "getDeviceNames", "()[Ljava/lang/String;");
    jobjectArray devices =
        (jobjectArray)env->CallObjectMethod(camera_enumerator, devicesid);

    std::vector<std::string> r;
    int count = env->GetArrayLength(devices);
    for (int i = 0; i < count; i++) {
      jstring device = (jstring)env->GetObjectArrayElement(devices, i);
      r.push_back(env->GetStringUTFChars(device, nullptr));
    }
    return r;
  }
  jobject createCapturer(JNIEnv* env,
                         jobject camera_enumerator,
                         std::string device) {
    // videoCapturer = enumerator.createCapturer(device, null /* eventsHandler */);
    jclass camcls = env->GetObjectClass(camera_enumerator);
    jmethodID createid =
        env->GetMethodID(camcls, "createCapturer",
                         "("
                         "Ljava/lang/String;"
                         "Lorg/webrtc/CameraVideoCapturer$CameraEventsHandler;"
                         ")"
                         "Lorg/webrtc/CameraVideoCapturer;");
    jobject capturer =
        env->CallObjectMethod(camera_enumerator, createid,
                              env->NewStringUTF(device.c_str()), nullptr);

    return capturer;
  }
  void initializeCapturer(JNIEnv* env,
                          jobject capturer,
                          jobject helper,
                          jobject context,
                          jobject observer) {
    // videoCapturer.initialize(videoCapturerSurfaceTextureHelper, applicationContext, nativeGetJavaVideoCapturerObserver(nativeClient));
    jclass capcls = env->GetObjectClass(capturer);
    jmethodID initid = env->GetMethodID(capcls, "initialize",
                                        "("
                                        "Lorg/webrtc/SurfaceTextureHelper;"
                                        "Landroid/content/Context;"
                                        "Lorg/webrtc/CapturerObserver;"
                                        ")V");
    env->CallVoidMethod(capturer, initid, helper, context, observer);
  }
  void startCapture(JNIEnv* env,
                    jobject capturer,
                    int width,
                    int height,
                    int target_fps) {
    // videoCapturer.startCapture(width, height, target_fps);
    jclass capcls = env->GetObjectClass(capturer);
    jmethodID startid = env->GetMethodID(capcls, "startCapture", "(III)V");
    env->CallVoidMethod(capturer, startid, width, height, target_fps);
  }

  bool Init(JNIEnv* env,
            rtc::Thread* signaling_thread,
            size_t width,
            size_t height,
            size_t target_fps,
            std::string video_capturer_device) {
    native_capturer_observer_.reset(new webrtc::ScopedJavaGlobalRef<jobject>(
        env, webrtc::jni::CreateJavaNativeCapturerObserver(env, this)));
    // 以下の Java コードを JNI 経由で呼んで何とか videoCapturer を作る

    // applicationContext = UnityPlayer.currentActivity.getApplicationContext();
    // videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", null);
    // Camera2Enumerator enumerator = new Camera2Enumerator(applicationContext);
    // videoCapturer = enumerator.createCapturer(enumerator.getDeviceNames()[0], null /* eventsHandler */);
    // videoCapturer.initialize(videoCapturerSurfaceTextureHelper, applicationContext, nativeGetJavaVideoCapturerObserver(nativeClient));
    // videoCapturer.startCapture(width, height, target_fps);

    auto context = getApplicationContext(env);
    auto helper = createSurfaceTextureHelper(env);
    auto camera2enumerator = createCamera2Enumerator(env, context);
    auto devices = enumDevices(env, camera2enumerator);
    RTC_LOG(LS_INFO) << "context: " << context;
    RTC_LOG(LS_INFO) << "helper: " << helper;
    for (auto device : devices) {
      RTC_LOG(LS_INFO) << "camera device: " << device;
    }
    auto capturer = createCapturer(env, camera2enumerator, devices[0]);
    initializeCapturer(env, capturer, helper, context,
                       native_capturer_observer_->obj());
    startCapture(env, capturer, width, height, target_fps);

    video_capturer_.reset(new webrtc::ScopedJavaGlobalRef<jobject>(
        env, webrtc::JavaParamRef<jobject>(capturer)));

    return true;
  }

  webrtc::ScopedJavaLocalRef<jobject> GetJavaVideoCapturerObserver(
      JNIEnv* env) {
    return webrtc::ScopedJavaLocalRef<jobject>(env, *native_capturer_observer_);
  }

 private:
  std::unique_ptr<webrtc::ScopedJavaGlobalRef<jobject>>
      native_capturer_observer_;
  std::unique_ptr<webrtc::ScopedJavaGlobalRef<jobject>> video_capturer_;
};
}

#endif // ANDROID_CAPTURER_H_INCLUDED
