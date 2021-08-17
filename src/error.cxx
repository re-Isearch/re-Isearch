/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "error.hxx"

#define   NAMESPACE

Z3950_ERROR::Z3950_ERROR()
{
  errorCode = 0;
}

Z3950_ERROR::~Z3950_ERROR()
{
}


Z3950_ERROR& Z3950_ERROR::operator=(const Z3950_ERROR& OtherError)
{
  errorCode = OtherError.errorCode;
  return *this;
}


NAMESPACE ostream& operator<<(NAMESPACE ostream& os, const Z3950_ERROR& Error)
{
  os << Error.errorCode << ": " << Error.ErrorMessage();
  return os;
}


int Z3950_ERROR::SetErrorCode(int Error)
{
  int old_err = errorCode;
  errorCode = Error;
  return errorCode != old_err;
}

int Z3950_ERROR::GetErrorCode() const
{
  return errorCode;
}

const char *Z3950_ERROR::ErrorMessage() const
{
  return ErrorMessage(errorCode);
}

extern "C" const char * (* __Private_IB_ErrorMessage)(int);

const char *Z3950_ERROR::ErrorMessage(int ErrorCode) const
{
  switch (ErrorCode) {
    case 0:   return "No Error";
    case 1:   return "Permanent system error";
    case 2:   return "Temporary system error";
    case 3:   return "Unsupported search";
    case 4:   return "Terms only exclusion (stop) words";
    case 5:   return "Too many argument words";
    case 6:   return "Too many boolean operators";
    case 7:   return "Too many truncated words";
    case 8:   return "Too many incomplete subfields";
    case 9:   return "Truncated words too short";
    case 10:  return "Invalid format for record number (search term)";
    case 11:  return "Too many characters in search statement";
    case 12:  return "Too many records retrieved";
    case 13:  return "Present request out of range";
    case 14:  return "System error in presenting records";
    case 15:  return "Record no authorized to be sent intersystem";
    case 16:  return "Record exceeds Preferred-message-size";
    case 17:  return "Record exceeds Maximum-record-size";
    case 18:  return "Result set not supported as a search term";
    case 19:  return "Only single result set as search term supported";
    case 20:  return "Only ANDing of a single result set as search term supported";
    case 21:  return "Result set exists and replace indicator off";
    case 22:  return "Result set naming not supported";
    case 23:  return "Combination of specified databases not supported";
    case 24:  return "Element set names not supported";
    case 25:  return "Specified element set name not valid for specified database";
    case 26:  return "Only a single element set name supported";
    case 27:  return "Result set no longer exists - unilaterally deleted by target";
    case 28:  return "Result set is in use";
    case 29:  return "One of the specified databases is locked";
    case 30:  return "Specified result set does not exist";
    case 31:  return "Resources exhausted - no results available";
    case 32:  return "Resources exhausted - unpredictable partial results available";
    case 33:  return "Resources exhausted - valid subset of results available";
    case 100: return "Unspecified error";
    case 101: return "Access-control failure";
    case 102: return "Security challenge required but could not be issued - request terminated";
    case 103: return "Security challenge required but could not be issued - record not included";
    case 104: return "Security challenge failed - record not included";
    case 105: return "Terminated by negative continue response";
    case 106: return "No abstract syntaxes agreed to for this record";
    case 107: return "Query type not supported";
    case 108: return "Malformed query";
    case 109: return "Database unavailable";
    case 110: return "Operator unsupported";
    case 111: return "Too many databases specified";
    case 112: return "Too many result sets created";
    case 113: return "Unsupported attribute type";
    case 114: return "Unsupported Use attribute";
    case 115: return "Unsupported value for Use attribute";
    case 116: return "Use attribute required but not supplied";
    case 117: return "Unsupported Relation attribute";
    case 118: return "Unsupported Structure attribute";
    case 119: return "Unsupported Position attribute";
    case 120: return "Unsupported Truncation attribute";
    case 121: return "Unsupported Attribute Set";
    case 122: return "Unsupported Completeness attribute";
    case 123: return "Unsupported attribute combination";
    case 124: return "Unsupported coded value for term";
    case 125: return "Malformed search term";
    case 126: return "Illegal term value for attribute";
    case 127: return "Unparsable format for un-normalized value";
    case 128: return "Illegal result set name";
    case 129: return "Proximity search of sets not supported";
    case 130: return "Illegal result set in proximity search";
    case 131: return "Unsupported proximity relation";
    case 132: return "Unsupported proximity unit code";
    case 133: return "Unsupported sort code";
    default:
	if (__Private_IB_ErrorMessage) return __Private_IB_ErrorMessage(ErrorCode);
	return "Unknown Error";
  }
}
