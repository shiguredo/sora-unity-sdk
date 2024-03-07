import subprocess
import logging
import os
import stat
import urllib.parse
import zipfile
import tarfile
import shutil
import platform
import multiprocessing
import argparse
from typing import Callable, NamedTuple, Optional, List, Union, Dict


logging.basicConfig(level=logging.DEBUG)


class ChangeDirectory(object):
    def __init__(self, cwd):
        self._cwd = cwd

    def __enter__(self):
        self._old_cwd = os.getcwd()
        logging.debug(f'pushd {self._old_cwd} --> {self._cwd}')
        os.chdir(self._cwd)

    def __exit__(self, exctype, excvalue, trace):
        logging.debug(f'popd {self._old_cwd} <-- {self._cwd}')
        os.chdir(self._old_cwd)
        return False


def cd(cwd):
    return ChangeDirectory(cwd)


def cmd(args, **kwargs):
    logging.debug(f'+{args} {kwargs}')
    if 'check' not in kwargs:
        kwargs['check'] = True
    if 'resolve' in kwargs:
        resolve = kwargs['resolve']
        del kwargs['resolve']
    else:
        resolve = True
    if resolve:
        args = [shutil.which(args[0]), *args[1:]]
    return subprocess.run(args, **kwargs)


# 標準出力をキャプチャするコマンド実行。シェルの `cmd ...` や $(cmd ...) と同じ
def cmdcap(args, **kwargs):
    # 3.7 でしか使えない
    # kwargs['capture_output'] = True
    kwargs['stdout'] = subprocess.PIPE
    kwargs['stderr'] = subprocess.PIPE
    kwargs['encoding'] = 'utf-8'
    return cmd(args, **kwargs).stdout.strip()


def rm_rf(path: str):
    if not os.path.exists(path):
        logging.debug(f'rm -rf {path} => path not found')
        return
    if os.path.isfile(path) or os.path.islink(path):
        os.remove(path)
        logging.debug(f'rm -rf {path} => file removed')
    if os.path.isdir(path):
        shutil.rmtree(path)
        logging.debug(f'rm -rf {path} => directory removed')


def mkdir_p(path: str):
    if os.path.exists(path):
        logging.debug(f'mkdir -p {path} => already exists')
        return
    os.makedirs(path, exist_ok=True)
    logging.debug(f'mkdir -p {path} => directory created')


if platform.system() == 'Windows':
    PATH_SEPARATOR = ';'
else:
    PATH_SEPARATOR = ':'


def add_path(path: str, is_after=False):
    logging.debug(f'add_path: {path}')
    if 'PATH' not in os.environ:
        os.environ['PATH'] = path
        return

    if is_after:
        os.environ['PATH'] = os.environ['PATH'] + PATH_SEPARATOR + path
    else:
        os.environ['PATH'] = path + PATH_SEPARATOR + os.environ['PATH']


def download(url: str, output_dir: Optional[str] = None, filename: Optional[str] = None) -> str:
    if filename is None:
        output_path = urllib.parse.urlparse(url).path.split('/')[-1]
    else:
        output_path = filename

    if output_dir is not None:
        output_path = os.path.join(output_dir, output_path)

    if os.path.exists(output_path):
        return output_path

    try:
        if shutil.which('curl') is not None:
            cmd(["curl", "-fLo", output_path, url])
        else:
            cmd(["wget", "-cO", output_path, url])
    except Exception:
        # ゴミを残さないようにする
        if os.path.exists(output_path):
            os.remove(output_path)
        raise

    return output_path


def read_version_file(path: str) -> Dict[str, str]:
    versions = {}

    lines = open(path).readlines()
    for line in lines:
        line = line.strip()

        # コメント行
        if line[:1] == '#':
            continue

        # 空行
        if len(line) == 0:
            continue

        [a, b] = map(lambda x: x.strip(), line.split('=', 2))
        versions[a] = b.strip('"')

    return versions


# dir 以下にある全てのファイルパスを、dir2 からの相対パスで返す
def enum_all_files(dir, dir2):
    for root, _, files in os.walk(dir):
        for file in files:
            yield os.path.relpath(os.path.join(root, file), dir2)


