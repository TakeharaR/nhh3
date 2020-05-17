# http3sharp

http3sharp は現在作成中の C# 製の HTTP/3 クライアントライブラリです。


### 現在作成済みのモジュール

- [quiche](https://github.com/cloudflare/quiche) の example の Windows 移植


### 将来的に作成予定のモジュール

- [quiche](https://github.com/cloudflare/quiche) の C/C++ ラッパ
- [quiche](https://github.com/cloudflare/quiche) の C# ラッパ(上記 C/C++ を利用)


# QuicheWindowsSample

``Example\QuicheWindowsSample`` は [quiche](https://github.com/cloudflare/quiche) の example を Windows 向けに動作可能に修正したものです。  
元としている example コードのバージョンは 0.4.0 です。  
対応している HTTP/3, QUIC の draft バージョンは 27 です。

### ビルド方法

Visual Studio 2019 が必要です。
``External\quiche`` を quiche の submodule 設定にしてありますので、ここで quiche をビルドしてから Example のビルドを行ってください。

quiche の .lib/.dll ビルド方法は以下を参照してください。  
[Re: C#(Unity)でHTTP/3通信してみる その壱 〜OSSの選定からビルドまで〜](https://qiita.com/takehara-ryo/items/1f3e19b54bd6fffcba77)


# License
This software is released under the MIT License, see LICENSE.