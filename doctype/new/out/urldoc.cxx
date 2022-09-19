#pragma ident  "%Z%%Y%%M%  %I% %G% %U% BSN"
/* ########################################################################

   ####################################################################### */

#include "urldoc.hxx"
#include "common.hxx"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>


URLDOC::URLDOC (PIDBOBJ DbParent, const STRING& Name):DOCTYPE (DbParent, Name)
{
}

void URLDOC::ParseRecords (const RECORD& FileRecord) 
{
  RECORD Record (FileRecord);
  STRING Url;

  FileRecord.GetFullFileName(&Url);
  if (Url.Search("://") == 0)
    {
      Record.SetDocumentType ("AUTODETECT");
    }
  else
   {
  Record.SetDocumentType ("AUTODETECT");
  Db->ParseRecords (Record);
}

URLDOC::~URLDOC ()
{
}
