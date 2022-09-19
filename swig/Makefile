target: What 
SRC=ib.i my_typedefs.i .version

What:
	@echo "usage is: Make [target]"
	@echo "          where target is all, Perl, Python or Tcl"

Python: .python
Perl: .perl
Tcl: .tcl
PHP: .php
XML: .xml
Java: .java

all: .perl .python .tcl 

.version: ../bin/Iindex
	./vergen

.python: Makefile.py $(SRC)
	make -f Makefile.py

.java: Makefile.js $(SRC)
	make -f Makefile.js

.perl: Makefile.pl $(SRC)
	make -f Makefile.pl

.tcl: Makefile.tcl $(SRC)
	make -f Makefile.tcl

.php: Makefile.php $(SRC)
	make -f Makefile.php

.xml:
	swig -xml -c++  -o IB.xml ib.i
