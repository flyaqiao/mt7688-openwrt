#! /bin/sh
SVNVER=`git rev-list HEAD | wc -l | awk '{print $1}'`
UPGDIR=/home/flyaqiao/nfsroot/civetweb/web/www/kitedtu/$SVNVER
mkdir -p $UPGDIR
UPGFILE=$UPGDIR/kitedtu.ipk
cp /home/flyaqiao/mt7688/openwrt-21.02/bin/packages/mipsel_24kc/base/kitedtu_1_mipsel_24kc.ipk $UPGFILE
UPGSH=$UPGDIR/kiteupg.sh
UPGURL="http://web.kitesoft.cn:8888/kitedtu/"$SVNVER"/kitedtu.ipk"
UPGMD5=`md5sum /home/flyaqiao/mt7688/openwrt-21.02/bin/packages/mipsel_24kc/base/kitedtu_1_mipsel_24kc.ipk | awk '{print $1}'`

echo "#! /bin/sh" > $UPGSH
echo "wget -q -O/tmp/kitedtu.ipk "$UPGURL >> $UPGSH
echo "echo \"$UPGMD5  /tmp/kitedtu.ipk\" > /tmp/kitedtu.md5" >> $UPGSH
echo "md5sum -c -s /tmp/kitedtu.md5" >> $UPGSH
echo "if [ \$? -eq 0 ]; then" >> $UPGSH
echo "  opkg remove kitedtu" >> $UPGSH
echo "  opkg install /tmp/kitedtu.ipk" >> $UPGSH
echo "  if [ \$? -eq 0 ]; then" >> $UPGSH
echo "    killall -9 kitedtu" >> $UPGSH
echo "    kitedtu 2>&1 >> /dev/null &" >> $UPGSH
echo "  else" >> $UPGSH
echo "    echo install fail" >> $UPGSH
echo "  fi" >> $UPGSH
echo "  rm /tmp/kitedtu.ipk" >> $UPGSH
echo "  rm /tmp/kitedtu.md5" >> $UPGSH
echo "else" >> $UPGSH
echo "  echo MD5 fail" >> $UPGSH
echo "fi" >> $UPGSH

echo "{\"url\":\"http://web.kitesoft.cn:8888/kitedtu/"$SVNVER"/kiteupg.sh\"}"