/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		df.hxx
Description:	Class DF - Data Field
@@@*/

#ifndef DF_HXX
#define DF_HXX

#include "fct.hxx"

class DF {
public:
  DF();
  DF(const DF& OtherDf);

  DF& operator=(const DF& OtherDf);

  void        SetFieldName(const STRING& NewFieldName) {
   (FieldName = NewFieldName).ToUpper();
  }
  void        GetFieldName(PSTRING StringBuffer) const;
  STRING      GetFieldName() const { return FieldName; }

  void        SetFct(const FCT& newFct)    { Fct = newFct; }
  void        SetFct(const FCLIST& FcList) { Fct = FcList; }
  void        SetFct(const FC& Fc)         { Fct = Fc;     }

  void        AddFct(const FC& Fc)         { Fct.AddEntry(Fc);     }
  void        AddFct(const FCLIST& Fclist) { Fct.AddEntry(Fclist); }
  void        AddFct(const FCLIST *Ptr)    { Fct.AddEntry(Ptr);    }
  void        AddFct(const FCT& fct)       { Fct.AddEntry(fct);    }

  bool Ok() const { return ! (FieldName.IsEmpty() || Fct.IsEmpty() ) ; }

  operator    STRING() const          { return FieldName; }
  operator    FCT() const             { return Fct;       }

  FCT         GetFct() const          { return Fct; }
  const FCLIST* GetFcListPtr () const { return Fct; }

  void        Write(PFILE fp) const;
  bool Read(PFILE fp);

 ~DF();
private:
  STRING      FieldName;
  FCT         Fct;
};

// Common Functions
inline void Write(const DF& Df, PFILE Fp)
{
  Df.Write(Fp);
}

inline bool Read(DF *DfPtr, PFILE Fp)
{
  return DfPtr->Read(Fp);
}

#endif
