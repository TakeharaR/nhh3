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

    private ulong _completedDownloadFileNum = 0;

    public void OnStartClick()
    {
        var list = new List<Nhh3.RequestParamaters>();
        for (ulong num = 0; num < DownloadFileNum; ++num)
        {
            // 保存パス
#if UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN
            var saveFilePath = $"{WorkPath}\\{num}";
#else
            // Android
            var saveFilePath = $"{Application.temporaryCachePath}\\{num}";
#endif
            list.Add(new Nhh3.RequestParamaters
            {
                //SaveFilePath = saveFilePath,
                Path = UriPath,
            });
        };
        Http3.PublishRequest(list);

        StartButton.interactable = false;
        _completedDownloadFileNum = 0;
    }

    private void Update()
    {
        CheckNetworkReachability();
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
