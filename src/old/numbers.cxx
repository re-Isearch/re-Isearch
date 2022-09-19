#include <stdlib.h>
#include "common.hxx"
#include "numbers.hxx"

static DOUBLE BAD_NUMBER = 1.17549435e-38F;

NUMERICOBJ::NUMERICOBJ ()
{
  val = BAD_NUMBER;
}

NUMERICOBJ::NUMERICOBJ(const STRING& s)
{
  if (s.IsNumber())
    {
      val = (NUMBER)s;
    }
  else if (s.IsDotNumber())
    {
      UINT8     x = 0;
      unsigned  i = 0;
      char      buf[20];
      for (const char *ptr = s.c_str(); *ptr && i < sizeof(buf)-1 ; ptr++)
	{
	  if ((buf[i] = *ptr) == '.' || *ptr == ':')
	    {
	      int base = *ptr == ':' ? 16 : 10;
	      buf[i] = '\0';
	      if (2*(i/2) != i )
		i++; // Add i for odd number
	      x <<= (base == 10 ? 8 : i*4);
	      x += (int)strtol(buf, (char **)NULL, base);
	      i = 0;
	    }
	  else i++;
	} 
      val = x;
    }
  else
    val = BAD_NUMBER;
}

GDT_BOOLEAN NUMERICOBJ::Ok() const
{
  return val != BAD_NUMBER;
}

void Write(const NUMERICOBJ& s, FILE *Fp)
{
  ::Write(s.val, Fp); 
}

GDT_BOOLEAN NUMERICOBJ::Read(FILE *Fp)
{
  if (::Read(&val, Fp))
    return GDT_TRUE;
  val = BAD_NUMBER;
  return GDT_FALSE;
}

GDT_BOOLEAN Read(NUMERICOBJ *p, FILE *Fp) {
  if (p) return p->Read(Fp);
  return GDT_FALSE;
}



NUMERICOBJ operator+(const NUMERICOBJ& val1, const NUMERICOBJ& val2)
{
  NUMERICOBJ val;

  val = val1;
  val += val2; 
  return val;
}
NUMERICOBJ operator-(const NUMERICOBJ& val1, const NUMERICOBJ& val2)
{
  NUMERICOBJ val;

  val = val1;
  val -= val2;
  return val;
}
NUMERICOBJ operator*(const NUMERICOBJ& val1, const NUMERICOBJ& val2)
{
  NUMERICOBJ val;

  val = val1;
  val *= val2;
  return val;
}
NUMERICOBJ operator/(const NUMERICOBJ& val1, const NUMERICOBJ& val2)
{
  NUMERICOBJ val;

  val = val1;
  val /= val2;
  return val;
}


NUMERICOBJ operator+(const NUMERICOBJ& val1, const DOUBLE val2)
{
  NUMERICOBJ val;

  val = val1;
  val += val2;
  return val;
}
NUMERICOBJ operator-(const NUMERICOBJ& val1, const DOUBLE val2)
{
  NUMERICOBJ val;

  val = val1;
  val -= val2;
  return val;
}
NUMERICOBJ operator*(const NUMERICOBJ& val1, const DOUBLE val2)
{
  NUMERICOBJ val;

  val = val1;
  val *= val2;
  return val;
}
NUMERICOBJ operator/(const NUMERICOBJ& val1, const DOUBLE val2)
{
  NUMERICOBJ val;

  val = val1;
  val /= val2;
  return val;
}


NUMERICALRANGE::NUMERICALRANGE()
{
  d_start = BAD_NUMBER;
  d_end   = BAD_NUMBER;
}

NUMERICALRANGE::NUMERICALRANGE(const STRING& RangeString)
{
  d_start = BAD_NUMBER;
  d_end   = BAD_NUMBER;
  SetRange(RangeString);
}

NUMERICALRANGE::~NUMERICALRANGE()
{
  ;
}

// Formats:
//   (x,y) [x,y] x..y x-y x:y x/y
// where x,y are floating point numbers
//

GDT_BOOLEAN NUMERICALRANGE::SetRange(const STRING& RangeString)
{
  int         fieldcount = 0;
  const char *tcp        = RangeString.c_str();
  DOUBLE      start      = BAD_NUMBER;
  DOUBLE      end        = BAD_NUMBER;

  while (isspace(*tcp)) tcp++;  // Skip blanks

  switch(*tcp) {
    case '\0': /* Empty String */; break;
    case '[': fieldcount = sscanf(tcp,"[%lf,%lf]", &start, &end); break;
    case '(': fieldcount = sscanf(tcp,"(%lf,%lf)", &start, &end); break;
    default:
    if ((fieldcount = sscanf(tcp,"%lf..%lf", &start, &end)) == 1)
      {
	if (start == (double)(int)(start))
	  {
	    long istart, iend;
	    if ((fieldcount = sscanf(tcp,"%ld..%ld", &istart, &iend)) == 2)
	      {
		end = iend;
		break;
	      }
	    else if ((fieldcount = sscanf(tcp,"%ld--%ld", &istart, &iend)) == 2)
	     {
		end = iend;
		break;
	     }
	  }
	if (!isdigit(*tcp) && (*tcp != '.'))
	  break; // Not a number
	while(isdigit(*tcp)) tcp++;
	if (*tcp == '.') tcp++;
	while (isdigit(*tcp)) tcp++;
	// Now do we have another number?
	if (*tcp == '-' && (isdigit(tcp[1]) || (tcp[1] == '.' && isdigit(tcp[3]) )))
	  {
	    // Format:  Num-Num2
	    end = atof(tcp+1);
	    fieldcount = 2;
	    break;
	  }
	while (isspace(*tcp)) tcp++;
	if (*tcp == ','  || *tcp == ':' || *tcp == '/')
	  {
	    tcp++;
	    while (isspace(*tcp)) tcp++;
	    if ((isdigit(*tcp) || (*tcp == '.' && isdigit(tcp[1])))
			|| (*tcp == '-' && isdigit(tcp[1])))
	      {
		// Looking at number
		end = atof(tcp);
		fieldcount = 2;
		break;
	      }
	  }
      }
  }
  d_start = start;
  d_end   = end;
  return fieldcount == 2;
}


