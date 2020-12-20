#!/usr/bin/env bash
cd c
PLATFORM=$(uname -s)

if [[ $PLATFORM = "Darwin" ]]
  then
    clang -shared -undefined dynamic_lookup -DJS_SHARED_LIBRARY -o ../libqws.so webserver.c
  else
    gcc -c -fPIC -o libqws.o webserver.c && gcc -shared -o ../libqws.so libqws.o
fi
