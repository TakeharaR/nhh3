nhh3 �� HTTP/3 �ʐM�p�� Unity �����A�Z�b�g�ł��B<br>
[cloudflare/quiche](https://github.com/cloudflare/quiche) �� C/C++ ���b�p�w�ł��� qwfs �ƁA qwfs ��p���� C# ���� HTTP/3 �N���C�A���g���C�u�����ł��� nhh3 ���琬��܂��B<br>
Windows (x64)�AAndroid (arm64-v8a) �ɑΉ����Ă��܂��B<br>

# nhh3

nhh3 �� Unity �� HTTP/3 �ʐM���s������ړI�Ƃ��� Unity �A�Z�b�g�ł��B<br>
``nhh3`` �f�B���N�g�����w�肵�� Unity �Ńv���W�F�N�g���J�����A���̂܂܃C���|�[�g���Ă��������B


## ���ӎ���

nhh3 �� Unity �� HTTP/3 �ʐM���s�������I�Ȏ����ł���A���i�ւ̑g�ݍ��݂͐������܂���B<br>
�X�ɍŐV�o�[�W������ 0.3 �n�̃C���^�[�t�F�[�X��d�l�����̂��̂ł��B<br>
����A�j��I�ȕύX�𔺂��啝�ȏC��������鎖������܂��B���p���ɂ͂����ӂ��������B<br>


## Android �œ��삳����ꍇ�̕⑫

- Nhh3.ConnectionOptions.CaCertsList �ɐM�����ꂽ�F�؋ǃ��X�g�t�@�C���̃p�X���w�肷��K�v������܂�
    - �T���v���̎��� Nhh3SampleCore.cs - CreateHttp3() ���̎������Q�l�ɂ��Ă�������
- ``armeabi-v7a`` �œ����������ꍇ�̓r���h�p bat ���C���� qwfs �y�� quiche �̃r���h�����s���Ă�������
    - Bat\build_qwfs_android.bat �� ``arm64-v8a`` �� ``armeabi-v7a`` �ɁANDK �o�[�W������ 19 �ɕύX����Γ����z��ł�(���m�F)


## �Ή����Ă����ȋ@�\�Ɛ�������

- qlog
    - Windows/Android �ɂē���m�F�ς�
- Connection Migration
    - ``PATH_CHALLENGE`` �ւ̃��X�|���X���Ԃ��Ă����A Connection Migration �Ɏ��s���邱�Ƃ�����܂�
- QPACK
    - Windows/Android �ɂē���m�F�ς�
    - Dyanamic Tabel �ɂ͔�Ή��ł� (quiche �̐�������)


## ���C�u�����̎g�p���@�ƒ��ӎ���

��{�I�Ȏg�����ɂ��Ă� ``unity\Assets\nhh3\Scripts\Nhh3Sharp.cs`` �̃��t�@�����X���Q�Ƃ��Ă��������B
�ȉ��A�g�ݍ��ݎ��̒��ӎ����ł��B

- �e��p�����[�^�ɂ���
    - �ʐM���s���T�[�o�ɂ���ėv���l���قȂ�ׁA�ȉ��̒l�̃f�t�H���g�l�͗]�T�������Đݒ�����Ă��܂�
        - ``QuicOptions.MaxConcurrentStreams``
        - ``Http3Options.MaxHeaderListSize``
        - ``Http3Options.QpackBlockedStreams``
    - ���̑��̃p�����[�^�����A�Ώۂɂ���ēK�؂Ȓl���قȂ�܂��̂ŁA�s���ɍ��킹�Đݒ肵�Ă�������
    - �p�����[�^���K�؂łȂ��ꍇ�A�ʐM���r���Ői�s���Ȃ��Ȃ邱�Ƃ�����܂�
        - qlog/qviz �œ��e���m�F���A�w�b�_�֘A�̃G���[�̏ꍇ�� ``Http3Options`` �̐ݒ���^���܂��傤
- Connection Migration
    - ``QuicOptions.DisableActiveMigration`` �Ő���\�ł�
    - ��L��ݒ肵����ŁA�T�[�o���Ή����Ă���ꍇ�ɂ̂� IP/Port �ύX���� Connection Migration �ɂ��ʐM���s���܂�
        - ``QuicOptions.DisableActiveMigration`` ���L���ɂ�������炸�T�[�o���Ή����Ă��Ȃ��ꍇ�̓n���h�V�F�C�N���玩���ōĎ��s����܂�
    - Windows �Ŏ����ꍇ�ɂ� ``nhh3.Reconnect `` ��ʐM�r���ɋ��ނ̂��ȒP�ł�
    - Android �Ŏ����ꍇ�ɂ� Wi-Fi �ƃL�����A�̒ʐM�̐؂�ւ����s���Ă݂Ă�������
    - �ȒP�Ȋm�F�̗��� : �T���v���Đ� �� �_�E�����[�h�{�^�������� �� �����܂ő҂� �� ``nhh3.Recconect`` or Wi-FI �ƃL�����A�ʐM�̐؂�ւ� �� �_�E�����[�h�{�^��������
- 0-RTT
    - ``ConnectionOptions.WorkPath`` ��ݒ肵����ŃR�l�N�V�������� ``QuicOptions.EnableEarlyData`` �Ő��䂪�\�ł�
        - ``ConnectionOptions.WorkPath`` �Ɏw�肵���p�X�� 0-RTT �p�̃Z�b�V��������ۑ������t�@�C�����ۑ�����܂�
    - ��L��ݒ肵����ŁA�T�[�o���Ή����Ă���ꍇ�ɂ̂� 0-RTT �ɂ��ʐM���s���܂�
- �ڍׂȃ��O���K�v�ȏꍇ
    - ``ConnectionOptions.EnableQuicheLog`` ��L���ɂ���� quiche �̃��O�� ``nhh3.SetDebugLogCallback`` �ɐݒ肵���R�[���o�b�N�ɏo�͂���܂�
    - 

## ���m�̖��

- �C���\��̖��
    - 301 ���ŃR���e���c����̍ۂɐ������������s���܂���
    - �v���O���X�̐���������Ȃ��Ȃ邱�Ƃ�����܂�
- �C��������̖��
    - �t�@�C������ qlog �ۑ��̃p�X���ɓ��{��̃t�@�C���p�X���w�肷��ƃN���b�V�����܂�
    - �\�[�X�R�[�h���� todo �ƃR�����g��������͂܂�����ƏC���\��ł�
    - �œK���ɂ��Ă͌���m�[�^�b�`�ł�


## ��������

- HTTP/3 �݂̂ɑΉ����Ă��܂��B HTTP/1.1 �y�� HTTP/2 �ł͒ʐM�ł��܂���
    - ���ׁ̈A alt-svc �ɂ��A�b�v�O���[�h�ɂ͑Ή����Ă��܂���
- iOS �ւ̑Ή��\��͂���܂���


## �T���v���V�[��

- SingleRequestSample
    - 1 �� HTTP/3 ���N�G�X�g�����s����T���v���ł�
- MultiRequestSample
    - Download File Num �Őݒ肵��������� HTTP/3 ���N�G�X�g�����s����T���v���ł�
    - �f�t�H���g�� 64 ����Ń_�E�����[�h���s���܂�

�Q�[���I�u�W�F�N�g Http3SharpHost �ɂĒʐM��� URL ��|�[�g���ݒ�\�ł��̂ŁA�������������ɂ���ĕύX���s���Ă��������B<br>
�f�t�H���g�l�ɂ� cloudflare ����� HTTP/3 �Ή��y�[�W�𗘗p�����Ē����Ă��܂��B


### �T���v���V�[���̒��ӎ���

- Android
    - �M�����ꂽ�F�؋ǃ��X�g�Ƃ��� [Mozilla �� 2023-01-10 ��](https://curl.se/docs/caextract.html) ���g�p���Ă��܂�
        - StreamingAssets �ɔz�u���Ă���܂�
        - apk ����̓W�J�ƍĔz�u�ɂ��Ă� Nhh3SampleCore.cs - CreateHttp3() ���̎������Q�l�ɂ��Ă�������
    - MultiRequestSample �̃t�@�C���ۑ��p�X�� ``Application.temporaryCachePath}\save`` �ɌŒ�ł�
        - �ύX�������ꍇ�� Nhh3SampleMulti.cs - OnStartClick() �̎������C�����Ă�������
- ����
    - �R�l�N�V�������̂��G���[�ɂȂ����ꍇ�ɂ̓��g���C�A�A�{�[�g�{�^���ŕ����ł��܂���B�A�v�����ċN�����Ă�������
    - Windows �� cloudflare-quic.com �ɐڑ�����ۂɂ͑��d���������Ȃ��悤�ł�
        - Windows �ő��d���̊m�F���������ꍇ�� www.facebook.com ���ʂ̃T�C�g�Ŏ����Ă݂Ă�������

# qwfs

qwfs (quiche wrapper for sharp) �� [cloudflare/quiche](https://github.com/cloudflare/quiche) �� Unity �ɑg�ݍ��݈Ղ����b�s���O���� C/C++ ���̃��C�u�����ł��B<br>
nhh3 �̗��p�ɓ��������d�l�Ŏ�������Ă���̂ŒP�̂ł̎g�p�͔񐄏��ł��B


## �r���h���@

Windows ���݂̂Ńr���h�\�� bat ��p�ӂ��Ă���܂��B<br>
(quiche, boringssl ���ꏏ�Ƀr���h���܂�)<br>
�ȉ��̎菇�Ŏ��s���Ă��������B

### ����

1. quiche �̓W�J
    - ``External/quiche`` �� quiche �� submodule �ݒ�ɂ��Ă���܂��̂ŁA�܂��� ``git submodule update --init --recursive`` ���� quiche ��W�J���Ă�������
    - quiche ���ōX�� boringssl �� submodule �ݒ肳��Ă���̂ł�������W�J�R�ꂪ�Ȃ��悤�����ӂ�������
2. cmake ���C���X�g�[�����ăp�X��ʂ�
    - ����m�F���s���� cmake �̃o�[�W������ 3.26.0-rc2 �ł�
    - ����� nasm, ninhja ���l�ɖ����I�Ƀp�X���w�肷��`���ɕύX����\��ł�
3. nasm ���_�E�����[�h���ēW�J����
    - �p�X��ʂ��K�v�͂���܂���(�r���h���ɖ����I�Ɏw��)
    - ����m�F���s���� nasm �̃o�[�W������ 2.16.01 �ł�
4. (Android ����) ninja ���_�E�����[�h���ēW�J����
    - �p�X��ʂ��K�v�͂���܂���(�r���h���ɖ����I�Ɏw��)
    - ����m�F���s���� ninja �̃o�[�W������ 1.11.1 �ł�
5. git ���C���X�g�[�����ăp�X��ʂ�


### �r���h�p bat �̎��s

- Windows
    - ``Bat\build_qwfs_windows.bat`` �Ɉȉ��̈������w�肵�Ď��s
        - ������ : �r���h�R���t�B�M�����[�V�����B ``release`` �w�莞�ɂ̓����[�X�r���h���A����ȊO�̍ۂ̓f�o�b�O�r���h���s���܂�
    - ��
        - ``$ build_qwfs_windows.bat release``
- Android
    - ``Bat\build_qwfs_android.bat`` �Ɉȉ��̈������w�肵�Ď��s
        - ������ : ``ninja.exe`` ���z�u���Ă���p�X
        - ������ : �r���h�R���t�B�M�����[�V�����B ``release`` �w�莞�ɂ̓����[�X�r���h���A����ȊO�̍ۂ̓f�o�b�O�r���h���s���܂�
    - ��
        - ``$ build_qwfs_android.bat G:\develop\ninja\ninja.exe release``
    - �⑫
        - ``cargo build`` �݂̂��� boringssl �̃r���h���ʂ�Ȃ������ׁA boringssl �� cmake �ŌʂɃr���h���s���Ă��܂�
        - �܂��A�ʂɃr���h���s���� boringssl ���Q�Ƃ���ׂ� ``quiche\Cargo.toml`` �����ς��Ă���܂�


### quiche �̉��ςɂ���

Connection Migration �y�� 0-RTT �ւ̑Ή��ׁ̈A quiche �� c ���b�p�w�ɕ\�o���Ă��Ȃ��֐��� ``src\ffi.rs`` �ɒǉ����Ă���܂��B
���ς����t�@�C���� ``Bat\patch`` �ȉ��ɔz�u���āA�r���h�p bat ���s���� ``External\quiche`` �ɓW�J����܂��B
quiche ���蓮�Ńr���h����ۂ� quiche �̃o�[�W�������グ��ۂ͂����ӂ��������B


# �ˑ����� OSS �ƃo�[�W�������

- quiche : 2183ded2b4ffcbcdedcdf595dc7e48c85e9913f3
- boringssl : quiche �� submodule �� �o�[�W���� �ɏ�����


# ����m�F�֘A���

- ����m�F���� Unity �o�[�W���� : 2022.1.23f1


# License
Copyright(C) Ryo Takehara<br>
See [LICENSE](https://github.com/TakeharaR/nhh3/blob/master/LICENSE) for more infomation.