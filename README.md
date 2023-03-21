nhh3 は HTTP/3 通信用の Unity 向けアセットです。<br>
[cloudflare/quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ層である qwfs と、 qwfs を用いた C# 製の HTTP/3 クライアントライブラリである nhh3 から成ります。<br>
Windows (x64)、Android (arm64-v8a) に対応しています。<br>

# nhh3

nhh3 は Unity で HTTP/3 通信を行う事を目的とした Unity アセットです。<br>
``nhh3`` ディレクトリを指定して Unity でプロジェクトを開くか、そのままインポートしてください。


## 注意事項

nhh3 は Unity で HTTP/3 通信を行う実験的な実装であり、製品への組み込みは推奨しません。<br>
更に最新バージョンの 0.3 系のインターフェースや仕様も仮のものです。<br>
今後、破壊的な変更を伴う大幅な修正が加わる事があります。利用時にはご注意ください。<br>


## Android で動作させる場合の補足

- Nhh3.ConnectionOptions.CaCertsList に信頼された認証局リストファイルのパスを指定する必要があります
    - サンプルの実装 Nhh3SampleCore.cs - CreateHttp3() 内の実装を参考にしてください
- ``armeabi-v7a`` で動かしたい場合はビルド用 bat を修正し qwfs 及び quiche のビルドを実行してください
    - Bat\build_qwfs_android.bat の ``arm64-v8a`` を ``armeabi-v7a`` に、NDK バージョンを 19 に変更すれば動く想定です(未確認)


## 対応している主な機能と制限事項

- qlog
    - Windows/Android にて動作確認済み
- Connection Migration
    - ``PATH_CHALLENGE`` へのレスポンスが返ってこず、 Connection Migration に失敗することがあります
- QPACK
    - Windows/Android にて動作確認済み
    - Dyanamic Tabel には非対応です (quiche の制限事項)


## ライブラリの使用方法と注意事項

基本的な使い方については ``unity\Assets\nhh3\Scripts\Nhh3Sharp.cs`` のリファレンスを参照してください。
以下、組み込み時の注意事項です。

- 各種パラメータについて
    - 通信を行うサーバによって要求値が異なる為、以下の値のデフォルト値は余裕を持って設定をしています
        - ``QuicOptions.MaxConcurrentStreams``
        - ``Http3Options.MaxHeaderListSize``
        - ``Http3Options.QpackBlockedStreams``
    - その他のパラメータも環境、対象によって適切な値が異なりますので、都合に合わせて設定してください
    - パラメータが適切でない場合、通信が途中で進行しなくなることがあります
        - qlog/qviz で内容を確認し、ヘッダ関連のエラーの場合は ``Http3Options`` の設定を疑いましょう
- Connection Migration
    - ``QuicOptions.DisableActiveMigration`` で制御可能です
    - 上記を設定した上で、サーバが対応している場合にのみ IP/Port 変更時に Connection Migration による通信が行われます
        - ``QuicOptions.DisableActiveMigration`` が有効にもかかわらずサーバが対応していない場合はハンドシェイクから自動で再実行されます
    - Windows で試す場合には ``nhh3.Reconnect `` を通信途中に挟むのが簡単です
    - Android で試す場合には Wi-Fi とキャリアの通信の切り替えを行ってみてください
    - 簡単な確認の流れ : サンプル再生 → ダウンロードボタンを押す → 完了まで待つ → ``nhh3.Recconect`` or Wi-FI とキャリア通信の切り替え → ダウンロードボタンを押す
- 0-RTT
    - ``ConnectionOptions.WorkPath`` を設定した上でコネクション毎に ``QuicOptions.EnableEarlyData`` で制御が可能です
        - ``ConnectionOptions.WorkPath`` に指定したパスに 0-RTT 用のセッション情報を保存したファイルが保存されます
    - 上記を設定した上で、サーバが対応している場合にのみ 0-RTT による通信が行われます
- 詳細なログが必要な場合
    - ``ConnectionOptions.EnableQuicheLog`` を有効にすると quiche のログが ``nhh3.SetDebugLogCallback`` に設定したコールバックに出力されます
    - 

## 既知の問題

- 修正予定の問題
    - 301 等でコンテンツが空の際に正しく処理が行われません
    - プログレスの数字が合わなくなることがあります
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
    - デフォルトは 64 並列でダウンロードを行います

ゲームオブジェクト Http3SharpHost にて通信先の URL やポート等設定可能ですので、試したい相手先によって変更を行ってください。<br>
デフォルト値には cloudflare さんの HTTP/3 対応ページを利用させて頂いています。


### サンプルシーンの注意事項

- Android
    - 信頼された認証局リストとして [Mozilla の 2023-01-10 版](https://curl.se/docs/caextract.html) を使用しています
        - StreamingAssets に配置してあります
        - apk からの展開と再配置については Nhh3SampleCore.cs - CreateHttp3() 内の実装を参考にしてください
    - MultiRequestSample のファイル保存パスは ``Application.temporaryCachePath}\save`` に固定です
        - 変更したい場合は Nhh3SampleMulti.cs - OnStartClick() の実装を修正してください
- 共通
    - コネクション自体がエラーになった場合にはリトライ、アボートボタンで復旧できません。アプリを再起動してください
    - Windows で cloudflare-quic.com に接続する際には多重化が効かないようです
        - Windows で多重化の確認をしたい場合は www.facebook.com 等別のサイトで試してみてください

# qwfs

qwfs (quiche wrapper for sharp) は [cloudflare/quiche](https://github.com/cloudflare/quiche) を Unity に組み込み易くラッピングした C/C++ 製のライブラリです。<br>
nhh3 の利用に特化した仕様で実装されているので単体での使用は非推奨です。


## ビルド方法

Windows 環境のみでビルド可能な bat を用意してあります。<br>
(quiche, boringssl も一緒にビルドします)<br>
以下の手順で実行してください。

### 準備

1. quiche の展開
    - ``External/quiche`` を quiche の submodule 設定にしてありますので、まずは ``git submodule update --init --recursive`` 等で quiche を展開してください
    - quiche 内で更に boringssl が submodule 設定されているのでそちらも展開漏れがないようご注意ください
2. cmake をインストールしてパスを通す
    - 動作確認を行った cmake のバージョンは 3.26.0-rc2 です
    - ※後日 nasm, ninhja 同様に明示的にパスを指定する形式に変更する予定です
3. nasm をダウンロードして展開する
    - パスを通す必要はありません(ビルド時に明示的に指定)
    - 動作確認を行った nasm のバージョンは 2.16.01 です
4. (Android 限定) ninja をダウンロードして展開する
    - パスを通す必要はありません(ビルド時に明示的に指定)
    - 動作確認を行った ninja のバージョンは 1.11.1 です
5. git をインストールしてパスを通す


### ビルド用 bat の実行

- Windows
    - ``Bat\build_qwfs_windows.bat`` に以下の引数を指定して実行
        - 第一引数 : ビルドコンフィギュレーション。 ``release`` 指定時にはリリースビルドを、それ以外の際はデバッグビルドを行います
    - 例
        - ``$ build_qwfs_windows.bat release``
- Android
    - ``Bat\build_qwfs_android.bat`` に以下の引数を指定して実行
        - 第一引数 : ``ninja.exe`` が配置してあるパス
        - 第二引数 : ビルドコンフィギュレーション。 ``release`` 指定時にはリリースビルドを、それ以外の際はデバッグビルドを行います
    - 例
        - ``$ build_qwfs_android.bat G:\develop\ninja\ninja.exe release``
    - 補足
        - ``cargo build`` のみだと boringssl のビルドが通らなかった為、 boringssl は cmake で個別にビルドを行っています
        - また、個別にビルドを行った boringssl を参照する為に ``quiche\Cargo.toml`` を改変してあります


### quiche の改変について

Connection Migration 及び 0-RTT への対応の為、 quiche の c ラッパ層に表出していない関数を ``src\ffi.rs`` に追加してあります。
改変したファイルは ``Bat\patch`` 以下に配置して、ビルド用 bat 実行時に ``External\quiche`` に展開されます。
quiche を手動でビルドする際や quiche のバージョンを上げる際はご注意ください。


# 依存する OSS とバージョン情報

- quiche : 2183ded2b4ffcbcdedcdf595dc7e48c85e9913f3
- boringssl : quiche の submodule の バージョン に準ずる


# 動作確認関連情報

- 動作確認した Unity バージョン : 2022.1.23f1


# License
Copyright(C) Ryo Takehara<br>
See [LICENSE](https://github.com/TakeharaR/nhh3/blob/master/LICENSE) for more infomation.