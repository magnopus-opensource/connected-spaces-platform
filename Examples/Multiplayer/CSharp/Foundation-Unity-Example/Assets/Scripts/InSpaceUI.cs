using System;
using UnityEngine;
using UnityEngine.UI;

public class InSpaceUI : MonoBehaviour
{
    public event Action OnExitSpace;

    [SerializeField] private Button exitSpaceButton;

    private void Awake()
    {
        exitSpaceButton.onClick.AddListener(ExitSpace);
    }
    
    private void OnDestroy()
    {
        exitSpaceButton.onClick.RemoveListener(ExitSpace);
    }

    private void ExitSpace()
    {
        OnExitSpace?.Invoke();
    }
}
