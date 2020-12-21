#!/usr/bin/env bash
cd c
PLATFORM=$(uname -s)

exit

if [[ $PLATFORM = "Darwin" ]]
  then
    clang -Wall -fPIC -shared -std=gnu17 -flto -L./ -lquickjs -DJS_SHARED_LIBRARY webserver.c -o ../libqws.so
  else
    gcc -c -fPIC -o libqws.o webserver.c -DJS_SHARED_LIBRARY -L. -lquickjs && gcc -fPIC -shared -o ../libqws.so libqws.o
fi
