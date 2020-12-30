OBJDIR=.obj
CC=clang
CFLAGS=-g -Wall -MMD -MF $(OBJDIR)/$(@F).d -Wno-array-bounds
LDFLAGS=-flto
CFLAGS_OPT=$(CFLAGS) -O2 -flto
DEFINES:=-D_GNU_SOURCE
CFLAGS+=$(DEFINES)

LIBS=/usr/local/lib/quickjs/libquickjs.a
INCLUDES=-I/usr/local/include/quickjs

$(OBJDIR):
	mkdir -p $(OBJDIR)

webserver_test: $(OBJDIR) $(OBJDIR)/webserver.o $(OBJDIR)/webserver_test.o
	$(CC) $(LDFLAGS) $(CFLAGS_OPT) -o $@ $(OBJDIR)/webserver_test.o $(OBJDIR)/webserver.o $(LIBS) -lm -ldl
	strip webserver_test

$(OBJDIR)/webserver.o: ./c/webserver.c
	$(CC) $(LDFLAGS) $(CFLAGS_OPT) -c $(INCLUDES) -o $@ ./c/webserver.c

$(OBJDIR)/webserver_test.o: webserver_test.c
	$(CC) $(LDFLAGS) $(CFLAGS_OPT) -c $(INCLUDES) -o $@ webserver_test.c

webserver_test.c: webserver_test.js
	qjsc -flto -D src/worker/server_worker.js -e -M webserver,webserver -m -o $@ webserver_test.js

clean:
	rm ./webserver_test
	rm webserver_test.c
	rm -rf $(OBJDIR)