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

query = "PERSONA/MACDUFF";
junk="/usr/index/SHAKESPEARE/SHAKESPEARE";
pdb = IDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

#log_init(255);

squery = SQUERY(query);
irset = pdb.Search(squery, ByScore);
total = irset.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

rset = irset.Fill(1,total);

MainMdt = pdb.GetMainMdt();

# Print the results....
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
  print hit_total, "hits";

  old_lfc = FC(0,0);
  count = 1;
  for k in range(0, hit_total):
	if k:
		print "-------------------------------";
	fcl = hits[k];
	fc = FC(fcl[0] + offset,fcl[1]+offset);
        lfc = pdb.GetAncestorFc (fc, "LINE");
	if (lfc.GetFieldStart() == old_lfc.GetFieldStart()):
		continue;
	if (lfc.GetLength() == 0) :
		continue;
	old_lfc = lfc;
	print "lfc = ", lfc.GetFieldStart(), ",", lfc.GetFieldEnd();

	print "Hit[",count, "/", k+1,"]: ", fcl, " = ",  pdb.GetPeerContent(fc);
	pfc = pdb.GetAncestorFc (fc, "SPEECH");
	if (not pfc.Contains(fc)) :
		print "*** Not part of a <SPEECH>!";
		continue;

	fct = pdb.GetDescendentsFCT (pfc, "SPEAKER");
	print pdb.GetPeerContent(FC(fct[0][0], fct[0][1])),":",  pdb.GetPeerContent(lfc);
	print " ";
	print pdb.GetPeerContent(pfc);
	print " ";
	count = count + 1;

