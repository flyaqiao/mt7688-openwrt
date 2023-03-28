#! /bin/sh
SVNVER=`git rev-list HEAD | wc -l | awk '{print $1}'`
UPGDIR=/home/flyaqiao/nfsroot/civetweb/web/www/kitedtu/$SVNVER
mkdir -p $UPGDIR
UPGFILE=$UPGDIR/kitedtuos.bin
SRCFILE=/home/flyaqiao/mt7688/openwrt-21.02/bin/targets/ramips/mt76x8/openwrt-ramips-mt76x8-WMS-76X8A-12816-squashfs-sysupgrade.bin
cp $SRCFILE  $UPGFILE
UPGSH=$UPGDIR/kiteosupg.sh
UPGURL="http://web.kitesoft.cn:8888/kitedtu/"$SVNVER"/kitedtuos.bin"
UPGMD5=`md5sum $SRCFILE | awk '{print $1}'`

echo "#! /bin/sh" > $UPGSH
echo "wget -q -O/tmp/kitedtuos.bin "$UPGURL >> $UPGSH
echo "echo \"$UPGMD5  /tmp/kitedtuos.bin\" > /tmp/kitedtuos.md5" >> $UPGSH
echo "md5sum -c -s /tmp/kitedtuos.md5" >> $UPGSH
echo "if [ \$? -eq 0 ]; then" >> $UPGSH
echo "  sysupgrade /tmp/kitedtuos.bin" >> $UPGSH
echo "else" >> $UPGSH
echo "  echo MD5 fail" >> $UPGSH
echo "fi" >> $UPGSH

echo "{\"ver\":$SVNVER, \"cmd\":\"kiteosupg.sh\"}"