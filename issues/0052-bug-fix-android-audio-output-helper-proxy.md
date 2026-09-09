# AndroidAudioOutputHelper.Dispose で AndroidJavaProxy を Dispose する

- Priority: Medium
- Created: 2026-08-27
- Completed: 2026-09-10
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

## 解決方法

コード変更は行わず closed にした。issue の前提 (AndroidJavaProxy が JNI GlobalRef を保持し、Dispose されない限り JNI reference table に残り続ける) が Unity の実装と矛盾し、提案されている修正 (`callbackProxy?.Dispose()`) は存在しないメソッドの呼び出しでコンパイルもできないため、バグ修正 issue としての根拠が成立しない。

一次資料・ソースでの照合結果:

- 現行コード: `Sora.cs` の `AndroidAudioOutputHelper.Dispose` は `soraAudioManager.Call("stop")` と `soraAudioManager.Dispose()` のみを呼び、`callbackProxy` (型は `ChangeRouteCallbackProxy : AndroidJavaProxy`) を操作していない (現状の記述自体は正しい)
- Unity 6.0 の Scripting API ドキュメント (`AndroidJavaProxy` クラスページ): Public Methods は `equals` / `hashCode` / `Invoke` / `toString` のみで、`Dispose` は存在しない (`AndroidJavaProxy.Dispose` ページは 404)。Unity 2019.4 の同ページでも同様
- Unity の C# リファレンスソース (Unity-Technologies/UnityCsReference の `Modules/AndroidJNI/AndroidJava.cs`): `AndroidJavaProxy` は `public class AndroidJavaProxy` であり `IDisposable` を実装せず、`Dispose()` メソッドも持たない。プロキシが保持するのは Java 側 Proxy インスタンスへの weak global reference (`AndroidJNI.NewWeakGlobalRef`) だけであり、破棄はファイナライザ `~AndroidJavaProxy()` の `AndroidJNISafe.DeleteWeakGlobalRef(proxyObject)` が行う (GC 依存)。強い GlobalRef は生成されないため、Dispose 漏れによる JNI reference table への積み上がりは構造的に発生しない
- `soraAudioManager` は `AndroidJavaObject` であり `Dispose()` が呼ばれているため、`GlobalJavaObjectRef` が保持する strong global ref も解放される。C# 側に未解放の JNI リソースは残らない
- 設計方針の 3 項目目 (コールバック実行中でも安全に解放できることを検証する) は、解放対象の `Dispose()` が存在しないため前提が成立しない

備考: polish-issue のレビューで本件が処理不能指摘 (issue の前提が一次資料と矛盾) として確定したため、`Polished:` は更新していない。
