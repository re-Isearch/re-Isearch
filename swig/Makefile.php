PROJECT=IB
CCC=g++
SHARED=-shared -s

########## Target Selection  ##############################
TYP=php
PREFIX=Php
OPTIONS= #-prefix $(PROJECT)
EXT=.php
############################################################

############################################################
#### This is the configuration stuff  ######################

SWIG=swig 
INCLUDE=/usr/local/include/php/Zend -I/usr/local/include/php/main -I/usr/local/include/php/TSRM
#OPT= -DSWIG -O5 -march=pentiumpro 
OPT= -DSWIG -O5 -march=athlon64 -mtune=k8

PIC=-fpic

############ end configuration #############################
############################################################

BIN_DIR=../$(TYP)
MODULE=$(PREFIX)$(PROJECT)
VERSION=`cat .version`
CCCFLAGS=-c $(PIC) $(OPT)
INC=-I/usr/local/include/$(TYP) -I../src -I$(INCLUDE)
LIBS=-lBSn -libDoc -libSearch -libIO -libProcess -lIBctype


target: dll 

OBJS=ib_$(TYP)wrap.o #pyglue.o

.version: ../bin/Iindex
	./vergen
	cp .version ../lib/$(TYP)

ib_$(TYP)wrap.cxx: ib.i my_typedefs.i .version
	$(SWIG) -$(TYP) -phpfull -c++ $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i

$(MODULE): ib.i my_typedefs.i
	$(SWIG) -c++ -$(TYP)  $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i

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

$(MODULE).so: $(OBJS)
	$(CCC) $(SHARED) -o $(MODULE).so.$(VERSION) $(OBJS)  -L../lib $(LIBS) 
	rm -f $(MODULE).so
	rm -f $(PROJECT)c.so
	ln -s $(MODULE).so.$(VERSION) $(MODULE).so
	ln -s $(MODULE).so $(PROJECT)c.so
