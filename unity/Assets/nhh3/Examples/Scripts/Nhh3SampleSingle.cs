using System.Collections.Generic;
using System.Text;

/// <summary>
///     単発のダウンロードを行うサンプルです.
/// </summary>
public class Nhh3SampleSingle : Nhh3SampleCore
{
    public void OnStartClick()
    {
        var list = new List<Nhh3.RequestParamaters>
        {
            new Nhh3.RequestParamaters
            {
                Path = UriPath,
                SaveFilePath = SaveFilePath,
                Headers = new Nhh3.Headers[4]
                {
                    new Nhh3.Headers{ Name = "hoge0", Value = "var0" },
                    new Nhh3.Headers{ Name = "hoge1", Value = "var1" },
                    new Nhh3.Headers{ Name = "hoge2", Value = "var2" },
                    new Nhh3.Headers{ Name = "hoge3", Value = "var3" },
                }
            }
        };
        Http3.PublishRequest(list);

        StartButton.interactable = false;
        Body.text = string.Empty;
    }

    private void Update()
    {
        CheckNetworkReachability();
        var resList = Http3Update();
        foreach (var res in resList)
        {
            if (null != res.Body)
            {
                var sb = new StringBuilder();
                sb.AppendLine("SUCCESS!!");
                sb.AppendLine($"[ResponseCode]{res.ResponseCode}");
                sb.AppendLine("[Headers]");
                foreach (var header in res.Headers)
                {
                    sb.AppendLine($"Name={header.Name} / Value={header.Value}");
                }
                sb.AppendLine("[Body]");
                sb.Append($"{System.Text.Encoding.ASCII.GetString(res.Body)}");
                Body.text = sb.ToString();
            }
        }
    }
}
