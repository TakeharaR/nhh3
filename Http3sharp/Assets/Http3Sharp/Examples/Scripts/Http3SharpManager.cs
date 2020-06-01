using AOT;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using UnityEngine.UI;

public class Http3SharpManager : MonoBehaviour
{
    [SerializeField]
    private GameObject manager = null;

    void Awake()
    {
        DontDestroyOnLoad(manager);
        
        Http3Sharp.SetDebugLogCallback(DebugLog);
        Http3Sharp.Initialize();
    }

    [MonoPInvokeCallback(typeof(Http3Sharp.DebugLogCallback))]
    private static void DebugLog(string message)
    {
        UnityEngine.Debug.Log(message);
    }

    void OnDestroy()
    {
        Http3Sharp.Uninitialize();
    }
}
