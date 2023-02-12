using AOT;
using UnityEngine;

public class Nhh3Manager : MonoBehaviour
{
    [SerializeField]
    private GameObject manager = null;

    void Awake()
    {
        DontDestroyOnLoad(manager);
        
        Nhh3.SetDebugLogCallback(DebugLog);
        Nhh3.Initialize();
    }

    [MonoPInvokeCallback(typeof(Nhh3.DebugLogCallback))]
    private static void DebugLog(string message)
    {
        UnityEngine.Debug.Log(message);
    }

    void OnDestroy()
    {
        Nhh3.Uninitialize();
    }
}
