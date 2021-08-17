/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/

#ifndef INFIX2RPN_HXX
#define INFIX2RPN_HXX

#include "tokengen.hxx"
#include "strstack.hxx"

class INFIX2RPN {
  public:
    INFIX2RPN ();
    INFIX2RPN (const STRING& StrInput, STRING *StrOutput);

    GDT_BOOLEAN    Parse (const STRING &StrInput, STRING *StrOutput);
    STRING         Parse(const STRING& Input) {
      STRING Temp;
      return Parse(Input, &Temp) ? Temp : NulString;
    }

    GDT_BOOLEAN    InputParsedOK () const;
    GDT_BOOLEAN    GetErrorMessage (STRING *Error) const;
    STRING         GetErrorMessage() const { return ErrorMessage; }

    void           SetDefaultOp(const STRING& Op) { DefaultOp = string2op (Op); }
    STRING         GetDefaultOp()                 { return op2string(DefaultOp); }

  private:
    void           ProcessOp (int op, STRSTACK& TheStack, STRING *Result);
    void           RegisterError (const STRING& Error);
    const STRING   op2string (int op) const;
    int            string2op (const STRING op) const;
    const STRING   StandardizeOpName (const STRING op) const;

    INT            TermsWithNoOps;
    STRING         ErrorMessage;
    int            DefaultOp;
};


#endif
