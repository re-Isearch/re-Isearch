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

query = "out spot PEER";
junk="/usr/index/SHAKESPEARE/SHAKESPEARE";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

#log_init(255);

irset = pdb.SearchRpn(query, ByScore); # RPN Query
total = irset.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

rset = irset.Fill(1,total);
#rset = irset.GetRset();

# Print the results....
for i in range(1,total+1):
  result = rset.GetEntry(i);
  score  = result.GetScore();
  print "[", i , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Present(result, ELEMENT_Brief);
  print "\tFormat: ", result.GetDoctype();
  print "\tFile:", result.GetFullFileName(), "  [", result.GetRecordStart(), "-", result.GetRecordEnd(), "]";
  print "\tDate: ", result.GetDate().RFCdate();
  print "Speaker(s): ", pdb.GetAncestorContent(result, "SPEECH/SPEAKER");
  print "   Line(s): ", pdb.GetAncestorContent(result, "LINE");

