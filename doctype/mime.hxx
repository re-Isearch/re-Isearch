#ifndef MIME_HXX
#define MIME_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif

class FILETYP {
public:
  FILETYP (const STRING& FileName, const char *FileCmd = NULL);

  STRING Doctype() const  { return doctype;     }
  STRING Author() const   { return author;      }
  STRING MimeType() const { return mimeType;    }
  STRING FormatDesc()     { return description; }
  STRING Summary() const  { return summary;     }


  GDT_BOOLEAN Doctype(PSTRING StringBufferPtr);
  GDT_BOOLEAN Author(PSTRING StringBufferPtr);
  GDT_BOOLEAN MimeType(PSTRING StringBufferPtr);
  GDT_BOOLEAN FormatDesc(PSTRING StringBufferPtr);
  GDT_BOOLEAN Summary(PSTRING StringBufferPtr);

  ~FILETYP ();
private:
  STRING doctype;
  STRING mimeType;
  STRING description;
  STRING author;
  STRING summary;
};

#endif
