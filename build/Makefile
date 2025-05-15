UNAME := $(shell uname)

all:
ifeq ($(UNAME), Darwin)
	make -j 5  -f Makefile.MacOS-OMP $(MFLAGS)  
endif
ifeq ($(UNAME), Linux)
	make -j 3  -f Makefile.ubuntu $(MFLAGS)
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris $(MFLAGS)
endif

install:
ifeq ($(UNAME), Darwin)
	make -j 5  -f Makefile.MacOS-OMP $(MFLAGS) install
endif
ifeq ($(UNAME), Linux)
	make -j 3  -f Makefile.ubuntu $(MFLAGS) install
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris $(MFLAGS) install
endif


plugins:
ifeq ($(UNAME), Darwin)
	make -j 5 -f Makefile.MacOS $(MFLAGS) plugins
endif
ifeq ($(UNAME), Linux)
	make -j 3  -f Makefile.ubuntu $(MFLAGS) plugins
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris $(MFLAGS) plugins
endif


clean:
	rm -f *.o ../src/*.o ../doctype/*.o ../doctype/*.sob core ../lib/lib*.dylib ../lib/lib*.so
	rm conf.h.inc

reset:
	rm -f ../src/*.o ../doctype/*.o *.o
