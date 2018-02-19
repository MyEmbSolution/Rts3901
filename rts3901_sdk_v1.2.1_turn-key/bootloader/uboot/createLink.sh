#!/bin/sh
rm -rf ./board/rlxboard
mkdir -p ./board

if grep  ^CONFIG_BOARD_RTS3901=y .config >/dev/null 2>&1; then
	mkdir -p ./bsp/RTS3901;
	ln -s -r ./bsp/RTS3901 ./board/rlxboard;
fi

if grep ^CONFIG_BOARD_RTS3903=y .config >/dev/null 2>&1; then
	mkdir -p ./bsp/RTS3903;
	ln -s -r ./bsp/RTS3903 ./board/rlxboard;
fi

if grep ^CONFIG_BOARD_RLE0745=y .config >/dev/null 2>&1; then
	mkdir -p ./bsp/RLE0745;
	ln -s -r ./bsp/RLE0745 ./board/rlxboard;
fi