GDT_BOOLEAN NUMERICALRANGE::Ok() const
{
  return d_start.val != BAD_NUMBER && d_end.val != BAD_NUMBER;
}

GDT_BOOLEAN NUMERICALRANGE::Defined() const
{
  return d_start.val != BAD_NUMBER || d_end.val != BAD_NUMBER;
}


static const          float CURRENCY_MODULO  = 50000.0;
static const unsigned short BAD_CURRENCY_VAL = (unsigned short)(CURRENCY_MODULO+1) ;

MONETARYOBJ::MONETARYOBJ ()
{
  Amount = 0;
  Fract  = BAD_CURRENCY_VAL;
}

MONETARYOBJ::MONETARYOBJ (const STRING& s)
{
  Amount = 0;
  Fract  = BAD_CURRENCY_VAL;
  Set(s);
}

GDT_BOOLEAN MONETARYOBJ::Set(const STRING& s)
{
  const char *ptr = s.c_str();

  while (isspace(*ptr)) ptr++;

  const size_t len  = strlen(ptr);

  // Largest number: 18446744073709551615
  if (len < 2 || len >= 64) return GDT_FALSE;

  char dup[64];

  memcpy(dup, ptr, len+1);

  char *tcp = dup + len;
  char  decimal = '.';
  const char  money = 164; // Also EURO in 8859-15
  const char  yen   = 165;
  const char  pound = 163;
  const char  cent  = 162;
  int   cents = 0;

  ptr = dup;
  if (*ptr == '$' || *ptr == yen || *ptr == pound || *ptr == money) ptr++;
  while (isspace(*ptr)) ptr++;

  while (--tcp >= ptr)
    {
      if (*tcp == cent)
	{
	  cents++;
	}
      else if (*tcp == '.' || *tcp == ',')
	{
	  decimal = *tcp;
	  *tcp++ = '\0';
	  break;
	}
    }
  if (tcp > ptr)
    {
      long double res = (long double)atof (tcp)*CURRENCY_MODULO;
      // 50000 = 100 cents -> 500 = 1 cent
      // 5000*x = 0.1
      Fract = (UINT2)res;
    }
  else
    Fract = 0;
  if ((Amount = atol (ptr)) == 0)
    {
       if (!_ib_isdigit(*ptr))
	Fract = BAD_CURRENCY_VAL;
    }
  else if (Fract == 0 && cents)
    {
      Set( Amount/100.0 );
    }
  return Ok();
}

MONETARYOBJ::MONETARYOBJ (const NUMBER x)
{
  Set(x);
}

MONETARYOBJ::MONETARYOBJ (const NUMERICOBJ& x)
{
  Set((NUMBER)x);
}


GDT_BOOLEAN MONETARYOBJ::Set (const NUMBER x)
{
  const long crowns = (long)x;

  Amount = (UINT4)crowns;
  Fract  = (UINT2)((x - crowns)*CURRENCY_MODULO);
  if (Amount == crowns) // It fits
    return GDT_TRUE;
  Fract += BAD_CURRENCY_VAL;
  return GDT_FALSE; // Does not fit
}

void MONETARYOBJ::Write(FILE *Fp) const
{
  ::Write(Amount, Fp);
  ::Write(Fract, Fp);
}

void Write(const MONETARYOBJ& s, FILE *Fp)
{
  s.Write(Fp);
}

GDT_BOOLEAN MONETARYOBJ::Read(FILE *Fp)
{
  Fract = BAD_CURRENCY_VAL;
  ::Read(&Amount, Fp);
  ::Read(&Fract, Fp);
  return Ok();
}
  

inline GDT_BOOLEAN Read(MONETARYOBJ *p, FILE *Fp)
{
  if (p) return p->Read(Fp);
  return GDT_FALSE;
}


#if 0 

#define BAD_BOOLEAN 0xFF

BOOLEANOBJ::BOLLEANOBJ ()
{
  val = BAD_BOOLEAN; 
}

BOOLEANOBJ::NUMERICOBJ(const STRING& s)
{
  if (s.GetLength())
    val = s.IsNumber() ? s.GetLong() != 0 : s.GetBool();
  else
    val = BAD_BOOLEAN;
}

GDT_BOOLEAN BOOLEANOBJ::Ok() const
{
  return val != BAD_BOOLEAN;
}

void Write(const BOOLEANOBJ& s, FILE *Fp)
{
  Write(s.val, Fp);
}

inline GDT_BOOLEAN Read(BOOLEANOBJ *p, FILE *Fp)
{
  BYTE x;
  if (Read(&x, Fp) == GDT_FALSE)
    x = BAD_BOOLEAN;
  if (p) p->val = x;
  return x != BAD_BOOLEAN;
}


#endif

