using AOT;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using UnityEngine.UI;

public class Http3SharpSampleCore : MonoBehaviour
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
    protected bool VerifyPeer = false;

    [SerializeField]
    protected string Port = "4433";      // デフォルト値は aioquic のサーバサンプルとの接続を想定

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

    protected Http3Sharp Http3 { get; set; } = null;
    protected Http3Sharp.Status Http3Status { get; set; } = Http3Sharp.Status.Wait;
    protected ulong Http3Progress { get; set; } = 0;

    /// <summary>
    ///     ダウンロードする予定のデータの最大値.
    ///     0 指定されている場合は Http3.Update の totalWriteSize が使われる.
    /// </summary>
    protected ulong TotalDownloadSize { get; set; } = 0;

    private Http3Sharp.Status _preStatus = Http3Sharp.Status.Wait;
    private bool _isRetry = false;   // リトライ中か

    private void Start()
    {
        RetryButton.interactable = false;
    }

    protected List<Http3Sharp.ResponseParamaters> Http3Update()
    {
        var resList = new List<Http3Sharp.ResponseParamaters>();

        if (null == Http3)
        {
            return resList;
        }

        // リクエストを開始していない、終了した状態でも Update は呼び出してコネクションの状態を監視する必要がある.
        resList = Http3.Update(out Http3Sharp.Status status, out ulong progress, out ulong totalWriteSize);
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
        if ((Http3Sharp.Status.Wait == status) && (Http3Sharp.Status.Wait != _preStatus))
        {
            _isRetry = false;
        }

        // エラー状態か
        if (Http3Sharp.Status.Wait > status)
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

        // スタート可能か → OK 且つリトライ、アボート中でないなら可能 ( Http3Sharp の仕様的にはいつでも詰めるがサンプルの仕様)
        StartButton.interactable = (Http3Sharp.Status.Wait == status);

        // リトライ可能か → エラー状態且つリトライ完了してないなら可能
        RetryButton.interactable = ((Http3Sharp.Status.Wait > status) && !_isRetry);

        // アボート可能か → アボート中以外なら可能
        AbortButton.interactable = (Http3Sharp.Status.Aborting != status);

        _preStatus = status;

        return resList;
    }

    protected void CreateHttp3(ulong maxMulitipleNum = 512)
    {
        if (null == Http3)
        {
            Http3 = new Http3Sharp(HostName, Port, new Http3Sharp.ConnectionOptions
            {
                VerifyPeer = VerifyPeer,
                QlogPath = QlogPath,
                Quic = new Http3Sharp.QuicOptions
                {
                    MaxIdelTimeout = 0,     // タイムアウト実験したい場合は 0 以上の値を指定してください.
                    InitialMaxStreamsBidi = maxMulitipleNum,
                    InitialMaxStreamsUni = maxMulitipleNum,
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
        }
    }

}