def versioned(func):
    def wrapper(version, version_file, *args, **kwargs):
        if 'ignore_version' in kwargs:
            if kwargs.get('ignore_version'):
                rm_rf(version_file)
            del kwargs['ignore_version']

        if os.path.exists(version_file):
            ver = open(version_file).read()
            if ver.strip() == version.strip():
                return

        r = func(version=version, *args, **kwargs)

        with open(version_file, 'w') as f:
            f.write(version)

        return r

    return wrapper


# アーカイブが単一のディレクトリに全て格納されているかどうかを調べる。
#
# 単一のディレクトリに格納されている場合はそのディレクトリ名を返す。
# そうでない場合は None を返す。
def _is_single_dir(infos: List[Union[zipfile.ZipInfo, tarfile.TarInfo]],
                   get_name: Callable[[Union[zipfile.ZipInfo, tarfile.TarInfo]], str],
                   is_dir: Callable[[Union[zipfile.ZipInfo, tarfile.TarInfo]], bool]) -> Optional[str]:
    # tarfile: ['path', 'path/to', 'path/to/file.txt']
    # zipfile: ['path/', 'path/to/', 'path/to/file.txt']
    # どちらも / 区切りだが、ディレクトリの場合、後ろに / が付くかどうかが違う
    dirname = None
    for info in infos:
        name = get_name(info)
        n = name.rstrip('/').find('/')
        if n == -1:
            # ルートディレクトリにファイルが存在している
            if not is_dir(info):
                return None
            dir = name.rstrip('/')
        else:
            dir = name[0:n]
        # ルートディレクトリに２個以上のディレクトリが存在している
        if dirname is not None and dirname != dir:
            return None
        dirname = dir

    return dirname


def is_single_dir_tar(tar: tarfile.TarFile) -> Optional[str]:
    return _is_single_dir(tar.getmembers(), lambda t: t.name, lambda t: t.isdir())


def is_single_dir_zip(zip: zipfile.ZipFile) -> Optional[str]:
    return _is_single_dir(zip.infolist(), lambda z: z.filename, lambda z: z.is_dir())


# 解凍した上でファイル属性を付与する
def _extractzip(z: zipfile.ZipFile, path: str):
    z.extractall(path)
    if platform.system() == 'Windows':
        return
    for info in z.infolist():
        if info.is_dir():
            continue
        filepath = os.path.join(path, info.filename)
        mod = info.external_attr >> 16
        if (mod & 0o120000) == 0o120000:
            # シンボリックリンク
            with open(filepath, 'r') as f:
                src = f.read()
            os.remove(filepath)
            with cd(os.path.dirname(filepath)):
                if os.path.exists(src):
                    os.symlink(src, filepath)
        if os.path.exists(filepath):
            # 普通のファイル
            os.chmod(filepath, mod & 0o777)


