/************************************************************************
************************************************************************/

/*@@@
File:    aps_ocr.hxx
Version: 1.00
Description:   Class APS_OCRDOC - Apex OCR format 
Author:     Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

#ifndef APS_OCRDOC_HXX
#define APS_OCRDOC_HXX

#include "defs.hxx"
#include "doctype.hxx"

class APS_OCRDOC : public DOCTYPE {
public:
  APS_OCRDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseRecords(const RECORD& FileRecord);
  void ParseFields(PRECORD NewRecord);
  void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
   const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
   const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, PSTRING StringBuffer) const;

  ~APS_OCRDOC();
};

typedef APS_OCRDOC* PAPS_OCRDOC;

#endif

