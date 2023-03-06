nhh3 �� HTTP/3 �ʐM�p�� Unity �����A�Z�b�g�ł��B<br>
[cloudflare/quiche](https://github.com/cloudflare/quiche) �� C/C++ ���b�p�w�ł��� qwfs �ƁA qwfs ��p���� C# ���� HTTP/3 �N���C�A���g���C�u�����ł��� nhh3 ���琬��܂��B<br>
Windows (x64)�AAndroid (arm64-v8a) �ɑΉ����Ă��܂��B<br>

# nhh3

nhh3 �� Unity �� HTTP/3 �ʐM���s������ړI�Ƃ��� Unity �A�Z�b�g�ł��B<br>
``nhh3`` �f�B���N�g�����w�肵�� Unity �Ńv���W�F�N�g���J�����A���̂܂܃C���|�[�g���Ă��������B


## ���ӎ���

nhh3 �� Unity �� HTTP/3 �̒ʐM���s�������I�Ȏ����ł��B<br>
���i�ւ̑g�ݍ��݂͐������܂���B
���݂̍ŐV�o�[�W������ 0.2.0 �ł��B
0.2.0 �͎����I�����ł���A�C���^�[�t�F�[�X��d�l�����̂��̂ł��B
����A�j��I�ȕύX�𔺂��啝�ȏC��������鎖������܂��B
���p���ɂ͂����ӂ��������B


## Android �œ��삳����ꍇ�̕⑫

- Nhh3.ConnectionOptions.CaCertsList �ɐM�����ꂽ�F�؋ǃ��X�g�t�@�C���̃p�X���w�肷��K�v������܂�
    - �T���v���̎��� Nhh3SampleCore.cs - CreateHttp3() ���̎������Q�l�ɂ��Ă�������
- ``armeabi-v7a`` �œ����������ꍇ�̓r���h�p bat ���C���� qwfs �y�� quiche �̃r���h�����s���Ă�������
    - Bat\build_qwfs_android.bat �� ``arm64-v8a`` �� ``armeabi-v7a`` �ɁANDK �o�[�W������ 19 �ɕύX����Γ����z��ł�(���m�F)


## ���m�̖��

- �C���\��̖��
    - ���X�ʐM���ł܂����܂܋A���Ă��Ȃ��Ȃ邱�Ƃ�����܂�
    - abort ���ɏ���Ƀ��g���C���邱�Ƃ�����܂�
    - 301 ���ŃR���e���c����̍ۂɐ������������s���܂���
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
    - �f�t�H���g�� 128 ����Ń_�E�����[�h���s���܂�

�Q�[���I�u�W�F�N�g Http3SharpHost �ɂĒʐM��� URL ��|�[�g���ݒ�\�ł��̂ŁA�������������ɂ���ĕύX���s���Ă��������B<br>
�f�t�H���g�l�ɂ� cloudflare ����� HTTP/3 �Ή��y�[�W�ł��� cloudflare-quic.com �𗘗p�����Ē����Ă��܂��B


### �T���v���V�[���̒��ӎ���

- Android
    - �M�����ꂽ�F�؋ǃ��X�g�Ƃ��� [Mozilla �� 2023-01-10 ��](https://curl.se/docs/caextract.html) ���g�p���Ă��܂�
        - StreamingAssets �ɔz�u���Ă���܂�
        - apk ����̓W�J�ƍĔz�u�ɂ��Ă� Nhh3SampleCore.cs - CreateHttp3() ���̎������Q�l�ɂ��Ă�������
    - MultiRequestSample �̃t�@�C���ۑ��p�X�� ``Application.temporaryCachePath}\save`` �ɌŒ�ł�
        - �ύX�������ꍇ�� Nhh3SampleMulti.cs - OnStartClick() �̎������C�����Ă�������
- ����
    - �O�q�����ʂ� alt-svc �ɂ��A�b�v�O���[�h�ɂ͑Ή����Ă��Ȃ��̂ŁA�ʐM���s���T�C�g�� HTTP/3 �Ή��̕��@�ɂ͗��ӂ��Ă�������


# qwfs

qwfs (quiche wrapper for sharp) �� [cloudflare/quiche](https://github.com/cloudflare/quiche) �� Unity �ɑg�ݍ��݈Ղ����b�s���O���� C/C++ ���̃��C�u�����ł��B<br>
nhh3 �̗��p�ɓ��������d�l�Ŏ�������Ă���̂ŒP�̂ł̎g�p�͔񐄏��ł��B


## �r���h���@


### ����

1. quiche �̓W�J
    - ``External/quiche`` �� quiche �� submodule �ݒ�ɂ��Ă���܂��̂ŁA�܂��� ``git submodule update --init --recursive`` ���� quiche ��W�J���Ă�������
    - quiche ���ōX�� boringssl �� submodule �ݒ肳��Ă���̂ł�������W�J�R�ꂪ�Ȃ��悤�����ӂ�������
2. cmake ���C���X�g�[�����ăp�X��ʂ�
    - ����m�F���s�����o�[�W������ 3.26.0-rc2 �ł�
    - ����� nasm, ninhja ���l�ɖ����I�Ƀp�X���w�肷��`���ɕύX����\��ł�
3. nasm ���_�E�����[�h���ēW�J����
    - �p�X��ʂ��K�v�͂���܂���(�r���h���ɖ����I�Ɏw��)
    - ����m�F���s�����o�[�W������ 2.16.01 �ł�
4. (Android ����) ninja ���_�E�����[�h���ēW�J����
    - �p�X��ʂ��K�v�͂���܂���(�r���h���ɖ����I�Ɏw��)
    - ����m�F���s�����o�[�W������ 1.11.1 �ł�


### �r���h�p bat �ɂ���

Windows ���݂̂Ńr���h�\�� bat ��p�ӂ��Ă���܂��B<br>
(quiche, boringssl ���ꏏ�Ƀr���h���܂�)<br>
�ȉ��̎菇�Ŏ��s���Ă��������B

- Windows
    - ``Bat\build_qwfs_windows.bat`` �Ɉȉ��̈������w�肵�Ď��s
        - ������ : ``nasm.exe`` ���z�u���Ă���p�X
        - ������ : VS 2022 �� ``VsMSBuildCmd.bat`` ���z�u���Ă���p�X
        - ��O���� : �r���h�R���t�B�M�����[�V�����B ``release`` �w�莞�ɂ̓����[�X�r���h���A����ȊO�̍ۂ̓f�o�b�O�r���h���s���܂�
    - ��
        - ``$ build_qwfs_windows.bat D:\develop_tool\nasm-2.16.01 "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools" release``
- Android
    - ``Bat\build_qwfs_android.bat`` �Ɉȉ��̈������w�肵�Ď��s
        - ������ : ``nasm.exe`` ���z�u���Ă���p�X
        - ������ : VS 2022 �� ``VsMSBuildCmd.bat`` ���z�u���Ă���p�X
        - ��O���� : ``ninja.exe`` ���z�u���Ă���p�X
        - ��l���� : �r���h�R���t�B�M�����[�V�����B ``release`` �w�莞�ɂ̓����[�X�r���h���A����ȊO�̍ۂ̓f�o�b�O�r���h���s���܂�
    - ��
        - ``$ build_qwfs_android.bat D:\develop_tool\nasm-2.16.01 "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools" G:\develop\ninja\ninja.exe release``
    - �⑫
        - ``cargo build`` �݂̂��� boringssl �̃r���h���ʂ�Ȃ������ׁA boringssl �� cmake �ŌʂɃr���h���s���Ă��܂�
        - �܂��A�ʂɃr���h���s���� boringssl ���Q�Ƃ���ׂ� ``quiche\Cargo.toml`` �����ς��Ă���܂�


# �ˑ����� OSS �ƃo�[�W�������

- quiche : 5a671fd66d913b0d247c9213a4ef68265277ea0d
- boringssl : quiche �� submodule �� �o�[�W���� �ɏ�����


# ����m�F�֘A���

- ����m�F���� Unity �o�[�W���� : 2022.1.23f1


# License
Copyright(C) Ryo Takehara
See [LICENSE](https://github.com/TakeharaR/nhh3/blob/master/LICENSE) for more infomation.