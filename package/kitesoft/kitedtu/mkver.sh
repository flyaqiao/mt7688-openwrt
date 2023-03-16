#! /bin/sh
#当前工程目录
work_path=$(dirname $(readlink -f $0))
echo $work_path
VER_FILE="${work_path}/src/version.h"
SVNVER=`git rev-list HEAD | wc -l | awk '{print $1}'`
GITVER="$(git rev-list HEAD -n 1 | cut -c 1-7)"

echo "Generated" $VER_FILE
echo "#ifndef __VERSION_H__" > $VER_FILE
echo "#define __VERSION_H__" >> $VER_FILE
echo "" >> $VER_FILE
echo "#define SVNVERSION $SVNVER" >> $VER_FILE
echo "#define GITVERSION \"$GITVER\"" >> $VER_FILE
echo "" >> $VER_FILE
echo "#endif" >> $VER_FILE

