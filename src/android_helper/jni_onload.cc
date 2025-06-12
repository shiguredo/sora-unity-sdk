/*
 *  Copyright 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <jni.h>
#undef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))

#include "rtc_base/logging.h"
#include "rtc_base/ssl_adapter.h"
#include "sdk/android/native_api/jni/class_loader.h"
#include "sdk/android/src/jni/jni_helpers.h"

namespace webrtc {
namespace jni {

extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved) {
  jint ret = InitGlobalJniVariables(jvm);
  RTC_DCHECK_GE(ret, 0);
  if (ret < 0)
    return -1;

  RTC_CHECK(webrtc::InitializeSSL()) << "Failed to InitializeSSL()";
  webrtc::InitClassLoader(GetEnv());
  //LoadGlobalClassReferenceHolder();
  //unity_plugin::LoadGlobalClassReferenceHolder();
  RTC_LOG(LS_INFO) << "JNI_OnLoad ret=" << ret;

  return ret;
}

extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM* jvm, void* reserved) {
  //FreeGlobalClassReferenceHolder();
  //unity_plugin::FreeGlobalClassReferenceHolder();
  RTC_CHECK(webrtc::CleanupSSL()) << "Failed to CleanupSSL()";

  RTC_LOG(LS_INFO) << "JNI_OnUnLoad";
}

}  // namespace jni
}  // namespace webrtc
