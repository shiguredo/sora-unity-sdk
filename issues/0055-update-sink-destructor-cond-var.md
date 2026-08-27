# UnityRenderer::Sink デストラクタの busy-wait を condition_variable に置き換える

- Priority: Medium
- Created: 2026-08-27
- Branch: update/sink-destructor-cond-var
- Polished: {YYYY-MM-DD}

## 目的

`src/unity_renderer.cpp` の `UnityRenderer::Sink` デストラクタが `updating_` フラグをポーリングしている実装を、`std::condition_variable::wait_for` によるイベント待ちに置き換えて、異常状態でも Unity メインスレッドが無限フリーズしないようにする。

## 現状

`UnityRenderer::Sink` のデストラクタは `deleting_` フラグをセットした後、`while (updating_) sleep_for(std::chrono::milliseconds(10))` で `updating_` が false になるまで無限にポーリングする。

`updating_` を false に戻すのは `TextureUpdateCallback` の end 分岐だが、レンダースレッドが停止しているケースや、Unity 側のエラーで end イベントが飛ばないケースでは、この待ちが永久に終わらず Unity メインスレッドがフリーズする。

上限のないポーリングであるため、フリーズ状態から利用者が復旧する手段は Unity プロセスの強制終了のみになる。

## 設計方針

- `deleting_` / `updating_` を `std::mutex` + `std::condition_variable` で保護する構造に変更する
- デストラクタは `cv.wait_for(lock, timeout, [] { return !updating_; })` で最大待ち時間を持たせる
- タイムアウトした場合は `RTC_LOG(LS_ERROR)` を出した上でデストラクタを続行する（アプリケーションを止めない）
- `TextureUpdateCallback` は end 分岐で mutex を取って `updating_ = false` にしてから `cv.notify_all()` する
- 別 issue で扱う `IdPointer::Lookup` と `Sink` 生存保証の race は本 issue の対象外（依存関係あり）

## 完了条件

- `~Sink` に無限ポーリングが残っていない
- タイムアウト時にログを出しながら続行する経路が実装されている
- 通常のトラック増減で Sink が正しく破棄され、Unity 側の描画に回帰がない
- `CHANGES.md` の `## develop` に該当記述を追記する
