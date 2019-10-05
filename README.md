# http3sharp

http3sharp は現在作成中の C# 製の HTTP/3 クライアントライブラリです。


### 現在作成済みのモジュール

- [quiche](https://github.com/cloudflare/quiche) の example の Windows 移植


### 将来的に作成予定のモジュール

- [quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ
- [quiche](https://github.com/cloudflare/quiche) の C# ラッパ(上記 C/C++ を利用)


# WindowsNativeClient

``Example\WindowsNativeClient`` は [quiche](https://github.com/cloudflare/quiche) の example を Windows 向けに動作可能に修正したものです。
元としている example コードのバージョンは ``d646a60b497be4a92046fbd705161164388b73d2`` です。
対応している HTTP/3, QUIC の draft バージョンは 23 です。

### ビルド方法

Visual Studio 2019 が必要です。
``Example\WindowsNativeClient\external\quiche`` に以下を配置して ``QuicheWrapperExample.sln`` でビルドしてください。

- ``quiche.dll``
- ``quiche.dll.lib``

quiche の .lib/.dll ビルド方法は以下を参照してください。
[【HTTP/3】C#でHTTP/3通信してみる その壱 ～OSSの選定からquicheの.lib/.dllビルドまで～](https://qiita.com/takehara-ryo/items/93a7885a3193796e5a7b)


# License
This software is released under the MIT License, see LICENSE.