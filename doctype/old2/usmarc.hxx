/*

File:        usmarc.hxx
Version:     1
Description: class USMARC - MARC records for library use
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef USMARC_HXX
#define USMARC_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

typedef struct mde {
  char field[4];
  char subfield[3]; // Zeros if it has no subfields
  char length[5];
  char offset[6];
} marc_dir_entry;

extern CHR *RecBuffer;
extern GPTYPE marcNumDirEntries;
extern GPTYPE marcRecordLength;
extern GPTYPE marcBaseAddr;

extern marc_dir_entry *marcDir;

class USMARC 
  : public DOCTYPE 
{
public:
  USMARC(PIDBOBJ DbParent);
  void   ParseRecords(const RECORD& FileRecord);   
  void   ParseFields(PRECORD NewRecord);
  GPTYPE ParseWords(CHR* DataBuffer, INT DataLength, INT DataOffset, 
		    GPTYPE* GpBuffer, INT GpLength);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 STRING* StringBuffer);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
		 const STRING& RecordSyntax, STRING* StringBuffer);
  ~USMARC();
private:
  void addSearchEntry(PDFT pdft, STRING fieldName, int fieldStart, int fieldEnd);
  void readFileContents(PRECORD NewRecord);
  int readRecordLength(void);
  int readBaseAddr(void);
  void readMarcStructure(PRECORD NewRecord);
  int usefulMarcField(char *fieldStr);
  int compareReg(char *s1, char *s2);
  char findNextTag(char *RecBuffer, int &pos, int &tagPos, int &tagLength);
};

typedef USMARC* PUSMARC;

#endif
