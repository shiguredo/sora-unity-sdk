# jni_onload.cc の JNI_OnUnLoad タイポを JNI 仕様通りに修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/jni-onunload-typo
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/android_helper/jni_onload.cc` に定義されている `JNI_OnUnLoad` は JNI 仕様の綴りと異なる（`L` が大文字）。JVM の `dlsym` は `JNI_OnUnload`（末尾 `l` 小文字）を検索するため、現状のシンボルは呼ばれず、`CleanupSSL()` を含む後始末が実行されない。仕様通りの綴りに修正する。

## 現状

`src/android_helper/jni_onload.cc` は `extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM* jvm, void* reserved)` を export している。しかし JNI 仕様（`jni.h` の宣言）は `JNI_OnUnload`（末尾 `l` 小文字）を要求する。

- C リンケージのため大文字小文字が有意
- JVM は `dlsym(handle, "JNI_OnUnload")` を検索するため、現在のシンボルは発見されない
- 結果として `CleanupSSL()` を含む後始末は永久に呼ばれない
- Android アプリのライフサイクルで .so が unload されるケースは少ないが、Editor Play/Stop 繰り返しで OpenSSL の状態が積み上がる
- 上流 libwebrtc の `webrtc/sdk/android/src/jni/jni_onload.cc` にも同じ typo があり、コピー起因の潜在バグである

## 設計方針

- `JNI_OnUnLoad` を `JNI_OnUnload` に置き換える
- 同時に `JNI_OnLoad` との対称性を確認する
- 上流 libwebrtc の typo は別途 upstream 修正を検討する（本 issue のスコープ外）

## 完了条件

- `src/android_helper/jni_onload.cc` のシンボル名が JNI 仕様通り `JNI_OnUnload` になっている
- Android で .so unload 時に `CleanupSSL()` のログが出ることを確認する
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
