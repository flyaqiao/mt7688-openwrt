# 判断是否软链接,如果是则切换到build.sh实际目录执行
if [ -L "$0" ]; then
    pushd $(dirname $(readlink $0))
else
    pushd $(dirname $0)
fi

rm -rf tmp && mkdir tmp && cp -r web_root tmp/
#find tmp -type f | xargs -n1 gzip
gcc pack.c -o pack
cd tmp && ../pack `find web_root -type f` > ../../src/packed_fs.c
cd ..
rm -rf tmp
rm pack

popd
