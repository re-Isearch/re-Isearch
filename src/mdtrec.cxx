/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)mdtrec.cxx  1.41 08/04/01 03:01:25 BSN"

/************************************************************************
************************************************************************/

/*@@@
File:		mdtrec.cxx
Version:	2.00
Description:	Class MDTREC - Multiple Document Table Record
Author:		Nassib Nassar, nrn@cnidr.org
Modifications:	Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#include <string.h>
#include "common.hxx"
#include "date.hxx"
#include "mdt.hxx"
#include "lang-codes.hxx"

#define SIZEOF_MAGIC 16 /* See mdt.cxx */

extern MDTHASHTABLE *_globalMDTHashTable;

MDTREC::MDTREC(MDT *Mdt)
{
  fileNameID = 0;
  pathNameID = 0;
  origfileNameID = 0;
  origpathNameID = 0;
  HashTable  = Mdt ? Mdt->MDTHashTable : _globalMDTHashTable;
  DocumentType[0] =    '\0';
  Key[0]               = '\0';
  GlobalFileStart      = 0;
  LocalRecordStart     = 0;
  LocalRecordEnd       = 0;
  Locale               = 0;
  Priority             = 0;
  Category             = 0;
  Property             = 0;

 /*
  * Rest elements are Dates so 
  SRCH_DATE      Date; 
  SRCH_DATE      DateModified;
  SRCH_DATE      DateCreated;
  SRCH_DATE      DateExpires;
*/

}

MDTREC::MDTREC(const MDTREC& OtherMdtRec)
{
  *this = OtherMdtRec;
}


MDTREC& MDTREC::operator=(const MDTREC& OtherMdtRec)
{
  memcpy(Key, OtherMdtRec.Key, DocumentKeySize);
  memcpy(DocumentType, OtherMdtRec.DocumentType, DocumentTypeSize);

  fileNameID = OtherMdtRec.fileNameID;
  pathNameID = OtherMdtRec.pathNameID;

  origfileNameID = OtherMdtRec.origfileNameID;
  origpathNameID = OtherMdtRec.origpathNameID;

  HashTable  = OtherMdtRec.HashTable;
  Locale= OtherMdtRec.Locale;
  GlobalFileStart = OtherMdtRec.GlobalFileStart;
#if 0
  GlobalFileEnd = OtherMdtRec.GlobalFileEnd;
#endif
  LocalRecordStart = OtherMdtRec.LocalRecordStart;
  LocalRecordEnd = OtherMdtRec.LocalRecordEnd;
  Date = OtherMdtRec.Date;
  DateCreated = OtherMdtRec.DateCreated;
  DateModified = OtherMdtRec.DateModified;
  Priority = OtherMdtRec.Priority;
  Category = OtherMdtRec.Category;
  Property = OtherMdtRec.Property; 
  return *this;
}

void MDTREC::FlipBytes()
{
  Swab(&pathNameID);
  Swab(&fileNameID);
  Swab(&GlobalFileStart);
#if 0
  Swab(&GlobalFileEnd);
#endif
  Swab(&LocalRecordStart);
  Swab(&LocalRecordEnd);
  Swab(&Date.d_date);
  Swab(&Date.d_rest);
  Swab(&DateCreated.d_date);
  Swab(&DateCreated.d_rest);
  Swab(&DateModified.d_date);
  Swab(&DateModified.d_rest);
  Swab(&Locale.Which);

  Swab(&Category);
  Swab(&Priority);
}


// Time To Live

int MDTREC::TTL() const
{
  return TTL(SRCH_DATE((const CHR*)NULL));
}


int MDTREC::TTL(const SRCH_DATE& Now) const
{
  if (DateExpires.Ok())
    {
      if (DateExpires <= Now)
	return 0;
      return DateExpires.MinutesDiff ( Now );
    }
  return -1;
}


void MDTREC::SetDeleted(const bool Flag)
{
  Property = Property & 0x7F;
  if (Flag) Property = Property | 0x80;
}

