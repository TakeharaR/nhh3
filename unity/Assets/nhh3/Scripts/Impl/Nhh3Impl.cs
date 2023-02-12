using AOT;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using UnityEngine.Assertions.Must;

public class Nhh3Impl
{
    public enum QwfsResult
    {
        CriticalError = -101,

        Error = -100,

        ErrorInvalidArg = -1,
        ErrorInvalidCall = -2,
        ErrorFileIO = -3,
        ErrorDoesNotExist = -4,

        Ok = 0,
    }

    #region Constants
    const ulong INVALID_HOST_ID = ulong.MaxValue;

    public static Nhh3.DebugLogCallback DebugLog { get; set; } = null;
    #endregion


    #region Callbacks
    public delegate void SuccessFileCallback(ulong hostId, ulong responseCode, IntPtr headers, ulong headersSize, string filePath);
    public delegate void SuccessBinaryCallback(ulong hostId, ulong responseCode, IntPtr headers, ulong headersSize, IntPtr body, ulong bodySize);
    public delegate void ErrorCallback(ulong hostId, Nhh3.Status status, string errorDetail);

    public struct Callbacks
    {
        public SuccessFileCallback SuccessFile { get; set; }
        public SuccessBinaryCallback SuccessBinary { get; set; }
        public ErrorCallback Error { get; set; }
    }
    #endregion


    #region Properties
    public String Authority { get; private set; } = string.Empty;
    #endregion


    #region Static
    public static void Initialize()
    {
        qwfsInitialize();

        if (null != DebugLog)
        {
            qwfsSetDebugOutput(DebugLog);
        }
    }

    public static void Uninitialize()
    {
        foreach (var host in _Nhh3ImplList)
        {
            host.Value.Destroy(false);
        }
        _Nhh3ImplList.Clear();
        _responseForEachHost.Clear();
        qwfsUninitialize();
    }

    private static Dictionary<ulong, Nhh3Impl> _Nhh3ImplList = new Dictionary<ulong, Nhh3Impl>();
    private static Dictionary<ulong, List<Nhh3.ResponseParamaters>> _responseForEachHost = new Dictionary<ulong, List<Nhh3.ResponseParamaters>>();
    #endregion


    #region Variables
    private ulong  _hostId = INVALID_HOST_ID;
    private ulong  _externalId = 0;     // そのうちコールバック側でリクエスト特定とかに使うかもしれないので仕込み
    private string _hostName = string.Empty;
    private string _port = string.Empty;

    private int _requestRetry = 0;
    private int _requestAbort = 0;

    private Task                    _task;
    private CancellationTokenSource _cToken;
    private object                  _lockObject;
    #endregion

    #region Functions
    public Nhh3Impl(string hostName, string port, Nhh3.ConnectionOptions options)
    {
        _cToken = new CancellationTokenSource();
        _lockObject = new object();
        Authority = hostName + port;
        _hostName = hostName;
        _port = port;

        // native 側のインスタンスの生成やコールバックの登録
        _hostId = qwfsCreate(hostName, port, new Callbacks { SuccessFile = RequestFileSuccess, SuccessBinary = RequestBinarySuccess, Error = RequestError });
        Debug.Assert(INVALID_HOST_ID != _hostId);

        // 
        if (!_responseForEachHost.ContainsKey(_hostId))
        {
            _responseForEachHost.Add(_hostId, new List<Nhh3.ResponseParamaters>());
        }

        // そのうち単独呼び出しにするかも
        
        if (!string.IsNullOrWhiteSpace(options.QlogPath))
        {
            if (!Directory.Exists(options.QlogPath))
            {
                Directory.CreateDirectory(options.QlogPath);
            }
        }
        var result = qwfsSetOptions(_hostId, options);
        Debug.Assert(QwfsResult.Ok == result);

        _Nhh3ImplList.Add(_hostId, this);

        // Update 用のタスクをこの時点から走らせ始める(リクエストが無くてもハンドシェイクが始まる為)
        _task = Task.Run(() =>
        {
            UpdateQwfs();
        });
    }

