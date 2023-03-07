UNAME := $(shell uname)

all:
ifeq ($(UNAME), Darwin)
	make  -f Makefile.MacOS $(MFLAGS)  
endif
ifeq ($(UNAME), Linux)
	make  -f Makefile.ubuntu $(MFLAGS)
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris $(MFLAGS)
endif

plugins:
ifeq ($(UNAME), Darwin)
	make  -f Makefile.MacOS $(MFLAGS) plugins
endif
ifeq ($(UNAME), Linux)
	make  -f Makefile.ubuntu $(MFLAGS) plugins
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris $(MFLAGS) plugins
endif


clean:
	rm -f *.o ../src/*.o ../doctype/*.o ../doctype/*.sob core ../lib/lib*.dylib ../lib/lib*.so
	rm conf.h.inc

