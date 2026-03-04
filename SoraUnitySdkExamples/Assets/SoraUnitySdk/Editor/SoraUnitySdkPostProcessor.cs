#if UNITY_IOS || UNITY_VISIONOS

using UnityEngine;
using UnityEditor;
using UnityEditor.iOS.Xcode;
using UnityEditor.Callbacks;

public class SoraUnitySdkPostProcessor
{
    [PostProcessBuildAttribute(500)]
    public static void OnPostprocessBuild(BuildTarget buildTarget, string pathToBuiltProject)
    {
        if (buildTarget != BuildTarget.iOS && buildTarget != BuildTarget.VisionOS)
        {
            return;
        }

        var projPath = pathToBuiltProject + "/Unity-iPhone.xcodeproj/project.pbxproj";
        if (buildTarget == BuildTarget.VisionOS)
        {
            projPath = projPath.Replace("Unity-iPhone.xcodeproj", "Unity-VisionOS.xcodeproj");
        }

        PBXProject proj = new PBXProject();
        proj.ReadFromFile(projPath);
#if UNITY_2019_3_OR_NEWER
        string guid = proj.GetUnityFrameworkTargetGuid();
#else
        string guid = proj.TargetGuidByName("Unity-iPhone");
#endif

        proj.AddBuildProperty(guid, "OTHER_LDFLAGS", "-ObjC");
        proj.SetBuildProperty(guid, "ENABLE_BITCODE", "NO");
        proj.AddFrameworkToProject(guid, "VideoToolbox.framework", false);
        if (buildTarget != BuildTarget.VisionOS) {
            proj.AddFrameworkToProject(guid, "GLKit.framework", false);
        }
        proj.AddFrameworkToProject(guid, "Network.framework", false);

        if (buildTarget == BuildTarget.iOS)
        {
            // libwebrtc.a には新しい libvpx が、libiPhone-lib.a には古い libvpx が入っていて、
            // デフォルトのリンク順序だと古い libvpx が使われてしまう。
            // それを回避するために libiPhone-lib.a を削除して新しく追加し直すことで
            // リンク順序を変えてやる。
            string fileGuid = proj.FindFileGuidByProjectPath("Libraries/libiPhone-lib.a");
            proj.RemoveFileFromBuild(guid, fileGuid);
            proj.AddFileToBuild(guid, fileGuid);
        }
        else if (buildTarget == BuildTarget.VisionOS)
        {
            // VisionOS 用の設定
            string visionFileGuid = proj.FindFileGuidByProjectPath("Libraries/libVisionOS-lib.a");
            proj.RemoveFileFromBuild(guid, visionFileGuid);
            proj.AddFileToBuild(guid, visionFileGuid);
        }
        proj.WriteToFile(projPath);
    }
}

#endif
