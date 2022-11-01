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


  bool Doctype(PSTRING StringBufferPtr);
  bool Author(PSTRING StringBufferPtr);
  bool MimeType(PSTRING StringBufferPtr);
  bool FormatDesc(PSTRING StringBufferPtr);
  bool Summary(PSTRING StringBufferPtr);

  ~FILETYP ();
private:
  STRING doctype;
  STRING mimeType;
  STRING description;
  STRING author;
  STRING summary;
};

#endif
