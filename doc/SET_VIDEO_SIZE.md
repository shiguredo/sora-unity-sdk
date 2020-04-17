## 解像度の変更方法
### 概要
解像度の変更方法について記載します。  
変更方法は[sora-unity-sdk-samples](https://github.com/shiguredo/sora-unity-sdk-samples)を参考例として記載しています。  

### 変更対象
- Sora.cs
- SoraSample.cs
- RawImage(pub/sub/multi_pubシーンのみ)

### 変更方法
#### Sora.cs
---------------------------------------
37行目、38行目の`VideoWidth`と`VideoHeight`を変更してください。  

[![Image from Gyazo](https://i.gyazo.com/f686032aa286bd552a077b9327f4d16c.png)](https://gyazo.com/f686032aa286bd552a077b9327f4d16c)  
#### SoraSample.cs
---------------------------------------
91行目のTexture2Dのパラメータを変更してください。  
参考：[UnityDocument:Texture2D.Texture2D](https://docs.unity3d.com/jp/460/ScriptReference/Texture2D-ctor.html)  

[![Image from Gyazo](https://i.gyazo.com/47ae4e50834e793a7d5e84d0a68788a2.png)](https://gyazo.com/47ae4e50834e793a7d5e84d0a68788a2)  
#### RawImage(pub/sub/multi_pubシーンのみ)
---------------------------------------
multi_pubsubシーンにはRawImageはありません。   
HierarchyからRawImageを選択し、Inspectorから`Width`と`Height`の値を変更してください。  

[![Image from Gyazo](https://i.gyazo.com/9ba94ab0b13edc2d4d4bf0d529e3ed14.png)](https://gyazo.com/9ba94ab0b13edc2d4d4bf0d529e3ed14)

#### 変更結果
- カメラから取得した映像
解像度が設定した1280x720になっています。
[![Image from Gyazo](https://i.gyazo.com/3e7b05d4a2467dcd211b95660a764910.png)](https://gyazo.com/3e7b05d4a2467dcd211b95660a764910)

- Unityのカメラから取得した映像
解像度が設定した1280x720になっています。
[![Image from Gyazo](https://i.gyazo.com/887c45d03a04d3758c3b90a836ea728c.png)](https://gyazo.com/887c45d03a04d3758c3b90a836ea728c)
