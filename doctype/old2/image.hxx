#ifndef IMAGE_HXX
#define IMAGE_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "resourcedoc.hxx"

class IMAGEGIF :  public RESOURCEDOC {
public:
  IMAGEGIF(PIDBOBJ DbParent, const STRING& Name);
  void SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const;
  void SourceMIMEContent(PSTRING StringPtr) const;
  ~IMAGEGIF();
};

class IMAGEPNG :  public RESOURCEDOC {
public:
  IMAGEPNG(PIDBOBJ DbParent, const STRING& Name);
  void SourceMIMEContent(const RESULT& ResultRecord,
        PSTRING StringPtr) const;
  void SourceMIMEContent(PSTRING StringPtr) const;
  ~IMAGEPNG();
};


class IMAGETIFF :  public RESOURCEDOC {
public:
  IMAGETIFF(PIDBOBJ DbParent, const STRING& Name);
  void SourceMIMEContent(const RESULT& ResultRecord,
        PSTRING StringPtr) const;
  void SourceMIMEContent(PSTRING StringPtr) const;
  ~IMAGETIFF();
};

class IMAGEJPEG :  public RESOURCEDOC {
public:
  IMAGEJPEG(PIDBOBJ DbParent, const STRING& Name);
  void SourceMIMEContent(const RESULT& ResultRecord,
        PSTRING StringPtr) const;
  void SourceMIMEContent(PSTRING StringPtr) const;
  ~IMAGEJPEG();
};


#endif