    public void Destroy(bool removeList = true)
    {
        if (_Nhh3ImplList.ContainsKey(_hostId))
        {
            _cToken.Cancel();
            _task.Wait();
            qwfsDestroy(_hostId);
            if (_responseForEachHost.ContainsKey(_hostId))
            {
                _responseForEachHost.Remove(_hostId);
            }
            if (removeList)
            {
                _Nhh3ImplList.Remove(_hostId);
            }
        }
        
    }

    public void PublishRequest(IEnumerable<Nhh3.RequestParamaters> requests)
    {
        lock (_lockObject)
        {
            foreach (var request in requests)
            {
                if (!string.IsNullOrWhiteSpace(request.SaveFilePath))
                {
                    var saveDir = Path.GetDirectoryName(request.SaveFilePath);
                    if (!Directory.Exists(saveDir))
                    {
                        Directory.CreateDirectory(saveDir);
                    }
                }
                if (!request.Path.StartsWith("/"))
                {
                    request.Path = "/" + request.Path;
                }
                var result = qwfsGetRequest(_hostId, _externalId, request.Path, request.SaveFilePath, request.Headers, (uint)request.Headers.Length);
                Debug.Assert(QwfsResult.Ok == result);
                ++_externalId;
            }
        }
    }

    public List<Nhh3.ResponseParamaters> Update(out Nhh3.Status status, out ulong progress, out ulong totalWriteSize)
    {
        // Update スレッドの監視とか本当はした方がいいがとりあえずなし

        status = Nhh3.Status.Wait;
        progress = 0;
        totalWriteSize = 0;
        _responseForEachHost[_hostId].Clear();

        lock (_lockObject)
        {
            var result = qwfsGetProgress(_hostId, out progress, out totalWriteSize);
            Debug.Assert(QwfsResult.Ok == result);

            result = qwfsIssueCallbacks(_hostId);
            Debug.Assert(QwfsResult.Ok == result);

            result = qwfsGetStatus(_hostId, out status);
            Debug.Assert(QwfsResult.Ok == result);
        }

        return _responseForEachHost[_hostId];
    }

    public void Retry()
    {
        Interlocked.Exchange(ref _requestRetry, 1);
    }

    public void Abort()
    {
        Interlocked.Exchange(ref _requestAbort, 1);
    }

    public string GetErrorDetail()
    {
        IntPtr ptr;
        var message = string.Empty;
        lock (_lockObject)
        {
            ptr = qwfsGetErrorDetail(_hostId);
        }
        if (IntPtr.Zero == ptr)
        {
            return string.Empty;
        }
        message = Marshal.PtrToStringAnsi(ptr);
        return message;
    }
    #endregion


    private static void ReadResponseHeaders(IntPtr headers, ulong headersSize, ref Nhh3.ResponseParamaters res)
    {
        int elementSize = Marshal.SizeOf(typeof(IntPtr));
        IntPtr ptr;
        res.Headers = new Nhh3.Headers[headersSize / 2];
        for (int num = 0; num < (int)headersSize; ++num)
        {
            ptr = Marshal.ReadIntPtr(headers, elementSize * num);
            if (0 == (num % 2))
            {
                res.Headers[num / 2].Name = Marshal.PtrToStringAnsi(ptr);
            }
            else
            {
                res.Headers[num / 2].Value = Marshal.PtrToStringAnsi(ptr);
            }
        }
    }

    #region Callback Functions
    [MonoPInvokeCallback(typeof(SuccessFileCallback))]
    private static void RequestFileSuccess(ulong hostId, ulong responseCode, IntPtr headers, ulong headersSize, string filePath)
    {
        Debug.Assert(_responseForEachHost.ContainsKey(hostId));

        var res = new Nhh3.ResponseParamaters
        {
            SaveFilePath = filePath,
            ResponseCode = responseCode,
        };
        ReadResponseHeaders(headers, headersSize, ref res);
        _responseForEachHost[hostId].Add(res);
    }

