// ------------------------------------------------------------------
// Copyright (c) Magnopus. All Rights Reserved.
// ------------------------------------------------------------------

using TMPro;
using UnityEngine;

/// <summary>
/// Shows Unity debug logger message
/// </summary>
public class DebugConsoleUI : MonoBehaviour
{
    [SerializeField] private TextMeshProUGUI consoleContent;

    private void OnEnable()
    {
        consoleContent.text = string.Empty;
        Application.logMessageReceived += Application_logMessageReceived;
    }

    private void OnDisable()
    {
        Application.logMessageReceived -= Application_logMessageReceived;
    }

    private void Application_logMessageReceived(string logString, string stackTrace, LogType type)
    {
        string color;
        switch (type)
        {
            case LogType.Error:
            case LogType.Assert:
            case LogType.Exception:
                color = "red";
                break;
            case LogType.Warning:
                color = "yellow";
                break;
            default:
                color = "white";
                break;
        }

        consoleContent.text = $"<color={color}>{logString}</color>\n" + consoleContent.text;
    }
}
