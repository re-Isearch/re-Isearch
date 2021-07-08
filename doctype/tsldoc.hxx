/*-@@@
File:		tsldoc.hxx
Version:	1.00
Description:	Class TSLDOC - TSL Tab Sep. List Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef TSLDOC_HXX
#define TSLDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "colondoc.hxx"
#endif
#include "buffer.hxx"

class TSLDOC :  public COLONDOC {
  friend class OZSEARCH;
  friend class TSVDOC;
public:
  TSLDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void AddFieldDefs();
  virtual void AfterIndexing();
  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);
  virtual void SourceMIMEContent(PSTRING StringBufferPtr) const;
  virtual void SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const;
  ~TSLDOC();

protected:
   void        SetIFS(const char *newValue) { IFS = newValue; };

private:
   INT         UseFirstRecord;
   ArraySTRING FieldNames;
   off_t        Lineno;
   size_t      Columns;
   STRING      FileName;
   MMAP        FileMap;
   BUFFER      Buffer;
   const char *IFS;
};

class TSVDOC : public TSLDOC {
  public:
    TSVDOC(PIDBOBJ DbParent, const STRING& Name) : TSLDOC(DbParent, Name) {
      UseFirstRecord = 1;
    }
    const char *Description(PSTRLIST List) const;
    ~TSVDOC() { ; }
};

typedef TSLDOC* PTSLDOC;

#endif
