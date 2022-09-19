#
# CNIDR Isearch SRU/SRW
#
# Answer a couple of questions and type 'make'
#
SHELL=/bin/sh
MAKE=make
RM=rm
RMFLAGS=-f

PREFIX=/home/warnock/src
BASE_DIR=$(PREFIX)/Isite2
SRC_DIR=$(BASE_DIR)/Isearch2
SRU_DIR=$(SRC_DIR)/sru-srw
BIN_DIR=$(SRC_DIR)/bin
LIB_DIR=$(BASE_DIR)/bin
INST_DIR=$(SRU_DIR)/cgi/bin

# cgi-bin Directory
# What is the path to your httpd server's cgi-bin directory?
#
# for Redhat Linux
#CGIBIN_PATH=/var/www/cgi-bin

# for SUSE Linux
#CGIBIN_PATH=/srv/www/cgi-bin

# for Debian Linux
#CGIBIN_PATH=/usr/lib/cgi-bin
# NCSA httpd default
#CGIBIN_PATH=/usr/local/etc/httpd/cgi-bin
#CGIBIN_PATH=$(BASE_DIR)/Isearch2/Isearch-cgi
CGIBIN_PATH=$(SRU_DIR)/cgi/bin


VER=2.02
BUILD_DATE=20050718

# Compiler  
# Must be an ANSI c++ compiler
#
CC=gcc
CPP=g++

# Compiler Flags
# Nothing fancy
#
#CFLAGS=-g -Wall
#CFLAGS=-O2 -Wall -DUNIX
DFLAGS=#-DDEBUG
WARN=-Wall -Wno-unused -Wno-deprecated
#CFLAGS=-g -mcpu=i486 -fwritable-strings -DUNIX -DHAVE_SSTREAM -DNETBOX_SOCKADDR_CAST -DREMOTE_INDEXING -DHAVE_SIGNAL  -Wall -Wno-unused -Wno-deprecated   -DBDB -DDBM42 $(DFLAGS)
CFLAGS=-g -mcpu=i486 -fwritable-strings -DLOG -DUNIX -DHAVE_SSTREAM -DNETBOX_SOCKADDR_CAST -DREMOTE_INDEXING -DHAVE_SIGNAL  -Wall -Wno-unused -Wno-deprecated -DGDBM $(DFLAGS)
CGI_EXT=.cgi
EXE_EXT=

# Isearch Source
# Where is your CNIDR Isearch code?
#
ISEARCH_DIR=$(SRC_DIR)
ISEARCH_LIB=libIsearch2.a

# Yaz-based library for handling CQL
YAZ_LIBNAME=yazcql
YAZ_DIR=cql
YAZ_LIB=lib$(YAZ_LIBNAME).a

# mxml library for handling XCQL
XML_LIBNAME=mxml
MXML_DIR=mxml
XML_LIB=lib$(XML_LIBNAME).a

# Thats all!  Type 'make'
#
DOCLIB=
INCLUDES=-I$(ISEARCH_DIR)/src -I$(ISEARCH_DIR)/doctype
LIB_PATH=-L$(LIB_DIR) -L.
#SYSLIBS=-lpthread  -lstdc++ -ldb -lz -lm
SYSLIBS=-lpthread  -lstdc++ -lgdbm -lz -lm
LIBS=-lIsearch2 $(SYSLIBS) $(DOCLIB)
LOCAL_LIBS=-l$(YAZ_LIBNAME) -l$(XML_LIBNAME)

OBJ=isrch_util.o isrch_cql.o cgi-util.o Config.o version.o DbInfo.o \
	sru.o sru_explain.o sru_searchretrieve.o sru_diagnostics.o 

H=cgi-util.hxx version.hxx Config.hxx DbInfo.hxx sru.hxx sru_diagnostics.hxx

MAPFILES=sru_cql.map sru_dc.map sru_geo.map sru_gils.map sru_rec.map
XML=isru.dtd isru.xsl

DIST=Isearch-sru v$(VER)
BINARIES=isearch-sru$(CGI_EXT)
LDFLAGS=-L/usr/local/lib #-static

