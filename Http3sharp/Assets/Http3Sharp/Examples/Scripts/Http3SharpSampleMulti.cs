using System.Collections.Generic;
using System.Text;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
///     aioquic の GET/NNNN 機能を用いて多重ダウンロードのテストを行うサンプルです.
///     ダウンロードするデータのサイズは 1K ～ 1M の間のランダム値です.
/// </summary>
public class Http3SharpSampleMulti : Http3SharpSampleCore
{
    [SerializeField]
    private Text CompletedDownloadFileNum = default;

    [SerializeField]
    private Slider DownloadProgressBar = default;

    /// <summary>
    ///     ダウンロードを行うファイル数.
    ///     下層で利用している quiche の問題により累計 129 個目のリクエストを出すとエラーが発生します.
    ///     エラー発生後は ABORT 等でコネクションを張り直す必要があります.
    /// </summary>
    [SerializeField]
    private ulong DownloadFileNum = 128;

    /// <summary>
    ///     同時並行でダウンロードするファイルの上限数.
    ///     ファイル I/O 等で詰まるのであまり大きな値にしても逆にパフォーマンス下がります.
    /// </summary>
    [SerializeField]
    private ulong MaxMultipleDownloadNum = 512;

    /// <summary>
    ///     ファイルを保存するディレクトリ.
    ///     この下に連番でファイルが作られます.
    /// </summary>
    [SerializeField]
    private string WorkDir = @"F:\tmp";


    private ulong _completedDownloadFileNum = 0;

    public void OnStartClick()
    {
        CreateHttp3(MaxMultipleDownloadNum);

        // 1k-10M のデータを DownloadFileNum 個ダウンロードする
        TotalDownloadSize = 0;
        var list = new List<Http3Sharp.RequestParamaters>();
        for (ulong num = 0; num < DownloadFileNum; ++num)
        {
            var fileSize = (ulong)new System.Random().Next(1, 1000) * 1024;
            TotalDownloadSize += fileSize;
            list.Add(new Http3Sharp.RequestParamaters
            {
                SaveFilePath = $"{WorkDir}\\{num}",
                Path = $"{fileSize}",
            });
        };
        Http3.PublishRequest(list);

        StartButton.interactable = false;
        _completedDownloadFileNum = 0;
    }

    private void Update()
    {
        var resList = Http3Update();
        _completedDownloadFileNum += (ulong)resList.Count;
        CompletedDownloadFileNum.text = $"{_completedDownloadFileNum}/{DownloadFileNum} FILE";
        DownloadProgressBar.value = (float)_completedDownloadFileNum /(float)DownloadFileNum;

        // 完了するまで再スタートはさせない
        StartButton.interactable = StartButton.interactable ? true : (_completedDownloadFileNum == DownloadFileNum);
    }

    public new void OnAbortClick()
    {
        _completedDownloadFileNum = 0;
        base.OnAbortClick();
    }
}
