
UNAME := $(shell uname)

all:
ifeq ($(UNAME), Darwin)
	make -j 2 -f Makefile.py.MacOS 
endif
ifeq ($(UNAME), Linux)
	make -j 2 -f Makefile.py.ubuntu
endif
ifeq ($(UNAME), Solaris)
	make -j 2 -f Makefile.py.solaris
endif


