#if UNITY_EDITOR

using System.IO;

using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEngine;


public class NativePluginBuildProcessor : IPreprocessBuildWithReport
{
    public int callbackOrder => 0;

    public void OnPreprocessBuild(BuildReport report)
    {
        Debug.Log($"NativePluginBuildProcessor: Filtering CSP binaries for platform {report.summary.platform}.");

        var importers = PluginImporter.GetImporters(report.summary.platform);

        foreach (var importer in importers)
        {
            if (!importer.isNativePlugin)
                continue;

            if (!importer.assetPath.Contains("ConnectedSpacesPlatform"))
                continue;

            importer.SetIncludeInBuildDelegate((_) => true);
        }

        Debug.Log($"NativePluginBuildProcesser: Done {report.summary.platform}.");
    }
}

#endif
