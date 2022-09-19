/*
ISOTEIA Project Metadata
*/

#ifndef GILS_ISOTEIA_HXX
# define GILS_ISOTEIA_HXX 1

#include <stdlib.h>
#include <ctype.h>
#include "platform.h"
#include "common.hxx"
#include "gilsxml.hxx"
#include "doc_conf.hxx"
#include "gpolyfield.hxx"


class GILS_ISOTEIA : public GILSXML {
public:
  GILS_ISOTEIA (PIDBOBJ DbParent, const STRING& Name);
  virtual void LoadFieldTable();

  virtual void ParseFields (PRECORD NewRecord);

  virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet, PSTRING StringBuffer) const;

  ~GILS_ISOTEIA();

  virtual const char *Description(PSTRLIST List) const;

  virtual DATERANGE  ParseDateRange(const STRING& Buffer) const;
  virtual int        ParseGPoly(const STRING&, GPOLYFLD*) const;
private:
  STRING     help;
};


#endif
