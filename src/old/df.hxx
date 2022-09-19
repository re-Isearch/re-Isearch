/*@@@
File:		df.hxx
Version:	1.00
Description:	Class DF - Data Field
Author:		Nassib Nassar, nrn@cnidr.org
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

  GDT_BOOLEAN Ok() const { return ! (FieldName.IsEmpty() || Fct.IsEmpty() ) ; }

  operator    STRING() const          { return FieldName; }
  operator    FCT() const             { return Fct;       }

  FCT         GetFct() const          { return Fct; }
  const FCLIST* GetFcListPtr () const { return Fct; }

  void        Write(PFILE fp) const;
  GDT_BOOLEAN Read(PFILE fp);

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

inline GDT_BOOLEAN Read(DF *DfPtr, PFILE Fp)
{
  return DfPtr->Read(Fp);
}

#endif
