/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/************************************************************************
************************************************************************/

#ifndef _GSTACK_HXX_
#define _GSTACK_HXX_

#include "gdt.h"
#include "glist.hxx"

/*
   Define some useful derived stack stuff
*/

class GSTACK : public GLIST {
public:
  GSTACK();
  INT GetSize();
  IATOM* Top();
  void Push(IATOM* a);
  IATOM* Pop();
private:
  GLIST Stack;
  IPOSITION* CurrentIndex;
};


#endif /* _GSTACK_HXX_ */

