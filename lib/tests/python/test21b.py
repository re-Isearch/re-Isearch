#!/user/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import sys
import string
from IB import *


class HIT:
    def __init__(self, speaker, line):
        self.speaker  = speaker
	self.line     = line
    def Speaker(self):
	return self.speaker
    def Line(self):
	return self.line
    def __repr__(self):
	return self.speaker+": "+ self.line;

#############################################################################
##
##
#############################################################################
shakespeare = "SHAKESPEARE";

query = sys.argv[1:] and sys.argv[1] or 'out spot PEER'
bard="/usr/index/"+shakespeare+"/"+shakespeare;
pdb = VIDB(bard);
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `bard`

print "# IB Index '"+shakespeare+"' has ", pdb.GetTotalRecords(), " plays";

#log_init(255);

squery = SQUERY(query);

rset = pdb.VSearch(QUERY(squery, ByScore));
total = rset.GetTotalEntries();
print "# Searching for: ", query;

#rset = irset.Fill(1,total);

MainMdt = pdb.GetMainMdt();

# Print the results....
rec = 0;
for i in range(1,total+1):
  result = rset.GetEntry(i);

  key = result.GetKey();
  entry = MainMdt.LookupByKey(key);
  mdtrec = MainMdt.GetEntry( entry );
  offset = mdtrec.GetGlobalFileStart() + mdtrec.GetLocalRecordStart();

  score  = result.GetScore();
  print "[", i , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Headline(result);

  hits = result.GetHitTable();
  hit_total = len(hits);

  hitTable = [];

  count = len(hitTable);
  if (count > 1):
	print "       -- ",count,"relevant lines:";
  for l in range(0, count):
        print "      ", hitTable[l];

