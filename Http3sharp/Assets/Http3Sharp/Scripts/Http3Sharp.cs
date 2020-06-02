using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Http3Sharp
{
    #region Enum Definitions
    /// <summary>
    ///     
    /// </summary>
    public enum Status
    {
        /// <summary>
        ///     メモリ不足等の継続実行不可能な致命的なエラーが発生しています.
        ///     Status がこの値以下になった場合はインスタンスを再生成してください.
        /// </summary>
        CriticalError = -100,

        /// <summary>
        ///     Native ライブラリ内での致命的エラーの発生.
        /// </summary>
        CriticalErrorQuicheInternal = -101,


        /// <summary>
        ///     継続可能なエラーが発生しています.
        ///     当エラー発生時には全てのリクエストが停止状態となります。
        ///     Status がこの値以下の(且つ CriticalError より大きい)場合は、 Retry メソッドを呼び出すことにより、停止状態となっているリクエストの再実行を行う事ができます.
        /// </summary>
        Error = -1,

        /// <summary>
        ///     ソケットの生成に失敗.
        /// </summary>
        ErrorSocket = -2,

        /// <summary>
        ///     DNS 解決に失敗.
        /// </summary>
        ErrorResolveHost = -3,

        /// <summary>
        ///     タイムアウトが発生.
        /// </summary>
        ErrorTimeout = -4,

        /// <summary>
        ///     サーバによりコネクションが閉じられた.
        /// </summary>
        ErrorConnectionClose = -5,

        /// <summary>
        ///     サーバから不正なデータが送信された(トランスポート層).
        /// </summary>
        ErrorInvalidServer = -6,

        /// <summary>
        ///     サーバから不正なデータが送信された(HTTP 層).
        /// </summary>
        ErrorInvalidResponse = -7,

        /// <summary>
        ///     TKS ハンドシェイクに失敗.
        /// </summary>
        ErrorTlsFaile = -8,

        /// <summary>
        ///     不明なネットワークエラーが発生.
        /// </summary>
        ErrorNetwork = -9,

        /// <summary>
        ///     ファイル操作関連に失敗.
        /// </summary>
        ErrorFileIO = -30,

        /// <summary>
        ///     正常状態(通信処理無し).
        /// </summary>
        Wait = 0,

        /// <summary>
        ///     正常状態(通信処理中).
        /// </summary>
        Connecting = 1,

        /// <summary>
        ///     正常状態(アボート処理中).
        /// </summary>
        Aborting = 3,
    }


    /// <summary>
    ///     輻輳制御アルゴリズム.
    /// </summary>
    enum CcType
    {
        NewReno,
        Cubic,
    };
    #endregion


    #region Constants
    /// <summary>
    ///     see : QuicOptions.MaxUdpPayloadSize
    ///     この値を利用するかは実際に利用するサーバ、経路やプロジェクトの都合に合わせて決定してください.
    /// </summary>
    public const ulong DefaultMaxUdpPayloadSize = 65527;

    /// <summary>
    ///     see : QuicOptions.InitialMaxDataSize
    ///     この値を利用するかは実際に利用するサーバ、経路やプロジェクトの都合に合わせて決定してください.
    /// </summary>
    public const ulong DefaultInitialMaxDataSize = 1024 * 1024;    // とりあえず 1MB

    /// <summary>
    ///     see : QuicOptions.InitialMaxStreamsBidi, QuicOptions.InitialMaxStreamsUni
    ///     この値を利用するかは実際に利用するサーバ、経路やプロジェクトの都合に合わせて決定してください.
    /// </summary>
    public const ulong DefaultMaxStreamSize = 128;
    #endregion


    #region Callback Definisions
    /// <summary>
    ///     Http3Sharp 及びその下層ライブラリである qwfs のデバッグログ出力を繋ぐための delegat.
    /// </summary>
    public delegate void DebugLogCallback(string message);
    #endregion


    #region Option Paramaters
    /// <summary>
    ///     HTTP/3 関連の設定を格納します.
    ///     各パラメータの詳細については HTTP/3 の Draft を参照してください.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class Http3Options
    {
        /// <summary>
        ///     HTTP/3 パラメータ "SETTINGS_MAX_HEADER_LIST_SIZE" (ヘッダリストに登録できるヘッダの最大数) の設定.
        ///     仕様的には 0 設定で無制限ですが、現状 0 を設定するとヘッダが返ってこなくなるので適切な値を設定してください.
        /// </summary>
        public ulong MaxHeaderListSize { get; set; } = 1024;

        /// <summary>
        ///     HTTP/3 パラメータ "SETTINGS_QPACK_MAX_TABLE_CAPACITY" (QPACK の動的テーブルの最大値) の設定.
        ///     ※こちらは 0 設定時の挙動は未確認です(0 でも通信は可能な所まで確認).
        /// </summary>
        public ulong QpackMaxTableCapacity { get; set; } = 0;

        /// <summary>
        ///     HTTP/3 パラメータ "SETTINGS_QPACK_BLOCKED_STREAMS" (ブロックされる可能性のあるストリーム数) の設定.
        ///     ※こちらは 0 設定時の挙動は未確認です(0 でも通信は可能な所まで確認).
        /// </summary>
        public ulong QpackBlockedStreams { get; set; } = 0;
    }

    /// <summary>
    ///     QUIC (トランスポートパラメータ) 関連の設定を格納します.
    ///     各パラメータの詳細については QUIC の Draft を参照してください.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public class QuicOptions
    {
        /// <summary>
        ///     トランスポートパラメータ "disable_active_migration" (コネクションマイグレーションの無効) の設定.
        ///     コネクションマイグレーションに対応していないサーバと通信する場合は true に設定してください.
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool DisableActiveMigration = false;

        /// <summary>
        ///     0-RTT を受け入れるかどうかの設定.
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool EnableEarlyData = true;

        /// <summary>
        ///     輻輳制御アルゴリズムの設定.
        /// </summary>
        CcType Cc { get; set; } = CcType.Cubic;

        /// <summary>
        ///     トランスポートパラメータ "max_udp_payload_size" (UDP パケット最大サイズ) の設定.
        ///     1200 - 65527 の値が設定可能です.
        /// </summary>
        public ulong MaxUdpPayloadSize { get; set; } = DefaultMaxUdpPayloadSize;

        /// <summary>
        ///     トランスポートパラメータ "max_idle_timeout" の設定.
        ///     単位は ms です.
        ///     0 を指定すると無制限になります.
        /// </summary>
        public ulong MaxIdelTimeout { get; set; } = 10UL * 1000UL;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_data" (コネクションに対する上限サイズ) の設定.
        /// </summary>
        public ulong InitialMaxData { get; set; } = DefaultInitialMaxDataSize;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_stream_data_bidi_local" (ローカル始動の双方向ストリームの初期フロー制御値) の設定.
        /// </summary>
        public ulong InitialmaxStreamDataBidiLocal { get; set; } = DefaultInitialMaxDataSize;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_stream_data_bidi_remote" (ピア始動の双方向ストリームの初期フロー制御値) の設定.
        /// </summary>
        public ulong InitialMaxStreamDataBidiRemote { get; set; } = DefaultInitialMaxDataSize;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_stream_data_uni" (単方向ストリームの初期フロー制御値) の設定.
        /// </summary>
        public ulong InitialMaxStreamDataUni { get; set; } = DefaultInitialMaxDataSize;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_streams_bidi" (作成可能な双方向ストリームの最大値) の設定.
        ///     クライアントからの作成最大値ではなく、サーバ側からの最大値であることに注意してください.
        ///     つまり、リクエストの最大数の制御には利用できません.
        /// </summary>
        public ulong InitialMaxStreamsBidi { get; set; } = DefaultMaxStreamSize;

        /// <summary>
        ///     トランスポートパラメータ "initial_max_streams_uni" (作成可能な単方向ストリームの最大値) の設定.
        /// </summary>
        public ulong InitialMaxStreamsUni { get; set; } = DefaultMaxStreamSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    /// <summary>
    ///     コネクション全体に関わるオプション設定を格納します.
    /// </summary>
    public class ConnectionOptions
    {
        /// <summary>
        ///     証明書の検証の有無.
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool VerifyPeer = true;

        /// <summary>
        ///     信頼された証明書リスト(証明書ストア) をファイルから設定する際のパス.
        ///     現在この値は使用できません (quiche が対応次第利用可能になる予定).
        /// </summary>
        [MarshalAs(UnmanagedType.LPStr)]
        public string CaCertsList = string.Empty;

        /// <summary>
        ///     qlog を使用する場合に指定するパス.
        ///     指定したパス内に qwfs_{hostname}_YYMMHHMMSS.qlog の名前で保存されます.
        /// </summary>
        [MarshalAs(UnmanagedType.LPStr)]
        public string QlogPath = string.Empty;

        /// <summary>
        ///     同時に送信可能なリクエストの最大値です.
        ///     実際のリクエスト最大値は、この値とサーバから送信されてくる initial_max_streams_bidi 及び MAX_STREAMS の値から決定されます.
        ///     最大限に並列ダウンロードの効率を上げたい場合はサーバサイドの設定と合わせてこの値を調整してください.
        /// </summary>
        public ulong MaxConcurrentStreams = 128;

        public Http3Options H3 { get; set; } = new Http3Sharp.Http3Options();

        public QuicOptions Quic { get; set; } = new Http3Sharp.QuicOptions();
    }
    #endregion


    #region Requests
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    /// <summary>
    ///     Header に関する情報を格納する.
    /// </summary>
    public struct Headers
    {
        /// <summary>
        ///     Header field names
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        ///     Header field value
        /// </summary>
        public string Value { get; set; }
    };

    /// <summary>
    ///     HTTP レスポンスに関連する情報が格納されます.
    ///     将来的に固有 ID の埋め込みやコールバック形式への変更等の手段で、どのリクエストに対するレスポンス化の紐づけを容易にする機構が入る予定です.
    /// </summary>
    public class ResponseParamaters
    {
        /// <summary>
        ///     see : Headers
        /// </summary>
        public Headers[] Headers { get; set; } = null;

        /// <summary>
        ///     HTTP レスポンスボディ.
        ///     RequestParamaters.SaveFilePath でファイルパスを設定しなかった場合に値が格納されます.
        /// </summary>
        public byte[] Body { get; set; } = null;

        /// <summary>
        ///     HTTP レスポンスボディをファイル保存する場合のパス(フルパス).
        ///     RequestParamaters.SaveFilePath で指定した値が格納されます.
        /// </summary>
        public string SaveFilePath { get; set; } = string.Empty;

        /// <summary>
        ///     HTTP レスポンスコード.
        ///     レスポンスコードの取得に失敗した場合は 0 が格納されます.
        /// </summary>
        public ulong ResponseCode { get; set; } = 0;

        /// <summary>
        ///     リクエストの最終ステータス.
        ///     Update で取得できるコネクション単位のステータスではなく、リクエスト(ストリーム)単位でのステータスが格納されます.
        ///     ResponseCode が 0 の際にはこの値を参照してください.
        /// </summary>
        public Status Status { get; set; } = Status.Wait;

        /// <summary>
        ///     リクエストで発生しているエラーの詳細.
        ///     Status がエラーの場合はこの値から詳細情報を取得してください.
        /// </summary>
        public string ErrorMessage { get; set; } = string.Empty;
    }

    /// <summary>
    ///     HTTP リクエストに関連する情報を格納してください.
    /// </summary>
    public class RequestParamaters
    {
        /// <summary>
        ///     リクエストしたい URI の path.
        ///     query, fragment を指定したい場合はこの値に含めてください.
        /// </summary>
        public string Path { get; set; } = string.Empty;

        /// <summary>
        ///     リクエスト結果(レスポンスボディ)をファイル保存したい場合のパス(フルパス).
        ///     この値が空の場合はメモリに保存されたレスポンスボディが ResponseParamaters.Body に格納されます.
        /// </summary>
        public string SaveFilePath { get; set; } = string.Empty;

        /// <summary>
        ///     see : Headers
        /// </summary>
        public Headers[] Headers { get; set; } = new Headers[0];
    }

    /// <summary>
    ///     現在使用できません.
    /// </summary>
    public class GetParamaters : RequestParamaters
    {
    }

    /// <summary>
    ///     現在使用できません.
    /// </summary>
    public class PostParamaters : RequestParamaters
    {
        public byte[] Data { get; set; }
    }
    #endregion


    #region Properties
    /// <summary>
    ///     Pseudo-Header の ":authority" に登録される値.
    ///     Http3Sharp のコンストラクタで渡した hostName 及び port が結合された値が格納されています.
    /// </summary>
    public string Authority => http3SharpImpl.Authority;
    #endregion


    #region Static Functions
    /// <summary>
    ///     Http3Sharp が依存する Native ライブラリの初期化処理を行います.
    ///     アプリケーション起動時に必ず呼び出してください.
    ///     重複呼び出し時には内部的に Uninitialize が呼び出され、全てのインスタンスが一度破棄されます.
    /// </summary>
    public static void Initialize()
    {
        Http3SharpImpl.Initialize();
    }

    /// <summary>
    ///     作成された全てのホストの Http3Sharp インスタンスが破棄されます.
    ///     Unity Editor 終了時等、アプリケーション終了時に必ず呼び出してください.
    /// </summary>
    public static void Uninitialize()
    {
        Http3SharpImpl.Uninitialize();
    }

    /// <summary>
    ///     Http3Sharp 及び下層で利用している Native ライブラリである qwfs のログ出力先を設定します.
    ///     インスタンス別ではなく、全てで共通の設定なので注意してください.
    ///     現状 Initialize 呼び出し前に呼び出す必要があります.
    ///     ※将来的にはインスタンス単位の設定や通信中にも切り替え可能になる予定です.
    /// </summary>
    /// <param name="debug">ログの出力を行う delegate.</param>
    public static void SetDebugLogCallback(DebugLogCallback debug)
    {
        Http3SharpImpl.DebugLog = debug;
    }
    #endregion


    #region Functions
    /// <summary>
    ///     Http3Sharp インスタンスの作成を行います.
    ///     インスタンスの作成後、 Http3Sharp は指定したホストに対して実行スレッドを別途作成し、ハンドシェイクを開始します.
    ///     ハンドシェイクの成否を監視する為、リクエストを発行する前から Update を呼び出し status をの値を確認してください.
    /// </summary>
    /// <param name="hostName">この値と port を合わせた Authority により Http3Sharp の Native インスタンスは管理されます.重複する HostName を登録しようとした場合は例外が送出されます.</param>
    /// <param name="port">この値を基に Http3Sharp の Native インスタンスが管理されます.重複する HostName を登録しようとした場合は例外が送出されます.</param>
    /// <param name="options">see : ConnectionOptions</param>
    public Http3Sharp(string hostName, string port, ConnectionOptions options)
    {
        http3SharpImpl = new Http3SharpImpl(hostName, port, options);
    }

    /// <summary>
    ///     Http3Sharp インスタンスを破棄します.
    ///     Http3Sharp は裏でスレッド動作を行う為、当処理を必ず呼び出し、スレッド・リソースの解放を行ってください.
    /// </summary>
    public void Destroy()
    {
        http3SharpImpl.Destroy();
    }

    /// <summary>
    ///     Http3Sharp の実行スレッドをロックし、終了したリクエストを取得します.
    ///     また、同時にステータスやプログレスも取得します.
    ///     ※ロックの回数を減らす為に一度に多くの処理を実行しています
    /// </summary>
    /// <param name="status">インスタンス全体のステータス</param>
    /// <param name="progress">前回 Update を呼び出してから今回呼び出すまでのプログレス値(byte)</param>
    /// <param name="totalWriteSize">現在通信中のリクエスト全体が今まで通信を行った値(byte).前フレームまでに通信が完了したものについては除外されるので注意してください.</param>
    /// <returns></returns>
    public List<ResponseParamaters> Update(out Status status, out ulong progress, out ulong totalWriteSize)
    {
        return http3SharpImpl.Update(out status, out progress, out totalWriteSize);
    }

    /// <summary>
    ///     HTTP リクエストを発行します.
    ///     通信中にも発行は可能です.
    /// </summary>
    /// <param name="requests">see RequestParamaters</param>
    public void PublishRequest(IEnumerable<RequestParamaters> requests)
    {
        http3SharpImpl.PublishRequest(requests);
    }

    /// <summary>
    ///     エラー状態になっているライブラリを再開します.
    ///     see : Status.Error
    /// </summary>
    public void Retry()
    {
        http3SharpImpl.Retry();
    }

    /// <summary>
    ///     通信中の処理を中断、もしくはエラー状態のライブラリの保持するリクエストを全て破棄します.
    ///     Abort された Http3Sharp インスタンスはハンドシェイクからやり直しを行います.
    ///     再度 PublishRequest を呼び出すことによりリクエストの再開が可能です.
    /// </summary>
    public void Abort()
    {
        http3SharpImpl.Abort();
    }

    /// <summary>
    ///     Http3Sharp インスタンス全体のエラー詳細を取得します.
    /// </summary>
    public string GetErrorDetail()
    {
        return http3SharpImpl.GetErrorDetail();
    }
    #endregion


    #region Private Module
    private readonly Http3SharpImpl http3SharpImpl = null;
    #endregion
}
