#!/bin/bash

cmake --build . --config Release
cd rundir/Release/bin/*

windeployqt obs*

signtool -f ../../../../../../mconf-deskshare/certificate/rnpcert20151203.pfx -p seRNPmc0nf_ obs*.exe
