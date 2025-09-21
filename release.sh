#!/bin/bash

PROJECT_DIR="${PROJECT_DIR:-/home/jetpclaptop/workspace/projects/SimplestApp}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/build/Android_Qt_6_9_0_android_arm64_v8a_Clang_arm64_v8a-Release}"
QT_HOST_PATH="${QT_HOST_PATH:-/home/jetpclaptop/qt_location/6.9.0/gcc_64}"
ANDROID_NDK="${ANDROID_NDK:-/home/jetpclaptop/Android/Sdk/ndk/29.0.14033849}"
ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-/home/jetpclaptop/Android/Sdk}"
QT_PATH="${QT_PATH:-/home/jetpclaptop/qt_location/6.9.0/android_arm64_v8a}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-30}"
ANDROID_ABI="${ANDROID_ABI:-arm64-v8a}"
JDK_PATH="${JDK_PATH:-/usr/lib/jvm/java-17-openjdk}"

KEYSTORE_PATH="${KEYSTORE_PATH:-/home/jetpclaptop/workspace/projects/SimplestApp/android_release_test.keystore}"
KEYSTORE_PASSWORD="${KEYSTORE_PASSWORD:-Piledriver}"
KEY_ALIAS="${KEY_ALIAS:-testapp}"
KEY_PASSWORD="${KEY_PASSWORD:-Piledriver}"

DEVICE_ID=""
if [ -n "$1" ]; then
    DEVICE_ID="$1"
    echo "Using device ID: $DEVICE_ID"
fi

CONAN_PROFILE="android_ndk_29"
echo "[settings]" > $CONAN_PROFILE
echo "os=Android" >> $CONAN_PROFILE
echo "arch=armv8" >> $CONAN_PROFILE
echo "compiler=clang" >> $CONAN_PROFILE
echo "compiler.version=20" >> $CONAN_PROFILE 
echo "compiler.cppstd=17" >> $CONAN_PROFILE 
echo "compiler.libcxx=c++_static" >> $CONAN_PROFILE
echo "build_type=Release" >> $CONAN_PROFILE
echo "os.api_level=${ANDROID_PLATFORM#android-}" >> $CONAN_PROFILE

# echo "[buildenv]" >> $CONAN_PROFILE
# echo "CC=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang" >> $CONAN_PROFILE
# echo "CXX=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++" >> $CONAN_PROFILE
# echo "AR=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ar" >> $CONAN_PROFILE
# echo "RANLIB=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-ranlib" >> $CONAN_PROFILE
# echo "LD=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/ld" >> $CONAN_PROFILE
# echo "STRIP=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip" >> $CONAN_PROFILE
# echo "AS=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-as" >> $CONAN_PROFILE
# echo "NM=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-nm" >> $CONAN_PROFILE

echo "[conf]" >> $CONAN_PROFILE
echo "tools.android:ndk_path=$ANDROID_NDK" >> $CONAN_PROFILE

echo "[buildenv]" >> $CONAN_PROFILE
echo "PATH+=(path)$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/" >> $CONAN_PROFILE
echo "CC=aarch64-linux-android24-clang" >> $CONAN_PROFILE
echo "CXX=aarch64-linux-android24-clang++" >> $CONAN_PROFILE

# echo "[tool_requires]" >> $CONAN_PROFILE
# echo "android-ndk/r25c" >> $CONAN_PROFILE


echo "Installing dependencies with Conan..."
conan install $PROJECT_DIR --profile=$CONAN_PROFILE --build=missing --output-folder=$BUILD_DIR

echo "Configuring CMake project..."
$QT_PATH/bin/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
    -DQT_USE_TARGET_ANDROID_BUILD_DIR:BOOL=ON \
    -DCMAKE_CXX_FLAGS_INIT:STRING= \
    -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON \
    -DCMAKE_COLOR_DIAGNOSTICS:BOOL=ON \
    -DQT_QMAKE_EXECUTABLE:FILEPATH=$QT_PATH/bin/qmake \
    -CMAKE_PREFIX_PATH:FILEPATH=$QT_HOST_PATH \
    -DCMAKE_CXX_COMPILER:FILEPATH=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang++ \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=$BUILD_DIR/build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=/home/jetpclaptop/Android/Sdk/ndk/29.0.14033849/build/cmake/android.toolchain.cmake \
    -DQT_HOST_PATH:FILEPATH=$QT_HOST_PATH \  
    -DANDROID_NDK_ROOT:PATH=$ANDROID_NDK \  
    -DANDROID_STL:STRING=c++_shared \ 
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DANDROID_USE_LEGACY_TOOLCHAIN_FILE:BOOL=OFF \
    -DCMAKE_PREFIX_PATH:PATH=$QT_PATH \
    -DCMAKE_GENERATOR:STRING=Ninja \
    -DANDROID_PLATFORM:STRING=$ANDROID_PLATFORM \
    # -DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH=$BUILD_DIR/.qtc/package-manager/auto-setup.cmake \
    -DANDROID_SDK_ROOT:PATH=$ANDROID_SDK_ROOT\
    -DCMAKE_FIND_ROOT_PATH:PATH=$QT_PATH \
    -DQT_HOST_PATH:PATH=$QT_HOST_PATH \  
    -DANDROID_ABI:STRING=$ANDROID_ABI\
    -DCMAKE_C_COMPILER:FILEPATH=$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/clang

echo "Building project..."
$QT_HOST_PATH/bin/qt-cmake --build $BUILD_DIR --target all

SIGN_PARAMS=""
if [ -f "$KEYSTORE_PATH" ]; then
    SIGN_PARAMS="--keystore $KEYSTORE_PATH --key-alias $KEY_ALIAS --store-pass $KEYSTORE_PASSWORD --key-pass $KEY_PASSWORD"
    echo "Using signing configuration:"
    echo "  Keystore: $KEYSTORE_PATH"
    echo "  Alias: $KEY_ALIAS"
    echo "  Keystore password: $KEYSTORE_PASSWORD"
    echo "  Key password: $KEY_PASSWORD"
else
    echo "WARNING: Keystore file not found at $KEYSTORE_PATH. Using unsigned APK (debug mode)."
fi

echo "Deploying Android APK..."
$QT_HOST_PATH/bin/androiddeployqt --input $BUILD_DIR/android-appSimplestApp-deployment-settings.json \
    --output $BUILD_DIR/android-build-appSimplestApp \
    --android-platform android-36 \
    --jdk "$JDK_PATH" \
    --gradle \
    --release \
    $SIGN_PARAMS \
    ${DEVICE_ID:+--device $DEVICE_ID}

if [ $? -eq 0 ]; then
    echo "Build and deployment completed successfully!"
else
    echo "Deployment failed! Check the logs above for details."
    exit 1
fi