/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

/*@@@
File:		opstack.hxx
Description:	Class OPSTACK - Operand/operator Stack
Author:		Nassib Nassar, nrn@cnidr.org
		Edward C. Zimmermann, edz@nonmonotonic.com

@@@*/

#ifndef OPSTACK_HXX
#define OPSTACK_HXX

#include "defs.hxx"
#include "string.hxx"
#include "opobj.hxx"
#include "irset.hxx"

class OPSTACK {
public:
  OPSTACK();
  OPSTACK(const OPSTACK& OtherOpstack);
  OPSTACK& operator=(const OPSTACK& OtherOpstack);
  OPSTACK& operator<<(const OPOBJ& Op);
  OPSTACK& operator<<(const POPOBJ& OpPtr);
  POPOBJ operator>>(POPOBJ& OpPtr);
  PIRSET operator>>(PIRSET& OpPtr);
  void        Reverse();
  void        Clear();
  GDT_BOOLEAN IsEmpty() const;
  ~OPSTACK();
private:
  void Push(const OPOBJ& Op);
  void Push(const POPOBJ& OpPtr);
  POPOBJ Pop();
  POPOBJ Head;
};

// typedef OPSTACK* POPSTACK;

#endif
