#!/bin/bash

cmake --build . --config Release
cd rundir/Release/bin/*

windeployqt obs*
