PROJECT=IB
CCC=g++
GCC_LIBDIR=/usr/local/lib/gcc-4.2.1

############################################################
#### This is the configuration stuff  ######################

SWIG=swig
TYP=java
PREFIX=J
OPTIONS= 
EXT=.java
MY_INC=-I/usr/local/jdk1.5.0/include -I/usr/local/jdk1.5.0/include/freebsd


############ end configuration #############################
############################################################
BIN_DIR=../$(TYP)
MODULE=$(PREFIX)$(PROJECT)
VERSION=`cat .version`
#PyIB
OPT=-c -fpic 
SHARED=-shared -s
INC=-I/usr/local/include/$(TYP) $(MY_INC) -I../src
LIBS=  -L$(GCC_LIBDIR) -lBSn -libDoc$(LIBX) -libSearch$(LIBX) -libIO -libProcess -lIBlocal


target: $(MODULE)c.so

OBJS=ib_$(TYP)wrap.o

.version: ../bin/Iindex
	./vergen
	touch $(MODULE).$(VERSION)$(EXT)
	rm $(MODULE).$(VERSION)$(EXT)

ib_$(TYP)wrap.cxx: ib.i my_typedefs.i .version
	@rm -f $(MODULE)$(EXT)
	$(SWIG) -c++  -$(TYP) $(OPTIONS)  -o ib_$(TYP)wrap.cxx ib.i

$(MODULE): ib.i my_typedefs.i
	@rm -f $(MODULE)$(EXT)
	$(SWIG) -c++ -$(TYP)  $(OPTIONS) -o ib_$(TYP)wrap.cxx ib.i

ib_$(TYP)wrap.o: ib_$(TYP)wrap.cxx
	$(CCC) $(OPT) $(INC) ib_$(TYP)wrap.cxx

$(MODULE)c.so: $(OBJS)
	$(CCC) $(SHARED) -o $(MODULE)c.$(VERSION).so $(OBJS) -R../lib:/opt/BSN/lib -L../lib $(LIBS) 
	rm -f $(MODULE)c.so
	ln -s $(MODULE)c.$(VERSION).so $(MODULE)c.so
	mv $(MODULE)$(EXT) $(MODULE).$(VERSION)$(EXT)
	ln -s $(MODULE).$(VERSION)$(EXT) $(MODULE)$(EXT)

