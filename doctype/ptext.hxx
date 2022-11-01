/*-
File:        ptext.hxx
Version:     1
Author:      Edward C. Zimmermann, edz@nonmonotonic.net
*/


#ifndef PTEXT_HXX
#define PTEXT_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

class PTEXT : public DOCTYPE {
public:
   PTEXT(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

  INT UnifiedNames (const STRING& tag, PSTRLIST Value) const
    { return DOCTYPE::UnifiedNames(tag, Value); }

   virtual void BeforeIndexing();
   virtual void AfterIndexing();

   virtual void AddFieldDefs();

   void BeforeSearching (QUERY *);

   virtual void ParseRecords(const RECORD& FileRecord);
   virtual void ParseFields (PRECORD NewRecord);
   virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax,  STRING *StringBufferPtr) const;
   ~PTEXT();

  bool IsIgnoreMetaField(const STRING& FieldName) const;
protected:
   PDFT     ParseStructure(FILE *Fp, const off_t Position = 0, const off_t Length = 0);
   void     ParseStructure(DFT *pdft, const char * const mem, const off_t Position, const off_t Length);
   void     InitFields();
   void     AllowZeroLengthPages (bool OnOff) { allowZeroLengthPages = OnOff; };

private:
   STRING      satzFieldName;
   STRING      paraFieldName;
   STRING      lineFieldName;
   STRING      pageFieldName;

   STRING      satzPath; // example Page\Paragraph\Line
   STRING      paraPath;
   STRING      linePath;

   STRING      firstsatzFieldName, firstparaFieldName, firstlineFieldName;
   STRING      headlineFieldName;
   bool allowZeroLengthPages;
   bool ParseBody;
   bool initFields;
   bool initAutoFields;
};

typedef PTEXT* PPTEXT;

#endif
