PROJECT=IB
CCC=g++
SHARED=-shared -s
VERSION=8.4

########## Target Selection  ##############################
TYP=tcl$(VERSION)
PREFIX=Tcl
OPTIONS= -namespace  #-prefix $(PROJECT)
EXT=.tcl
############################################################

############################################################
#### This is the configuration stuff  ######################

SWIG=swig 
TCLDIR=/usr/local/include/tcl8.4/
#OPT= -DSWIG -O5 -march=pentiumpro 
OPT= -DSWIG -O5 -march=athlon64 -mtune=k8 -DUSE_TCL_STUBS


PIC=-fpic

############################################################
INCLUDE=/usr/local/include/tcl8.4/

LIBEXTRA= $(TCLDIR/lib -ltclstub$(VERSION)

############ end configuration #############################
############################################################

BIN_DIR=../$(TYP)
MODULE=$(PREFIX)$(PROJECT)
VERSION=`cat .version`
CCCFLAGS=-c $(PIC) $(OPT)
INC=-I/usr/local/include/$(TYP) -I../src -I$(INCLUDE)
LIBS=-lBSn -libDoc -libSearch -libIO -libProcess -lIBctype $(LIBEXTRA)

target: $(MODULE).so dll  

OBJS=ib_$(TYP)wrap.o #pyglue.o

.version: ../bin/Iindex
	./vergen
	cp .version ../lib/$(TYP)

ib_$(TYP)wrap.cxx: ib.i my_typedefs.i .version
	$(SWIG) -tcl -c++ $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i

$(MODULE): ib.i my_typedefs.i
	$(SWIG) -c++ -tcl  $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i


ib_$(TYP)wrap.o: ib_$(TYP)wrap.cxx
	$(CCC) $(CCCFLAGS) $(INC) ib_$(TYP)wrap.cxx

pyglue.o: pyglue.cxx
	$(CCC) $(CCCFLAGS) $(INC) pyglue.cxx


dll: ../lib/$(TYP)/$(MODULE).so
 
../lib/$(TYP)/$(MODULE).so: $(MODULE).so
	cp -f $(MODULE).so.$(VERSION) ../lib/$(TYP)
	cp -f .version ../lib/$(TYP)
	rm -f ../lib/$(TYP)/$(MODULE).so
	cd ../lib/$(TYP); ln -s $(MODULE).so.$(VERSION) $(MODULE).so; touch $(PROJECT)$(EXT); 

$(MODULE).so : $(OBJS)
	$(CCC) $(SHARED) -o $(MODULE).so.$(VERSION) $(OBJS)  -L../lib $(LIBS) 
	rm -f $(MODULE).so
	rm -f $(PROJECT)c.so
	ln -s $(MODULE).so.$(VERSION) $(MODULE).so
	ln -s $(MODULE).so $(PROJECT)c.so
