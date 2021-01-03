#!/usr/bin/env bash

if [[ $# -eq 0 ]] ; then
    echo "QWS Makefile Generator :: usage

<path>/create-build-makefile.sh <path to your main script>
f.e.: <path>/create-build-makefile.sh /Users/myuser/work/server/server.js"
    exit 0
fi

SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
CALLPATH=$(pwd)

MAKESOURCE="APPSHORT=\$(shell basename \"\$(APPSCRIPT)\" .js)
APPPATH=\$(shell dirname \"\$(APPSCRIPT)\")

ifeq (\$(shell uname -s),Darwin)
CONFIG_DARWIN=y
endif
OBJDIR=\$(APPPATH)/.obj
ifdef CONFIG_DARWIN
CC=clang
else
CC=gcc
endif
CFLAGS=-g -Wall -MMD -MF \$(OBJDIR)/\$(@F).d -Wno-array-bounds
LDFLAGS=-flto
CFLAGS_OPT=\$(CFLAGS) -O2 -flto
DEFINES:=-D_GNU_SOURCE
CFLAGS+=\$(DEFINES)

LIBS=/usr/local/lib/quickjs/libquickjs.a
INCLUDES=-I/usr/local/include/quickjs

.PHONY: default
default: \$(APPSHORT)

\$(OBJDIR):
	mkdir -p \$(OBJDIR)

\$(APPSHORT): \$(OBJDIR) \$(OBJDIR)/quickwebserver.o \$(OBJDIR)/\$(APPSHORT).o
	\$(CC) \$(LDFLAGS) \$(CFLAGS_OPT) -o \$@ \$(OBJDIR)/\$(APPSHORT).o \$(OBJDIR)/quickwebserver.o \$(LIBS) -lm -ldl
	strip \$(APPPATH)/\$(APPSHORT)
	rm \$(APPPATH)/\$(APPSHORT).c
	rm -rf \$(OBJDIR)

\$(OBJDIR)/quickwebserver.o: \$(QWSINSTALLED)/c/quickwebserver.c
	\$(CC) \$(LDFLAGS) \$(CFLAGS_OPT) -c \$(INCLUDES) -o \$@ \$(QWSINSTALLED)/c/quickwebserver.c

\$(OBJDIR)/\$(APPSHORT).o: \$(APPPATH)/\$(APPSHORT).c
	\$(CC) \$(LDFLAGS) \$(CFLAGS_OPT) -c \$(INCLUDES) -o \$@ \$(APPSHORT).c

\$(APPPATH)/\$(APPSHORT).c: \$(APPSCRIPT)
	qjsc -flto -D \$(QWSINSTALLED)/src/worker/server_worker.build.js -e -M quickwebserver,quickwebserver -m -o \$@ \$(APPSCRIPT)

clean:
	rm ./\$(APPSHORT)
	rm \$(APPPATH)/\$(APPSHORT).c
	rm -rf \$(OBJDIR)"

MAKEFILE="APPSCRIPT=$1
QWSINSTALLED=$SCRIPTPATH
$MAKESOURCE"

echo "$MAKEFILE" > $CALLPATH/Makefile