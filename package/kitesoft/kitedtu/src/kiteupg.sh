#! /bin/sh

mkdir -p /mnt/nfs
mount -o nolock ssh.kitesoft.cn:/home/flyaqiao/mt7688/openwrt-21.02/bin/packages/mipsel_24kc/base /mnt/nfs
opkg remove kitedtu
killall -9 kitedtu
opkg install /mnt/nfs/kitedtu_1.0-1_mipsel_24kc.ipk
kitedtu &
