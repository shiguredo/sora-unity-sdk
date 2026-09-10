# DataChannelSignaling と IgnoreDisconnectWebsocket を nullable にする

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/change-data-channel-signaling-nullable
- Polished: {YYYY-MM-DD}
- Reporter: @miosakuma

## 目的

`Sora.Config` から `EnableDataChannelSignaling` と `EnableIgnoreDisconnectWebsocket` を削除し、`DataChannelSignaling` と `IgnoreDisconnectWebsocket` を `bool?` に変更する。未設定の場合は送信せず、`true` / `false` を指定した場合はその値を明示的に送信する。

有効・無効を切り替えるフラグと実際の値の 2 つで管理する現状の方式では、`false` を明示指定しても `data_channel_signaling` / `ignore_disconnect_websocket` が送信されない。Simulcast や Spotlight と同じ「未設定なら送信しない」方式に統一し、フラグの二重管理をなくす。

## 現状

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.Config` に次のフィールドがある。

- `EnableDataChannelSignaling` と `DataChannelSignaling`
- `EnableIgnoreDisconnectWebsocket` と `IgnoreDisconnectWebsocket`

`Sora.Connect` は `EnableDataChannelSignaling` が `true` のときだけ `cc.SetDataChannelSignaling` を、`EnableIgnoreDisconnectWebsocket` が `true` のときだけ `cc.SetIgnoreDisconnectWebsocket` を呼んでいる。このため値を `false` にしても未指定扱いとなり、サーバー側の判断で DataChannel に切り替えられる可能性がある。

`proto/sora_conf_internal.proto` の `data_channel_signaling` と `ignore_disconnect_websocket` は `optional bool` であり、`SetDataChannelSignaling` / `SetIgnoreDisconnectWebsocket` を呼び出したときだけ送信される。

`SoraUnitySdkExamples/Assets/SoraSample.cs` の `SoraSample.OnClickStart()` では、`dataChannelSignaling` を `EnableDataChannelSignaling` と `DataChannelSignaling` の両方へ、`ignoreDisconnectWebsocket` を `EnableIgnoreDisconnectWebsocket` と `IgnoreDisconnectWebsocket` の両方へ設定しており、この問題をコメントで説明している。

Simulcast は `bool?` であり、`Sora.Connect` では `HasValue` のときだけ `SetSimulcast` を呼ぶ方式になっている。

## 設計方針

- `Sora.Config.DataChannelSignaling` と `Sora.Config.IgnoreDisconnectWebsocket` を `bool?` に変更する
- デフォルトは null とし、null の場合は送信しない
- `Sora.Config.EnableDataChannelSignaling` と `Sora.Config.EnableIgnoreDisconnectWebsocket` を削除する
- `Sora.Connect` では Simulcast と同じく `HasValue` のときだけ `SetDataChannelSignaling` / `SetIgnoreDisconnectWebsocket` を呼ぶ
- `SoraSample.OnClickStart()` は `DataChannelSignaling = dataChannelSignaling` と `IgnoreDisconnectWebsocket = ignoreDisconnectWebsocket` に変更し、二重指定と未指定の挙動を説明するコメントを削除する
- 後方互換性のない変更のため `CHANGES.md` の `## develop` に `[CHANGE]` を追記する

## 完了条件

- `Sora.Config` から `EnableDataChannelSignaling` と `EnableIgnoreDisconnectWebsocket` が削除されている
- `DataChannelSignaling` と `IgnoreDisconnectWebsocket` が `bool?` になっている
- null の場合は `data_channel_signaling` と `ignore_disconnect_websocket` が送信されない
- `true` / `false` を指定した場合はその値が送信される
- `SoraSample.cs` が新しい設定方法に追従している
- `CHANGES.md` の `## develop` に `[CHANGE]` が追記されている
