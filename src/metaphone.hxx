/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/

#ifndef _METAPHONE_H
#define _METAPHONE_H 1

#include "common.hxx"
#include "string.hxx"

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
class MString : public STRING
{
private:
  void           Init();
  int            length, last;
  GDT_BOOLEAN    alternate;
  STRING         primary, secondary;
public:
  MString();
  MString(const char*);
  MString(const STRING&);
  GDT_BOOLEAN SlavoGermanic();
  GDT_BOOLEAN IsVowel(int at);
  inline void MetaphAdd(const char* main);
  inline void MetaphAdd(const char* main, const char* alt);
  GDT_BOOLEAN StringAt(int start, int length, ... );
  MString&    MakeUpperAscii();
  UINT8       DoubleMetaphone(STRING &metaph, STRING &metaph2);

};

inline UINT8  DoubleMetaphone(const STRING& Input) {
  STRING s1, s2;
  return MString(Input).DoubleMetaphone(s1, s2);
}

GDT_BOOLEAN MetaphoneMatch(const UINT8 Hash1, const UINT8 Hash2);

inline GDT_BOOLEAN MetaphoneMatch(const STRING& Term1, const STRING& Term2) {
  return MetaphoneMatch( DoubleMetaphone(Term1), DoubleMetaphone(Term2) );
}
inline GDT_BOOLEAN MetaphoneMatch(const UINT8 Hash1, const STRING& Term2) {
  return MetaphoneMatch( Hash1, DoubleMetaphone(Term2) );
}
inline GDT_BOOLEAN MetaphoneMatch(const STRING& Term1, const UINT8 Hash2) {
  return MetaphoneMatch( DoubleMetaphone(Term1), Hash2 );
}


#endif /* _METAPHONE_H */

