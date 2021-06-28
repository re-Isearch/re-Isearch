#!/opt/BSN/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

import sys
import string
from IB import *

import sys
sys.path.append('/opt/BSN/lib/python' )


#############################################################################
##
##
#############################################################################

junk=sys.argv[1:] and sys.argv[1] or "/tmp/JUNK";
query = sys.argv[2:] and sys.argv[2] or 'chain test OR'
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements with ", pdb.GetTotalWords(), " words";
rset = pdb.VSearchRpn(query, ByScore, 0); # RPN Query
if rset:
  total = rset.GetTotalEntries();
  print "Searching for: ", query;
  print "Got = ", total, " Records"; 

  # Print the results....
  for i in range(1,total+1):
    result = rset.GetEntry(i);
    score  = result.GetScore();
    hits   = result.GetHitTable();
    print "[", i , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Present(result, ELEMENT_Brief);
    print "Result = ", result;
#    print pdb.GetFieldData(result, "KEYWORDS");
    print "\tFormat: ", result.GetDoctype();
    print "\tFile:", result.GetFullFileName(), "  [", result.GetRecordStart(), "-", result.GetRecordEnd(), "]";
    print "\tDate: ", result.GetDate().RFCdate();
    print "\tHitTotal:", result.GetHitTotal();
    print "        with ", result.GetAuxCount(), " different";
    print "\t\tHit Table:", hits;
    print "---\n";
