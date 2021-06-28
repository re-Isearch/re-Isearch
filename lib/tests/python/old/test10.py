#!/opt/BSN/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

import sys
import string
from IB import *

junk="/tmp/JUNK";
pdb = IDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

pdb.AddRecord ("shakespeare.xml");

pdb.SetMergeStatus(iMerge);

pdb.BeforeIndexing();

if not pdb.Index() :
  print "Indexing error encountered";

pdb.AfterIndexing();
