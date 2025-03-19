/*
 * Copyright 2025 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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