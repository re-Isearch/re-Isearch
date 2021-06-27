#!/opt/BSN/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

import sys
import string
from IB import *


#############################################################################
##
##
#############################################################################

query = "macbeth";
junk="/tmp/JUNK";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

record = pdb.GetDocumentInfo(2);
print "Path=",record.GetFullFileName();
print "    bytes", record.GetRecordStart(), "-", record.GetRecordEnd();
print "Locale=", record.GetLocale();
print "Date=", record.GetDate();
print "DateCreated=", record.GetDateCreated();
print "DateModified=", record.GetDateModified();
print "Priority=", record.GetPriority();
print "Category=", record.GetCategory();

