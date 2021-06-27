#!/user/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% nonmonotonic"

import sys
import string
from IB import *


class HIT:
    def __init__(self, title, link):
        self.title = title
	self.link  = link
    def Title(self):
	return self.title
    def Link(self):
	return self.link
    def __repr__(self):
	return "<A HREF=\"" + self.link + "\">"+self.title+"</A>";

#############################################################################
##
##
#############################################################################

def unique(seq, idfun=None):
    # order preserving
    if idfun is None:
        def idfun(x): return x
    seen = {}
    result = []
    for item in seq:
        marker = idfun(item)
        if marker in seen: continue
        seen[marker] = 1
        result.append(item)
    return result

query = sys.argv[1:] and sys.argv[1] or 'bim laden'
junk="/usr/index/REUTERS_LARGE/data/XML/index/REUTERS"
pdb = IDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

#log_init(255);

squery = SQUERY(query);
irset = pdb.Search(squery, ByScore);
total = irset.GetTotalEntries();
print "Searching for: ", query;

rset = irset.Fill(1,total);

MainMdt = pdb.GetMainMdt();

# Print the results....
rec = 0;
for i in range(1,total+1):
  if i:
	print "============================================";
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
        lfc = pdb.GetAncestorFc (fc, "NEWSITEM");
	if (lfc.GetFieldStart() == old_lfc.GetFieldStart()):
		continue;
	if (lfc.GetLength() == 0) :
		continue;
	old_lfc = lfc;

	fct = pdb.GetDescendentsFCT (lfc, "TITLE");
	title =  pdb.GetPeerContent(FC(fct[0][0], fct[0][1]));
	fct = pdb.GetDescendentsFCT (lfc, "HEADLINE");
	link  = pdb.GetPeerContent(FC(fct[0][0], fct[0][1]));

	trueHit = HIT(title, link);
	hitTable.append(trueHit);


  print "Total Hits=", len(hitTable);
  for l in range(0, len(hitTable)):
        print "<!-- ",l+1, "-->", hitTable[l];

