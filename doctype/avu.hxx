/*

File:        avu.hxx
Version:     1
Description: class AVU - MARC records for library use
Author:      Erik Scott, Scott Technologies, Inc.
*/


#ifndef AVU_HXX
#define AVU_HXX

#ifndef DOCTYPE_HXX
#include "defs.hxx"
#include "doctype.hxx"
#endif

#include "usmarc.hxx"

class AVU
  : public USMARC
{
public:
  AVU(PIDBOBJ DbParent);
  void   ParseFields(PRECORD NewRecord);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
	       STRING* StringBuffer);
  void   Present(const RESULT& ResultRecord, const STRING& ElementSet,
	       const STRING& RecordSyntax, STRING* StringBuffer);
  ~AVU();
};

typedef AVU* PAVU;

#endif
