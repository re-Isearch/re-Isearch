//
// HTMLEntities.cc
//
// Implementation of HTMLEntities
//
//
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>

#include "common.hxx"
#include "HTMLEntities.hxx"

#define EXPERIMENTAL 1


//*****************************************************************************
HTMLEntities::HTMLEntities()
{
  trans = new Dictionary(131, 10.0f);
  init();
}


//*****************************************************************************
HTMLEntities::~HTMLEntities()
{
  trans->Release();
  delete trans;
}

//*****************************************************************************

static const char *Entities[255];

STRING HTMLEntities::entity(unsigned short Ch) const
{
  if (Ch < 256)
    {
      const char *tp =  Entities[Ch];
      if (tp)
	return STRING("&") + tp + ';';
      return STRING((char)Ch);
    }
  return STRING().form("&#%04d;", Ch);
}


//*****************************************************************************
#if 0
inline const int _convert(const int Ch)
{
  switch (Ch) {
    case 8217: return '\xb4';
    case 8364: return '\x80';  //Euro
    case 8194: case 8195: case 8201: return ' ';
    case 8211: case 8212: return '-';
    case 8216: case 8217: return '\'');
                                        else if(_nChar == 8217) _sChar = _T("'");^M
                                        else if(_nChar == 8218) _sChar = _T("'");^M
                                        else if(_nChar == 8220) _sChar = _T("\"");^M
                                        else if(_nChar == 8221) _sChar = _T("\"");^M
                                        else if(_nChar == 8222) _sChar = _T("\"");^M
                                        else if(_nChar == 8230) _sChar = _T("...");^M
                                        else if(_nChar == 8240) _sChar = _T("o/oo");^M
                                        else if(_nChar == 8249) _sChar = _T("<");^M
                                        else if(_nChar == 8250) _sChar = _T(">");^M
                                        else if(_nChar == 8482) _sChar = _T("(TM)");^M
                                        else if(_nChar == 8722) _sChar = _T("-");^M
                                        else if(_nChar == 8594) _sChar = _T("->");^M
                                        else                                    _nChar = 0;^M
}
#endif


int HTMLEntities::translate(const char *entity) const
{
#if EXPERIMENTAL
  long val =  -1;
#else
  long val = ' ';           // Unrecognized entity.  Change it into a space...
#endif
  if (entity && *entity)
    {
      Object *object;
      if (*entity == '#')
	{
	  if (isdigit(entity[1]))
	    return atoi(entity + 1); // This looks like a numeric entity.  That's fine.
	  if ((entity[1] == 'x' || entity[1] == 'X') && isxdigit (entity[2])) // hex?
	    val = (int)strtol(entity+2, (char **)NULL, 16);
          else if (entity[1] == 'o' && isdigit (entity[2])) // oct?
            val = (int)strtol(entity+2, (char **)NULL, 8);
	  else
	    val = (int)strtol(entity+1, (char **)NULL, 0); // What is it?
	  if (val == 8364) val = 0x80; // EURO symbol
	}
      else if ((object = trans->Find(entity)) != NULL)
	val = (long)object;
    }
  return (int)val;
}

//*****************************************************************************

void HTMLEntities::init()
{
  const struct {
    const char    *entity;
    unsigned char  equiv;
  } entities[] = {
  { "quot",        34},
  { "num",         35},
  { "dollar",      36},
  { "percnt",      37},
  { "amp",         38},
  { "apos",        39}, // <--- NOT in HTML (XML 1.0)
  { "lpar",        40},
  { "rpar",        41},
  { "ast",         42},
  { "plus",        43},
  { "comma",       44},
  { "hyphen",      45},
  { "mdash",       45},
  { "period",      46},
  { "sol",         47},
  { "colon",       58},
  { "semi",        59},
  { "lt",          60}, 
  { "equals",      61},
  { "gt",          62},
  { "quest",       63},
  { "commat",      64},
  { "lsqb",        91},
  { "bsol",        92},
  { "sbsol",       92},
  { "rsqb",        93},
  { "lowbar",      95},
  { "lcub",        123},
  { "verbar",      124},
  { "rcub",        125},
  { "euro",        128},
  { "trade",       153},
  { "nbsp",        ' ' /*160*/},
  { "iexcl",       161},
  { "cent",        162},
  { "pound",       163},
  { "curren",      164},
  { "yen",         165},
  { "brvbar",      166},
  { "sect",        167},
  { "die",         168},
  { "Dot",         168},
  { "uml",         168},
  { "copy",        169},
  { "ordf",        170},
  { "laquo",       171},
  { "not",         172},
  { "shy",         173},
  { "reg",         174},
  { "hibar",       175},
  { "macr",        175},
  { "deg",         176},
  { "plusmn",      177},
  { "sup2",        178},
  { "sup3",        179},
  { "acute",       180},
  { "micro",       181},
  { "para",        182},
  { "middot",      183},
  { "cedil",       184},
  { "sup1",        185},
  { "ordm",        186},
  { "raquo",       187},
  { "frac14",      188},
  { "frac12",      189},
  { "frac34",      190},
  { "iquest",      191},
  { "Agrave",      192},
  { "Aacute",      193},
  { "Acirc",       194},
  { "Atilde",      195},
  { "Auml",        196},
  { "Aring",       197},
  { "AElig",       198},
  { "Ccedil",      199},
  { "Egrave",      200},
  { "Eacute",      201},
  { "Ecirc",       202},
  { "Euml",        203},
  { "Iacute",      204},
  { "Igrave",      205},
  { "Icirc",       206},
  { "Iuml",        207},
  { "ETH",         208},
  { "Ntilde",      209},
  { "Ograve",      210},
  { "Oacute",      211},
  { "Ocirc",       212},
  { "Otilde",      213},
  { "Ouml",        214},
  { "times",       215},
  { "Oslash",      216},
  { "Ugrave",      217},
  { "Uacute",      218},
  { "Ucirc",       219},
  { "Uuml",        220},
  { "Yacute",      221},
  { "THORN",       222},
  { "szlig",       223},
  { "agrave",      224},
  { "aacute",      225},
  { "acirc",       226},
  { "atilde",      227},
  { "auml",        228},
  { "aring",       229},
  { "aelig",       230},
  { "ccedil",      231},
  { "egrave",      232},
  { "eacute",      233},
  { "ecirc",       234},
  { "euml",        235},
  { "igrave",      236},
  { "iacute",      237},
  { "icirc",       238},
  { "iuml",        239},
  { "eth",         240},
  { "ntilde",      241},
  { "ograve",      242},
  { "oacute",      243},
  { "ocirc",       244},
  { "otilde",      245},
  { "ouml",        246},
  { "divide",      247},
  { "oslash",      248},
  { "ugrave",      249},
  { "uacute",      250},
  { "ucirc",       251},
  { "uuml",        252},
  { "yacute",      253},
  { "thorn",       254},
  { "yuml",        255}
  };
#define SIZEOF(X) (sizeof(X)/sizeof(X[0]))
  for (size_t i = 0; i < SIZEOF(entities); i++)
    {
      trans->Add(entities[i].entity, ((Object *) ((long) entities[i].equiv)));
      Entities[i] = entities[i].entity;
    }
}

