# SoraUnitySdkPostProcessor.cs の到達不能な #else 分岐を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-post-processor-dead-branch
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs` に残っている `#if UNITY_2019_3_OR_NEWER` の `#else` 分岐を削除する。

## 現状

`SoraUnitySdkPostProcessor.cs` には `#if UNITY_2019_3_OR_NEWER` と `#else` の分岐が書かれている。

README で対応 Unity は 6000.0 / 6000.3 と明記されており、Unity 2019.3 未満はサポート対象外となっている。この状態では `#else` 分岐は永遠に到達不能な dead ブロックである。

## 設計方針

- `#if UNITY_2019_3_OR_NEWER` / `#else` / `#endif` の分岐ごと削除し、有効な分岐の中身だけを残す
- 対応 Unity バージョンの明示について README や PostProcessor 冒頭コメントに残す必要があるかを確認し、必要なら 1 行のコメントで残す
- 挙動変更は無い

## 完了条件

- `SoraUnitySdkPostProcessor.cs` から到達不能な `#else` 分岐が消えている
- 対応する Unity バージョンでのビルドが通り、iOS の PostProcessor 動作に回帰が無い
