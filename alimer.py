#!/usr/bin/env python
# -*- coding: ascii -*-

# Alimer
# Copyright (c) 2018 Amer Koleci and contributors.
# Licensed under the MIT License.

import argparse
import shutil
import multiprocessing
import os
import platform
import subprocess
import sys
VERSION = '0.9.0'
enableLogVerbose = False

def logError(message):
    print("[ERROR] %s" % message)
    sys.stdout.flush()
    if 0 == sys.platform.find("win"):
        pauseCmd = "pause"
    else:
        pauseCmd = "read"
    subprocess.call(pauseCmd, shell=True)
    sys.exit(1)


def logInfo(message):
    print("[INFO] %s" % message)
    sys.stdout.flush()


def logWarning(message):
    print("[WARN] %s" % message)
    sys.stdout.flush()

def logVerbose(message):
    if enableLogVerbose:
        logInfo(message)

def FindProgramFilesFolder():
    env = os.environ
    if "64bit" == platform.architecture()[0]:
        if "ProgramFiles(x86)" in env:
            programFilesFolder = env["ProgramFiles(x86)"]
        else:
            programFilesFolder = r"C:\Program Files (x86)"
    else:
        if "ProgramFiles" in env:
            programFilesFolder = env["ProgramFiles"]
        else:
            programFilesFolder = r"C:\Program Files"
    return programFilesFolder


def FindVS2017OrUpFolder(programFilesFolder, vsVersion, vsName):
    tryVswhereLocation = programFilesFolder + \
        "\\Microsoft Visual Studio\\Installer\\vswhere.exe"
    if os.path.exists(tryVswhereLocation):
        vsLocation = subprocess.check_output([tryVswhereLocation,
                                              "-latest",
                                              "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                              "-property", "installationPath",
                                              "-version", "[%d.0,%d.0)" % (
                                                  vsVersion, vsVersion + 1),
                                              "-prerelease"]).decode().split("\r\n")[0]
        tryFolder = vsLocation + "\\VC\\Auxiliary\\Build\\"
        tryVcvarsall = "VCVARSALL.BAT"
        if os.path.exists(tryFolder + tryVcvarsall):
            return tryFolder
    else:
        names = ("Preview", vsName)
        skus = ("Community", "Professional", "Enterprise")
        for name in names:
            for sku in skus:
                tryFolder = programFilesFolder + \
                    "\\Microsoft Visual Studio\\%s\\%s\\VC\\Auxiliary\\Build\\" % (
                        name, sku)
                tryVcvarsall = "VCVARSALL.BAT"
                if os.path.exists(tryFolder + tryVcvarsall):
                    return tryFolder
    logError("Could NOT find VS%s.\n" % vsName)
    return ""


def FindVS2019Folder(programFilesFolder):
    return FindVS2017OrUpFolder(programFilesFolder, 16, "2019")


class BatchCommand:
    def __init__(self, hostPlatform):
        self.commands = []
        self.hostPlatform = hostPlatform

    def AddCommand(self, cmd):
        self.commands += [cmd]

    def Execute(self):
        batchFileName = "scBuild."
        if "windows" == self.hostPlatform:
            batchFileName += "bat"
        else:
            batchFileName += "sh"
        batchFile = open(batchFileName, "w")
        batchFile.writelines([cmd_line + "\n" for cmd_line in self.commands])
        batchFile.close()
        if "windows" == self.hostPlatform:
            retCode = subprocess.call(batchFileName, shell=True)
        else:
            subprocess.call("chmod 777 " + batchFileName, shell=True)
            retCode = subprocess.call("./" + batchFileName, shell=True)
        os.remove(batchFileName)
        return retCode


