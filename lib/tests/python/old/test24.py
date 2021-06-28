#!/opt/BSN/bin/python
#-------------------------------------------------
#ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

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

bard="/usr/index/"+shakespeare+"/"+shakespeare;
pdb = IDB(bard);
if not pdb.IsDbCompatible():
  raise ValueError, "The specified database '%s' is not compatible with this version. Re-index!" % `bard`

print "# IB Index '"+shakespeare+"' has ", pdb.GetTotalRecords(), " plays";

#log_init(255);


query = "beschaffungsmanagement:3 OR beschaffungsmarketing:3 OR beschaffungsmarkt:3 OR beschaffungsplanung:3 OR beschaffungsprozesse:3 OR (deterministische:3 FOLLOWS beschaffung:3) OR einkaufspolitik:3 OR (stochastische:3 FOLLOWS beschaffung:3) OR strategien:2 OR strategie OR (c:3 FOLLOWS teilemanagement:3) OR beschaffungsmarktforschung:3 OR (double:4 FOLLOWS sourcing:4) OR (global:4 FOLLOWS sourcing:4) OR (modular:4 FOLLOWS sourcing:4) OR (multiple:4 FOLLOWS sourcing:4) OR (single:4 FOLLOWS sourcing:4) OR sourcing:3 OR methoden:2 OR methode OR lieferant:3 OR lieferanten:2 OR logistikdienstleister:3 OR rahmenvertraege:3 OR tul:4 OR spediteur:3 OR spediteure:2 OR spediteuren:2 OR spediteurs:2 OR stammlieferant:3 OR vertraege:3 OR vertrag:2 OR vertraegen:2 OR vertrages:2 OR vertrags:2 OR zulieferpyramide:3 OR partner:2 OR partnern OR partners OR beschaffungskosten:3 OR einkaufscontrolling:3 OR einkaufsverhandlungen:3 OR incoterms:3 OR wiederbeschaffungszeit:3 OR zahlungskonditionen:3 OR konditionen:2 OR kondition OR einfuhr:3 OR einfahre:2 OR einfahren:2 OR einfahrend:2 OR einfahrest:2 OR einfahret:2 OR einfahrt:2 OR einfuehrt:2 OR einfuehre:2 OR einfuhren:2 OR einfueren:2 OR einfuehrest:2 OR einfuehret:2 OR einfuhrst:2 OR einfuhrt:2 OR eingefahren:2 OR einzufahren:2 OR eust:4 OR einfuhrumsatzsteuer:3 OR inbound:3 OR jis:4 OR (just:3 FOLLOWS in:3 FOLLOWS sequence:3) OR jit:4 OR (just:3 FOLLOWS in:3 FOLLOWS time:3) OR sendungverfolgung:3 OR stapler:3 OR staplern:2 OR staplers:2 OR we:4 OR wareneingang:3 OR wa:4 OR warenausgang:3 OR wareneingangskontrolle:3 OR zoll:3 OR zoelle:2 OR zoellen:2 OR zolles:2 OR zolln:2 OR zolls:2 OR gezollt:2 OR zolle:2 OR zollen:2 OR zollend:2 OR zollest:2 OR zollet:2 OR zollst:2 OR zollt:2 OR zollte:2 OR zollten:2 OR zolltest:2 OR zolltet:2 OR zollware:3 OR transport:2 OR transporte OR transporten OR transportes OR transports" 

squery = SQUERY(query);

print squery.GetRpnTerm();