# zip または tar.gz ファイルを展開する。
#
# 展開先のディレクトリは {output_dir}/{output_dirname} となり、
# 展開先のディレクトリが既に存在していた場合は削除される。
#
# もしアーカイブの内容が単一のディレクトリであった場合、
# そのディレクトリは無いものとして展開される。
#
# つまりアーカイブ libsora-1.23.tar.gz の内容が
# ['libsora-1.23', 'libsora-1.23/file1', 'libsora-1.23/file2']
# であった場合、extract('libsora-1.23.tar.gz', 'out', 'libsora') のようにすると
# - out/libsora/file1
# - out/libsora/file2
# が出力される。
#
# また、アーカイブ libsora-1.23.tar.gz の内容が
# ['libsora-1.23', 'libsora-1.23/file1', 'libsora-1.23/file2', 'LICENSE']
# であった場合、extract('libsora-1.23.tar.gz', 'out', 'libsora') のようにすると
# - out/libsora/libsora-1.23/file1
# - out/libsora/libsora-1.23/file2
# - out/libsora/LICENSE
# が出力される。
def extract(file: str, output_dir: str, output_dirname: str, filetype: Optional[str] = None):
    path = os.path.join(output_dir, output_dirname)
    logging.info(f"Extract {file} to {path}")
    if filetype == 'gzip' or file.endswith('.tar.gz'):
        rm_rf(path)
        with tarfile.open(file) as t:
            dir = is_single_dir_tar(t)
            if dir is None:
                os.makedirs(path, exist_ok=True)
                t.extractall(path)
            else:
                logging.info(f"Directory {dir} is stripped")
                path2 = os.path.join(output_dir, dir)
                rm_rf(path2)
                t.extractall(output_dir)
                if path != path2:
                    logging.debug(f"mv {path2} {path}")
                    os.replace(path2, path)
    elif filetype == 'zip' or file.endswith('.zip'):
        rm_rf(path)
        with zipfile.ZipFile(file) as z:
            dir = is_single_dir_zip(z)
            if dir is None:
                os.makedirs(path, exist_ok=True)
                # z.extractall(path)
                _extractzip(z, path)
            else:
                logging.info(f"Directory {dir} is stripped")
                path2 = os.path.join(output_dir, dir)
                rm_rf(path2)
                # z.extractall(output_dir)
                _extractzip(z, output_dir)
                if path != path2:
                    logging.debug(f"mv {path2} {path}")
                    os.replace(path2, path)
    else:
        raise Exception('file should end with .tar.gz or .zip')


def clone_and_checkout(url, version, dir, fetch, fetch_force):
    if fetch_force:
        rm_rf(dir)

    if not os.path.exists(os.path.join(dir, '.git')):
        cmd(['git', 'clone', url, dir])
        fetch = True

    if fetch:
        with cd(dir):
            cmd(['git', 'fetch'])
            cmd(['git', 'reset', '--hard'])
            cmd(['git', 'clean', '-df'])
            cmd(['git', 'checkout', '-f', version])


def git_clone_shallow(url, hash, dir):
    rm_rf(dir)
    mkdir_p(dir)
    with cd(dir):
        cmd(['git', 'init'])
        cmd(['git', 'remote', 'add', 'origin', url])
        cmd(['git', 'fetch', '--depth=1', 'origin', hash])
        cmd(['git', 'reset', '--hard', 'FETCH_HEAD'])


@versioned
def install_android_ndk(version, install_dir, source_dir):
    archive = download(
        f'https://dl.google.com/android/repository/android-ndk-{version}-linux.zip',
        source_dir)
    rm_rf(os.path.join(install_dir, 'android-ndk'))
    extract(archive, output_dir=install_dir, output_dirname='android-ndk')


@versioned
def install_webrtc(version, source_dir, install_dir, platform: str):
    win = platform.startswith("windows_")
    filename = f'webrtc.{platform}.{"zip" if win else "tar.gz"}'
    rm_rf(os.path.join(source_dir, filename))
    archive = download(
        f'https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/{version}/{filename}',
        output_dir=source_dir)
    rm_rf(os.path.join(install_dir, 'webrtc'))
    extract(archive, output_dir=install_dir, output_dirname='webrtc')


class WebrtcInfo(NamedTuple):
    version_file: str
    webrtc_include_dir: str
    webrtc_library_dir: str
    clang_dir: str
    libcxx_dir: str


def get_webrtc_info(webrtcbuild: bool, source_dir: str, build_dir: str, install_dir: str) -> WebrtcInfo:
    webrtc_source_dir = os.path.join(source_dir, 'webrtc')
    webrtc_build_dir = os.path.join(build_dir, 'webrtc')
    webrtc_install_dir = os.path.join(install_dir, 'webrtc')

    if webrtcbuild:
        return WebrtcInfo(
            version_file=os.path.join(source_dir, 'webrtc-build', 'VERSION'),
            webrtc_include_dir=os.path.join(webrtc_source_dir, 'src'),
            webrtc_library_dir=os.path.join(webrtc_build_dir, 'obj')
            if platform.system() == 'Windows' else webrtc_build_dir, clang_dir=os.path.join(
                webrtc_source_dir, 'src', 'third_party', 'llvm-build', 'Release+Asserts'),
            libcxx_dir=os.path.join(webrtc_source_dir, 'src', 'third_party', 'libc++', 'src'),)
    else:
        return WebrtcInfo(
            version_file=os.path.join(webrtc_install_dir, 'VERSIONS'),
            webrtc_include_dir=os.path.join(webrtc_install_dir, 'include'),
            webrtc_library_dir=os.path.join(install_dir, 'webrtc', 'lib'),
            clang_dir=os.path.join(install_dir, 'llvm', 'clang'),
            libcxx_dir=os.path.join(install_dir, 'llvm', 'libcxx'),
        )


