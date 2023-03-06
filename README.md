nhh3 は HTTP/3 通信用の Unity 向けアセットです。<br>
[cloudflare/quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ層である qwfs と、 qwfs を用いた C# 製の HTTP/3 クライアントライブラリである nhh3 から成ります。<br>
Windows (x64)、Android (arm64-v8a) に対応しています。<br>

# nhh3

nhh3 は Unity で HTTP/3 通信を行う事を目的とした Unity アセットです。<br>
``nhh3`` ディレクトリを指定して Unity でプロジェクトを開くか、そのままインポートしてください。


## 注意事項

nhh3 は Unity で HTTP/3 の通信を行う実験的な実装です。<br>
製品への組み込みは推奨しません。
現在の最新バージョンは 0.2.0 です。
0.2.0 は実験的実装であり、インターフェースや仕様も仮のものです。
今後、破壊的な変更を伴う大幅な修正が加わる事があります。
利用時にはご注意ください。


## Android で動作させる場合の補足

- Nhh3.ConnectionOptions.CaCertsList に信頼された認証局リストファイルのパスを指定する必要があります
    - サンプルの実装 Nhh3SampleCore.cs - CreateHttp3() 内の実装を参考にしてください
- ``armeabi-v7a`` で動かしたい場合はビルド用 bat を修正し qwfs 及び quiche のビルドを実行してください
    - Bat\build_qwfs_android.bat の ``arm64-v8a`` を ``armeabi-v7a`` に、NDK バージョンを 19 に変更すれば動く想定です(未確認)


## 既知の問題

- 修正予定の問題
    - 時々通信が固まったまま帰ってこなくなることがあります
    - abort 時に勝手にリトライすることがあります
    - 301 等でコンテンツが空の際に正しく処理が行われません
- 修正が未定の問題
    - ファイル名や qlog 保存のパス等に日本語のファイルパスを指定するとクラッシュします
    - ソースコード内で todo とコメントがある個所はまったりと修正予定です
    - 最適化については現状ノータッチです


## 制限事項

- HTTP/3 のみに対応しています。 HTTP/1.1 及び HTTP/2 では通信できません
    - この為、 alt-svc によるアップグレードには対応していません
- iOS への対応予定はありません


## サンプルシーン

- SingleRequestSample
    - 1 つの HTTP/3 リクエストを実行するサンプルです
- MultiRequestSample
    - Download File Num で設定した分並列に HTTP/3 リクエストを実行するサンプルです
    - デフォルトは 128 並列でダウンロードを行います

ゲームオブジェクト Http3SharpHost にて通信先の URL やポート等設定可能ですので、試したい相手先によって変更を行ってください。<br>
デフォルト値には cloudflare さんの HTTP/3 対応ページである cloudflare-quic.com を利用させて頂いています。


### サンプルシーンの注意事項

- Android
    - 信頼された認証局リストとして [Mozilla の 2023-01-10 版](https://curl.se/docs/caextract.html) を使用しています
        - StreamingAssets に配置してあります
        - apk からの展開と再配置については Nhh3SampleCore.cs - CreateHttp3() 内の実装を参考にしてください
    - MultiRequestSample のファイル保存パスは ``Application.temporaryCachePath}\save`` に固定です
        - 変更したい場合は Nhh3SampleMulti.cs - OnStartClick() の実装を修正してください
- 共通
    - 前述した通り alt-svc によるアップグレードには対応していないので、通信を行うサイトの HTTP/3 対応の方法には留意してください


# qwfs

qwfs (quiche wrapper for sharp) は [cloudflare/quiche](https://github.com/cloudflare/quiche) を Unity に組み込み易くラッピングした C/C++ 製のライブラリです。<br>
nhh3 の利用に特化した仕様で実装されているので単体での使用は非推奨です。


## ビルド方法


### 準備

1. quiche の展開
    - ``External/quiche`` を quiche の submodule 設定にしてありますので、まずは ``git submodule update --init --recursive`` 等で quiche を展開してください
    - quiche 内で更に boringssl が submodule 設定されているのでそちらも展開漏れがないようご注意ください
2. cmake をインストールしてパスを通す
    - 動作確認を行ったバージョンは 3.26.0-rc2 です
    - ※後日 nasm, ninhja 同様に明示的にパスを指定する形式に変更する予定です
3. nasm をダウンロードして展開する
    - パスを通す必要はありません(ビルド時に明示的に指定)
    - 動作確認を行ったバージョンは 2.16.01 です
4. (Android 限定) ninja をダウンロードして展開する
    - パスを通す必要はありません(ビルド時に明示的に指定)
    - 動作確認を行ったバージョンは 1.11.1 です


### ビルド用 bat について

Windows 環境のみでビルド可能な bat を用意してあります。<br>
(quiche, boringssl も一緒にビルドします)<br>
以下の手順で実行してください。

- Windows
    - ``Bat\build_qwfs_windows.bat`` に以下の引数を指定して実行
        - 第一引数 : ``nasm.exe`` が配置してあるパス
        - 第二引数 : VS 2022 の ``VsMSBuildCmd.bat`` が配置してあるパス
        - 第三引数 : ビルドコンフィギュレーション。 ``release`` 指定時にはリリースビルドを、それ以外の際はデバッグビルドを行います
    - 例
        - ``$ build_qwfs_windows.bat D:\develop_tool\nasm-2.16.01 "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools" release``
- Android
    - ``Bat\build_qwfs_android.bat`` に以下の引数を指定して実行
        - 第一引数 : ``nasm.exe`` が配置してあるパス
        - 第二引数 : VS 2022 の ``VsMSBuildCmd.bat`` が配置してあるパス
        - 第三引数 : ``ninja.exe`` が配置してあるパス
        - 第四引数 : ビルドコンフィギュレーション。 ``release`` 指定時にはリリースビルドを、それ以外の際はデバッグビルドを行います
    - 例
        - ``$ build_qwfs_android.bat D:\develop_tool\nasm-2.16.01 "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools" G:\develop\ninja\ninja.exe release``
    - 補足
        - ``cargo build`` のみだと boringssl のビルドが通らなかった為、 boringssl は cmake で個別にビルドを行っています
        - また、個別にビルドを行った boringssl を参照する為に ``quiche\Cargo.toml`` を改変してあります


# 依存する OSS とバージョン情報

- quiche : 5a671fd66d913b0d247c9213a4ef68265277ea0d
- boringssl : quiche の submodule の バージョン に準ずる


# 動作確認関連情報

- 動作確認した Unity バージョン : 2022.1.23f1


# License
Copyright(C) Ryo Takehara
See [LICENSE](https://github.com/TakeharaR/nhh3/blob/master/LICENSE) for more infomation.