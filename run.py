import argparse
import logging
import multiprocessing
import os
import platform
import shlex
from typing import List, Optional

from buildbase import (
    add_path,
    build_sora,
    build_webrtc,
    cd,
    cmake_path,
    cmd,
    cmdcap,
    get_sora_info,
    get_webrtc_info,
    install_android_ndk,
    install_cmake,
    install_llvm,
    install_protobuf,
    install_protoc_gen_jsonif,
    install_sora_and_deps,
    install_webrtc,
    mkdir_p,
    read_version_file,
)

logging.basicConfig(level=logging.DEBUG)


BASE_DIR = os.path.abspath(os.path.dirname(__file__))


def install_deps(
    platform: str,
    build_platform: str,
    source_dir,
    build_dir,
    install_dir,
    debug,
    local_webrtc_build_dir: Optional[str],
    local_webrtc_build_args: List[str],
    local_sora_cpp_sdk_dir: Optional[str],
    local_sora_cpp_sdk_args: List[str],
):
    with cd(BASE_DIR):
        version = read_version_file("VERSION")

        # Android NDK
        if platform == "android":
            install_android_ndk_args = {
                "version": version["ANDROID_NDK_VERSION"],
                "version_file": os.path.join(install_dir, "android-ndk.version"),
                "source_dir": source_dir,
                "install_dir": install_dir,
            }
            install_android_ndk(**install_android_ndk_args)

        # WebRTC
        if local_webrtc_build_dir is None:
            install_webrtc_args = {
                "version": version["WEBRTC_BUILD_VERSION"],
                "version_file": os.path.join(install_dir, "webrtc.version"),
                "source_dir": source_dir,
                "install_dir": install_dir,
                "platform": platform,
            }
            install_webrtc(**install_webrtc_args)
        else:
            build_webrtc_args = {
                "platform": platform,
                "local_webrtc_build_dir": local_webrtc_build_dir,
                "local_webrtc_build_args": local_webrtc_build_args,
                "debug": debug,
            }
            build_webrtc(**build_webrtc_args)

        webrtc_info = get_webrtc_info(platform, local_webrtc_build_dir, install_dir, debug)

        # Windows は MSVC を使うので不要
        # macOS と iOS は Apple Clang を使うので不要
        # Android は libc++ のために必要
        # webrtc-build をソースビルドしてる場合は既にローカルにあるので不要
        if (
            platform not in ("windows_x86_64", "macos_x86_64", "macos_arm64", "ios")
            and local_webrtc_build_dir is None
        ):
            webrtc_version = read_version_file(webrtc_info.version_file)

            # LLVM
            tools_url = webrtc_version["WEBRTC_SRC_TOOLS_URL"]
            tools_commit = webrtc_version["WEBRTC_SRC_TOOLS_COMMIT"]
            libcxx_url = webrtc_version["WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_URL"]
            libcxx_commit = webrtc_version["WEBRTC_SRC_THIRD_PARTY_LIBCXX_SRC_COMMIT"]
            buildtools_url = webrtc_version["WEBRTC_SRC_BUILDTOOLS_URL"]
            buildtools_commit = webrtc_version["WEBRTC_SRC_BUILDTOOLS_COMMIT"]
            install_llvm_args = {
                "version": f"{tools_url}.{tools_commit}."
                f"{libcxx_url}.{libcxx_commit}."
                f"{buildtools_url}.{buildtools_commit}",
                "version_file": os.path.join(install_dir, "llvm.version"),
                "install_dir": install_dir,
                "tools_url": tools_url,
                "tools_commit": tools_commit,
                "libcxx_url": libcxx_url,
                "libcxx_commit": libcxx_commit,
                "buildtools_url": buildtools_url,
                "buildtools_commit": buildtools_commit,
            }
            install_llvm(**install_llvm_args)

        # CMake
        install_cmake_args = {
            "version": version["CMAKE_VERSION"],
            "version_file": os.path.join(install_dir, "cmake.version"),
            "source_dir": source_dir,
            "install_dir": install_dir,
            "platform": "",
            "ext": "tar.gz",
        }
        if build_platform == "windows_x86_64":
            install_cmake_args["platform"] = "windows-x86_64"
            install_cmake_args["ext"] = "zip"
        elif build_platform in ("macos_x86_64", "macos_arm64"):
            install_cmake_args["platform"] = "macos-universal"
        elif build_platform == "ubuntu-22.04_x86_64":
            install_cmake_args["platform"] = "linux-x86_64"
        else:
            raise Exception("Failed to install CMake")
        install_cmake(**install_cmake_args)

        if build_platform in ("macos_x86_64", "macos_arm64"):
            add_path(os.path.join(install_dir, "cmake", "CMake.app", "Contents", "bin"))
        else:
            add_path(os.path.join(install_dir, "cmake", "bin"))

        # Sora C++ SDK
        if local_sora_cpp_sdk_dir is None:
            install_sora_and_deps(platform, source_dir, install_dir)
        else:
            build_sora(
                platform,
                local_sora_cpp_sdk_dir,
                local_sora_cpp_sdk_args,
                debug,
                local_webrtc_build_dir,
            )

        # protobuf
        install_protobuf_args = {
            "version": version["PROTOBUF_VERSION"],
            "version_file": os.path.join(install_dir, "protobuf.version"),
            "source_dir": source_dir,
            "install_dir": install_dir,
            "platform": "",
        }
        if build_platform == "windows_x86_64":
            install_protobuf_args["platform"] = "win64"
        elif build_platform in ("macos_x86_64", "macos_arm64"):
            install_protobuf_args["platform"] = "osx-universal_binary"
        elif build_platform == "ubuntu-22.04_x86_64":
            install_protobuf_args["platform"] = "linux-x86_64"
        else:
            raise Exception("Failed to install Protobuf")
        install_protobuf(**install_protobuf_args)

        # protoc-gen-jsonif
        install_jsonif_args = {
            "version": version["PROTOC_GEN_JSONIF_VERSION"],
            "version_file": os.path.join(install_dir, "protoc-gen-jsonif.version"),
            "source_dir": source_dir,
            "install_dir": install_dir,
            "platform": "",
        }
        if build_platform == "windows_x86_64":
            install_jsonif_args["platform"] = "windows-amd64"
        elif build_platform == "macos_x86_64":
            install_jsonif_args["platform"] = "darwin-amd64"
        elif build_platform == "macos_arm64":
            install_jsonif_args["platform"] = "darwin-arm64"
        elif build_platform == "ubuntu-22.04_x86_64":
            install_jsonif_args["platform"] = "linux-amd64"
        else:
            raise Exception("Failed to install protoc-gen-jsonif")
        install_protoc_gen_jsonif(**install_jsonif_args)


