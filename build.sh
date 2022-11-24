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
echo '编译内核'
gcc -m32 -c kernel/main.c && ld -m elf_i386 main.o -Ttext 0xc0001500 -e main -o bin/kernel.bin

echo 写入MBR
dd if=bin/mbr.bin of=hd60M.img bs=512 count=1 conv=notrunc

echo 写入LOADER
dd if=bin/loader.bin of=hd60M.img bs=512 count=3  seek=2 conv=notrunc

echo 写入 kernel
dd if=bin/kernel.bin of=hd60M.img bs=512 count=200  seek=9 conv=notrunc

