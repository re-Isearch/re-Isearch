#!/opt/BSN/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

import sys
import string
import array
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

foo = 0, IDB(bard);
for i in range(1, 10) :
  print "Open", i;
  array2 = i, IDB(bard);
  foo += array2;

print foo;

raise ValueError, "Foo"

pdb = IDB(bard);
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `bard`

print "# IB Index '"+shakespeare+"' has ", pdb.GetTotalRecords(), " plays";

#log_init(255);

squery = SQUERY(query);
irset = pdb.Search(squery, ByScore);
total = irset.GetTotalEntries();
print "# Searching for: ", query;

rset = irset.Fill(1,total);

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

  old_lfc = FC(0,0);
  for k in range(0, hit_total):
	fcl = hits[k];
	fc = FC(fcl[0] + offset,fcl[1]+offset);
        lfc = pdb.GetAncestorFc (fc, "SPEECH");
	if (lfc.GetFieldStart() == old_lfc.GetFieldStart()):
		continue;
	if (lfc.GetLength() == 0) :
		continue;
	old_lfc = lfc;

	fct = pdb.GetDescendentsFCT (lfc, "SPEAKER");
	if (len(fct) == 0):
		continue;
	speaker =  pdb.GetPeerContent(FC(fct[0][0], fct[0][1]));

        node =  pdb.GetPeerNode(FC(fct[0][0], fct[0][1]));
        print "SPEAKER NODE: ", node.Name() ;

	line_fc = pdb.GetAncestorFc (fc, "LINE");
	if (line_fc.GetLength() == 0):
		continue;
#	if (fc.GetFieldStart() < line_fc.GetFieldStart()) :
#		continue;
	line  = pdb.GetPeerContent(line_fc);

	trueHit = HIT(speaker, line);
	hitTable.append(trueHit);

  count = len(hitTable);
  if (count > 1):
	print "       -- ",count,"relevant lines:";
  for l in range(0, count):
        print "      ", hitTable[l];

