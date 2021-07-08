#include "image.hxx"

//-----------------------------------
IMAGEGIF::IMAGEGIF (PIDBOBJ DbParent, const STRING& Name) :
   RESOURCEDOC (DbParent, Name)
{
}

void IMAGEGIF::SourceMIMEContent(const RESULT& ResultRecord,
   PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr); // No then default..
}

void IMAGEGIF::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "image/gif";
}

IMAGEGIF::~IMAGEGIF ()
{
}


//-----------------------------------

IMAGEPNG::IMAGEPNG (PIDBOBJ DbParent, const STRING& Name) :
   RESOURCEDOC (DbParent, Name)
{
}

void IMAGEPNG::SourceMIMEContent(const RESULT& ResultRecord,
   PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr); // No then default..
}

void IMAGEPNG::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "image/png";
}

IMAGEPNG::~IMAGEPNG ()
{
}


//-----------------------------------

IMAGETIFF::IMAGETIFF (PIDBOBJ DbParent, const STRING& Name) :
   RESOURCEDOC (DbParent, Name)
{
}

void IMAGETIFF::SourceMIMEContent(const RESULT& ResultRecord,
   PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr); // No then default..
}

void IMAGETIFF::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "image/tiff";
}

IMAGETIFF::~IMAGETIFF ()
{
}


//-----------------------------------

IMAGEJPEG::IMAGEJPEG (PIDBOBJ DbParent, const STRING& Name) :
   RESOURCEDOC (DbParent, Name)
{
}

void IMAGEJPEG::SourceMIMEContent(const RESULT& ResultRecord,
   PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr); // No then default..
}

void IMAGEJPEG::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "image/jpeg";
}

IMAGEJPEG::~IMAGEJPEG ()
{
}


//-----------------------------------

