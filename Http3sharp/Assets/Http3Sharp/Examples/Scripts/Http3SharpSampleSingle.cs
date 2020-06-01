using System.Collections.Generic;
using System.Text;

/// <summary>
///     単発のダウンロードを行うサンプルです.
/// </summary>
public class Http3SharpSampleSingle : Http3SharpSampleCore
{
    public void OnStartClick()
    {
        CreateHttp3();
        var list = new List<Http3Sharp.RequestParamaters>
        {
            new Http3Sharp.RequestParamaters
            {
                Path = Path,
                SaveFilePath = SaveFilePath,
                Headers = new Http3Sharp.Headers[4]
                {
                    new Http3Sharp.Headers{ Name = "hoge0", Value = "var0" },
                    new Http3Sharp.Headers{ Name = "hoge1", Value = "var1" },
                    new Http3Sharp.Headers{ Name = "hoge2", Value = "var2" },
                    new Http3Sharp.Headers{ Name = "hoge3", Value = "var3" },
                }
            }
        };
        Http3.PublishRequest(list);

        StartButton.interactable = false;
        Body.text = string.Empty;
    }

    private void Update()
    {
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