def get_build_platform():
    arch = platform.machine()
    if arch in ("AMD64", "x86_64"):
        arch = "x86_64"
    elif arch in ("aarch64", "arm64"):
        arch = "arm64"
    else:
        raise Exception(f"Arch {arch} not supported")

    os = platform.system()
    if os == "Windows":
        if arch == "x86_64":
            return "windows_x86_64"
        else:
            raise Exception("Unknown windows arch")
    elif os == "Darwin":
        return f"macos_{arch}"
    elif os == "Linux":
        release = read_version_file("/etc/os-release")
        os = release["NAME"]
        if os == "Ubuntu":
            osver = release["VERSION_ID"]
            if osver == "22.04":
                if arch == "x86_64":
                    return "ubuntu-22.04_x86_64"
                else:
                    raise Exception("Unknown ubuntu arch")
            else:
                raise Exception("Unexpected Ubuntu version")
        else:
            raise Exception(f"OS {os} not supported")
    else:
        raise Exception(f"OS {os} not supported")


AVAILABLE_TARGETS = [
    "windows_x86_64",
    "macos_arm64",
    "ubuntu-22.04_x86_64",
    "ios",
    "android",
]

BUILD_PLATFORM = {
    "windows_x86_64": ["windows_x86_64"],
    "macos_x86_64": ["macos_x86_64", "macos_arm64", "ios"],
    "macos_arm64": ["macos_x86_64", "macos_arm64", "ios"],
    "ubuntu-22.04_x86_64": ["ubuntu-22.04_x86_64", "android"],
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--relwithdebinfo", action="store_true")
    parser.add_argument("--local-webrtc-build-dir", type=os.path.abspath)
    parser.add_argument("--local-webrtc-build-args", default="", type=shlex.split)
    parser.add_argument("--local-sora-cpp-sdk-dir", type=os.path.abspath)
    parser.add_argument("--local-sora-cpp-sdk-args", default="", type=shlex.split)
    parser.add_argument("target", choices=AVAILABLE_TARGETS)

    args = parser.parse_args()

    platform = args.target
    build_platform = get_build_platform()
    if build_platform not in BUILD_PLATFORM:
        raise Exception(f"Build platform {build_platform} is not supported.")
    if platform not in BUILD_PLATFORM[build_platform]:
        raise Exception(
            f"Target {platform} is not supported on this build platform {build_platform}."
        )

    configuration_dir = "debug" if args.debug else "release"
    source_dir = os.path.join(BASE_DIR, "_source", platform, configuration_dir)
    build_dir = os.path.join(BASE_DIR, "_build", platform, configuration_dir)
    install_dir = os.path.join(BASE_DIR, "_install", platform, configuration_dir)
    mkdir_p(source_dir)
    mkdir_p(build_dir)
    mkdir_p(install_dir)

    install_deps(
        platform,
        build_platform,
        source_dir,
        build_dir,
        install_dir,
        args.debug,
        args.local_webrtc_build_dir,
        args.local_webrtc_build_args,
        args.local_sora_cpp_sdk_dir,
        args.local_sora_cpp_sdk_args,
    )

    if args.debug:
        configuration = "Debug"
    elif args.relwithdebinfo:
        configuration = "RelWithDebInfo"
    else:
        configuration = "Release"

    unity_build_dir = os.path.join(build_dir, "sora_unity_sdk")
    mkdir_p(unity_build_dir)
    with cd(unity_build_dir):
        webrtc_info = get_webrtc_info(
            platform, args.local_webrtc_build_dir, install_dir, args.debug
        )
        sora_info = get_sora_info(platform, args.local_sora_cpp_sdk_dir, install_dir, args.debug)
        webrtc_version = read_version_file(webrtc_info.version_file)
        webrtc_commit = webrtc_version["WEBRTC_COMMIT"]
        with cd(BASE_DIR):
            version = read_version_file("VERSION")
            sora_unity_sdk_version = version["SORA_UNITY_SDK_VERSION"]
            sora_unity_sdk_commit = cmdcap(["git", "rev-parse", "HEAD"])
            android_native_api_level = version["ANDROID_NATIVE_API_LEVEL"]

        cmake_args = []
        cmake_args.append(f"-DCMAKE_BUILD_TYPE={configuration}")
        cmake_args.append(f"-DSORA_UNITY_SDK_PACKAGE={platform}")
        cmake_args.append(f"-DSORA_UNITY_SDK_VERSION={sora_unity_sdk_version}")
        cmake_args.append(f"-DSORA_UNITY_SDK_COMMIT={sora_unity_sdk_commit}")
        cmake_args.append(f"-DBOOST_ROOT={cmake_path(sora_info.boost_install_dir)}")
        cmake_args.append("-DWEBRTC_LIBRARY_NAME=webrtc")
        cmake_args.append(f"-DWEBRTC_INCLUDE_DIR={cmake_path(webrtc_info.webrtc_include_dir)}")
        cmake_args.append(f"-DWEBRTC_LIBRARY_DIR={cmake_path(webrtc_info.webrtc_library_dir)}")
        cmake_args.append(f"-DWEBRTC_COMMIT={webrtc_commit}")
        cmake_args.append(f"-DSORA_DIR={cmake_path(sora_info.sora_install_dir)}")
        cmake_args.append(f"-DCLANG_DIR={cmake_path(os.path.join(webrtc_info.clang_dir))}")
        cmake_args.append(f"-DPROTOBUF_DIR={cmake_path(os.path.join(install_dir, 'protobuf'))}")
        cmake_args.append(
            f"-DPROTOC_GEN_JSONIF_DIR={cmake_path(os.path.join(install_dir, 'protoc-gen-jsonif'))}"
        )
        if platform == "windows_x86_64":
            pass
        elif platform in ("macos_x86_64", "macos_arm64"):
            sysroot = cmdcap(["xcrun", "--sdk", "macosx", "--show-sdk-path"])
            arch = "x86_64" if platform == "macos_x86_64" else "arm64"
            target = "x86_64-apple-darwin" if platform == "macos_x86_64" else "arm64-apple-darwin"
            cmake_args.append(f"-DCMAKE_SYSTEM_PROCESSOR={arch}")
            cmake_args.append(f"-DCMAKE_OSX_ARCHITECTURES={arch}")
            cmake_args.append(f"-DCMAKE_C_COMPILER_TARGET={target}")
            cmake_args.append(f"-DCMAKE_CXX_COMPILER_TARGET={target}")
            cmake_args.append(f"-DCMAKE_OBJCXX_COMPILER_TARGET={target}")
            cmake_args.append(f"-DCMAKE_SYSROOT={sysroot}")
        elif platform == "ios":
            cmake_args += ["-G", "Xcode"]
            cmake_args.append("-DCMAKE_SYSTEM_NAME=iOS")
            cmake_args.append('-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"')
            cmake_args.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0")
            cmake_args.append("-DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO")
        elif platform == "android":
            toolchain_file = os.path.join(
                install_dir, "android-ndk", "build", "cmake", "android.toolchain.cmake"
            )
            cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
            cmake_args.append(f"-DANDROID_PLATFORM=android-{android_native_api_level}")
            cmake_args.append(f"-DANDROID_NATIVE_API_LEVEL={android_native_api_level}")
            cmake_args.append("-DANDROID_ABI=arm64-v8a")
            cmake_args.append("-DANDROID_STL=none")
            cmake_args.append(
                f"-DLIBCXX_INCLUDE_DIR={cmake_path(os.path.join(webrtc_info.libcxx_dir, 'include'))}"
            )
            cmake_args.append("-DANDROID_CPP_FEATURES=exceptions rtti")
            # r23b には ANDROID_CPP_FEATURES=exceptions でも例外が設定されない問題がある
            # https://github.com/android/ndk/issues/1618
            cmake_args.append("-DCMAKE_ANDROID_EXCEPTIONS=ON")
        elif platform in ("ubuntu-22.04_x86_64"):
            cmake_args.append(
                f"-DCMAKE_C_COMPILER={os.path.join(webrtc_info.clang_dir, 'bin', 'clang')}"
            )
            cmake_args.append(
                f"-DCMAKE_CXX_COMPILER={os.path.join(webrtc_info.clang_dir, 'bin', 'clang++')}"
            )
            cmake_args.append(
                f"-DLIBCXX_INCLUDE_DIR={cmake_path(os.path.join(webrtc_info.libcxx_dir, 'include'))}"
            )
        else:
            raise Exception(f"Platform {platform} not supported.")

        cmd(["cmake", BASE_DIR, *cmake_args])

        if platform == "ios":
            cmd(
                [
                    "cmake",
                    "--build",
                    ".",
                    f"-j{multiprocessing.cpu_count()}",
                    "--config",
                    configuration,
                    "--target",
                    "SoraUnitySdk",
                    "--",
                    "-arch",
                    "x86_64",
                    "-sdk",
                    "iphonesimulator",
                ]
            )
            cmd(
                [
                    "cmake",
                    "--build",
                    ".",
                    f"-j{multiprocessing.cpu_count()}",
                    "--config",
                    configuration,
                    "--target",
                    "SoraUnitySdk",
                    "--",
                    "-arch",
                    "arm64",
                    "-sdk",
                    "iphoneos",
                ]
            )
            cmd(
                [
                    "lipo",
                    "-create",
                    "-output",
                    os.path.join(unity_build_dir, "libSoraUnitySdk.a"),
                    os.path.join(unity_build_dir, "Release-iphonesimulator", "libSoraUnitySdk.a"),
                    os.path.join(unity_build_dir, "Release-iphoneos", "libSoraUnitySdk.a"),
                ]
            )
        else:
            cmd(
                [
                    "cmake",
                    "--build",
                    ".",
                    f"-j{multiprocessing.cpu_count()}",
                    "--config",
                    configuration,
                ]
            )


if __name__ == "__main__":
    main()