@versioned
def install_llvm(version, install_dir,
                 tools_url, tools_commit,
                 libcxx_url, libcxx_commit,
                 buildtools_url, buildtools_commit):
    llvm_dir = os.path.join(install_dir, 'llvm')
    rm_rf(llvm_dir)
    mkdir_p(llvm_dir)
    with cd(llvm_dir):
        # tools の update.py を叩いて特定バージョンの clang バイナリを拾う
        git_clone_shallow(tools_url, tools_commit, 'tools')
        with cd('tools'):
            cmd(['python3',
                os.path.join('clang', 'scripts', 'update.py'),
                '--output-dir', os.path.join(llvm_dir, 'clang')])

        # 特定バージョンの libcxx を利用する
        git_clone_shallow(libcxx_url, libcxx_commit, 'libcxx')

        # __config_site のために特定バージョンの buildtools を取得する
        git_clone_shallow(buildtools_url, buildtools_commit, 'buildtools')
        shutil.copyfile(os.path.join(llvm_dir, 'buildtools', 'third_party', 'libc++', '__config_site'),
                        os.path.join(llvm_dir, 'libcxx', 'include', '__config_site'))


@versioned
def install_boost(version, source_dir, install_dir, sora_version, platform: str):
    win = platform.startswith("windows_")
    filename = f'boost-{version}_sora-cpp-sdk-{sora_version}_{platform}.{"zip" if win else "tar.gz"}'
    rm_rf(os.path.join(source_dir, filename))
    archive = download(
        f'https://github.com/shiguredo/sora-cpp-sdk/releases/download/{sora_version}/{filename}',
        output_dir=source_dir)
    rm_rf(os.path.join(install_dir, 'boost'))
    extract(archive, output_dir=install_dir, output_dirname='boost')


def cmake_path(path: str) -> str:
    return path.replace('\\', '/')


@versioned
def install_cmake(version, source_dir, install_dir, platform: str, ext):
    url = f'https://github.com/Kitware/CMake/releases/download/v{version}/cmake-{version}-{platform}.{ext}'
    path = download(url, source_dir)
    extract(path, install_dir, 'cmake')
    # Android で自前の CMake を利用する場合、ninja へのパスが見つけられない問題があるので、同じディレクトリに symlink を貼る
    # https://issuetracker.google.com/issues/206099937
    if platform.startswith('linux'):
        with cd(os.path.join(install_dir, 'cmake', 'bin')):
            cmd(['ln', '-s', '/usr/bin/ninja', 'ninja'])


@versioned
def install_sora(version, source_dir, install_dir, platform: str):
    win = platform.startswith("windows_")
    filename = f'sora-cpp-sdk-{version}_{platform}.{"zip" if win else "tar.gz"}'
    rm_rf(os.path.join(source_dir, filename))
    archive = download(
        f'https://github.com/shiguredo/sora-cpp-sdk/releases/download/{version}/{filename}',
        output_dir=source_dir)
    rm_rf(os.path.join(install_dir, 'sora'))
    extract(archive, output_dir=install_dir, output_dirname='sora')


