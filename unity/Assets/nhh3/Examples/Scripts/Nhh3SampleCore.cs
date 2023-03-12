using AOT;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;

public class Nhh3SampleCore : MonoBehaviour
{
    #region SerializeField
    [SerializeField]
    protected Button StartButton = default;

    [SerializeField]
    protected Button RetryButton = default;

    [SerializeField]
    protected Button AbortButton = default;

    [SerializeField]
    protected Text Status = default;

    [SerializeField]
    protected Text Progress = default;

    [SerializeField]
    protected Text Body = default;

    [SerializeField]
    protected Text ErrorDetail = default;

    [SerializeField]
    protected bool VerifyPeer = true;

    [SerializeField]
    protected bool EnableConnectionMigration = true;

    [SerializeField]
    protected bool EnableEarlyData = true;

    [SerializeField]
    protected string Port = "443";

    [SerializeField]
    protected string HostName = "localhost";

    [SerializeField]
    protected string UriPath = "";      // URI の Path の指定

    [SerializeField]
    protected string SaveFilePath = string.Empty;   // ファイル保存したい場合はフルパスを指定

    [SerializeField]
    protected string QlogPath = string.Empty;   // qlog を保存したい場合はフルパスを指定

    /// <summary>
    ///     ファイルを保存するディレクトリ(Windows 時限定).
    ///     この下に連番でファイルが作られます.
    ///     Android 使用時は Application.temporaryCachePath}\\save に保存されます.
    /// </summary>
    [SerializeField]
    protected string WorkPath = string.Empty;   // 0-RTT 用のセッションファイルなどの一時ファイルを配置するパス

    /// <summary>
    ///     同時並行でダウンロードするファイルの上限数.
    ///     ファイル I/O 等で詰まるのであまり大きな値にしても逆にパフォーマンス下がります.
    /// </summary>
    [SerializeField]
    private ulong MaxMultipleDownloadNum = 512;

    [SerializeField]
    protected GameObject manager = null;
    #endregion

    protected Nhh3 Http3 { get; set; } = null;
    protected Nhh3.Status Http3Status { get; set; } = Nhh3.Status.Wait;
    protected ulong Http3Progress { get; set; } = 0;

    private Nhh3.Status _preStatus = Nhh3.Status.Wait;
    private bool _isRetry = false;   // リトライ中か
    private NetworkReachability _preNetworkReachability = NetworkReachability.NotReachable;
    private NetworkReachability _networkReachability = NetworkReachability.NotReachable;

    private void Start()
    {
        RetryButton.interactable = false;
        CreateHttp3(MaxMultipleDownloadNum);
    }

    protected List<Nhh3.ResponseParamaters> Http3Update()
    {
        var resList = new List<Nhh3.ResponseParamaters>();

        if (null == Http3)
        {
            return resList;
        }

        // リクエストを開始していない、終了した状態でも Update は呼び出してコネクションの状態を監視する必要がある.
        resList = Http3.Update(out Nhh3.Status status, out ulong progress, out ulong totalWriteSize);
        Http3Status = status;
        if (0 < progress)
        {
            Http3Progress = progress;
        }
        // このフレームにダウンロードした値 / 今までにダウンロードした値
        Progress.text = $"{Http3Progress}/{totalWriteSize}";

        // ステータスの確認
        Status.text = status.ToString();

        // 通信完了状態か
        if ((Nhh3.Status.Wait == status) && (Nhh3.Status.Wait != _preStatus))
        {
            _isRetry = false;
        }

        // エラー状態か
        if (Nhh3.Status.Wait > status)
        {
            var sb = new StringBuilder();
            sb.AppendLine("ERROR!!");
            sb.AppendLine($"{Http3.GetErrorDetail()}");
            ErrorDetail.text = sb.ToString();
            // エラー状態になったら Retry か Abort してあげる必要がある
        }
        else
        {
            ErrorDetail.text = string.Empty;
        }

        // スタート可能か → OK 且つリトライ、アボート中でないなら可能 ( Nhh3 の仕様的にはいつでも詰めるがサンプルの仕様)
        StartButton.interactable = (Nhh3.Status.Wait == status);

        // リトライ可能か → エラー状態且つリトライ完了してないなら可能
        RetryButton.interactable = ((Nhh3.Status.Wait > status) && !_isRetry);

        // アボート可能か → アボート中以外なら可能
        AbortButton.interactable = (Nhh3.Status.Aborting != status);

        _preStatus = status;

        return resList;
    }

    protected void CreateHttp3(ulong maxMulitipleNum = 512)
    {
        if (null == Http3)
        {
#if UNITY_ANDROID
            // Android は信頼された CA リストを明示的に与える必要がある
            // このサンプルでは StreamingAssets に配置してある
            // 出自 → Mozilla の 2023-01-10 版 https://curl.se/docs/caextract.html
            // quiche がファイルパスを要求するので Application.temporaryCachePath に一度ファイル出力してあげる
            var caPath = Path.Combine(Application.streamingAssetsPath, "cacert.pem");
            var createCaPath = Path.Combine(Application.temporaryCachePath, "cacert.pem");
            using (var uwr = UnityWebRequest.Get(caPath))
            {
                uwr.SendWebRequest();
                while (!uwr.isDone)
                {
                    System.Threading.Thread.Sleep(100);
                }
                byte[] caBytes = uwr.downloadHandler.data;
                File.WriteAllBytes(createCaPath, caBytes);
            }
            // 0-RTT 用のセッションファイルパスを配置するディレクトリの指定
            QlogPath = Application.temporaryCachePath;
            WorkPath = Application.temporaryCachePath;
#endif
            _networkReachability = Application.internetReachability;
            _preNetworkReachability = _networkReachability;

            Http3 = new Nhh3(HostName, Port, new Nhh3.ConnectionOptions
            {
                VerifyPeer = VerifyPeer,
                QlogPath = QlogPath,
                WorkPath = WorkPath,
#if UNITY_ANDROID
                CaCertsList = createCaPath,
#endif
                MaxConcurrentStreams = maxMulitipleNum,
                EnableQuicheLog = !string.IsNullOrEmpty(QlogPath),
                Quic = new Nhh3.QuicOptions
                {
                    MaxIdelTimeout = 0,     // タイムアウト実験したい場合は 0 以上の値を指定してください.
                    DisableActiveMigration = !EnableConnectionMigration,
                    EnableEarlyData = EnableEarlyData,
                }
            });
        }
        Clear();
    }

    // ネットワークの切り替わりを検出したら再接続要求を行う.
    // Connection Migration 対応している場合は内部で自動的に発動する.
    protected void CheckNetworkReachability()
    {
        _preNetworkReachability = _networkReachability;
        _networkReachability = Application.internetReachability;
        if (_preNetworkReachability != _networkReachability)
        {
            if (_networkReachability != NetworkReachability.NotReachable)
            {
                Http3.Reconnect();
            }
        }
    }

    public void OnRetryClick()
    {
        if (null != Http3)
        {
            Http3.Retry();
            _isRetry = true;
            Clear();
        }
    }

    public void OnAbortClick()
    {
        if (null != Http3)
        {
            Http3.Abort();
            Clear();
        }
    }

    private void Clear()
    {
        Progress.text = $"0/0";
        if (null != Body)
        {
            Body.text = string.Empty;
        }
    }

    public void OnDestroy()
    {
        if (null != Http3)
        {
            Http3.Destroy();
            Http3 = null;
        }
    }

}
