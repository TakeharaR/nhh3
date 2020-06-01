http3sharp は HTTP/3 通信用の Unity 向けアセットです。
[quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ層である qwfs と、 qwfs を用いた C# 製の HTTP/3 クライアントライブラリである http3sharp から成ります。


# http3sharp

http3sharp は Unity で HTTP/3 通信を行う事を目的とした Unity アセットです。
``Http3sharp`` ディレクトリを指定して Unity でプロジェクトを開くか、そのままインポートしてください。

実行には qwfs のビルドが必要です。
後述する手順で必ず qwfs のビルドを行ってください。
qwfs のビルドが成功すると ``Http3sharp/Assets/Http3Sharp/Plugins\Windows`` フォルダに quiche.dll 及び qwfs.dll が自動的にコピーされるので、 dll の手動コピーは不要です。


## 既知の問題・制限事項

- 累計リクエストが peer 及び client の ``initial_max_streams_bidi`` の値を超えるとリクエストが発行できなくなります
    - 当問題発生時には Http3Sharp インスタンスの再生成もしくは ``Http3Sharp.Abort`` 呼び出しを行ってください
- quiche のデバッグログ出力には非対応です
- ファイル名や qlog 保存のパス等に日本語のファイルパスを指定するとクラッシュします
- 最適化は一切施していません
- その他ソースコード内で todo とコメントがある個所は修正予定です



# qwfs

qwfs (quiche wrapper for sharp) は [quiche](https://github.com/cloudflare/quiche) を Unity に組み込み易くラッピングした C/C++ 製のライブラリです。
http3sharp の利用に特化した仕様で実装されているので単体での使用は非推奨です。

#### ビルド方法

Visual Studio 2019 が必要です。
``External/quiche`` を quiche の submodule 設定にしてありますので、ここで quiche をビルドしてから ``Example/QuicheWindowsSample/QuicheWindowsSample.sln`` を使いビルドを行ってください。

quiche の .lib/.dll ビルド方法は以下を参照してください。  
[Re: C#(Unity)でHTTP/3通信してみる その壱 ～OSSの選定からビルドまで～](https://qiita.com/takehara-ryo/items/1f3e19b54bd6fffcba77)


# QuicheWindowsSample

[quiche](https://github.com/cloudflare/quiche) の example を Windows 移植したものです。
元としている quiche の example コードのバージョンは 0.4.0 です。  
詳細は以下の記事を参照してください。
[Re: C#(Unity)でHTTP/3通信してみる その弐 ～Windowsでquicheのサンプルを動かしてみる～](https://qiita.com/takehara-ryo/items/4cbb5c291b2e94b4eafd)
※当サンプルは今後メンテナンスされない可能性があります

ビルドには http3sharp と同様に quiche のビルドが必要です。
``External/quiche`` を quiche の submodule 設定にしてありますので、ここで quiche をビルドしてから ``qwfs/qwfs.sln`` を使いビルドを行ってください。


# 注意事項

現在の最新バージョンは 0.1.0 です。
0.1.0 は実験的実装であり、インターフェースや仕様も仮のものです。
今後、破壊的な変更を伴う大幅な修正が加わる事があります。
利用時にはご注意ください。


# 対応プラットフォーム

バージョン 0.1.0 では Windows にのみ対応しています。
将来的には Android/iOS に対応予定です。


# バージョン関連情報

- 対応している HTTP/3, QUIC の draft バージョン : 27
- 動作確認している Unity バージョン : 2019.3.14f
- quiche : 0.4.0
- boringssl : quiche の submodule の バージョン に準ずる


# License
This software is released under the MIT License, see LICENSE.