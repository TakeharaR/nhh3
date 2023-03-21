using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Nhh3
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
        Bbr,
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
    public const ulong DefaultInitialMaxDataSize = 100 * 1024 * 1024;

    /// <summary>
    ///     see : QuicOptions.InitialMaxStreamsBidi, QuicOptions.InitialMaxStreamsUni
    ///     この値を利用するかは実際に利用するサーバ、経路やプロジェクトの都合に合わせて決定してください.
    /// </summary>
    public const ulong DefaultMaxStreamSize = 128;
    #endregion


    #region Callback Definisions
    /// <summary>
    ///     Nhh3 及びその下層ライブラリである qwfs のデバッグログ出力を繋ぐための delegat.
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
        ///     1024 だと足りなくなることがままあるので少し多めの設定にしています.
        /// </summary>
        public ulong MaxHeaderListSize { get; set; } = 100 * 1024;

        /// <summary>
        ///     HTTP/3 パラメータ "SETTINGS_QPACK_MAX_TABLE_CAPACITY" (QPACK の動的テーブルの最大値) の設定.
        ///     quiche が DYNAMIC テーブルには未対応の為に 0 を指定してください.
        /// </summary>
        public ulong QpackMaxTableCapacity { get; set; } = 0;

        /// <summary>
        ///     HTTP/3 パラメータ "SETTINGS_QPACK_BLOCKED_STREAMS" (ブロックされる可能性のあるストリーム数) の設定.
        ///     1024 だと足りなくなることがままあるので少し多めの設定にしています.
        /// </summary>
        public ulong QpackBlockedStreams { get; set; } = 100 * 1024;

        /// <summary>
        /// CONNECT プロトコルを使用するかどうか.
        /// 現状 HTTP/3 にのみ対応しているので、基本的には false を指定して変更しないでください.
        /// </summary>
        public bool SettingsEnableConnectProtocol { get; set; } = false;
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
        public bool DisableActiveMigration = true;

        /// <summary>
        ///     0-RTT を受け入れるかどうかの設定.
        ///     この設定を true にした上で ConnectionOptions.WorkPath にセッション情報を保存するパスを指定してください.
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool EnableEarlyData = false;

        /// <summary>
        ///     輻輳制御アルゴリズムの設定.
        /// </summary>
        CcType Cc { get; set; } = CcType.Bbr;

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

        /// <summary>
        ///     コネクションに用いる最大ウィンドウサイズ.
        /// </summary>
        public ulong MaxConnectionWindowSize { get; set; } = 24U * 1024U * 1024U;   // from quiche /src/args.rs

        /// <summary>
        ///     ストリームに用いる最大ウィンドウサイズ.
        /// </summary>
        public ulong MaxStreamWindowSize { get; set; } = 16U * 1024U * 1024U;    // from quiche /src/args.rs

        /// <summary>
        ///     アクティブなコネクション ID の上限値.
        ///     Connection Migration 等に対応する際は多めに入れておく.
        /// </summary>
        public ulong ActiveConnectionIdLimit { get; set; } = 16;
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
        ///     quiche のログ出力を行うかどうか.
        ///     出力先は DebugLogCallback です(この為 DebugLogCallback の指定がないと出力されません).
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool EnableQuicheLog = false;

        /// <summary>
        ///     信頼された証明書リスト(証明書ストア) をファイルから設定する際のパス.
        ///     .pem 形式のファイルのパスを指定してください.
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
        ///     0-RTT 用のセッションファイルなどの一時ファイルを配置するパス.
        ///     指定したパス内にコネクション毎に 0-RTT のセッションファイルを配置します.
        ///     ※将来的にはダウンロードしたデータの一時ファイルも生成予定.
        /// </summary>
        [MarshalAs(UnmanagedType.LPStr)]
        public string WorkPath = string.Empty;

        /// <summary>
        ///     同時に送信可能なリクエストの最大値です.
        ///     実際のリクエスト最大値は、この値とサーバから送信されてくる initial_max_streams_bidi 及び MAX_STREAMS の値から決定されます.
        ///     最大限に並列ダウンロードの効率を上げたい場合はサーバサイドの設定と合わせてこの値を調整してください.
        /// </summary>
        public ulong MaxConcurrentStreams = 64;

        public Http3Options H3 { get; set; } = new Nhh3.Http3Options();

        public QuicOptions Quic { get; set; } = new Nhh3.QuicOptions();
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
    ///     Nhh3 のコンストラクタで渡した hostName 及び port が結合された値が格納されています.
    /// </summary>
    public string Authority => Nhh3Impl.Authority;
    #endregion


    #region Static Functions
    /// <summary>
    ///     Nhh3 が依存する Native ライブラリの初期化処理を行います.
    ///     アプリケーション起動時に必ず呼び出してください.
    ///     重複呼び出し時には内部的に Uninitialize が呼び出され、全てのインスタンスが一度破棄されます.
    /// </summary>
    public static void Initialize()
    {
        Nhh3Impl.Initialize();
    }

    /// <summary>
    ///     作成された全てのホストの Nhh3 インスタンスが破棄されます.
    ///     Unity Editor 終了時等、アプリケーション終了時に必ず呼び出してください.
    /// </summary>
    public static void Uninitialize()
    {
        Nhh3Impl.Uninitialize();
    }

    /// <summary>
    ///     Nhh3 及び下層で利用している Native ライブラリである qwfs のログ出力先を設定します.
    ///     インスタンス別ではなく、全てで共通の設定なので注意してください.
    ///     現状 Initialize 呼び出し前に呼び出す必要があります.
    ///     ※将来的にはインスタンス単位の設定や通信中にも切り替え可能になる予定です.
    /// </summary>
    /// <param name="debug">ログの出力を行う delegate.</param>
    public static void SetDebugLogCallback(DebugLogCallback debug)
    {
        Nhh3Impl.DebugLog = debug;
    }
    #endregion


    #region Functions
    /// <summary>
    ///     Nhh3 インスタンスの作成を行います.
    ///     インスタンスの作成後、 Nhh3 は指定したホストに対して実行スレッドを別途作成し、ハンドシェイクを開始します.
    ///     ハンドシェイクの成否を監視する為、リクエストを発行する前から Update を呼び出し status をの値を確認してください.
    /// </summary>
    /// <param name="hostName">この値と port を合わせた Authority により Nhh3 の Native インスタンスは管理されます.重複する HostName を登録しようとした場合は例外が送出されます.</param>
    /// <param name="port">この値を基に Nhh3 の Native インスタンスが管理されます.重複する HostName を登録しようとした場合は例外が送出されます.</param>
    /// <param name="options">see : ConnectionOptions</param>
    public Nhh3(string hostName, string port, ConnectionOptions options)
    {
        Nhh3Impl = new Nhh3Impl(hostName, port, options);
    }

    /// <summary>
    ///     Nhh3 インスタンスを破棄します.
    ///     Nhh3 は裏でスレッド動作を行う為、当処理を必ず呼び出し、スレッド・リソースの解放を行ってください.
    /// </summary>
    public void Destroy()
    {
        Nhh3Impl.Destroy();
    }

    /// <summary>
    ///     Nhh3 の実行スレッドをロックし、終了したリクエストを取得します.
    ///     また、同時にステータスやプログレスも取得します.
    ///     ※ロックの回数を減らす為に一度に多くの処理を実行しています
    /// </summary>
    /// <param name="status">インスタンス全体のステータス</param>
    /// <param name="progress">前回 Update を呼び出してから今回呼び出すまでのプログレス値(byte)</param>
    /// <param name="totalWriteSize">現在通信中のリクエスト全体が今まで通信を行った値(byte).前フレームまでに通信が完了したものについては除外されるので注意してください.</param>
    /// <returns></returns>
    public List<ResponseParamaters> Update(out Status status, out ulong progress, out ulong totalWriteSize)
    {
        return Nhh3Impl.Update(out status, out progress, out totalWriteSize);
    }

    /// <summary>
    ///     HTTP リクエストを発行します.
    ///     通信中にも発行は可能です.
    /// </summary>
    /// <param name="requests">see RequestParamaters</param>
    public void PublishRequest(IEnumerable<RequestParamaters> requests)
    {
        Nhh3Impl.PublishRequest(requests);
    }

    /// <summary>
    ///     エラー状態になっているライブラリを再開します.
    ///     see : Status.Error
    /// </summary>
    public void Retry()
    {
        Nhh3Impl.Retry();
    }

    /// <summary>
    ///     通信中の処理を中断、もしくはエラー状態のライブラリの保持するリクエストを全て破棄します.
    ///     Abort された Nhh3 インスタンスはハンドシェイクからやり直しを行います.
    ///     再度 PublishRequest を呼び出すことによりリクエストの再開が可能です.
    /// </summary>
    public void Abort()
    {
        Nhh3Impl.Abort();
    }

    /// <summary>
    ///     強制的にネットワークの再接続を行います.
    ///     再接続時には、 QuicOptions.DisableActiveMigration が false 且つ通信対象が対応している場合のみ Connection Migration が発動します.
    /// </summary>
    public void Reconnect()
    {
        Nhh3Impl.Reconnect();
    }

    /// <summary>
    ///     Nhh3 インスタンス全体のエラー詳細を取得します.
    /// </summary>
    public string GetErrorDetail()
    {
        return Nhh3Impl.GetErrorDetail();
    }
    #endregion


    #region Private Module
    private readonly Nhh3Impl Nhh3Impl = null;
    #endregion
}