@versioned
def install_protobuf(version, source_dir, install_dir, platform: str):
    # platform:
    # - linux-aarch_64
    # - linux-ppcle_64
    # - linux-s390_64
    # - linux-x86_32
    # - linux-x86_64
    # - osx-aarch_64
    # - osx-universal_binary
    # - osx-x86_64
    # - win32
    # - win64
    url = f'https://github.com/protocolbuffers/protobuf/releases/download/v{version}/protoc-{version}-{platform}.zip'
    path = download(url, source_dir)
    rm_rf(os.path.join(install_dir, 'protobuf'))
    extract(path, install_dir, 'protobuf')
    # なぜか実行属性が消えてるので入れてやる
    for file in os.scandir(os.path.join(install_dir, 'protobuf', 'bin')):
        if file.is_file:
            os.chmod(file.path, file.stat().st_mode | stat.S_IXUSR)


@versioned
def install_protoc_gen_jsonif(version, source_dir, install_dir, platform: str):
    # platform:
    # - darwin-amd64
    # - darwin-arm64
    # - linux-amd64
    # - windows-amd64
    url = f'https://github.com/melpon/protoc-gen-jsonif/releases/download/{version}/protoc-gen-jsonif.tar.gz'
    rm_rf(os.path.join(source_dir, 'protoc-gen-jsonif.tar.gz'))
    path = download(url, source_dir)
    jsonif_install_dir = os.path.join(install_dir, 'protoc-gen-jsonif')
    rm_rf(jsonif_install_dir)
    extract(path, install_dir, 'protoc-gen-jsonif')
    # 自分の環境のバイナリを <install-path>/bin に配置する
    shutil.copytree(
        os.path.join(jsonif_install_dir, *platform.split('-')),
        os.path.join(jsonif_install_dir, 'bin'))
    # なぜか実行属性が消えてるので入れてやる
    for file in os.scandir(os.path.join(jsonif_install_dir, 'bin')):
        if file.is_file:
            os.chmod(file.path, file.stat().st_mode | stat.S_IXUSR)


BASE_DIR = os.path.abspath(os.path.dirname(__file__))


