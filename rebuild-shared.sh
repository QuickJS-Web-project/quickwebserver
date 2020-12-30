#!/usr/bin/env bash
cd c
PLATFORM=$(uname -s)

if [[ $PLATFORM = "Darwin" ]]
  then
    clang -Wall -MMD -MF ../quickwebserver.o.d -Wno-array-bounds -O2 -flto -c -I/usr/local/include/quickjs -o ../quickwebserver.o quickwebserver.c
    clang -Wall -fPIC -shared -std=gnu17 -flto -L./ -lquickjs -DJS_SHARED_LIBRARY quickwebserver.c -o ../quickwebserver.so
  else
    gcc -c -fPIC -o ../quickwebserver.o quickwebserver.c -DJS_SHARED_LIBRARY -L. -lquickjs && gcc -fPIC -shared -o ../quickwebserver.so ../quickwebserver.o
fi
