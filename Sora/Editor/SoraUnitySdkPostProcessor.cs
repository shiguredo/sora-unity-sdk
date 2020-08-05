using UnityEngine;
using UnityEditor;
using UnityEditor.iOS.Xcode;
using UnityEditor.Callbacks;

public class SoraUnitySdkPostProcesser
{
    [PostProcessBuildAttribute(500)]
    public static void OnPostprocessBuild(BuildTarget buildTarget, string pathToBuiltProject)
    {
        if (buildTarget != BuildTarget.iOS)
        {
            return;
        }

        var projPath = pathToBuiltProject + "/Unity-iPhone.xcodeproj/project.pbxproj";
        PBXProject proj = new PBXProject();
        proj.ReadFromFile(projPath);
#if UNITY_2019_3_OR_NEWER
        string guid = proj.GetUnityFrameworkTargetGuid();
#else
        string guid = proj.TargetGuidByName("Unity-iPhone");
#endif

        proj.AddBuildProperty(guid, "OTHER_LDFLAGS", "-ObjC");
        proj.AddFrameworkToProject(guid, "VideoToolbox.framework", false);
        proj.AddFrameworkToProject(guid, "GLKit.framework", false);
        // libwebrtc.a には新しい libvpx が、libiPhone-lib.a には古い libvpx が入っていて、
        // デフォルトのリンク順序だと古い libvpx が使われてしまう。
        // それを回避するために libiPhone-lib.a を削除して新しく追加し直すことで
        // リンク順序を変えてやる。
        string fileGuid = proj.FindFileGuidByProjectPath("Libraries/libiPhone-lib.a");
        proj.RemoveFileFromBuild(guid, fileGuid);
        proj.AddFileToBuild(guid, fileGuid);

        proj.WriteToFile(projPath);
    }
}
