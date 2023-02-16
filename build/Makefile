UNAME := $(shell uname)

all:
ifeq ($(UNAME), Darwin)
	make -j 2 -f Makefile.MacOS 
endif
ifeq ($(UNAME), Linux)
	make -j 2 -f Makefile.ubuntu
endif
ifeq ($(UNAME), Solaris)
	make -j 2 -f Makefile.solaris
endif

clean:
	rm -f *.o ../src/*.o ../doctype/*.o ../doctype/*.sob core 
	rm conf.h.inc

