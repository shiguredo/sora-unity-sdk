#ifndef ANDROID_CAPTURER_H_INCLUDED
#define ANDROID_CAPTURER_H_INCLUDED

#include <android/log.h>

#include "api/media_stream_interface.h"
#include "sdk/android/native_api/jni/scoped_java_ref.h"
#include "sdk/android/native_api/video/video_source.h"
#include "sdk/android/src/jni/android_video_track_source.h"
#include "sdk/android/src/jni/native_capturer_observer.h"

namespace sora {

class AndroidCapturer : public webrtc::jni::AndroidVideoTrackSource {
 public:
  AndroidCapturer(JNIEnv* env, rtc::Thread* signaling_thread);
  ~AndroidCapturer();

  void Stop();

  static rtc::scoped_refptr<AndroidCapturer> Create(
      JNIEnv* env,
      rtc::Thread* signaling_thread,
      size_t width,
      size_t height,
      size_t target_fps,
      std::string video_capturer_device);

  static bool EnumVideoDevice(JNIEnv* env,
                              std::function<void(std::string, std::string)> f);

 private:
  static webrtc::ScopedJavaLocalRef<jobject> getApplicationContext(JNIEnv* env);
  static webrtc::ScopedJavaLocalRef<jobject> createSurfaceTextureHelper(
      JNIEnv* env);
  static webrtc::ScopedJavaLocalRef<jobject> createCamera2Enumerator(
      JNIEnv* env,
      jobject context);
  static std::vector<std::string> enumDevices(JNIEnv* env,
                                              jobject camera_enumerator);
  static webrtc::ScopedJavaLocalRef<jobject>
  createCapturer(JNIEnv* env, jobject camera_enumerator, std::string device);
  static void initializeCapturer(JNIEnv* env,
                                 jobject capturer,
                                 jobject helper,
                                 jobject context,
                                 jobject observer);
  static void startCapture(JNIEnv* env,
                           jobject capturer,
                           int width,
                           int height,
                           int target_fps);
  static void stopCapture(JNIEnv* env, jobject capturer);
  static void dispose(JNIEnv* env, jobject capturer);

  bool Init(JNIEnv* env,
            rtc::Thread* signaling_thread,
            size_t width,
            size_t height,
            size_t target_fps,
            std::string video_capturer_device);

  webrtc::ScopedJavaLocalRef<jobject> GetJavaVideoCapturerObserver(JNIEnv* env);

 private:
  JNIEnv* env_ = nullptr;
  std::unique_ptr<webrtc::ScopedJavaGlobalRef<jobject>>
      native_capturer_observer_;
  std::unique_ptr<webrtc::ScopedJavaGlobalRef<jobject>> video_capturer_;
};
}

#endif // ANDROID_CAPTURER_H_INCLUDED
