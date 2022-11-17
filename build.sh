#!/bin/bash

tmpdir=_builds

# build
function build()
{
	dir=$tmpdir$1
	cmake -H. -B$dir -DCMAKE_BUILD_TYPE=$1

	if [ 0 != $? ]; then
        	exit 1;
	fi

	cmake --build $dir --config $1
	if [ 0 != $? ]; then
        	exit 1;
	fi

	# test
	pushd $dir && ctest -C Debug -VV

	if [ 0 != $? ]; then
		exit 1;
	fi

	popd
}

# build "Debug"
build "Release"


echo 'success';

echo 写入MBR
dd if=bin/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

echo 写入LOADER
dd if=bin/loader.bin of=hd60M.img bs=512 count=2  seek=2 conv=notrunc
