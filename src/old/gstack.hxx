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

