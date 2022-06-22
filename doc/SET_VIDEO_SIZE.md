## 解像度の変更方法
### 概要
解像度の変更には「送信する映像のサイズ」、「受信するテクスチャのサイズ」、「Unityの表示上のサイズ」の3つを変える必要があります。  
ここでの変更方法は[sora-unity-sdk-samples](https://github.com/shiguredo/sora-unity-sdk-samples)を参考例として記載しています。  

### 変更対象
- SoraSample.cs
- RawImage(pub/sub/multi_pubシーンのみ)

### 変更方法
#### 送信する映像のサイズの変更
---------------------------------------
##### SoraSample.cs
`VideoWidth`と`VideoHeight`パラメータを追加してください。  
[![Image from Gyazo](https://i.gyazo.com/36bad2d5d625a7e63107a9a2a5db7984.png)](https://gyazo.com/36bad2d5d625a7e63107a9a2a5db7984)

#### 受信するテクスチャのサイズの変更
---------------------------------------
##### SoraSample.cs
テクスチャを生成するパラメータを変更してください。  
参考：[UnityDocument:Texture2D.Texture2D](https://docs.unity3d.com/jp/460/ScriptReference/Texture2D-ctor.html)  

- pub/subの場合
[![Image from Gyazo](https://i.gyazo.com/47ae4e50834e793a7d5e84d0a68788a2.png)](https://gyazo.com/47ae4e50834e793a7d5e84d0a68788a2)  
- multi_pub_sub/multi_subの場合
[![Image from Gyazo](https://i.gyazo.com/50d3e2699d008e59d2649733aae6b7ea.png)](https://gyazo.com/50d3e2699d008e59d2649733aae6b7ea)

#### Unityの表示上のサイズの変更
##### RawImage(pub/sub/multi_pubシーンのみ)
---------------------------------------
HierarchyからRawImageを選択し、Inspectorから`Width`と`Height`の値を変更してください。  
`Width`と`Height`を変更すると設定した値によっては「開始」と「終了」ボタンが隠れてしまうため、  
Hierarchyから「ButtonStart」と「ButtonEnd」を選択して少し上に動かしてください。  

[![Image from Gyazo](https://i.gyazo.com/9ba94ab0b13edc2d4d4bf0d529e3ed14.png)](https://gyazo.com/9ba94ab0b13edc2d4d4bf0d529e3ed14)

参考：`Width`と`Height`を変更するとGameビューでは以下のように変化します。  
[![Image from Gyazo](https://i.gyazo.com/791329a7ea7d5524cb781027ef918446.png)](https://gyazo.com/791329a7ea7d5524cb781027ef918446)  

###### multi_sendrecvシーンを変更したい場合
multi_sendrecvシーンは動的に必要なイメージ数が変わるため、あらかじめ設定するRawImageはありません。   
その場合はHierarchyのCanvas/BaseTrackの変更とCanvas/Scroll Viewのサイズ変更をしてください。  
[![Image from Gyazo](https://i.gyazo.com/e025bc6392b4424e1b25d0b6f95b2589.png)](https://gyazo.com/e025bc6392b4424e1b25d0b6f95b2589)

#### 変更結果
- Unityでの表示
[![Image from Gyazo](https://i.gyazo.com/1b5cbd74888c36e3923ec99910db5955.png)](https://gyazo.com/1b5cbd74888c36e3923ec99910db5955)

- Unityから送信した映像の表示
設定した1280x720になっています。
[![Image from Gyazo](https://i.gyazo.com/3e7b05d4a2467dcd211b95660a764910.png)](https://gyazo.com/3e7b05d4a2467dcd211b95660a764910)
