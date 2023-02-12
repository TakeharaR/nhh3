using AOT;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
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
    protected string Port = "443";

    [SerializeField]
    protected string HostName = "localhost";

    [SerializeField]
    protected string Path = "";      // URI の Path の指定

    [SerializeField]
    protected string SaveFilePath = string.Empty;   // ファイル保存したい場合はフルパスを指定

    [SerializeField]
    protected string QlogPath = string.Empty;   // qlog を保存したい場合はフルパスを指定

    [SerializeField]
    protected GameObject manager = null;
    #endregion

    protected Nhh3 Http3 { get; set; } = null;
    protected Nhh3.Status Http3Status { get; set; } = Nhh3.Status.Wait;
    protected ulong Http3Progress { get; set; } = 0;

    /// <summary>
    ///     ダウンロードする予定のデータの最大値.
    ///     0 指定されている場合は Http3.Update の totalWriteSize が使われる.
    /// </summary>
    protected ulong TotalDownloadSize { get; set; } = 0;

    private Nhh3.Status _preStatus = Nhh3.Status.Wait;
    private bool _isRetry = false;   // リトライ中か

    private void Start()
    {
        RetryButton.interactable = false;
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
        Http3Progress = progress;
        if (0 == TotalDownloadSize)
        {
            // このフレームにダウンロードした値 / 今までにダウンロードした値
            Progress.text = $"{Http3Progress}/{totalWriteSize}";
        }
        else
        {
            if (0 < totalWriteSize)
            {
                // 今までにダウンロードした値 / ダウンロード予定の値の合計値
                Progress.text = $"{totalWriteSize}/{TotalDownloadSize}";
            }
        }

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
            Http3 = new Nhh3(HostName, Port, new Nhh3.ConnectionOptions
            {
                VerifyPeer = VerifyPeer,
                QlogPath = QlogPath,
                MaxConcurrentStreams = maxMulitipleNum,
                Quic = new Nhh3.QuicOptions
                {
                    MaxIdelTimeout = 0,     // タイムアウト実験したい場合は 0 以上の値を指定してください.
                }
            });
        }
        Clear();
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
