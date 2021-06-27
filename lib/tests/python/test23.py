#!/user/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import sys
import string
from IB import *


#############################################################################
##
##
#############################################################################

query = sys.argv[1:] and sys.argv[1] or 'ba-nking'
junk="/opt/nonmonotonic/data/REUTERS2";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

elements = pdb.GetTotalRecords();

print "Database ", junk, " has ", elements, " elements";

if elements > 0:

	p = pdb.GetIDB(1);
	print "IDB =", p;
	mdt = p.GetMainMdt();
	print "MDT = ", mdt;
	irset = pdb.SearchSmart(query);
	rset = None;
	total = 0;
	if irset != None:
		rset = irset.GetRset(20);
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
