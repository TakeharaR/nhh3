using System.Collections.Generic;
using System.Text;
using UnityEngine;
using UnityEngine.UI;

/// <summary>
///     aioquic の GET/NNNN 機能を用いて多重ダウンロードのテストを行うサンプルです.
///     ダウンロードするデータのサイズは 1K ～ 1M の間のランダム値です.
/// </summary>
public class Nhh3SampleMulti : Nhh3SampleCore
{
    [SerializeField]
    private Text CompletedDownloadFileNum = default;

    [SerializeField]
    private Slider DownloadProgressBar = default;

    /// <summary>
    ///     ダウンロードを行うファイル数.
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
    ///     ファイルを保存するディレクトリ(Windows 時限定).
    ///     この下に連番でファイルが作られます.
    ///     Android 使用時は Application.temporaryCachePath}\\save に保存されます.
    /// </summary>
    [SerializeField]
    private string WorkDir = "";


    private ulong _completedDownloadFileNum = 0;

    public void OnStartClick()
    {
        CreateHttp3(MaxMultipleDownloadNum);

        // 1k-10M のデータを DownloadFileNum 個ダウンロードする
        TotalDownloadSize = 0;
        var list = new List<Nhh3.RequestParamaters>();
        for (ulong num = 0; num < DownloadFileNum; ++num)
        {
            var fileSize = (ulong)new System.Random().Next(1, 1000) * 1024;
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
            var saveFilePath = $"{WorkDir}\\{num}";
#else
            // Android
            var saveFilePath = $"{Application.temporaryCachePath}\\save\\{num}";
#endif
            TotalDownloadSize += fileSize;
            list.Add(new Nhh3.RequestParamaters
            {
                SaveFilePath = saveFilePath,
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
