[database]
DBList=fgdc

[fgdc]
Type=ISEARCH
IndexName=test
Path=/home/warnock/src/Isite2/db
Title=A/WWW Enterprises FGDC Sample Records
Description=SRW/SRU gateway to FGDC sample records distributed with Isite
FieldMaps={geo=sru_geo.map},{gils=sru_gils.map},{dc=sru_dc.map},{cql=sru_cql.map}
Synonyms=off

[sgml]
Type=ISEARCH
Path=/home/warnock/src/Isite2/db
Title=A/WWW Enterprises FGDC Sample SGML Records
Description=SRW/SRU gateway to FGDC sample records distributed with Isite
FieldMaps={geo=sru_geo.map},{gils=sru_gils.map},{dc=sru_dc.map},{marc=sru_marc.map},{cql=sru_cql.map}
Synonyms=off

[framework]
Type=ISEARCH
IndexName=framework
Path=/home/warnock/src/Isite2/db
Title=GOS Framework
Description=SRW/SRU gateway to Geospatial OneStop sample Framework metadata records hosted by A/WWW Enterprises
FieldMaps={geo=sru_geo.map},{cql=sru_cql.map},{dc=sru_dc.map}
Synonyms=off
