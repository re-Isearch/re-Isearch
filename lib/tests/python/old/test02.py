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
junk="/opt/BSN/data/BSNWEB";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

elements = pdb.GetTotalRecords();

print "Database ", junk, " has ", elements, " elements";

total = 10;
if elements > 0:

        rset = pdb.VSearchRpn(query, ByScore, 300, total); # RPN Query
        print type(rset);
	rset = pdb.VSearchRpn(query, ByScore, 300); # RPN Query
	print type(rset);
	print rset;
        rset = pdb.VSearchRpn(query, ByScore); # RPN Query
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

pdb = None; # Delete
