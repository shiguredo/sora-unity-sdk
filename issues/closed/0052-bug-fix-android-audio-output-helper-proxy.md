# AndroidAudioOutputHelper.Dispose で AndroidJavaProxy を Dispose する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/android-audio-output-helper-proxy
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `AndroidAudioOutputHelper` が保持する `AndroidJavaProxy` を `Dispose` で解放し、Android 実行時に JNI reference table にリークするのを防ぐ。

## 現状

`Sora.cs` の `AndroidAudioOutputHelper.Dispose` は `soraAudioManager.Call("stop")` を呼んだ後 `soraAudioManager.Dispose()` を実行するが、コンストラクタで確保した `AndroidJavaProxy callbackProxy` を Dispose していない。

`AndroidJavaProxy` は Java 側で Proxy インスタンスを生成し JNI GlobalRef を保持する。`Dispose` されない限り GlobalRef は JNI reference table に残り続ける。

Unity アプリケーションが `AndroidAudioOutputHelper` の生成・破棄を繰り返す（例: 接続と切断の反復）と、参照が積み上がり最終的に Android の JNI reference table overflow で `FATAL EXCEPTION: JNI reference table overflow` に至る。

## 設計方針

- `AndroidAudioOutputHelper.Dispose` で `callbackProxy?.Dispose()` を呼び出す
- `disposed` フラグと組み合わせて Dispose 後の再 Dispose を安全化する（既存パターンに従う）
- Java 側 Proxy が呼び出し中のスレッドから叩かれるケースを検証し、Dispose 時に onChangeRoute 相当のコールバックが実行中でも安全に解放できるようにする

## 完了条件

- `AndroidAudioOutputHelper.Dispose` で `callbackProxy` の Dispose が呼ばれている
- 生成・破棄を繰り返しても JNI reference table にリークが積まれないことが Android 実機で確認できる
- 既存の Android 音声出力先切り替え機能が回帰していない
- `CHANGES.md` の `## develop` に `[FIX] AndroidAudioOutputHelper.Dispose で AndroidJavaProxy を解放する` を追記する
