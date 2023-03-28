rm -rf tmp && mkdir tmp && cp -r web_root tmp/
#find tmp -type f | xargs -n1 gzip
gcc pack.c -o pack
cd tmp && ../pack `find web_root -type f` > ../../src/packed_fs.c
cd ..
rm -rf tmp
rm pack