bool MDTREC::GetDeleted() const
{
  return (Property & 0x7f) != Property;
}


bool MDTREC::SetKey(const STRING& NewKey)
{
  bool Ok = true;
  if (NewKey.GetLength() > DocumentKeySize)
    {
      message_log (LOG_WARN, "Record Key too long (%d), truncated..", NewKey.GetLength());
      Ok = false;
      NewKey.Right(DocumentKeySize).Copy(Key, DocumentKeySize);
    }
  else
    NewKey.Copy(Key, DocumentKeySize);
  return Key[0] != '\0' && Ok;
}


void MDTREC::SetDocumentType(const DOCTYPE_ID& NewDoctypeId)
{
  SetDocumentType(NewDoctypeId.Name);
  Property = (BYTE)NewDoctypeId.Id | (Property & 0x80);
}

void MDTREC::SetDocumentType(const STRING& NewDocumentType)
{
  size_t Len = NewDocumentType.GetLength() + 1;
  if (Len > DocumentTypeSize)
    {
      if (Len - 1 != DocumentTypeSize)
        message_log (LOG_WARN, "Doctype name too long (%d), truncated..", Len);
      Len = DocumentTypeSize;
    }
  NewDocumentType.Copy(DocumentType, Len);
}

static const STRING dotDir ("./");

void MDTREC::SetOrigPathname (const PATHNAME& Pathname)
{
  const STRING filename ( Pathname.GetFileName() );

  if (filename.IsEmpty() || filename == ".")
    {
      origpathNameID = 0;
      origfileNameID = 0 ;
    }
  else
    {
      SetOrigPath (Pathname.GetPath());
      SetOrigFileName (filename);
    }
}

void MDTREC::SetPathname (const PATHNAME& Pathname)
{
cerr << "MDREC:SetPathname = " << Pathname << endl;
  SetPath (Pathname.GetPath());
  SetFileName (Pathname.GetFileName());
}

void MDTREC::SetOrigPath(const STRING& NewPath)
{
//cerr << "MDTREC SetOrigPath = " << NewPath << endl;
  origpathNameID = (NewPath.IsEmpty() ? 0 : HashTable->AddPath( AddTrailingSlash(NewPath) ));
}
void MDTREC::SetOrigFileName(const STRING& NewFileName)
{
  origfileNameID = (NewFileName.IsEmpty() ?  0 : HashTable->AddFileName(NewFileName));

}
STRING MDTREC::GetOrigPath() const
{
  if (HashTable)
    return HashTable->GetPath(origpathNameID ? origpathNameID : pathNameID);
  return NulString;
}
STRING MDTREC::GetOrigFileName() const
{
  if (HashTable)
    return HashTable->GetFileName(origfileNameID ? origfileNameID : fileNameID);
  return NulString;
}

void MDTREC::SetPath(const STRING& NewPath)
{
  pathNameID = HashTable->AddPath( NewPath.IsEmpty() ?  dotDir : AddTrailingSlash(NewPath) );
}

STRING MDTREC::GetPath() const
{
  return HashTable ? HashTable->GetPath(pathNameID) : NulString;
}

void MDTREC::SetFileName(const STRING& NewFileName)
{
   fileNameID = HashTable->AddFileName(NewFileName); 
}

STRING MDTREC::GetFileName() const
{
 return HashTable ? HashTable->GetFileName(fileNameID) : NulString;
}


PATHNAME MDTREC::GetPathname() const
{
  return PATHNAME( GetPath(), GetFileName());
}

PATHNAME MDTREC::GetOrigPathname() const 
{
  if (origpathNameID == 0 && origfileNameID == 0)
    return GetPathname();
  return PATHNAME (GetOrigPath(), GetOrigFileName() );
}

