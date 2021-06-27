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

query = sys.argv[1:] and sys.argv[1] or 'chain test OR'
junk="/tmp/XML";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";
print "Fields = ", pdb.GetFields();

rset = pdb.VSearchRpn(query, ByScore, 300); # RPN Query
total = rset.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

# Print the results....
for i in range(1,total+1):
  result = rset.GetEntry(i);
  area = pdb.Context(result, "___", "____") ;
  datum = result.GetDate();

  score  = result.GetScore();
  hits   = result.GetHitTable();
  print "[", i , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Present(result, ELEMENT_Brief);
  print "\tFormat: ", result.GetDoctype();
  print "\tFile:", result.GetFullFileName(), "  [", result.GetRecordStart(), "-", result.GetRecordEnd(), "]";
  print "\tDate: ", datum.RFCdate();
  print "\tMatch: ", area;