if __name__ == "__main__":
     # Parse argument first.
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--target', help="Build target", choices=['windows', 'uwp', 'android', 'linux', 'ios', 'osx', 'html5'])
    parser.add_argument('--generator', help="Generator to use", choices=['vs2019', 'ninja', 'vs2019-ninja'])
    parser.add_argument('-c', '--compiler', help="Build compiler", choices=['msvc', 'clang', 'gcc'])
    parser.add_argument('--architecture', help="Build architecture", choices=['x64', 'x86', 'arm', 'arm64', 'arm7', 'arm8'])
    parser.add_argument('--config', help="Build config", choices=['Debug', 'Dev', 'Profile', 'Release'])
    parser.add_argument('--verbose', action='store_true', help="Log verbose")
    parser.add_argument('--compile', action='store_false', help='Run compile')
    parser.add_argument('--install', action='store_true', help="Run install")
    parser.add_argument('--continuous', action='store_true', help="Run continuous build (github workflow)")

    parser.parse_args()
    args = parser.parse_args()

    hostPlatform = sys.platform
    if 0 == hostPlatform.find("win"):
        hostPlatform = "windows"
    elif 0 == hostPlatform.find("linux"):
        hostPlatform = "linux"
    elif 0 == hostPlatform.find("darwin"):
        hostPlatform = "osx"

    # Parse arguments
    if args.target is None:
        target = hostPlatform
    else:
        target = args.target

    if args.generator is None:
        if target == "windows":
            generator = "vs2019"
        else:
            generator = "ninja"
    else:
        generator = args.generator

    multiConfig = (generator.find("vs") == 0)
    isNinja = (generator.find("ninja") != -1)

    if args.architecture is None:
        architecture = "x64"
    else:
        architecture = args.architecture

    if args.config is None:
        configuration = "Release"
    else:
        configuration = args.config

    if generator == "vs2019" or generator == "vs2019-ninja":
        compiler = "vc142"
    else:
        compiler = "gcc"      

    # Enable log verbosity first.
    if (args.verbose):
        enableLogVerbose = True

    compile = False
    if (args.compile):
        compile = True

    install = False
    if (args.install):
        install = True

    continuous = False
    if (args.continuous):
        continuous = True

    if continuous:
        install = True

    if install:
        compile = True

    # Setup parameters
    originalDir = os.path.abspath(os.curdir)

    # Ensure build folder exists
    buildDir = "build"
    if not os.path.exists(buildDir):
        os.mkdir(buildDir)
   
    buildSystem = generator

    if continuous:
        buildFolderName = "continuous"
    else:
        buildFolderName = "%s-%s-%s" % (target, buildSystem, architecture)
        if not multiConfig:
            buildFolderName += "-%s" % configuration

    buildDir = os.path.join(buildDir, buildFolderName)
    if not os.path.exists(buildDir):
        os.mkdir(buildDir)

    os.chdir(buildDir)
    buildDir = os.path.abspath(os.curdir)

    logInfo('Build target: ' + target)
    logInfo('Generating build files: {}-{}-{}'.format(target, buildSystem, architecture))

    parallel = multiprocessing.cpu_count()
    batCmd = BatchCommand(hostPlatform)

    if target == "windows" or target == "uwp":
        programFilesFolder = FindProgramFilesFolder()
        vsFolder = FindVS2019Folder(programFilesFolder)

        if "x64" == architecture:
            vcOption = "amd64"
            vcArch = "x64"
        elif "x86" == architecture:
            vcOption = "x86"
            vcArch = "Win32"
        elif "arm64" == architecture:
            vcOption = "amd64_arm64"
            vcArch = "ARM64"
        elif "arm" == architecture:
            vcOption = "amd64_arm"
            vcArch = "ARM"
        else:
            logError("Unsupported architecture.\n")
        vcToolset = ""
        if (buildSystem == "vs2019") and (compiler == "vc141"):
            vcOption += " -vcvars_ver=14.1"
            vcToolset = "v141,"
        elif (buildSystem == "vs2019") and (compiler == "vc140"):
            vcOption += " -vcvars_ver=14.0"
            vcToolset = "v140,"

        batCmd.AddCommand("@call \"%sVCVARSALL.BAT\" %s" % (vsFolder, vcOption))
        batCmd.AddCommand("@cd /d \"%s\"" % buildDir)

    if isNinja:
        if target == "windows" or target == "uwp":
            batCmd.AddCommand("set CC=cl.exe")
            batCmd.AddCommand("set CXX=cl.exe")
    
        if (target == "html5"):
            if "windows" == hostPlatform:
                emscriptenSDKDir = os.environ.get('EMSDK')
                emsdk_env = os.path.join(emscriptenSDKDir, "emsdk_env.bat")
                emsdk_toolchain_file = os.path.join(emscriptenSDKDir, "upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")

                batCmd.AddCommand("call \"%s\"" % (emsdk_env))
                batCmd.AddCommand("cmake -G \"Ninja\" -DCMAKE_TOOLCHAIN_FILE=\"%s\" -DCMAKE_BUILD_TYPE=\"%s\" -DCMAKE_INSTALL_PREFIX=\"sdk\" ../../" % (emsdk_toolchain_file, configuration))
            else:
                batCmd.AddCommand("source ${EMSDK}/emsdk_env.sh")
                batCmd.AddCommand("cmake -G \"Ninja\" -DCMAKE_TOOLCHAIN_FILE=${EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=\"%s\" -DCMAKE_INSTALL_PREFIX=\"sdk\" ../../" % (configuration))

            # Generate files to run servers
            if not os.path.exists(os.path.join(buildDir, "bin")):
                os.mkdir(os.path.join(buildDir, "bin"))

            _batchFileExt = "sh"
            if "windows" == hostPlatform:
                _batchFileExt = "bat"

            # Python 2
            serverFile = open(os.path.join(buildDir, "bin", "python_server." + _batchFileExt), "w")
            serverFile.writelines("python -m SimpleHTTPServer")
            serverFile.close()

            # Python 3
            serverFile = open(os.path.join(buildDir, "bin", "python_server3." + _batchFileExt), "w")
            serverFile.writelines("python -m http.server")
            serverFile.close()
        elif (target == "android"):
            androidNDKDir = os.environ.get('ANDROID_NDK')
            batCmd.AddCommand("cmake -G \"Ninja\" -DANDROID_NDK=\"%s\" -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_BUILD_TYPE=\"%s\" -DCMAKE_INSTALL_PREFIX=\"sdk\" ../../" % (androidNDKDir, configuration))
        else:
            batCmd.AddCommand("cmake -G Ninja -DCMAKE_BUILD_TYPE=\"%s\" -DCMAKE_INSTALL_PREFIX=\"sdk\" ../../" % (configuration))

        batCmd.AddCommand("ninja -j%d" % parallel)

        # Add install command
        if install:
            batCmd.AddCommand("cmake --build . --target install")
    else:
        if buildSystem == "vs2019":
            generator = "Visual Studio 16"

        if (buildSystem == "uwp"):
            batCmd.AddCommand("cmake -G \"%s\" -T host=x64 -DCMAKE_INSTALL_PREFIX=\"sdk\" -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -A %s ../../" % (generator, vcArch))
        else:
            batCmd.AddCommand("cmake -G \"%s\" -T %shost=x64 -DCMAKE_INSTALL_PREFIX=\"sdk\" -A %s ../../" % (generator, vcToolset, vcArch))

        batCmd.AddCommand("MSBuild ALL_BUILD.vcxproj /nologo /m:%d /v:m /p:Configuration=%s,Platform=%s" % (parallel, configuration, architecture))

        # Add install command
        if install:
            batCmd.AddCommand("cmake --build . --target install --config Release")

    if batCmd.Execute() != 0:
        logError("Batch command execute failed.")

    # Restore original directory
    os.chdir(originalDir)