void HTMLEntities::add(const char *entity, long value)
{
  trans->Add(entity, (Object *)value);
}

//*****************************************************************************
// This method does the same as the translate method, but it will also advance
// the character pointer to the next character after the entity.
int HTMLEntities::translate(size_t *offset, char **entityStart) const
{
   char            entity[12];
   size_t          i = 0;
   char           *orig = *entityStart;
   char           *pos =  orig;


   if (*pos== '&')
      pos++;		// Don't need the '&' that starts the entity

   while ( (isalnum(*pos) || *pos == '.' || *pos == '-' || *pos == '#')
	&& i < 10) {
      entity[i++] =  *pos++;
   }
   entity[i] = '\0';
   if (i >= 10 || i == 0) {
      //
      // This must be a bogus entity.  It can't be more than 10 characters
      // long.  Well, just assume it was an error and return just the '&'.
      //
      *entityStart = orig + 1;
      return '&'; // No need to increment white space
   }
/*
   if (*pos == ';')
      pos++;		// A final ';' is used up.
*/

//cerr << "Translate (" << entity << ")";
 int result = translate (entity); // Translate

//cerr << "--> " << (char)result << " val=" << result << endl;

#if EXPERIMENTAL
  if (result == -1)
    {
      pos = orig;
      result = *orig;
    }
#endif

  // Set the positions
  *offset += pos - orig; // Need extra white space after term
  *entityStart = pos + 1;

  return result; 
}


void HTMLEntities::normalize(char *input, size_t len) const
{
  size_t          pos   = 0;
  size_t          count = 0;
  REGISTER char  *buffer= input;
  REGISTER char  *ptr   = buffer;

  do {
    if (*ptr == '\0')
      {
	*buffer++ = ' ';
	ptr++;
	break; // Don't bother doing more!
      }
    else
      {
	unsigned char ch;
	ch = (unsigned char)(*buffer = ((*ptr == '&') ? translate(&pos, &ptr) : *ptr++));
	if (pos && !IsTermChar(ch) && !IsDotInWord(ch))
	  {
	    memset(buffer, ' ', pos);
	    buffer[pos] = (char)ch;
	    buffer += pos + 1;
	    count  += pos;
	    pos = 0;
          }
	else
	  buffer++;
      }
  } while (++count < len);
  if (ptr > buffer)
    memset(buffer, ' ', ptr - buffer - 1); // Added -1
}


void HTMLEntities::normalize2(char *input, size_t len) const
{
  size_t          pos   = 0;
  size_t          count = 0;
  REGISTER char  *buffer= input;
  REGISTER char  *ptr   = buffer;

  do {
    if (*ptr == '\0')
      {
	*buffer++ = ' ';
	ptr++;
	break; // Don't bother doing more!
      }
    else
      {
	*buffer = ((*ptr == '&') ? translate(&pos, &ptr) : *ptr++);
	if (pos && (isspace((unsigned char)*buffer) || *buffer == '\0'))
	  {
	    char ch = *buffer;
	    memset(buffer, ' ', pos);
	    buffer[pos] = ch;
	    buffer += pos + 1;
	    count  += pos;
	    pos = 0;
          }
	else
	  buffer++;
      }
  } while (++count < len);
  if (ptr > buffer)
    memset(buffer, ' ', ptr - buffer - 1); // Added -1
}



void HTMLEntities::normalize(char *buffer) const
{
  size_t pos = 0;
  char *ptr = buffer;
  while (*ptr)
    {
      if (*ptr == '&')
	*buffer = translate(&pos, &ptr);
      else
	*buffer = *ptr++;
      if (pos && isspace((unsigned char)*buffer))
	{
	  memset(buffer, ' ', pos+1);
	  buffer += pos + 1;
	  pos = 0;
	}
      else buffer++;
    }
  if (ptr > buffer)
    memset(buffer, ' ', ptr - buffer - 1); // Added -1
}

#if DEBUG

main(int argc, char **argv)
{
  char tmp[1024];
  HTMLEntities Test;

  strcpy(tmp, argv[1] );
  printf("Input = '%s'\n", argv[1]);
  Test.normalize(tmp) ;
  printf("Output= '%s'\n", tmp ); 
}

#endif
