#! /bin/sh
#当前工程目录
work_path=$(dirname $(readlink -f $0))
echo $work_path
VER_FILE="${work_path}/src/version.h"
MAJOR_VER=`git describe --tags | sed 's/V\(.*\)-\(.*\)-g\(.*\)/\1/'`
MINOR_VER=`git describe --tags | sed 's/V\(.*\)-\(.*\)-g\(.*\)/\2/'`
HASH_VER=`git describe --tags | sed 's/V\(.*\)-\(.*\)-g\(.*\)/\3/'`

echo "Generated" $VER_FILE
echo "#ifndef __VERSION_H__" > $VER_FILE
echo "#define __VERSION_H__" >> $VER_FILE
echo "" >> $VER_FILE
echo "#define MAJOR_VER $MAJOR_VER" >> $VER_FILE
echo "#define MINOR_VER $MINOR_VER" >> $VER_FILE
echo "#define HASH_VER \"$HASH_VER\"" >> $VER_FILE
echo "" >> $VER_FILE
echo "#endif" >> $VER_FILE

