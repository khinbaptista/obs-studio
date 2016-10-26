#!/bin/bash

cmake --build . --config Debug || exit 0
exit 0

cd rundir/Release/bin/*
windeployqt obs* || exit 0

signtool sign -f ../../../../../../mconf-deskshare/certificate/rnpcert20151203.pfx -p seRNPmc0nf_ obs*.exe