void MDTREC::SetFullFileName(const STRING& NewFullPath)
{
  size_t   pathLength = NewFullPath.GetLength();

  if (pathLength == 0)
    {
      message_log (LOG_PANIC, "Nil Path for a MDTREC?");
      return;
    }
  if (HashTable == NULL)
    {
      message_log (LOG_PANIC, "Nil Hash Table in MDTREC. Can't add paths!");
      return;
    }
  const char * const path =  NewFullPath.c_str();
  const char *       ptr  =  path + pathLength; // point to end;
  while (--ptr > path)
    {
      if (*ptr == PathSepChar() || *ptr == '/' || *ptr == '\\')
        {
	  const STRING PathComponent (  NewFullPath.SubString(1, ptr-path+1) );
	  const STRING FileComponent (  ptr + 1 );
          pathNameID = HashTable->AddPath ( PathComponent );
	  fileNameID = HashTable->AddFileName( FileComponent );
          return; // DONE
        }
    }
  // Some defaults
  pathNameID = HashTable->AddPath ( dotDir );
  fileNameID = HashTable->AddFileName( NewFullPath );
}

SRCH_DATE MDTREC::GetDate(FILE *fp, INT Index) const
{
  SRCH_DATE date;
  const char msg[] = "Read error to read MDT date element %d";
  if (Index )
    {
      if (fseek (fp, (Index - 1) * sizeof (MDTREC) + SIZEOF_MAGIC, SEEK_SET) != -1)
        {
	  if (fread (&date, sizeof(SRCH_DATE), 1, fp) > 1)
	     message_log (LOG_ERRNO, msg, "Read", Index);
        }
      else
	message_log (LOG_ERRNO, msg, "Seek", Index);
    }
  return date;
}

bool MDTREC::Write(FILE *fp, INT Index) const
{
  if (fp == NULL)
    {
      message_log (LOG_PANIC, "Can't write MDT element: MDTREC::Write(NULL, %d)", Index);
      return false;
    }
  if (Index)
    {
      if (fseek (fp, (Index - 1) * sizeof (MDTREC) + SIZEOF_MAGIC, SEEK_SET) == -1)
	{
	  message_log (LOG_ERRNO, "Seek error to write MDT element %d", Index); 
	  return false; // ERROR
	}
    }
  // Purify and Valgrind: Ignore Unitialized Memory Warning!
  return fwrite (this, sizeof (MDTREC),  1, fp) == 1;
}

bool MDTREC::Read(FILE *fp, INT Index)
{
  MDTHASHTABLE  *ht = HashTable;
  if (fp == NULL)
    {
      message_log (LOG_PANIC, "Can't read MDT element: MDTREC::Read(NULL, %d)", Index);
      return false;
    }
  if (Index)
    {
      // Purify: Ignore Unitialized Memory Warning!
      if (-1 == fseek (fp, (Index - 1) * sizeof (MDTREC) + SIZEOF_MAGIC, SEEK_SET))
	{
	  SetDeleted(true);
	  return false;
	}
    }
  if (fread (this, sizeof (MDTREC), 1, fp) != 1)
    return false;

  HashTable = ht;
  return true;
}


bool MDTREC::IsDeleted(FILE *fp, INT Index)
{
  BYTE deleted = 0xFF;
  if (-1 != fseek (fp, Index * sizeof (MDTREC) + SIZEOF_MAGIC - sizeof(BYTE), SEEK_SET))
    if (fread (&deleted, sizeof (BYTE), 1, fp) != 1) return true; // Error handle as deleted
  return deleted & 0xF0;
}


STRING MDTREC::Dump() const
{
  const GPTYPE Start = GetGlobalFileStart ();
  return STRING().form(
#ifdef _WIN32
	"%s\t%I64d-%I64d\t%s%s" 
#else
	"%s\t%lld-%lld\t%s%s"
#endif
	, GetKey().c_str(),
	(long long)(Start+GetLocalRecordStart()),
	(long long)(Start + GetLocalRecordEnd()),
	GetFileName().c_str(),
	GetDeleted() ? " <deleted>" : "" );
}

ostream& operator<<(ostream& os, const MDTREC& mdtrec)
{
  return os << mdtrec.Dump();
}


MDTREC::~MDTREC() { }

