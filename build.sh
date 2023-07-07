#!/bin/bash

warnings="-Wno-writable-strings -Wno-format-security -Wno-c++11-extensions -Wno-deprecated-declarations"

if [[ "$(uname)" == "Linux" ]]; then
    echo "Running on Linux"
    libs="-lX11 -lGL"
    includes="-Ithird_party"
    outputFile=schnitzel
    queryProcesses=$(echo "ps ax")

elif [[ "$(uname)" == "Darwin" ]]; then
    echo "Running on Mac"
    libs="-framework Cocoa"
    sdkpath=$(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
    cflags="-isysroot ${sdkpath} -I${sdkpath}/System/Library/Frameworks/Cocoa.framework/Headers"
    objc_dep="src/mac_platform.m"
    outputFile=schnitzel
    # clean up old object files
    rm -f src/*.o
else
    echo "Not running on Linux"
    libs="-luser32 -lgdi32 -lopengl32"
    includes="-Ithird_party"
    outputFile=schnitzel.exe
    queryProcesses=$(echo "tasklist")

    timestamp=$(date +%s)
    rm -f game_* # Remove old game_* files
    clang++ "src/game.cpp" -shared -o game_$timestamp.dll $warnings
    mv game_$timestamp.dll game.dll
fi

processRunning=$($queryProcesses | grep $outputFile)

if [ -z "$processRunning" ]; then
    echo "Process not running, building Engine"
    clang++ $includes $cflags -g "src/main.cpp" $objc_dep -o $outputFile $libs $warnings
fi

