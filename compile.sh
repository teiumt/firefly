#!/bin/sh
ffly_target=
ffly_linker=
if [ $(bash find.bash "$3" "--ffly-client") -eq 0 ]; then
	ffly_target="FFLY_CLIENT"
	ffly_linker=-lffly_client
elif [ $(bash find.bash "$3" "--ffly-server") -eq 0 ]; then
	ffly_target="FFLY_SERVER"
	ffly_linker=-lffly_server
elif [ $(bash find.bash "$3" "--ffly-studio") -eq 0 ]; then
	ffly_target="FFLY_STUDIO"
	ffly_linker=-lffly_studio
elif [ $(bash find.bash "$3" "--ffly-worker") -eq 0 ]; then
	ffly_target="FFLY_WORKER"
	ffly_linker=-lffly_worker
elif [ $(bash find.bash "$3" "--ffly-test") -eq 0 ]; then
	ffly_target="FFLY_TEST"
	ffly_linker=-lffly_test
fi

. ./configure.sh "$3 $4"

echo "building for '$ffly_target'"
echo "src: $2, dest: $1"
echo "extra options: $4"

ffly_window=
if [ $(bash find.bash "$FFLY_ARGS" "--using-x11") -eq 0 ]; then
	ffly_window=-DUSING_X11
elif [ $(bash find.bash "$FFLY_ARGS" "--using-xcb") -eq 0 ]; then
	ffly_window=-DUSING_XCB
fi

extra_defines=
if [ $(bash find.bash "$FFLY_ARGS" "--with-room-manager") -eq 0 ]; then
	extra_defines="-DROOM_MANAGER"
fi

if [ $(bash find.bash "$FFLY_ARGS" "--with-obj-manager") -eq 0 ]; then
	extra_defines="$extra_defines -D__WITH_OBJ_MANAGER"
fi

if [ $(bash find.bash "$FFLY_ARGS" "--rm-layering") -eq 0 ]; then
	extra_defines="$extra_defines -D__RM_LAYERING"
fi

if [ $(bash find.bash "$FFLY_ARGS" "--with-uni-manager") -eq 0 ]; then
	extra_defines="$extra_defines -D__WITH_UNI_MANAGER"
fi

make FFLY_TARGET=$ffly_target FFLY_WINDOW=$ffly_window EXTRA_DEFINES="$extra_defines"
g++ -std=c++11 $ARC $CXXFLAGS -D$ffly_target $ffly_window $extra_defines $GPU_CL_TYPE $CUDART_INC $CUDART_LIB -Iinc -Llib -Wall -o $1 $2 $ffly_linker $LDFLAGS