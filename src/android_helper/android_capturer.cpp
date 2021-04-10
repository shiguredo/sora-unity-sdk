#include "android_capturer.h"

#include "api/media_stream_interface.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "sdk/android/native_api/jni/class_loader.h"
#include "sdk/android/native_api/jni/scoped_java_ref.h"
#include "sdk/android/native_api/video/video_source.h"
#include "sdk/android/src/jni/android_video_track_source.h"
#include "sdk/android/src/jni/native_capturer_observer.h"

namespace sora {

AndroidCapturer::AndroidCapturer(JNIEnv* env, rtc::Thread* signaling_thread)
    : webrtc::jni::AndroidVideoTrackSource(signaling_thread, env, false, true) {
}
AndroidCapturer::~AndroidCapturer() {
  RTC_LOG(LS_INFO) << "AndroidCapturer::~AndroidCapturer";
  Stop();
}

void AndroidCapturer::Stop() {
  RTC_LOG(LS_INFO) << "AndroidCapturer::Stop";
  if (video_capturer_ != nullptr) {
    RTC_LOG(LS_INFO) << "AndroidCapturer::stopCapture";
    stopCapture(env_, video_capturer_->obj());
    dispose(env_, video_capturer_->obj());
    video_capturer_.reset();
    native_capturer_observer_.reset();
  }
}

rtc::scoped_refptr<AndroidCapturer> AndroidCapturer::Create(
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

bool AndroidCapturer::EnumVideoDevice(
    JNIEnv* env,
    std::function<void(std::string, std::string)> f) {
  auto context = getApplicationContext(env);
  auto camera2enumerator = createCamera2Enumerator(env, context.obj());
  auto devices = enumDevices(env, camera2enumerator.obj());

  // デバイス名@[+frontfacing][+backfacing]
  for (auto device : devices) {
    // camera2enumerator.isFrontFacing(device)
    // camera2enumerator.isBackFacing(device)
    webrtc::ScopedJavaLocalRef<jclass> camcls(
        env, env->GetObjectClass(camera2enumerator.obj()));
    webrtc::ScopedJavaLocalRef<jstring> device_str(
        env, env->NewStringUTF(device.c_str()));
    jmethodID frontid = env->GetMethodID(camcls.obj(), "isFrontFacing",
                                         "(Ljava/lang/String;)Z");
    jboolean frontfacing = env->CallBooleanMethod(camera2enumerator.obj(),
                                                  frontid, device_str.obj());
    jmethodID backid =
        env->GetMethodID(camcls.obj(), "isBackFacing", "(Ljava/lang/String;)Z");
    jboolean backfacing = env->CallBooleanMethod(camera2enumerator.obj(),
                                                 backid, device_str.obj());

    auto name = device + "@";
    if (frontfacing) {
      name += "+frontfacing";
    }
    if (backfacing) {
      name += "+backfacing";
    }
    f(name, name);
  }

  return true;
}

webrtc::ScopedJavaLocalRef<jobject> AndroidCapturer::getApplicationContext(
    JNIEnv* env) {
  // applicationContext = UnityPlayer.currentActivity.getApplicationContext();
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

  return context;
}
webrtc::ScopedJavaLocalRef<jobject> AndroidCapturer::createSurfaceTextureHelper(
    JNIEnv* env) {
  // videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", null);
  webrtc::ScopedJavaLocalRef<jclass> helpcls =
      webrtc::GetClass(env, "org/webrtc/SurfaceTextureHelper");
  jmethodID createid =
      env->GetStaticMethodID(helpcls.obj(), "create",
                             "(Ljava/lang/String;Lorg/webrtc/EglBase$Context;)"
                             "Lorg/webrtc/SurfaceTextureHelper;");
  webrtc::ScopedJavaLocalRef<jstring> str(
      env, env->NewStringUTF("VideoCapturerThread"));
  webrtc::ScopedJavaLocalRef<jobject> helper(
      env,
      env->CallStaticObjectMethod(helpcls.obj(), createid, str.obj(), nullptr));

  return helper;
}
webrtc::ScopedJavaLocalRef<jobject> AndroidCapturer::createCamera2Enumerator(
    JNIEnv* env,
    jobject context) {
  // Camera2Enumerator enumerator = new Camera2Enumerator(applicationContext);
  webrtc::ScopedJavaLocalRef<jclass> camcls =
      webrtc::GetClass(env, "org/webrtc/Camera2Enumerator");
  jmethodID ctorid =
      env->GetMethodID(camcls.obj(), "<init>", "(Landroid/content/Context;)V");
  webrtc::ScopedJavaLocalRef<jobject> enumerator(
      env, env->NewObject(camcls.obj(), ctorid, context));

  // Camera1Enumerator enumerator = new Camera1Enumerator();
  //webrtc::ScopedJavaLocalRef<jclass> camcls =
  //    webrtc::GetClass(env, "org/webrtc/Camera1Enumerator");
  //jmethodID ctorid = env->GetMethodID(camcls.obj(), "<init>", "()V");
  //webrtc::ScopedJavaLocalRef<jobject> enumerator(
  //    env, env->NewObject(camcls.obj(), ctorid));

  return enumerator;
}
std::vector<std::string> AndroidCapturer::enumDevices(
    JNIEnv* env,
    jobject camera_enumerator) {
  // for (string device in enumerator.getDeviceNames()) { }
  webrtc::ScopedJavaLocalRef<jclass> camcls(
      env, env->GetObjectClass(camera_enumerator));
  jmethodID devicesid =
      env->GetMethodID(camcls.obj(), "getDeviceNames", "()[Ljava/lang/String;");
  webrtc::ScopedJavaLocalRef<jobjectArray> devices(
      env, (jobjectArray)env->CallObjectMethod(camera_enumerator, devicesid));

  std::vector<std::string> r;
  int count = env->GetArrayLength(devices.obj());
  for (int i = 0; i < count; i++) {
    webrtc::ScopedJavaLocalRef<jstring> device(
        env, (jstring)env->GetObjectArrayElement(devices.obj(), i));
    r.push_back(env->GetStringUTFChars(device.obj(), nullptr));
  }
  return r;
}
webrtc::ScopedJavaLocalRef<jobject> AndroidCapturer::createCapturer(
    JNIEnv* env,
    jobject camera_enumerator,
    std::string device) {
  // videoCapturer = enumerator.createCapturer(device, null /* eventsHandler */);
  webrtc::ScopedJavaLocalRef<jclass> camcls(
      env, env->GetObjectClass(camera_enumerator));
  jmethodID createid =
      env->GetMethodID(camcls.obj(), "createCapturer",
                       "("
                       "Ljava/lang/String;"
                       "Lorg/webrtc/CameraVideoCapturer$CameraEventsHandler;"
                       ")"
                       "Lorg/webrtc/CameraVideoCapturer;");
  webrtc::ScopedJavaLocalRef<jstring> str(env,
                                          env->NewStringUTF(device.c_str()));
  webrtc::ScopedJavaLocalRef<jobject> capturer(
      env,
      env->CallObjectMethod(camera_enumerator, createid, str.obj(), nullptr));

  return capturer;
}
void AndroidCapturer::initializeCapturer(JNIEnv* env,
                                         jobject capturer,
                                         jobject helper,
                                         jobject context,
                                         jobject observer) {
  // videoCapturer.initialize(videoCapturerSurfaceTextureHelper, applicationContext, nativeGetJavaVideoCapturerObserver(nativeClient));
  webrtc::ScopedJavaLocalRef<jclass> capcls(env, env->GetObjectClass(capturer));
  jmethodID initid = env->GetMethodID(capcls.obj(), "initialize",
                                      "("
                                      "Lorg/webrtc/SurfaceTextureHelper;"
                                      "Landroid/content/Context;"
                                      "Lorg/webrtc/CapturerObserver;"
                                      ")V");
  env->CallVoidMethod(capturer, initid, helper, context, observer);
}
void AndroidCapturer::startCapture(JNIEnv* env,
                                   jobject capturer,
                                   int width,
                                   int height,
                                   int target_fps) {
  // videoCapturer.startCapture(width, height, target_fps);
  webrtc::ScopedJavaLocalRef<jclass> capcls(env, env->GetObjectClass(capturer));
  jmethodID startid = env->GetMethodID(capcls.obj(), "startCapture", "(III)V");
  env->CallVoidMethod(capturer, startid, width, height, target_fps);
}
void AndroidCapturer::stopCapture(JNIEnv* env, jobject capturer) {
  // videoCapturer.stopCapture();
  webrtc::ScopedJavaLocalRef<jclass> capcls(env, env->GetObjectClass(capturer));
  jmethodID stopid = env->GetMethodID(capcls.obj(), "stopCapture", "()V");
  env->CallVoidMethod(capturer, stopid);
}
void AndroidCapturer::dispose(JNIEnv* env, jobject capturer) {
  // videoCapturer.dispose();
  webrtc::ScopedJavaLocalRef<jclass> capcls(env, env->GetObjectClass(capturer));
  jmethodID disposeid = env->GetMethodID(capcls.obj(), "dispose", "()V");
  env->CallVoidMethod(capturer, disposeid);
}

bool AndroidCapturer::Init(JNIEnv* env,
                           rtc::Thread* signaling_thread,
                           size_t width,
                           size_t height,
                           size_t target_fps,
                           std::string video_capturer_device) {
  env_ = env;

  // CreateJavaNativeCapturerObserver の実装は以下のようになっている。
  //
  // ScopedJavaLocalRef<jobject> CreateJavaNativeCapturerObserver(
  //     JNIEnv* env,
  //     rtc::scoped_refptr<AndroidVideoTrackSource> native_source) {
  //   return Java_NativeCapturerObserver_Constructor(
  //       env, NativeToJavaPointer(native_source.release()));
  // }
  //
  // ここで新しく this に対して scoped_refptr を構築して release() しているため、参照カウントが増えたままになっている。
  // Java 側からこの参照カウントを減らしたりはしていないようなので、このままだとリークしてしまう。
  // なので直後に this->Release() を呼んでカウントを調節する。
  native_capturer_observer_.reset(new webrtc::ScopedJavaGlobalRef<jobject>(
      webrtc::jni::CreateJavaNativeCapturerObserver(env, this)));
  this->Release();

  // 以下の Java コードを JNI 経由で呼んで何とか videoCapturer を作る

  // applicationContext = UnityPlayer.currentActivity.getApplicationContext();
  // videoCapturerSurfaceTextureHelper = SurfaceTextureHelper.create("VideoCapturerThread", null);
  // Camera2Enumerator enumerator = new Camera2Enumerator(applicationContext);
  // videoCapturer = enumerator.createCapturer(enumerator.getDeviceNames()[0], null /* eventsHandler */);
  // videoCapturer.initialize(videoCapturerSurfaceTextureHelper, applicationContext, nativeGetJavaVideoCapturerObserver(nativeClient));
  // videoCapturer.startCapture(width, height, target_fps);

  auto context = getApplicationContext(env);
  auto camera2enumerator = createCamera2Enumerator(env, context.obj());
  auto devices = enumDevices(env, camera2enumerator.obj());
  for (auto device : devices) {
    RTC_LOG(LS_INFO) << "camera device: " << device;
  }

  // devices の中から探しているデバイスがあるかを調べる。
  // video_capturer_device が空文字だった場合は 0 番目を利用する。
  // また、video_capturer_device の @ 以降は付帯情報なので無視して一致を見る。
  auto search_device =
      video_capturer_device.substr(0, video_capturer_device.find_last_of('@'));
  int index;
  if (search_device.empty()) {
    index = 0;
  } else {
    auto it = std::find(devices.begin(), devices.end(), search_device);
    if (it == devices.end()) {
      RTC_LOG(LS_ERROR) << "Camera device not found. Searching device is "
                        << video_capturer_device;
      return false;
    }
    index = std::distance(devices.begin(), it);
  }

  auto capturer = createCapturer(env, camera2enumerator.obj(), devices[index]);
  auto helper = createSurfaceTextureHelper(env);
  initializeCapturer(env, capturer.obj(), helper.obj(), context.obj(),
                     native_capturer_observer_->obj());
  startCapture(env, capturer.obj(), width, height, target_fps);

  video_capturer_.reset(
      new webrtc::ScopedJavaGlobalRef<jobject>(env, capturer));

  return true;
}

webrtc::ScopedJavaLocalRef<jobject>
AndroidCapturer::GetJavaVideoCapturerObserver(JNIEnv* env) {
  return webrtc::ScopedJavaLocalRef<jobject>(env, *native_capturer_observer_);
}

}  // namespace sora
