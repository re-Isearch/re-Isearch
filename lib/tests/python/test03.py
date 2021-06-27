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

query = sys.argv[1:] and sys.argv[1] or 'chain test OR'

#log_init (LOG_ALL);
junk="/tmp/JUNK";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

print "Search for", query;

#irset = pdb.SearchRpn(query, ByScore); # RPN Query
#print irset;
#rset = irset.GetRset();

rset = pdb.VSearchRpn(query, ByScore);



total = rset.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

# Print the results....
for i in range(0,total):
  print "---- ", i, " of ", total;
  result = rset[i];
  print "result type = ", result;
  score  = result.GetScore();
  print "score = ", score;
  print "[", i+1 , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Present(result, ELEMENT_Brief);
  print "\tFormat: ", result.GetDoctype();
  print "\tFile:", result.GetFullFileName(), "  [", result.GetRecordStart(), "-", result.GetRecordEnd(), "]";
  print "\tDate: ", result.GetDate().RFCdate();
  print "Table = ", result.XMLHitTable();
  print "Total = ", result.GetHitTotal();
  print "RefCount = ", result.GetRefcount_();
  print "Total = ", result.GetHitTotal();

print "Done";
