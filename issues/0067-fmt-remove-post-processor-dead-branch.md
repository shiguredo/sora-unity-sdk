# SoraUnitySdkPostProcessor.cs の到達不能な #else 分岐を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-post-processor-dead-branch
- Polished: 2026-09-11

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs` に残っている `#if UNITY_2019_3_OR_NEWER` の `#else` 分岐を削除する。

## 現状

`SoraUnitySdkPostProcessor.cs` の `OnPostprocessBuild` には `#if UNITY_2019_3_OR_NEWER` / `#else` / `#endif` の分岐が書かれており、`#else` 側は `proj.TargetGuidByName("Unity-iPhone")` の 1 行である。

README と `SoraUnitySdkExamples/README.md` で対応 Unity は 6000.0 / 6000.3 と明記されており、Unity 2019.3 未満はサポート対象外となっている。`UNITY_2019_3_OR_NEWER` はサポート対象の Unity では常に定義されるため、`#else` 分岐は永遠に到達不能な dead ブロックである。

## 設計方針

- `#if UNITY_2019_3_OR_NEWER` / `#else` / `#endif` の分岐ごと削除し、有効な分岐の中身だけを残す
- `#else` 分岐の削除により、なぜ `GetUnityFrameworkTargetGuid()` を使うのかが見えなくなるため、呼び出しの直前に以下の 1 行コメントを残す

```csharp
        // ビルド設定は Unity-iPhone ターゲットではなく UnityFramework ターゲットへ適用する
        string guid = proj.GetUnityFrameworkTargetGuid();
```

- 挙動変更は無い

## 完了条件

- `SoraUnitySdkPostProcessor.cs` から到達不能な `#else` 分岐が消えている
- `GetUnityFrameworkTargetGuid()` の呼び出しに、上記の 1 行コメントが付いている
- サポート対象の Unity (6000.0 / 6000.3) でのビルドが通り、iOS の PostProcessor 動作に回帰が無い
