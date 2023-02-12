nhh3 は HTTP/3 通信用の Unity 向けアセットです。
[quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ層である qwfs と、 qwfs を用いた C# 製の HTTP/3 クライアントライブラリである nhh3 から成ります。


# nhh3

nhh3 は Unity で HTTP/3 通信を行う事を目的とした Unity アセットです。
``nhh3`` ディレクトリを指定して Unity でプロジェクトを開くか、そのままインポートしてください。

実行には qwfs のビルドが必要です。
後述する手順で必ず qwfs のビルドを行ってください。
qwfs のビルドが成功すると ``nhh3/Assets/nhh3/Plugins/Windows`` フォルダに quiche.dll 及び qwfs.dll が自動的にコピーされるので、 dll の手動コピーは不要です。


## 注意事項

nhh3 は Unity で HTTP/3 の通信を行う実験的な実装です。
製品への組み込みは推奨しません。


## 既知の問題・制限事項

- quiche のデバッグログ出力には非対応です
- 301 等でコンテンツが空の際に正しく処理が行われません
- ファイル名や qlog 保存のパス等に日本語のファイルパスを指定するとクラッシュします
- 最適化は一切施していません
- その他ソースコード内で todo とコメントがある個所は修正予定です
- HTTP/3 のみに対応しています
    - この為、 alt-svc によるアップグレードには対応していません

## サンプルシーン

- SingleRequestSample
    - 1 つの HTTP/3 リクエストを実行するサンプルです
- MultiRequestSample
    - Download File Num で設定した分並列に HTTP/3 リクエストを実行するサンプルです
    - デフォルトは 128 並列でダウンロードを行います

ゲームオブジェクト Http3SharpHost にて通信先の URL やポート等設定可能ですので、試したい相手先によって変更を行ってください。


# qwfs

qwfs (quiche wrapper for sharp) は [quiche](https://github.com/cloudflare/quiche) を Unity に組み込み易くラッピングした C/C++ 製のライブラリです。
nhh3 の利用に特化した仕様で実装されているので単体での使用は非推奨です。

#### ビルド方法

Visual Studio 2022 が必要です。
``External/quiche`` を quiche の submodule 設定にしてありますので、ここで quiche をビルドしてから ``Example/QuicheWindowsSample/QuicheWindowsSample.sln`` を使いビルドを行ってください。

quiche の .lib/.dll ビルド方法は以下を参照してください。  
[Re: C#(Unity)でHTTP/3通信してみる その壱 ～OSSの選定からビルドまで～](https://qiita.com/takehara-ryo/items/1f3e19b54bd6fffcba77)
ちなみに、 quiche 側で対応が入った為、 BoringSSL の個別ビルドは不要になりました。
最新版の cmake, nasm をインストールして、パスを通せばそのまま quiche のビルドが成功します。
ビルド時のコマンドは ``cargo build --features ffi --release`` を使用してください。
qwfs からはデフォルトの quiche のビルドパスを参照するので、ビルドを行った後の設定は不要です。


# QuicheWindowsSample

[quiche](https://github.com/cloudflare/quiche) の example を Windows 移植したものです。
元としている quiche の example コードのバージョンは 5a671fd66d913b0d247c9213a4ef68265277ea0d です。
詳細は以下の記事を参照してください。
[Re: C#(Unity)でHTTP/3通信してみる その弐 ～Windowsでquicheのサンプルを動かしてみる～](https://qiita.com/takehara-ryo/items/4cbb5c291b2e94b4eafd)
※当サンプルは今後メンテナンスされない可能性があります

ビルドには nhh3 と同様に quiche のビルドが必要です。
``External/quiche`` を quiche の submodule 設定にしてありますので、ここで quiche をビルドしてから ``qwfs/qwfs.sln`` を使いビルドを行ってください。


# 注意事項

現在の最新バージョンは 0.2.0 です。
0.2.0 は実験的実装であり、インターフェースや仕様も仮のものです。
今後、破壊的な変更を伴う大幅な修正が加わる事があります。
利用時にはご注意ください。


# 対応プラットフォーム

バージョン 0.1.0 では Windows にのみ対応しています。
将来的には Android に対応予定です。


# バージョン関連情報

- 動作確認した Unity バージョン : 2022.1.23f1
- quiche : 5a671fd66d913b0d247c9213a4ef68265277ea0d
- boringssl : quiche の submodule の バージョン に準ずる


# License
This software is released under the MIT License, see LICENSE.