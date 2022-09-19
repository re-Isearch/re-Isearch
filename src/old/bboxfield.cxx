#include <math.h>
#undef logf
#include "bboxfield.hxx"
#include "magic.hxx"


BBOXFLD::BBOXFLD()
{
  GlobalStart = 0;
  for (size_t i=0; i < 4; i++)
    Vertices[i] = 0;
  Extent = 0;
}

BBOXFLD::BBOXFLD(const char *String)
{
  Set(String);
}

int BBOXFLD::Set(const char * String)
{
  DOUBLE      No   =0;
  DOUBLE      West =0;
  DOUBLE      So   =0;
  DOUBLE      East =0;
  const char *tp = String;
  char        tmp[256];
  char        endCh = 0;

  while (isspace(*tp)) tp++;

  // Note: May contain RECT{  (Observation of Nov 2005)
  // so lets skip alpha
  while (isalpha(*tp)) tp++;


  if      (*tp == '[') endCh = ']';
  else if (*tp == '(') endCh = ')';
  else if (*tp == '<') endCh = '>';
  else if (*tp == '{') endCh = '}';

  if (endCh)
    {
      tp++; // Now skip the character
      while (isspace(*tp)) tp++;
    }

  for (size_t i=0; i<sizeof(tmp)/sizeof(char) && *tp; tp++)
    {
      if (*tp == ',' || *tp == ';' || iscntrl(*tp))
        {
          tmp[i++] = ' ';
        }
      else if (*tp == endCh)
        {
          tmp[i] = '\0';
          break;
        }  
      else
        tmp[i++] = *tp;
    }

  // Now expect canonical term order
  switch (sscanf(tmp,"%lf %lf %lf %lf",&No,&West, &So, &East))
    {
      case 4:
	setNorth( No );
	setSouth( So );
	setWest (West);
	setEast (East);
//cerr << "North = " << No << " West=" << West << " South=" << So << " East=" << East <<endl;
	setExtent();
	return 4;
	break;
    }
  return 0;
}


BBOXFLD::BBOXFLD(const BBOXFLD& OtherField)
{
  *this = OtherField;
}

BBOXFLD& BBOXFLD::operator=(const BBOXFLD& OtherField)
{
  GlobalStart = OtherField.GlobalStart;
  for (size_t i=0; i<4; i++)
    Vertices[i] = OtherField.Vertices[i];
  Extent = OtherField.Extent;
  return *this;
}

BBOXFLD::operator    STRING () const
{
  STRING result ("{");

  for (size_t i=0; i<4; i++)
    {
      if (i) result << ",";
      result << _getVal(Vertices[i]);
    }
  result << "}";
  return result;
}

void       BBOXFLD::Dump(ostream& os) const
{
  os << " ptr: " << GlobalStart << endl << (STRING)*this << endl; 
}


void       BBOXFLD::Write(FILE *fp) const
{
  ::Write(GlobalStart, fp);
  for (size_t i=0; i< 4; i++)
    ::Write(Vertices[i], fp);
  ::Write(Extent, fp);
}

void       BBOXFLD::Write(FILE *fp, GPTYPE Offset) const
{
  ::Write(GlobalStart+Offset, fp);
  for (size_t i=0; i< 4; i++)
    ::Write(Vertices[i], fp);
  ::Write(Extent, fp);
}



int        BBOXFLD::Read(FILE *fp)
{
  ::Read(&GlobalStart, fp);
  if (GlobalStart == (GPTYPE)-1 && feof(fp))
    return -1;
  for (size_t i=0; i<4; i++)
    ::Read(&Vertices[i], fp);
  ::Read(&Extent, fp);
  return 0;
}

DOUBLE  BBOXFLD::getExtent() const
{
  return _getVal(Extent);
}

DOUBLE BBOXFLD::setExtent(DOUBLE Val)
{
  DOUBLE extent;
  if (Val <= 0.0)
    extent = fabs((getEast() - getWest()) * (getNorth() - getSouth()));
  else
    extent = fabs(Val);
  Extent = (UINT4) _setVal(extent);
  return extent;
}


void Write(const BBOXFLD& s, FILE *Fp) { s.Write(Fp); }
void Read(BBOXFLD *s, FILE *Fp)        { s->Read(Fp); }

