#!/usr/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import sys
import string
from IB import *


#############################################################################
##
##
#############################################################################

query = "macbeth";
junk="/usr/index/SHAKESPEARE/SHAKESPEARE";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

#log_init(255);

irset0 = pdb.SearchRpn(query, ByScore); # RPN Query
total = irset0.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

query = "line/love";
irset1 = pdb.SearchRpn(query, ByScore); # RPN Query
total = irset1.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records";


field = "scene";

print "Now within ", field ;

irset = irset0.Within(irset1, field);
total = irset.GetTotalEntries();

print "Got ", total;

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
  print "Table = ", result.XMLHitTable();
  print "Total = ", result.GetHitTotal();
  print "RefCount = ", result.GetRefcount_();
  print "Total = ", result.GetHitTotal();
