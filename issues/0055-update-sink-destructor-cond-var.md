# UnityRenderer::Sink デストラクタの busy-wait を condition_variable に置き換える

- Priority: Medium
- Created: 2026-08-27
- Branch: update/sink-destructor-cond-var
- Polished: 2026-09-10

## 目的

`src/unity_renderer.cpp` の `UnityRenderer::Sink` デストラクタが `updating_` フラグをポーリングしている実装を、`std::condition_variable::wait_for` によるイベント待ちに置き換えて、異常状態でも Unity メインスレッドが無限フリーズしないようにする。

## 現状

`UnityRenderer::Sink` のデストラクタは `deleting_` フラグをセットした後、`while (updating_) sleep_for(std::chrono::milliseconds(10))` で `updating_` が false になるまで無限にポーリングする。

`updating_` は `TextureUpdateCallback` の Begin 分岐で true になり、End 分岐で false に戻る。ただし Begin 分岐は `deleting_` が立っているときは更新を始めずに `updating_ = false` に戻して return するため、デストラクタのポーリングは次の Begin イベントが発火すれば抜け得る。

レンダースレッドが停止しているケースや、Unity 側のエラーで texture 更新のコールバックが発火しなくなるケースでは、`updating_` が true のまま End も Begin も来ず、この待ちが永久に終わらず Unity メインスレッドがフリーズする。

上限のないポーリングであるため、フリーズ状態から利用者が復旧する手段は Unity プロセスの強制終了のみになる。

## 設計方針

- `deleting_` / `updating_` を `std::mutex` + `std::condition_variable` で保護する構造に変更する
- デストラクタは `cv.wait_for(lock, timeout, [] { return !updating_; })` で最大待ち時間を持たせる
- タイムアウトした場合は `RTC_LOG(LS_ERROR)` を出した上でデストラクタを続行する（アプリケーションを止めない）
- `TextureUpdateCallback` は End 分岐で mutex を取って `updating_ = false` にしてから `cv.notify_all()` する
- `TextureUpdateCallback` の Begin 分岐は `deleting_` が立っているとき現状と同様に更新を開始せず、mutex を取って `updating_ = false` にしてから `cv.notify_all()` して return する（デストラクタがタイムアウト待ちにならず即座に続行できるようにする）
- `IdPointer::Lookup` と `Sink` 生存保証の race は「UnityRenderer Sink の TextureUpdateCallback race による SEGV を修正する」issue (0011) の対象であり、本 issue では変更しない
  - 本 issue は 0011 の完了後に実装する。0011 の `Lookup` が `shared_ptr` を返す設計（`TextureUpdateCallback` が保持している間は `Sink` が破棄されない）が前提であり、0011 未実装のまま本 issue を実装すると、タイムアウトしてデストラクタを続行した時点で、進行中の更新が破棄済み `Sink` を触る UAF を新たに作る

## 完了条件

- `~Sink` に無限ポーリングが残っていない
- タイムアウト時にログを出しながら続行する経路が実装されている
- 通常のトラック増減で Sink が正しく破棄され、Unity 側の描画に回帰がない
- `CHANGES.md` の `## develop` に該当記述を追記する