    [MonoPInvokeCallback(typeof(SuccessBinaryCallback))]
    private static void RequestBinarySuccess(ulong hostId, ulong responseCode, IntPtr headers, ulong headersSize, IntPtr body, ulong bodySize)
    {
        Debug.Assert(_responseForEachHost.ContainsKey(hostId));

        var res = new Nhh3.ResponseParamaters
        {
            ResponseCode = responseCode,
        };
        ReadResponseHeaders(headers, headersSize, ref res);

        // todo : でかいサイズの場合の非同期読み込み, int 上限越えのサイズ対応
        if ((IntPtr.Zero != body) && (0 < bodySize))
        {
            res.Body = new byte[bodySize];
            unsafe
            {
                using (UnmanagedMemoryStream src = new UnmanagedMemoryStream((byte*)body.ToPointer(), (long)bodySize))
                {
                    src.Read(res.Body, 0, (int)bodySize);
                };
            }
        }
        _responseForEachHost[hostId].Add(res);
    }

    [MonoPInvokeCallback(typeof(ErrorCallback))]
    private static void RequestError(ulong hostId, Nhh3.Status status, string errorDetail)
    {
        Debug.Assert(_responseForEachHost.ContainsKey(hostId));

        var res = new Nhh3.ResponseParamaters
        {
            Status = status,
            ErrorMessage = errorDetail,
        };
        // header 欲しいかも
        _responseForEachHost[hostId].Add(res);
    }
    #endregion


    #region Private Functions
    private void UpdateQwfs()
    {
        while (true)
        {
            if (_cToken.Token.IsCancellationRequested)
            {
                break;
            }

            try
            {
                QwfsResult result;
                if (1 == Interlocked.Exchange(ref _requestRetry, 0))
                {
                    lock (_lockObject)
                    {
                        result = qwfsRetry(_hostId);
                    }
                    Debug.Assert(QwfsResult.Ok == result);
                }
                if (1 == Interlocked.Exchange(ref _requestAbort, 0))
                {
                    lock (_lockObject)
                    {
                        result = qwfsAbort(_hostId);
                    }
                    Debug.Assert(QwfsResult.Ok == result);
                }

                lock (_lockObject)
                {
                    result = qwfsUpdate(_hostId);
                    // ここの結果は色々返るが status 側で見るのでスルー(要改善)
                }

                // todo : 処理負荷を見た wait
                Thread.Sleep(1);
            }
            catch (Exception e)
            {
                // callback 内で例外発生時にスレッドが消し飛んでしまうのを抑止
                // 本当はエラー停止すべきなので暫定
                UnityEngine.Debug.Log(e.Message);
            }
        }
        _cToken.Dispose();
    }
    #endregion


    #region Native Functions

#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN || UNITY_ANDROID
    private const string PluginName = "qwfs";
#elif UNITY_IOS
    private const string PluginName = "__Internal";
#endif

    [DllImport(PluginName)]
    private static extern void qwfsInitialize();

    [DllImport(PluginName)]
    private static extern void qwfsUninitialize();

    [DllImport(PluginName, CharSet = CharSet.Ansi)]
    private static extern ulong qwfsCreate([In] string hostName, [In] string port, Nhh3Impl.Callbacks callbacks);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsDestroy(ulong hostId);

    [DllImport(PluginName, CharSet = CharSet.Ansi)]
    private static extern QwfsResult qwfsSetOptions(ulong hostId, Nhh3.ConnectionOptions options);

    [DllImport(PluginName, CharSet = CharSet.Ansi)]
    private static extern QwfsResult qwfsGetRequest(ulong hostId, ulong streamId, [In] string path, [In] string saveFilePath, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 5)] Nhh3.Headers[] headers, ulong headersSize);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsUpdate(ulong hostId);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsIssueCallbacks(ulong hostId);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsGetStatus(ulong hostId, out Nhh3.Status status);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsGetProgress(ulong hostId, out ulong progress, out ulong totalWriteSize);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsRetry(ulong hostId);

    [DllImport(PluginName)]
    private static extern QwfsResult qwfsAbort(ulong hostId);

    [DllImport(PluginName)]
    private static extern IntPtr qwfsGetErrorDetail(ulong hostId);

    [DllImport(PluginName, CharSet = CharSet.Ansi)]
    private static extern QwfsResult qwfsSetDebugOutput(Nhh3.DebugLogCallback debugLog);

    #endregion
}