all: yazlib xmllib isearch-sru done

yazlib: $(YAZ_LIB)

#$(YAZ_LIB):
#	cd $(YAZ_DIR);make lib CC=$(CPP) CPP=$(CPP) \
#	INCLUDES="$(INCLUDES) -I$(ISEARCH_DIR)/sru-srw"

$(YAZ_LIB):
	cd $(YAZ_DIR);make lib CFLAGS="-g -mcpu=i486 -fwritable-strings -DYAZ_NOLOG=1 -DYAZ_NOOID=1 -Wall -Wno-unused -Wno-deprecated" INCLUDES="-I."

xmllib: $(XML_LIB)

$(XML_LIB):
	cd $(MXML_DIR);./configure;make CC=$(CC) CPP=$(CPP)
	cp $(MXML_DIR)/$(XML_LIB) .

isearch-sru: isearch-sru$(CGI_EXT)

version.o: version.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -DVERS="$(VER), Release $(BUILD_DATE)" -o $@ -c version.cxx

Config.o: Config.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c Config.cxx

DbInfo.o: DbInfo.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c DbInfo.cxx

cgi-util.o: cgi-util.hxx cgi-util.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c cgi-util.cxx

sru_explain.o: sru_explain.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c sru_explain.cxx

sru_diagnostics.o: sru_diagnostics.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c sru_diagnostics.cxx

sru_searchretrieve.o: sru_searchretrieve.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c sru_searchretrieve.cxx

sru.o: sru.cxx $(H)
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c sru.cxx

isrch_cql.o: isrch_cql.cxx isrch_cql.hxx
	$(CPP) $(CFLAGS) $(INCLUDES) -I./$(YAZ_DIR) -I./$(YAZ_DIR)/yaz -o $@ -c isrch_cql.cxx

isrch_util.o: isrch_util.cxx $(H) isrch_sru.hxx
	$(CPP) $(CFLAGS) $(INCLUDES) -I./$(MXML_DIR) -o $@ -c isrch_util.cxx

isrch_sru.o: isrch_sru.cxx $(H) isrch_sru.hxx
	$(CPP) $(CFLAGS) $(INCLUDES) -o $@ -c isrch_sru.cxx

isearch-sru$(CGI_EXT): isrch_sru.o $(H) $(OBJ) $(LIB_DIR)/$(ISEARCH_LIB) $(YAZ_LIB)
	$(CPP) $(LDFLAGS) -o $@ isrch_sru.o $(OBJ) \
		$(LIB_PATH) $(LIBS) -L. $(LOCAL_LIBS)


$(ISEARCH_LIB):$(LIB_DIR)/$(ISEARCH_LIB)
	@echo ""

$(LIB_DIR)/$(ISEARCH_LIB):
	@echo "You need to make Isearch"

done:
	@echo ""
	@echo "Isearch-sru version $(VER), Release $(BUILD_DATE) built."
	@echo "Don't forget to edit the config files in cgi/bin."
	@echo ""

srcdist:
	cp README ../$(DIST).README;$(MAKE) clean;\
		cd ..;tar cvf $(DIST).tar $(DIST);gzip $(DIST).tar

clean:
	$(RM) $(RMFLAGS) *.o *.a *~ $(BINARIES) *.html
	cd $(YAZ_DIR);$(MAKE) clean
	cd $(MXML_DIR);$(MAKE) clean

realclean:
	$(RM) $(RMFLAGS) *.o *.a *~ $(BINARIES) *.html
	cd $(YAZ_DIR);$(MAKE) clean
	cd $(MXML_DIR);$(MAKE) clean

binclean:
	$(RM) $(RMFLAGS) $(BINARIES)

install:
	cp $(BINARIES) $(CGIBIN_PATH)
	@echo
	@echo $(DIST) installed.

install_all:
	cp $(BINARIES) $(CGIBIN_PATH)
	cp Isearch-sru.ini Isearch-db.sru $(CGIBIN_PATH)
	cp $(MAPFILES) $(CGIBIN_PATH)
	cp $(XML) $(CGIBIN_PATH)
	@echo
	@echo $(DIST) installed.
