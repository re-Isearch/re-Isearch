#!/usr/bin/python3
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import string
import sys
sys.path.append('../lib/python%s.%s' % (sys.version_info[0:2]))

print ("Now load IB");
from IB import * 



junk="/tmp/REUT";
print ("Hello");

pdb = VIDB(junk);
print ("This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID()) );



