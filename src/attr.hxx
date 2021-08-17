/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		attr.hxx
Description:	Class ATTR - Attribute
@@@*/

#ifndef ATTR_HXX
#define ATTR_HXX

#include "defs.hxx"
#include "string.hxx"

class ATTR {
public:
  ATTR();
  ATTR(const ATTR& OtherAttr);

  ATTR& operator=(const ATTR& OtherAttr);

  void SetSetId(const STRING& NewSetId)     { SetId = NewSetId;      }
  void GetSetId(PSTRING StringBuffer) const { *StringBuffer = SetId; }

  void SetAttrType(const INT NewAttrType) { AttrType = NewAttrType; }
  INT GetAttrType() const                 { return AttrType;        }

  void SetAttrValue(const STRING& NewAttrValue) { AttrValue = NewAttrValue;  }
  void SetAttrValue(const INT NewAttrValue)     { AttrValue = NewAttrValue;  }
  STRING GetAttrValue(STRING *StringBuffer) const {
    if (StringBuffer) return *StringBuffer = AttrValue;
    return AttrValue;
  }
  INT GetAttrValue() const                      { return AttrValue.GetInt(); }

  void Write(PFILE Fp) const;
  GDT_BOOLEAN Read(PFILE Fp);
  ~ATTR();
private:
  STRING SetId;
  INT4   AttrType;
  STRING AttrValue;
};

typedef ATTR* PATTR;

// Common functions
inline void Write(const ATTR& Attr, FILE *Fp)
{
  Attr.Write(Fp);
}
inline GDT_BOOLEAN Read(PATTR AttrPtr, FILE *Fp)
{
  return AttrPtr->Read(Fp);
}

#endif
