#!/usr/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import string
import sys
sys.path.append('../lib/python%s.%s' % (sys.version_info[0:2]))

print ("Now load IB");
import IB



junk= "/tmp/FOO";
print ("Hello");

print( type(junk))
pdb = IB.VIDB(junk);
print ("This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID()) );



