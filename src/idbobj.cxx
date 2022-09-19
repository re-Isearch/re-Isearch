/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:        idbobj.cxx
Version:     1.00
Description: Class IDBOBJ
@@@ */
#ifdef __GNUG__
#pragma implementation "idbobj.hxx"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
//#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include "dirent.hxx"
#include "common.hxx"
#include "idbobj.hxx"
#include "doctype.hxx"


void IDBOBJ::AddFieldType(const STRING& Definition)
{
  FieldTypes.AddEntry(Definition);
}

void IDBOBJ::AddFieldType(const STRING& FieldName, FIELDTYPE FieldType)
{
  STRING name (FieldName);
  STRING val  ((int)FieldType);
  STRING old_val = FieldTypes.GetValue(name.MakeUpper());

  if (old_val.GetLength())
    {
      // Have an old value
      if (old_val != val)
	message_log (LOG_WARN, "Field %s had a type change from %s to %s", name.c_str(), old_val.c_str(), val.c_str());
      else
	return; // Already in table
    }
  FieldTypes.AddEntry(name, val); 
}

FIELDTYPE  IDBOBJ::GetFieldType(const STRING& FieldName)
{
  STRING field (FieldName);
  STRING s ( FieldTypes.GetValue(field.MakeUpper()) );

  return s.IsEmpty() ? FIELDTYPE::any : (BYTE)s.GetInt();
}

GDT_BOOLEAN IDBOBJ::_write_resource_path(const STRING& filepath, const RECORD& Filerecord, STRING *path) const
{
  STRING mime;
  DOCTYPE *ptr = GetDocTypePtr ( Filerecord.GetDocumentType());
  if (ptr != NULL)
    ptr->SourceMIMEContent(&mime);

  return _write_resource_path (filepath, Filerecord, mime, path);
}

GDT_BOOLEAN IDBOBJ::_write_resource_path(const STRING& filepath,
	const RECORD& Filerecord, const STRING& mime, STRING *path) const
{
  const STRING FullPath ( useRelativePaths ?
	RelativizePathname(Filerecord.GetFullFileName()) : Filerecord.GetFullFileName());
  const STRING urifile ( filepath + _DbExt(ExtPath) );
  FILE  *fp;  

  if (path) *path = urifile;
  if ((fp = fopen(urifile, "wb")) != NULL)
    {
      FC     Fc (Filerecord.GetRecordStart(), Filerecord.GetRecordEnd());
 
      FullPath.Write(fp);
      Fc.Write(fp);
      mime.Write(fp);
      fclose(fp);
      return GDT_TRUE;
    }
  return GDT_FALSE;
}
         
void IDBOBJ::_get_resource_path(STRING *fullPathPtr) const
{
  if (fullPathPtr != NULL)
    {
      STRING path ( _get_resource_path ( *fullPathPtr) );
      if (path.GetLength())
        *fullPathPtr = path;
    }
}
  
    
STRING IDBOBJ::_get_resource_path(const STRING& FullPath) const
{
  STRING path (FullPath);
  STRING urifile = FullPath + _DbExt(ExtPath);
  if (FileExists(urifile))
    {
      FILE  *fp = fopen(urifile, "rb");
      if (fp != NULL)
        {
          path.Read(fp);
          fclose(fp);
        }
    }
  return ResolvePathname(path);
}

void  IDBOBJ::SetWorkingDirectory()
{
  if (WorkingDirectory.IsEmpty() || WorkingDirectory.Equals(".")) {
    if ((WorkingDirectory = GetCwd()).IsEmpty()) {
      message_log (LOG_ERRNO, "Could not determine current directory.");
      WorkingDirectory = "./";
    } else
      AddTrailingSlash(&WorkingDirectory);
  }
}  
    
void  IDBOBJ::SetWorkingDirectory(const STRING& Dir)
{
  if ((Dir.IsEmpty() && WorkingDirectory.IsEmpty()) || Dir.Equals("."))
    SetWorkingDirectory();
  else
    WorkingDirectory = AddTrailingSlash(Dir);
};
  

GDT_BOOLEAN IDBOBJ::ResolvePathname(STRING *Path) const
{
  if (Path)
    {
      if (IsAbsoluteFilePath (*Path))
        return GDT_TRUE;
      if (!WorkingDirectory.IsEmpty())
        {
          Path->Prepend( AddTrailingSlash(WorkingDirectory) );
         __Realpath(Path);
          return GDT_TRUE;
        }
    }
  return GDT_FALSE;
}
  
    
STRING IDBOBJ::ResolvePathname(const STRING& Path) const
{
  if (IsAbsoluteFilePath( Path ) || WorkingDirectory.IsEmpty())  
    return Path;
  return __Realpath (AddTrailingSlash(WorkingDirectory) + Path);
}

STRING IDBOBJ::RelativizePathname(const STRING& Path) const
{
  return ::RelativizePathname(Path, WorkingDirectory);
}
