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
    clang -shared -undefined dynamic_lookup -DJS_SHARED_LIBRARY -o ../libqws.so webserver.c > /dev/null
  else
    gcc -c -fPIC -o libqws.o webserver.c > /dev/null && gcc -shared -o ../libqws.so libqws.o > /dev/null
fi

printf "\nDONE!\n"