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

            // Only include appropriate CSP binary
            var ext = Path.GetExtension(importer.assetPath);

            // TODO: Remove this hack that always includes iOS and macOS binaries.
            // This was put in place because we currently only include release binaries due to debug binary sizes.
            if (report.summary.platform == BuildTarget.iOS || report.summary.platform == BuildTarget.StandaloneOSX || report.summary.platform == BuildTarget.VisionOS)
                importer.SetIncludeInBuildDelegate((_) => true);
            else if (report.summary.options.HasFlag(BuildOptions.Development) && !importer.assetPath.EndsWith($"_D{ext}"))
                importer.SetIncludeInBuildDelegate((_) => false);
            else if ((!report.summary.options.HasFlag(BuildOptions.Development)) && importer.assetPath.EndsWith($"_D{ext}"))
                importer.SetIncludeInBuildDelegate((_) => false);

            // Fix symbol stripping for VisionOS
            if (report.summary.platform == BuildTarget.VisionOS)
                PlayerSettings.SetAdditionalIl2CppArgs("--linker-flags=\"-Wl,-force_load,libConnectedSpacesPlatform.a\"");
        }

        Debug.Log($"NativePluginBuildProcesser: Done {report.summary.platform}.");
    }
}

#endif