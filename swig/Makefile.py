CC_ABI_VERSION=2
PYVERSION=2.7

############

#CC=$(CPATH)g++42
#GCC_LIBDIR=/usr/local/lib/gcc-4.2.1

CC=g++



CCC=$(CC) -fabi-version=$(CC_ABI_VERSION)
#SHARED=-G -s
SHARED=-shared

RPATH=-R
RPATH=-Wl,-rpath,

PROJECT=IB$(LIBX)

#CC=CC
#CCC=$(CC) -m64
#cc=cc

EXTRA= -lnsl -ldl -lm -lmagic


########## Target Selection  ##############################
TYP=python
PREFIX=Py
OPTIONS= -shadow -modern -new_repr -module $(PROJECT) -interface $(PREFIX)$(PROJECT) 
EXT=.py

############################################################

############################################################
#### This is the configuration stuff  ######################

SWIG=swig-2
INCLUDE=/usr/include/python$(PYVERSION)
#OPT= -DSWIG -O5 -march=pentiumpro 
MOPT= -DSWIG -O5 -march=athlon64 -mtune=k8 $(OPT)
MOPT= -DSWIG 



PIC=-fpic
#PIC=-Kpic

############ end configuration #############################
############################################################

BIN_DIR=../$(TYP)$(PYVERSION)
LIB_DIR=../lib/$(TYP)$(PYVERSION)
MODULE=$(PREFIX)$(PROJECT)
VERSION=`cat .version`
CCCFLAGS=-c $(PIC) $(MOPT)
INC=-I../src -I$(INCLUDE)
LIBS= -L$(GCC_LIBDIR) -libApps -libDoc -libSearch -libIO -libProcess -libLocal -libUtils $(EXTRA)


target: dll 

OBJS=ib_$(TYP)wrap.o #pyglue.o

#64:
#	$(MAKE) -f $(MAKEFILE) LIBX=32 DOPT=-DO_BUILD_IB64

.version: ../bin/Iindex
	./vergen
	touch $(MODULE).$(VERSION)$(EXT)
	rm $(PROJECT).$(VERSION)$(EXT)
	cp .version $(LIB_DIR) 

ib_$(TYP)wrap.cxx: ib.i my_typedefs.i .version
	@rm -f $(PROJECT)$(EXT)
	@rm -f $(PROJECT).$(VERSION)$(EXT)
	$(SWIG) -$(TYP) -c++ $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i
	rm -f $(PROJECT).$(VERSION)$(EXT)
	mv $(PROJECT)$(EXT) $(PROJECT).$(VERSION)$(EXT)
	ln -s $(PROJECT).$(VERSION)$(EXT) $(PROJECT)$(EXT)

$(MODULE): ib.i my_typedefs.i
	@rm -f $(PROJECT)$(EXT)
	@rm -f $(PROJECT).$(VERSION)$(EXT)
	$(SWIG) -c++ -$(TYP)  $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i
	rm -f $(PROJECT).$(VERSION)$(EXT)
	mv $(PROJECT)$(EXT) $(PROJECT).$(VERSION)$(EXT)
	ln -s $(PROJECT).$(VERSION)$(EXT) $(PROJECT)$(EXT)


ib_$(TYP)wrap.o: ib_$(TYP)wrap.cxx
	$(CCC) $(CCCFLAGS) $(INC) ib_$(TYP)wrap.cxx

pyglue.o: pyglue.cxx
	$(CCC) $(CCCFLAGS) $(INC) pyglue.cxx


dll: $(LIB_DIR)/$(MODULE).so
 
$(LIB_DIR)/$(MODULE).so: $(MODULE).so
	cp -f $(MODULE).so.$(VERSION) $(LIB_DIR)
	cp -f $(PROJECT).$(VERSION)$(EXT) $(LIB_DIR)
	cp -f .version $(LIB_DIR)
	rm -f $(LIB_DIR)/$(MODULE).so
	cd $(LIB_DIR); ln -s $(MODULE).so.$(VERSION) $(MODULE).so; touch $(PROJECT)$(EXT); rm $(PROJECT)$(EXT); ln -s $(PROJECT).$(VERSION)$(EXT) $(PROJECT)$(EXT)

$(MODULE).so: $(OBJS)
	$(CCC) $(SHARED)  -o $(MODULE).so.$(VERSION) $(OBJS)  -L../lib $(RPATH)/opt/BSN/lib:../lib:../: $(LIBS) 
	rm -f $(MODULE).so
	rm -f $(PROJECT)c.so
	ln -s $(MODULE).so.$(VERSION) $(MODULE).so
	ln -s $(MODULE).so $(PROJECT)c.so
