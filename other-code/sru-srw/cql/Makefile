#
# Makefile for creating the library of Yaz CQL routines
#
ISITE=Isite2
ISEARCH=Isearch2

PREFIX=/usr/local/bin
BASE_DIR=/home/warnock/src
SRC_DIR=$(BASE_DIR)/$(ISITE)
BIN_DIR=$(SRC_DIR)
LIB_DIR=..

# Compiler?
#
# Must be a c++ compiler
#
CC=gcc
CPP=g++

#
# Other programs
SHELL=/bin/sh
MAKE=make
AR=ar
ARFLAGS=r
RM=rm -f

YAZ_LIBNAME=yazcql
YAZ_LIB=$(LIB_DIR)/lib$(YAZ_LIBNAME).a

#
# Warnings for all
WARN=-Wall -Wno-unused -Wno-deprecated

# Linux
#
PLATFORM=SUSE92-`uname -s`
#DFLAGS=# -DDEBUG -DFIREWALLS -DDEBUGCLASS # -DDEBUGRPN -DVERBOSE -DSAPI_DEBUG
DEFS=-DYAZ_NOLOG=1 -DYAZ_NOOID=1
CFLAGS=-g -mcpu=i486 -fwritable-strings $(DEFS) $(WARN) $(DFLAGS)
EXE_EXT=
MYRANLIB=ranlib
MYRANLIB_TEST= [ -f /usr/bin/ranlib -o -f /bin/ranlib ]

INCLUDES=-I.

OBJ=cql.o cqlutil.o nmemsdup.o nmem.o xmalloc.o cqlstdio.o cqlstring.o \
	xcqlutil.o

all: lib

bin: cql2xcql$(EXE_EXT)

lib: $(YAZ_LIB)
$(YAZ_LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(YAZ_LIB) $(OBJ) $(DBASE_OBJ)
	-if $(MYRANLIB_TEST) ; then $(MYRANLIB) $(YAZ_LIB); else true; fi

cql2xcql: $(OBJ) cql2xcql.o
	$(CPP) $(LDFLAGS) -o $@ $(OBJ) cql2xcql.o

.c.o:
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c $<

clean:
	$(RM) *~ *.o cql2xcql$(EXE_EXT) core $(LIB) $(YAZ_LIB)
	cd yaz;$(RM) *~ *.o core
