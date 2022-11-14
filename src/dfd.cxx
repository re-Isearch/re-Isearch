/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*-@@@
File:		dfd.cxx
Description:	Class DFD - Data Field Definition
@@@*/

#include "common.hxx"
#include "dfd.hxx"

#pragma ident  "@(#)dfd.cxx"


DFD::DFD()
{
  FileNumber = 0;
}

DFD::DFD(const DFD& OtherDfd)
{
  FileNumber = OtherDfd.FileNumber;
  Attributes = OtherDfd.Attributes;
}

DFD& DFD::operator=(const DFD& OtherDfd)
{
  FileNumber = OtherDfd.FileNumber;
  Attributes = OtherDfd.Attributes;
  return *this;
}

#if 0
bool DFD::GetFieldName(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldName(StringBuffer);
}

bool DFD::GetFieldType(PSTRING StringBuffer) const
{
  return Attributes.AttrGetFieldType(StringBuffer);
}
#endif


bool  DFD::checkFieldName(const STRING& Fieldname) const
{
  static const char ReservedViolationError[] =
        "Fieldname '%s' is a reserved name. Record presentations MAY hang!!";
  static const char ReservedViolationWarning[] =
        "Fieldname '%s' %s create problems. Single letter fields are reserved.";
  static const char ReservedViolationFatal[] = 
	"Fieldname '%s' contains a reserved character '%c'. Skipping.";
  int               pos = 0;

 if (Fieldname.IsEmpty())
    {
      return false;
    }
  if (Fieldname == FULLTEXT_MAGIC)
    {
      message_log (LOG_ERROR, ReservedViolationError, Fieldname.c_str());
    }
  else if (Fieldname.GetLength() == 1)
    {
      // Install the ones we don't want to complain about
      switch (Fieldname[0]) {
	case 'A': case 'a': case 'P': case 'D': case '.': case '@': break;
	case 'S': case 'L': case 'M': case 'H':
	  message_log (LOG_ERROR, ReservedViolationWarning, Fieldname.c_str(), "can");
	  break;
	default:
	  message_log (LOG_WARN, ReservedViolationWarning, Fieldname.c_str(), "may");
      }
    }

  // Fields should not contain \, *, ?, { or [
  // -- can be extended to block other characters if desired
  else for (const char *ptr = Fieldname.c_str(); *ptr; ptr++)	  
    {
      // Don't want \ and don't want regular expression/wildcards
      //
      // see fieldmatch.cxx:  bool STRING::FieldMatch
      //
      if (*ptr == *__AncestorDescendantSeperator ||
		      *ptr == '[' || *ptr == '*' || *ptr == '?' || *ptr == '{')
      {
	message_log (LOG_ERROR, ReservedViolationFatal, Fieldname.c_str(), *ptr );
	return false;
      }
    }
  return true;
}


DFD::~DFD()
{
}
