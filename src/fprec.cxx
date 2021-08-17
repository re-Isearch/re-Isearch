/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		fprec.cxx
Description:	Class FPREC - File Pointer Record
@@@*/

#include "common.hxx"
#include "fprec.hxx"
#ifdef _WIN32
# include <io.h>
#endif

#pragma ident  "@(#)fprec.cxx"


FPREC::FPREC() {
  FilePointer = 0;
  Priority = 0;
  RefCount  = 0;
}

FPREC::FPREC(const FPREC& OtherFprec)
{
  *this = OtherFprec;
}


FPREC& FPREC::operator=(const FPREC& OtherFprec) {
  FileName    = OtherFprec.FileName;
  FilePointer = OtherFprec.FilePointer;
  OpenMode    = OtherFprec.OpenMode;
  RefCount    = OtherFprec.RefCount;
  Priority    = OtherFprec.Priority;
  return *this;
}

void FPREC::SetFileName(const STRING& NewFileName)
{
  FileName = ExpandFileSpec(NewFileName);
}

FPREC::~FPREC() {
}

FPREC::operator STRING () const
{
#if _WIN32
  if (FilePointer)
    return STRING().form("FREC('%s','%s')=%d // Ref=%d)",
        FileName.c_str(), OpenMode.c_str(), (int)_get_osfhandle (fileno(FilePointer)), RefCount);
 else
   return STRING().form("FREC('%s','%s')=NULL // Ref=%d)",
        FileName.c_str(), OpenMode.c_str(), RefCount);
#else
  return STRING().form("FREC('%s','%s')=%p // Ref=%d)",
	FileName.c_str(), OpenMode.c_str(), FilePointer, RefCount); 
#endif
}


void FPREC::Dispose()
{
  if (FilePointer)
    {
if (FilePointer == stdout || FilePointer == stderr) cerr << "DISOSE Stdio?" << endl;
      if (RefCount > 1)
	message_log (LOG_WARN, "FPREC::Dispose(): Streams were still open to '%s' (RefCount=%d)?",
		FileName.c_str(), RefCount-1);
      fclose (FilePointer);
      FilePointer = NULL;
    }
  FileName = NulString;
  RefCount = 0;
}

