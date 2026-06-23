// Android AAR フォーマットは依存関係のメタデータ（ POM ）を含まないため、
// Sora.aar を flatDir 経由で取り込んでも推移的依存が自動解決されない。
// そのため Sora.aar が必要とする依存をビルド時に build.gradle に注入する。
//
// バージョンは sora-cpp-sdk の依存関係ツリーと一致させること。
// 新しい依存が増えた場合は dependency 変数に行を追加する。
#if UNITY_EDITOR && UNITY_ANDROID

using System.IO;
using UnityEditor.Android;
using UnityEngine;

public class SoraAndroidDependencyInjector : IPostGenerateGradleAndroidProject
{
    // 他のプラグインの後処理が終わった後に実行するため callbackOrder は 100。
    public int callbackOrder => 100;

    public void OnPostGenerateGradleAndroidProject(string basePath)
    {
        string gradlePath = Path.Combine(basePath, "build.gradle");
        string content = File.ReadAllText(gradlePath);
        string dependency = "implementation 'androidx.core:core:1.9.0'";

        // Unity の mainTemplate.gradle が生成する行全体を置き換える
        string searchPattern = "implementation fileTree(dir: 'libs', include: ['*.jar'])";
        if (!content.Contains(searchPattern))
        {
            Debug.LogWarning("[SoraAndroidDependencyInjector] Pattern not found in build.gradle");
            return;
        }

        content = content.Replace(searchPattern, searchPattern + "\n    " + dependency);
        File.WriteAllText(gradlePath, content);
        Debug.Log("[SoraAndroidDependencyInjector] Injected " + dependency);
    }
}

#endif
