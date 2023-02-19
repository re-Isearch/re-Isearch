UNAME := $(shell uname)

all:
ifeq ($(UNAME), Darwin)
	make  -f Makefile.MacOS 
endif
ifeq ($(UNAME), Linux)
	make  -f Makefile.ubuntu
endif
ifeq ($(UNAME), Solaris)
	make  -f Makefile.solaris
endif

clean:
	rm -f *.o ../src/*.o ../doctype/*.o ../doctype/*.sob core ../lib/lib*.dylib ../lib/lib*.so
	rm conf.h.inc

