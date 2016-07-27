#!/bin/bash -xe

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

echo "------ Compiling and packaging ------"
make package

echo "------ Running macdeployqt to include dependencies into the bundle ------"
cd _CPack_Packages/Darwin/DragNDrop/obs-studio-*/
/opt/local/libexec/qt5/bin/macdeployqt obs.app

echo "------ Fixing path references for OBS plugins"
cd obs.app/Contents/Resources/obs-plugins/

DEV_LIB_PATH="/opt/local/lib"
PORTABLE_PATH="@rpath/../../Frameworks"

LIB_NAME="libavcodec.57.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libavfilter.6.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libavdevice.57.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libavutil.55.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libswscale.4.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libavformat.57.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libswresample.2.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-ffmpeg.so

LIB_NAME="libx264.148.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" obs-x264.so

LIB_NAME="libfreetype.6.dylib"
install_name_tool -change "$DEV_LIB_PATH/$LIB_NAME" "$PORTABLE_PATH/$LIB_NAME" text-freetype2.so

cd ../../../../

echo "------ Signing code ------"
codesign --deep --sign "Developer ID Application: Mconf Tecnologia Ltda" obs.app