def install_deps(platform: str, build_platform: str, source_dir, build_dir, install_dir, debug):
    with cd(BASE_DIR):
        version = read_version_file('VERSION')

        # Android NDK
        if platform == 'android':
            install_android_ndk_args = {
                'version': version['ANDROID_NDK_VERSION'],
                'version_file': os.path.join(install_dir, 'android-ndk.version'),
                'source_dir': source_dir,
                'install_dir': install_dir,
            }
            install_android_ndk(**install_android_ndk_args)

        # WebRTC
        install_webrtc_args = {
            'version': version['WEBRTC_BUILD_VERSION'],
            'version_file': os.path.join(install_dir, 'webrtc.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'platform': platform,
        }

        install_webrtc(**install_webrtc_args)

        webrtc_info = get_webrtc_info(
            False, source_dir, build_dir, install_dir)
        webrtc_version = read_version_file(webrtc_info.version_file)

        # Windows は MSVC を使うので不要
        # macOS と iOS は Apple Clang を使うので不要
        # Android は libc++ のために必要
        if platform not in ('windows_x86_64', 'macos_x86_64', 'macos_arm64', 'ios'):
            # LLVM
            tools_url = webrtc_version['WEBRTC_SRC_TOOLS_URL']
            tools_commit = webrtc_version['WEBRTC_SRC_TOOLS_COMMIT']
            libcxx_url = webrtc_version['WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_URL']
            libcxx_commit = webrtc_version['WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_COMMIT']
            buildtools_url = webrtc_version['WEBRTC_SRC_BUILDTOOLS_URL']
            buildtools_commit = webrtc_version['WEBRTC_SRC_BUILDTOOLS_COMMIT']
            install_llvm_args = {
                'version':
                    f'{tools_url}.{tools_commit}.'
                    f'{libcxx_url}.{libcxx_commit}.'
                    f'{buildtools_url}.{buildtools_commit}',
                'version_file': os.path.join(install_dir, 'llvm.version'),
                'install_dir': install_dir,
                'tools_url': tools_url,
                'tools_commit': tools_commit,
                'libcxx_url': libcxx_url,
                'libcxx_commit': libcxx_commit,
                'buildtools_url': buildtools_url,
                'buildtools_commit': buildtools_commit,
            }
            install_llvm(**install_llvm_args)

        # Boost
        install_boost_args = {
            'version': version['BOOST_VERSION'],
            'version_file': os.path.join(install_dir, 'boost.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'sora_version': version['SORA_CPP_SDK_VERSION'],
            'platform': platform,
        }
        install_boost(**install_boost_args)

        # CMake
        install_cmake_args = {
            'version': version['CMAKE_VERSION'],
            'version_file': os.path.join(install_dir, 'cmake.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'platform': '',
            'ext': 'tar.gz'
        }
        if build_platform == 'windows_x86_64':
            install_cmake_args['platform'] = 'windows-x86_64'
            install_cmake_args['ext'] = 'zip'
        elif build_platform in ('macos_x86_64', 'macos_arm64'):
            install_cmake_args['platform'] = 'macos-universal'
        elif build_platform == 'ubuntu-20.04_x86_64':
            install_cmake_args['platform'] = 'linux-x86_64'
        else:
            raise Exception('Failed to install CMake')
        install_cmake(**install_cmake_args)

        if build_platform in ('macos_x86_64', 'macos_arm64'):
            add_path(os.path.join(install_dir, 'cmake',
                     'CMake.app', 'Contents', 'bin'))
        else:
            add_path(os.path.join(install_dir, 'cmake', 'bin'))

        # Sora C++ SDK
        install_sora_args = {
            'version': version['SORA_CPP_SDK_VERSION'],
            'version_file': os.path.join(install_dir, 'sora.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'platform': platform,
        }
        install_sora(**install_sora_args)

        # protobuf
        install_protobuf_args = {
            'version': version['PROTOBUF_VERSION'],
            'version_file': os.path.join(install_dir, 'protobuf.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'platform': '',
        }
        if build_platform == 'windows_x86_64':
            install_protobuf_args['platform'] = 'win64'
        elif build_platform in ('macos_x86_64', 'macos_arm64'):
            install_protobuf_args['platform'] = 'osx-universal_binary'
        elif build_platform == 'ubuntu-20.04_x86_64':
            install_protobuf_args['platform'] = 'linux-x86_64'
        else:
            raise Exception('Failed to install Protobuf')
        install_protobuf(**install_protobuf_args)

        # protoc-gen-jsonif
        install_jsonif_args = {
            'version': version['PROTOC_GEN_JSONIF_VERSION'],
            'version_file': os.path.join(install_dir, 'protoc-gen-jsonif.version'),
            'source_dir': source_dir,
            'install_dir': install_dir,
            'platform': '',
        }
        if build_platform == 'windows_x86_64':
            install_jsonif_args['platform'] = 'windows-amd64'
        elif build_platform == 'macos_x86_64':
            install_jsonif_args['platform'] = 'darwin-amd64'
        elif build_platform == 'macos_arm64':
            install_jsonif_args['platform'] = 'darwin-arm64'
        elif build_platform == 'ubuntu-20.04_x86_64':
            install_jsonif_args['platform'] = 'linux-amd64'
        else:
            raise Exception('Failed to install protoc-gen-jsonif')
        install_protoc_gen_jsonif(**install_jsonif_args)


def get_build_platform():
    arch = platform.machine()
    if arch in ('AMD64', 'x86_64'):
        arch = 'x86_64'
    elif arch in ('aarch64', 'arm64'):
        arch = 'arm64'
    else:
        raise Exception(f'Arch {arch} not supported')

    os = platform.system()
    if os == 'Windows':
        if arch == 'x86_64':
            return 'windows_x86_64'
        else:
            raise Exception('Unknown windows arch')
    elif os == 'Darwin':
        return f'macos_{arch}'
    elif os == 'Linux':
        release = read_version_file('/etc/os-release')
        os = release['NAME']
        if os == 'Ubuntu':
            osver = release['VERSION_ID']
            if osver == '20.04':
                if arch == 'x86_64':
                    return 'ubuntu-20.04_x86_64'
                else:
                    raise Exception('Unknown ubuntu arch')
            else:
                raise Exception('Unexpected Ubuntu version')
        else:
            raise Exception(f'OS {os} not supported')
    else:
        raise Exception(f'OS {os} not supported')


AVAILABLE_TARGETS = ['windows_x86_64', 'macos_arm64',
                     'ubuntu-20.04_x86_64', 'ios', 'android']

BUILD_PLATFORM = {
    'windows_x86_64': ['windows_x86_64'],
    'macos_x86_64': ['macos_x86_64', 'macos_arm64', 'ios'],
    'macos_arm64': ['macos_x86_64', 'macos_arm64', 'ios'],
    'ubuntu-20.04_x86_64': ['ubuntu-20.04_x86_64', 'android'],
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--debug", action='store_true')
    parser.add_argument("--relwithdebinfo", action='store_true')
    parser.add_argument("target", choices=AVAILABLE_TARGETS)

    args = parser.parse_args()

    platform = args.target
    build_platform = get_build_platform()
    if build_platform not in BUILD_PLATFORM:
        raise Exception(f'Build platform {build_platform} is not supported.')
    if platform not in BUILD_PLATFORM[build_platform]:
        raise Exception(
            f'Target {platform} is not supported on this build platform {build_platform}.')

    configuration_dir = 'debug' if args.debug else 'release'
    source_dir = os.path.join(BASE_DIR, '_source', platform, configuration_dir)
    build_dir = os.path.join(BASE_DIR, '_build', platform, configuration_dir)
    install_dir = os.path.join(
        BASE_DIR, '_install', platform, configuration_dir)
    mkdir_p(source_dir)
    mkdir_p(build_dir)
    mkdir_p(install_dir)

    install_deps(platform, build_platform, source_dir,
                 build_dir, install_dir, args.debug)

    if args.debug:
        configuration = 'Debug'
    elif args.relwithdebinfo:
        configuration = 'RelWithDebInfo'
    else:
        configuration = 'Release'

    unity_build_dir = os.path.join(build_dir, 'sora_unity_sdk')
    mkdir_p(unity_build_dir)
    with cd(unity_build_dir):
        webrtc_info = get_webrtc_info(
            False, source_dir, build_dir, install_dir)
        webrtc_version = read_version_file(webrtc_info.version_file)
        webrtc_commit = webrtc_version['WEBRTC_COMMIT']
        with cd(BASE_DIR):
            version = read_version_file('VERSION')
            sora_unity_sdk_version = version['SORA_UNITY_SDK_VERSION']
            sora_unity_sdk_commit = cmdcap(['git', 'rev-parse', 'HEAD'])
            android_native_api_level = version['ANDROID_NATIVE_API_LEVEL']

        cmake_args = []
        cmake_args.append(f'-DCMAKE_BUILD_TYPE={configuration}')
        cmake_args.append(f'-DSORA_UNITY_SDK_PACKAGE={platform}')
        cmake_args.append(f'-DSORA_UNITY_SDK_VERSION={sora_unity_sdk_version}')
        cmake_args.append(f'-DSORA_UNITY_SDK_COMMIT={sora_unity_sdk_commit}')
        cmake_args.append(
            f"-DBOOST_ROOT={cmake_path(os.path.join(install_dir, 'boost'))}")
        cmake_args.append('-DWEBRTC_LIBRARY_NAME=webrtc')
        cmake_args.append(
            f"-DWEBRTC_INCLUDE_DIR={cmake_path(webrtc_info.webrtc_include_dir)}")
        cmake_args.append(
            f"-DWEBRTC_LIBRARY_DIR={cmake_path(webrtc_info.webrtc_library_dir)}")
        cmake_args.append(f"-DWEBRTC_COMMIT={webrtc_commit}")
        cmake_args.append(
            f"-DSORA_DIR={cmake_path(os.path.join(install_dir, 'sora'))}")
        cmake_args.append(
            f"-DPROTOBUF_DIR={cmake_path(os.path.join(install_dir, 'protobuf'))}")
        cmake_args.append(
            f"-DPROTOC_GEN_JSONIF_DIR={cmake_path(os.path.join(install_dir, 'protoc-gen-jsonif'))}")
        if platform == 'windows_x86_64':
            pass
        elif platform in ('macos_x86_64', 'macos_arm64'):
            sysroot = cmdcap(['xcrun', '--sdk', 'macosx', '--show-sdk-path'])
            arch = 'x86_64' if platform == 'macos_x86_64' else 'arm64'
            target = 'x86_64-apple-darwin' if platform == 'macos_x86_64' else 'arm64-apple-darwin'
            cmake_args.append(f'-DCMAKE_SYSTEM_PROCESSOR={arch}')
            cmake_args.append(f'-DCMAKE_OSX_ARCHITECTURES={arch}')
            cmake_args.append(f'-DCMAKE_C_COMPILER_TARGET={target}')
            cmake_args.append(f'-DCMAKE_CXX_COMPILER_TARGET={target}')
            cmake_args.append(f'-DCMAKE_OBJCXX_COMPILER_TARGET={target}')
            cmake_args.append(f'-DCMAKE_SYSROOT={sysroot}')
        elif platform == 'ios':
            cmake_args += ['-G', 'Xcode']
            cmake_args.append('-DCMAKE_SYSTEM_NAME=iOS')
            cmake_args.append('-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"')
            cmake_args.append('-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0')
            cmake_args.append('-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO')
        elif platform == 'android':
            toolchain_file = os.path.join(
                install_dir, 'android-ndk', 'build', 'cmake', 'android.toolchain.cmake')
            cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
            cmake_args.append(
                f"-DANDROID_PLATFORM=android-{android_native_api_level}")
            cmake_args.append(
                f"-DANDROID_NATIVE_API_LEVEL={android_native_api_level}")
            cmake_args.append('-DANDROID_ABI=arm64-v8a')
            cmake_args.append('-DANDROID_STL=none')
            cmake_args.append(
                f"-DLIBCXX_INCLUDE_DIR={cmake_path(os.path.join(webrtc_info.libcxx_dir, 'include'))}")
            cmake_args.append('-DANDROID_CPP_FEATURES=exceptions rtti')
            # r23b には ANDROID_CPP_FEATURES=exceptions でも例外が設定されない問題がある
            # https://github.com/android/ndk/issues/1618
            cmake_args.append('-DCMAKE_ANDROID_EXCEPTIONS=ON')
        elif platform == 'ubuntu-20.04_x86_64':
            cmake_args.append(
                f"-DCMAKE_C_COMPILER={os.path.join(webrtc_info.clang_dir, 'bin', 'clang')}")
            cmake_args.append(
                f"-DCMAKE_CXX_COMPILER={os.path.join(webrtc_info.clang_dir, 'bin', 'clang++')}")
            cmake_args.append(
                f"-DLIBCXX_INCLUDE_DIR={cmake_path(os.path.join(webrtc_info.libcxx_dir, 'include'))}")
        else:
            raise Exception(f'Platform {platform} not supported.')

        cmd(['cmake', BASE_DIR, *cmake_args])

        if platform == 'ios':
            cmd(['cmake', '--build', '.', f'-j{multiprocessing.cpu_count()}',
                 '--config', configuration,
                 '--target', 'SoraUnitySdk',
                 '--',
                 '-arch', 'x86_64',
                 '-sdk', 'iphonesimulator'])
            cmd(['cmake', '--build', '.', f'-j{multiprocessing.cpu_count()}',
                 '--config', configuration,
                 '--target', 'SoraUnitySdk',
                 '--',
                 '-arch', 'arm64',
                 '-sdk', 'iphoneos'])
            cmd(['lipo', '-create',
                 '-output', os.path.join(unity_build_dir, 'libSoraUnitySdk.a'),
                 os.path.join(unity_build_dir,
                              'Release-iphonesimulator', 'libSoraUnitySdk.a'),
                 os.path.join(unity_build_dir, 'Release-iphoneos', 'libSoraUnitySdk.a')])
        else:
            cmd(['cmake', '--build', '.',
                f'-j{multiprocessing.cpu_count()}', '--config', configuration])


if __name__ == '__main__':
    main()
