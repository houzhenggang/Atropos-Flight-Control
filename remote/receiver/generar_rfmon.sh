#!/bin/bash
HT=~/OpenWrt/barrier_breaker
AT=$HT/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2



export PATH=$PATH:$AT/bin
export STAGING_DIR=$AT
export CC=mips-openwrt-linux-gcc
PACKS=$HT"/build_dir/target-mips_34kc_uClibc-0.9.33.2/"
PCAP=$PACKS"libpcap-1.5.3"
GCRYPT=$PACKS"libgcrypt-1.6.1/ipkg-install/usr"
GERR=$PACKS"libgpg-error-1.12/ipkg-install/usr"

export LDFLAGS="-L"$PCAP" -L"$GCRYPT"/lib  -L"$GERR"/lib" 
TARGET=rfmon_recv
echo $LDFLAGS
$CC $LDFLAGS  -o $TARGET $TARGET".c" -I$PCAP  -lpcap  -I$GCRYPT"/include"     -lgcrypt -lgpg-error -I$GERR"/include"

scp $TARGET root@192.168.250.71:/tmp/$TARGET


