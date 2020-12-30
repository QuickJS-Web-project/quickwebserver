#!/usr/bin/env bash

cd c

PLATFORM=$(uname -s)

printf "QuickWebServer: building shared lib on $PLATFORM\nThis action might take a while, please be patient\n"

printf "Step 1. Downloading all sources\n"

curl -L -s https://github.com/bellard/quickjs/tarball/master  --output quickjs_src.tar.gz
tar -xzf quickjs_src.tar.gz > /dev/null && cp -r ./bellard-quickjs*/* ./ > /dev/null
rm -rf bellard-quickjs* quickjs_src.tar.gz > /dev/null
curl -s https://raw.githubusercontent.com/jeremycw/httpserver.h/master/httpserver.h --output httpserver.h > /dev/null

printf "Step 2. Building QuickJS\n"

make > /dev/null

printf "Step 3. Building server shared lib\n"

if [[ $PLATFORM = "Darwin" ]]
  then
    clang -Wall -MMD -MF ../quickwebserver.o.d -Wno-array-bounds -O2 -flto -c -I/usr/local/include/quickjs -o ../quickwebserver.o quickwebserver.c
    clang -Wall -fPIC -shared -std=gnu17 -flto -L./ -lquickjs -DJS_SHARED_LIBRARY quickwebserver.c -o ../quickwebserver.so
  else
    gcc -c -fPIC -o ../quickwebserver.o quickwebserver.c -DJS_SHARED_LIBRARY -L. -lquickjs && gcc -fPIC -shared -o ../quickwebserver.so ../quickwebserver.o
fi

printf "\nDONE!\n"