#!/bin/bash


STDIR=~/openwrt/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2/
STDIR=~/openwrt/staging_dir/toolchain-mips_34kc_gcc-5.2.0_musl-1.1.11/
export PATH=$PATH:$STDIR"bin"
export STAGING_DIR=$STDIR
export CC=mips-openwrt-linux-gcc

WEB=../WEB/cgi-bin/
rm -f imu
rm -f $WEB"output"
rm -f $WEB"input"

$CC  -lm -O3 -o imu imu.c -Wno-implicit-function-declaration
$CC  -lm -O3 -o $WEB"output" $WEB"output.c" -Wno-implicit-function-declaration
$CC  -lm -O3 -o $WEB"input" $WEB"input.c" -Wno-implicit-function-declaration


