nhh3 �� HTTP/3 �ʐM�p�� Unity �����A�Z�b�g�ł��B
[quiche](https://github.com/cloudflare/quiche) �� C/C++ ���b�p�w�ł��� qwfs �ƁA qwfs ��p���� C# ���� HTTP/3 �N���C�A���g���C�u�����ł��� nhh3 ���琬��܂��B


# nhh3

nhh3 �� Unity �� HTTP/3 �ʐM���s������ړI�Ƃ��� Unity �A�Z�b�g�ł��B
``nhh3`` �f�B���N�g�����w�肵�� Unity �Ńv���W�F�N�g���J�����A���̂܂܃C���|�[�g���Ă��������B

���s�ɂ� qwfs �̃r���h���K�v�ł��B
��q����菇�ŕK�� qwfs �̃r���h���s���Ă��������B
qwfs �̃r���h����������� ``nhh3/Assets/nhh3/Plugins/Windows`` �t�H���_�� quiche.dll �y�� qwfs.dll �������I�ɃR�s�[�����̂ŁA dll �̎蓮�R�s�[�͕s�v�ł��B


## ���ӎ���

nhh3 �� Unity �� HTTP/3 �̒ʐM���s�������I�Ȏ����ł��B
���i�ւ̑g�ݍ��݂͐������܂���B


## ���m�̖��E��������

- quiche �̃f�o�b�O���O�o�͂ɂ͔�Ή��ł�
- 301 ���ŃR���e���c����̍ۂɐ������������s���܂���
- �t�@�C������ qlog �ۑ��̃p�X���ɓ��{��̃t�@�C���p�X���w�肷��ƃN���b�V�����܂�
- �œK���͈�؎{���Ă��܂���
- ���̑��\�[�X�R�[�h���� todo �ƃR�����g��������͏C���\��ł�
- HTTP/3 �݂̂ɑΉ����Ă��܂�
    - ���ׁ̈A alt-svc �ɂ��A�b�v�O���[�h�ɂ͑Ή����Ă��܂���

## �T���v���V�[��

- SingleRequestSample
    - 1 �� HTTP/3 ���N�G�X�g�����s����T���v���ł�
- MultiRequestSample
    - Download File Num �Őݒ肵��������� HTTP/3 ���N�G�X�g�����s����T���v���ł�
    - �f�t�H���g�� 128 ����Ń_�E�����[�h���s���܂�

�Q�[���I�u�W�F�N�g Http3SharpHost �ɂĒʐM��� URL ��|�[�g���ݒ�\�ł��̂ŁA�������������ɂ���ĕύX���s���Ă��������B


# qwfs

qwfs (quiche wrapper for sharp) �� [quiche](https://github.com/cloudflare/quiche) �� Unity �ɑg�ݍ��݈Ղ����b�s���O���� C/C++ ���̃��C�u�����ł��B
nhh3 �̗��p�ɓ��������d�l�Ŏ�������Ă���̂ŒP�̂ł̎g�p�͔񐄏��ł��B

#### �r���h���@

Visual Studio 2022 ���K�v�ł��B
``External/quiche`` �� quiche �� submodule �ݒ�ɂ��Ă���܂��̂ŁA������ quiche ���r���h���Ă��� ``Example/QuicheWindowsSample/QuicheWindowsSample.sln`` ���g���r���h���s���Ă��������B

quiche �� .lib/.dll �r���h���@�͈ȉ����Q�Ƃ��Ă��������B  
[Re: C#(Unity)��HTTP/3�ʐM���Ă݂� ���̈� �`OSS�̑I�肩��r���h�܂Ł`](https://qiita.com/takehara-ryo/items/1f3e19b54bd6fffcba77)
���Ȃ݂ɁA quiche ���őΉ����������ׁA BoringSSL �̌ʃr���h�͕s�v�ɂȂ�܂����B
�ŐV�ł� cmake, nasm ���C���X�g�[�����āA�p�X��ʂ��΂��̂܂� quiche �̃r���h���������܂��B
�r���h���̃R�}���h�� ``cargo build --features ffi --release`` ���g�p���Ă��������B
qwfs ����̓f�t�H���g�� quiche �̃r���h�p�X���Q�Ƃ���̂ŁA�r���h���s������̐ݒ�͕s�v�ł��B


# QuicheWindowsSample

[quiche](https://github.com/cloudflare/quiche) �� example �� Windows �ڐA�������̂ł��B
���Ƃ��Ă��� quiche �� example �R�[�h�̃o�[�W������ 5a671fd66d913b0d247c9213a4ef68265277ea0d �ł��B
�ڍׂ͈ȉ��̋L�����Q�Ƃ��Ă��������B
[Re: C#(Unity)��HTTP/3�ʐM���Ă݂� ���̓� �`Windows��quiche�̃T���v���𓮂����Ă݂�`](https://qiita.com/takehara-ryo/items/4cbb5c291b2e94b4eafd)
�����T���v���͍��チ���e�i���X����Ȃ��\��������܂�

�r���h�ɂ� nhh3 �Ɠ��l�� quiche �̃r���h���K�v�ł��B
``External/quiche`` �� quiche �� submodule �ݒ�ɂ��Ă���܂��̂ŁA������ quiche ���r���h���Ă��� ``qwfs/qwfs.sln`` ���g���r���h���s���Ă��������B


# ���ӎ���

���݂̍ŐV�o�[�W������ 0.2.0 �ł��B
0.2.0 �͎����I�����ł���A�C���^�[�t�F�[�X��d�l�����̂��̂ł��B
����A�j��I�ȕύX�𔺂��啝�ȏC��������鎖������܂��B
���p���ɂ͂����ӂ��������B


# �Ή��v���b�g�t�H�[��

�o�[�W���� 0.1.0 �ł� Windows �ɂ̂ݑΉ����Ă��܂��B
�����I�ɂ� Android �ɑΉ��\��ł��B


# �o�[�W�����֘A���

- ����m�F���� Unity �o�[�W���� : 2022.1.23f1
- quiche : 5a671fd66d913b0d247c9213a4ef68265277ea0d
- boringssl : quiche �� submodule �� �o�[�W���� �ɏ�����


# License
This software is released under the MIT License, see LICENSE.