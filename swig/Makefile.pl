PROJECT=IB
CCC=g++
SHARED=-shared -s

########## Target Selection  ##############################
TYP=perl
PREFIX=Pl
OPTIONS= -const  # -opt -interface $(MODULE)
EXT=.pm
############################################################

############################################################
#### This is the configuration stuff  ######################

SWIG=swig 
INCLUDE=/usr/local/include/perl5
OPT= -DSWIG -O5 -march=pentiumpro 

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

OBJS=ib_$(TYP)wrap.o

.version: ../bin/Iindex
	./vergen
	touch $(MODULE).$(VERSION)$(EXT)
	rm $(PROJECT).$(VERSION)$(EXT)
	cp .version ../lib/$(TYP)

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


dll: ../lib/$(TYP)/$(MODULE).so
 
../lib/$(TYP)/$(MODULE).so: $(MODULE).so
	cp -f $(MODULE).so.$(VERSION) ../lib/$(TYP)
	cp -f $(PROJECT).$(VERSION)$(EXT) ../lib/$(TYP)
	cp -f .version ../lib/$(TYP)
	rm -f ../lib/$(TYP)/$(MODULE).so
	cd ../lib/$(TYP); ln -s $(MODULE).so.$(VERSION) $(MODULE).so; touch $(PROJECT)$(EXT); rm $(PROJECT)$(EXT); ln -s $(PROJECT).$(VERSION)$(EXT) $(PROJECT)$(EXT)

$(MODULE).so: $(OBJS)
	$(CCC) $(SHARED) -o $(MODULE).so.$(VERSION) $(OBJS)  -L../lib $(LIBS) 
	rm -f $(MODULE).so
	rm -f $(PROJECT)c.so
	ln -s $(MODULE).so.$(VERSION) $(MODULE).so
	ln -s $(MODULE).so $(PROJECT)c.so
