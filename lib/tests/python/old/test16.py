#!/opt/BSN/bin/python -OO 
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

query = "love hate PEER";
junk="/usr/index/SHAKESPEARE/SHAKESPEARE";
pdb = VIDB(junk);
print "This is PyIB version %s/%s" % (string.split(sys.version)[0], pdb.GetVersionID());
print "Copyright (c) 1999 Basis Systeme netzwerk/Munich";
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `junk`

print "Database ", junk, " has ", pdb.GetTotalRecords(), " elements";

#log_init(255);

irset = pdb.SearchRpn(query, ByScore); # RPN Query
total = irset.GetTotalEntries();
print "Searching for: ", query;
print "Got = ", total, " Records"; 

rset = irset.Fill(1,total);
#rset = irset.GetRset();

# Print the results....
for i in range(1,total+1):
  result = rset.GetEntry(i);
  score  = result.GetScore();
  print "[", i , "] ", rset.GetScaledScore(score, 100), " ", score, " ", pdb.Present(result, ELEMENT_Brief);

  lines    = pdb.GetAncestorContent(result, "LINE");
  speakers = pdb.GetAncestorContent(result, "SPEECH/SPEAKER");
  scenes   = pdb.GetAncestorContent(result, "SCENE/TITLE");
  t_lines    = len(lines);
  t_speakers = len(speakers);
  t_scenes   = len(scenes);

  match = 0;
  if (t_lines == t_speakers) and (t_lines == t_scenes) :
	match = 1;

  if match :
	for j in range(0, t_lines):
		print "        Scene: ", scenes[j];
		print "      Speaker: ", speakers[j];
		print "         Line: ", lines[j];
		print "";
  else:
	print "    Scenes(s): ", scenes;
	print "  Speakers(s): ", speakers;
	print "     Lines(s): ", lines;


  actors = pdb.GetAncestorContent(result, "SCENE/SPEAKER");
  actors.sort(lambda x, y: cmp(x.lower(), y.lower()))
  print "List of Speaker(s) in the scene :", unique(actors); 
  print " ";

