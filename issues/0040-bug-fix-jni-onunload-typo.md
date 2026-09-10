# jni_onload.cc の JNI_OnUnLoad タイポを JNI 仕様通りに修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/jni-onunload-typo
- Polished: 2026-09-10
- Milestone: 2026.2.0

## 目的

`src/android_helper/jni_onload.cc` に定義されている `JNI_OnUnLoad` は JNI 仕様の綴りと異なる（`L` が大文字）。JNI 仕様（Invocation API の Library and Version Management）は VM が `JNI_OnUnload`（末尾 `l` 小文字）を検索して呼び出すことを定めており、現状のシンボルは呼ばれず、`CleanupSSL()` を含む後始末が実行されない。仕様通りの綴りに修正する。

## 現状

`src/android_helper/jni_onload.cc` は `extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM* jvm, void* reserved)` を export している。JNI 仕様（Invocation API の Library and Version Management）は `JNI_OnUnload`（末尾 `l` 小文字）を要求する。

- C リンケージのため大文字小文字が有意
- VM は `dlsym(handle, "JNI_OnUnload")` でシンボルを検索するため、現在のシンボルは発見されない
- 結果として `CleanupSSL()` を含む後始末は呼ばれない
- JNI 仕様では、ネイティブライブラリを保持するクラスローダーが GC されたときに `JNI_OnUnload` が呼ばれる。Android の通常のアプリケーションプロセスでは .so のアンロードはまれで実害の再現は容易ではないが、JNI 仕様に反しており修正する
- 上流 libwebrtc の `webrtc/sdk/android/src/jni/jni_onload.cc` にも同じ typo があり、コピー起因の潜在バグである

## 設計方針

- `JNI_OnUnLoad` を `JNI_OnUnload` に置き換える（関数定義と、関数内のログメッセージ `"JNI_OnUnLoad"` の両方）
- `JNI_OnLoad` と同様に `extern "C"` かつ `JNIEXPORT` で export し、シグネチャ `void JNI_OnUnload(JavaVM* jvm, void* reserved)` を維持する
- 上流 libwebrtc の typo は別途 upstream 修正を検討する（本 issue のスコープ外）

## 完了条件

- `src/android_helper/jni_onload.cc` に `JNI_OnUnLoad` が残っていない（関数名とログメッセージの両方）
- ビルド済みの .so を `nm -D` 等で確認し、`JNI_OnUnload` が export されている
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
