#
#	Makefile for RuYi Technical Analysis
#
# switches:
#	define the ones you want in the CFLAGS definition...
#
#	TRACE		- turn on tracing/debugging code
#
# only for linux platform
#
ifeq	($(OPTIMIZE),)
OPTIMIZE=-O2
endif
ifeq	($(DEBUGS),)
DEBUGS= -g
endif

INCLS=-I../include

CFLAGS=$(DEBUGS) $(OPTIMIZE) -std=gnu11 -fPIC -Wall $(INCLS)
CXXFLAGS=$(DEBUGS) $(OPTIMIZE) -std=c++11 -fPIC -Wall $(INCLS)
LDFLAGS=-pthread -shared -Wl,--version-script=export.cmap \
	-Wl,--warn-unresolved-symbols -lrt

LIB = libklocaltime.so
objs =	klocaltime.o
srcs =	klocaltime.c

all:	$(LIB)

clean:
	rm -f *.o *.a *.so *.dbg
	@rm -rf bin obj

libklocaltime.so: $(objs)
	$(CC) $(LDFLAGS) -o $@ $(objs) -ldl
	@objcopy --only-keep-debug $@ $@.dbg
	@objcopy --strip-debug --strip-unneeded $@
	@objcopy --add-gnu-debuglink=$@.dbg $@
	@cp $@ $@.dbg ../lib

klocaltime.o: klocaltime.c
	$(CC) $(CFLAGS) -c $<

depend:
	sed "/^# Automatically/q" GNUmakefile >Makefile.1
	mv Makefile.1 GNUmakefile
	$(CC) $(CFLAGS) -MM $(srcs) >> GNUmakefile
	echo "# Makefile ends here" >> GNUmakefile

# Automatically generated dependencies
klocaltime.o: klocaltime.c
# Makefile ends here
