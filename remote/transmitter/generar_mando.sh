#!/bin/bash
HT=~/openwrt
AT=$HT/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2
TARGET=udp_emisora
rm -f $TARGET"_interceptor"
export PATH=$PATH:$AT/bin
export STAGING_DIR=$AT
export CC=mipsel-openwrt-linux-gcc

$CC -v
PACKS=$HT"/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/"
PCAP=$PACKS"libpcap-1.5.3"
GCRYPT=$PACKS"libgcrypt-1.6.1/ipkg-install/usr"
GERR=$PACKS"libgpg-error-1.12/ipkg-install/usr"

export LDFLAGS="-L"$PCAP" -L"$GCRYPT"/lib  -L"$GERR"/lib" 

echo $LDFLAGS
$CC $LDFLAGS  -o $TARGET"_interceptor" $TARGET".c" -I$PCAP  -lpcap  -I$GCRYPT"/include" -lgcrypt -lgpg-error -I$GERR"/include"

scp  $TARGET"_interceptor" root@192.168.1.1:/tmp/$TARGET"_interceptor"

#ssh -p 443 root@192.168.251.113 "/tmp/$TARGET wlan0"

