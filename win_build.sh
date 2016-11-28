#!/bin/bash

CERT_FILE="rnpcert20151203.pfx"
CERT_PASS="seRNPmc0nf_"
BITS="32"
VS_VERSION="2015"
CONFIG="Release"
CLEAN="NO"
SIGN="NO"
DEPS_PATH="../dependencies/win$BITS/include"
QT_DIR="C:\\Qt5\\5.7\\msvc2015"

pointer=1
while [[ $pointer -le $# ]]; do
	if [[ ${!pointer} != "-"* ]]; then ((pointer++)) # not a parameter flag so advance pointer
	else
		param=${!pointer}
		((pointer_plus = pointer + 1))
		slice_len=1

		case $param in
			# paramter-flags with arguments
			-b|--bits) BITS=${!pointer_plus}; ((slice_len++));;		# 32 | 64
			-vs|--vs-version) VS_VERSION=${!pointer_plus}; ((slice_len++));;# 2013 | 2015
			-c|--config) CONFIG=${!pointer_plus}; ((slice_len++));;		# debug | release
			-qt|--qt-dir) QT_DIR=${!pointer_plus}; ((slice_len++));;	# path to qt (architecture must be the same as BITS)
			--deps) DEPS_PATH=${!pointer_plus}; ((slice_len++));;		# dependencies path

			# binary flags
			-s|--sign) SIGN="YES";;		# sign the executable
			--clean) CLEAN="YES";;		# clean everything (old builds) before start
		esac

		# splice out pointer frame from positional list
		[[ $pointer -gt 1 ]] \
			&& set -- ${@:1:((pointer - 1))} ${@:((pointer + $slice_len)):$#} \
			|| set -- ${@:((pointer + $slice_len)):$#};
	fi
done

DEPS_PATH=$(realpath ${DEPS_PATH})

if [ $CLEAN = "YES" ] ; then
	echo "Cleaning older builds"
	rm -rf build
fi

mkdir -p build
cd build

if [ $VS_VERSION = "2013" ] ;  then
	CMAKE_GENERATOR="Visual Studio 12 2013"
elif [ $VS_VERSION = "2015" ] ; then
	CMAKE_GENERATOR="Visual Studio 14 2015"
fi

if [ $BITS = "64" ] ;  then
	CMAKE_GENERATOR="$CMAKE_GENERATOR Win64"
fi

echo "Using Qt from $QT_DIR"

echo ""; echo "`tput setaf 3 && tput bold`------ CMake ------`tput sgr0`"
cmake -DDepsPath:PATH=${DEPS_PATH} -DQTDIR:PATH=${QT_DIR} -DCOPY_DEPENDENCIES:BOOL=YES -G"$CMAKE_GENERATOR" ../

echo ""; echo "`tput setaf 3 && tput bold`------ Compiling ${CONFIG}------`tput sgr0`"
cmake --build . --config $CONFIG || exit 0

echo ""; echo "`tput setaf 3 && tput bold`------ Including dependencies ------`tput sgr0`"
cd "rundir/${CONFIG}/bin/${BITS}bit"
"${QT_DIR}/bin/windeployqt" obs${BITS}.exe || exit 0

if [ $SIGN = "YES" ] ; then
	echo ""; echo "`tput setaf 3 && tput bold`------ Signing executables ------`tput sgr0`"
	signtool sign -f ../../../../../../mconf-deskshare/certificate/$CERT_FILE -p $CERT_PASS obs*.exe
fi

echo "Success."
