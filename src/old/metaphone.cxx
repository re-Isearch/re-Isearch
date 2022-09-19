////////////////////////////////////////////////////////////////////////////////
// Double Metaphone (c) 1998, 1999 by Lawrence Philips
//
//
////////////////////////////////////////////////////////////////////////////////
#include "metaphone.hxx"

#define AND &&
#define OR ||


// added by edz@bsn.com
static int _code(const STRING& buf)
{
  int           num = 0;
  unsigned char ch;
  for (size_t i = 0; i < buf.GetLength(); i++)
    {
      ch = (unsigned char)(buf.GetAt(i));
      num = num << 8;
      num += ch;
    }
  num &= 0xFFFFFFFF;
  return num;
}

// Collapse Latin1
static const unsigned char ISOLAT1[] = {
     0,   1,   2,   3,   4,   5,   6,   7,
     8,   9,  10,  11,  12,  13,  14,  15,
    16,  17,  18,  19,  20,  21,  22,  23,
    24,  25,  26,  27,  28,  29,  30,  31,
    32,  33,  34,  35,  36,  37,  38,  39,
    40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,
    56,  57,  58,  59,  60,  61,  62,  63,
    64, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
   'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
   'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
   'X', 'Y', 'Z',  91,  92,  93,  94,  95,
    96, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
   'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
   'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
   'X', 'Y', 'Z', 123, 124, 125, 126, 127,
   128, 129, 130, 131, 132, 133, 134, 135,
   136, 137, 138, 139, 140, 141, 142, 143,
   144, 145, 146, 147, 148, 149, 150, 151,
   152, 153, 154, 155, 156, 157, 158, 159,
   160, 161, 162, 163, 164, 165, 166, 167,
   168, 168, 170, 171, 172, 173, 174, 175,
   176, 177, 178, 179, 180, 181, 182, 183,
   184, 185, 186, 187, 188, 189, 190, 191,
   'A', 'A', 'A', 'A', 'A', 'A', 'A', 'C',
   'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
   208, 'N', 'O', 'O', 'O', 'O', 'O', 'O',
   'O', 'U', 'U', 'U', 'U', 'Y', 222, 223,
   'A', 'A', 'A', 'A', 'A', 'A', 'A', 'C',
   'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
   240, 'N', 'O', 'O', 'O', 'O', 'O', 'O',
   'O', 'U', 'U', 'U', 'U', 'Y', 254, 255
};

MString& MString::MakeUpperAscii()
{
  const size_t len = Len();
  unsigned char *p = (unsigned char *)m_pchData;

  AllocBeforeWrite(len);
  for (size_t i=0; i < len; i++)   
    { 
      UCHR ch = p[i];
      m_pchData[i] = (ib_islatin1(CHARSET(), ch) ? ISOLAT1[ch] : _ib_toupper(ch));
    }
  return *this;
}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
MString::MString()
{
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
MString::MString(const char* in) : STRING(in)
{
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
MString::MString(const STRING& in) : STRING(in)
{
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
GDT_BOOLEAN MString::SlavoGermanic()
{
  if ((Find('W') > -1) OR (Find('K') > -1) OR (Find("CZ") > -1) OR (Find("WITZ") > -1))
    return GDT_TRUE;

  return GDT_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
inline void MString::MetaphAdd(const char* main)
{
  if (*main)
    {
      primary += main;
      secondary += main;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
inline void MString::MetaphAdd(const char* main, const char* alt)
{
  if (*main)
    primary += main;
  if (*alt)
    {
      alternate = GDT_TRUE;
      if (alt[0] != ' ')
        secondary += alt;
    }
  else
    if (*main AND (main[0] != ' '))
      secondary += main;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
GDT_BOOLEAN MString::IsVowel(int at)
{

  if ((at < 0) OR (at >= length))
    return GDT_FALSE;

  char it = GetAt(at);

  if ((it == 'A') OR (it == 'E') OR (it == 'I') OR (it == 'O') OR (it == 'U') OR (it == 'Y') )
    return GDT_TRUE;

  return GDT_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
GDT_BOOLEAN MString::StringAt(int start, int length, ... )
{
//  char    buffer[64];
  char     *test = NULL; // = buffer;
  STRING    target = Mid(start, length);

  va_list sstrings;
  va_start(sstrings, length);

  do
    {
      if ((test = va_arg(sstrings, char*)) != NULL && *test && (target == test))
        return GDT_TRUE;
    }
  while (test && *test != '\0'); 

  va_end(sstrings);

  return GDT_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// main deal
////////////////////////////////////////////////////////////////////////////////
UINT8 MString::DoubleMetaphone(STRING &metaph, STRING &metaph2)
{
  UINT4 num_low  = 0;
  UINT4 num_high = 0;

  for (size_t i=1; i< Length(); i++)
    {
      UCHR Ch = GetChr(i);
      if (isdigit(i))
        {
	  if (!metaph.IsEmpty()) metaph.Clear();
	  if (!metaph2.IsEmpty()) metaph2.Clear();
	  return 0;
        }
      if (!isspace(Ch)) break;
    }

  int current = 0;

  length = GetLength();
  if (length < 1)
    return 0;
  last = length - 1;//zero based index

  alternate = GDT_FALSE;

  MakeUpperAscii();

  //pad the original string so that we can index beyond the edge of the world
  Insert(GetLength(), "     ");

  //skip these when at start of word
  if (StringAt(0, 2, "GN", "KN", "PN", "WR", "PS", NULL))
    current += 1;

  //Initial 'X' is pronounced 'Z' e.g. 'Xavier'
  if (GetAt(0) == 'X')
    {
      MetaphAdd("S"); //'Z' maps to 'S'
      current += 1;
    }

  ///////////main loop//////////////////////////
  while ((primary.GetLength() < 4) OR (secondary.GetLength() < 4))
    {
      if (current >= length)
        break;

      switch ( GetAt(current))
        {
        case 'A':
        case 'E':
        case 'I':
        case 'O':
        case 'U':
        case 'Y':
          if (current == 0)
            //all init vowels now map to 'A'
            MetaphAdd("A");
          current +=1;
          break;

        case 'B':

          //"-mb", e.g", "dumb", already skipped over...
          MetaphAdd("P");

          if (GetAt(current + 1) == 'B')
            current +=2;
          else
            current +=1;
          break;

        case 'Ç':
          MetaphAdd("S");
          current += 1;
          break;

        case 'C':
          //various germanic
          if ((current > 1)
              AND !IsVowel(current - 2)
              AND StringAt((current - 1), 3, "ACH", NULL)
              AND ((GetAt(current + 2) != 'I') AND ((GetAt(current + 2) != 'E')
                                                    OR StringAt((current - 2), 6, "BACHER", "MACHER", NULL)) ))
            {
              MetaphAdd("K");
              current +=2;
              break;
            }

          //special case 'caesar'
          if ((current == 0) AND StringAt(current, 6, "CAESAR", NULL))
            {
              MetaphAdd("S");
              current +=2;
              break;
            }

          //italian 'chianti'
          if (StringAt(current, 4, "CHIA", NULL))
            {
              MetaphAdd("K");
              current +=2;
              break;
            }

          if (StringAt(current, 2, "CH", NULL))
            {
              //find 'michael'
              if ((current > 0) AND StringAt(current, 4, "CHAE", NULL))
                {
                  MetaphAdd("K", "X");
                  current +=2;
                  break;
                }

              //greek roots e.g. 'chemistry', 'chorus'
              if ((current == 0)
                  AND (StringAt((current + 1), 5, "HARAC", "HARIS", NULL)
                       OR StringAt((current + 1), 3, "HOR", "HYM", "HIA", "HEM", NULL))
                  AND !StringAt(0, 5, "CHORE", NULL))
                {
                  MetaphAdd("K");
                  current +=2;
                  break;
                }

              //germanic, greek, or otherwise 'ch' for 'kh' sound
              if ((StringAt(0, 4, "VAN ", "VON ", NULL) OR StringAt(0, 3, "SCH", NULL))
                  // 'architect but not 'arch', 'orchestra', 'orchid'
                  OR StringAt((current - 2), 6, "ORCHES", "ARCHIT", "ORCHID", NULL)
                  OR StringAt((current + 2), 1, "T", "S", NULL)
                  OR ((StringAt((current - 1), 1, "A", "O", "U", "E", NULL) OR (current == 0))
                      //e.g., 'wachtler', 'wechsler', but not 'tichner'
                      AND StringAt((current + 2), 1, "L", "R", "N", "M", "B", "H", "F", "V", "W", " ", NULL)))
                {
                  MetaphAdd("K");
                }
              else
                {
                  if (current > 0)
                    {
                      if (StringAt(0, 2, "MC", NULL))
                        //e.g., "McHugh"
                        MetaphAdd("K");
                      else
                        MetaphAdd("X", "K");
                    }
                  else
                    MetaphAdd("X");
                }
              current +=2;
              break;
            }
          //e.g, 'czerny'
          if (StringAt(current, 2, "CZ", NULL) AND !StringAt((current - 2), 4, "WICZ", NULL))
            {
              MetaphAdd("S", "X");
              current += 2;
              break;
            }

          //e.g., 'focaccia'
          if (StringAt((current + 1), 3, "CIA", NULL))
            {
              MetaphAdd("X");
              current += 3;
              break;
            }

          //double 'C', but not if e.g. 'McClellan'
          if (StringAt(current, 2, "CC", NULL) AND !((current == 1) AND (GetAt(0) == 'M')))
            //'bellocchio' but not 'bacchus'
            if (StringAt((current + 2), 1, "I", "E", "H") AND !StringAt((current + 2), 2, "HU", NULL))
              {
                //'accident', 'accede' 'succeed'
                if (((current == 1) AND (GetAt(current - 1) == 'A'))
                    OR StringAt((current - 1), 5, "UCCEE", "UCCES", NULL))
                  MetaphAdd("KS");
                //'bacci', 'bertucci', other italian
                else
                  MetaphAdd("X");
                current += 3;
                break;
              }
            else //Pierce's rule
              {
                MetaphAdd("K");
                current += 2;
                break;
              }

          if (StringAt(current, 2, "CK", "CG", "CQ", NULL))
            {
              MetaphAdd("K");
              current += 2;
              break;
            }

          if (StringAt(current, 2, "CI", "CE", "CY", NULL))
            {
              //italian vs. english
              if (StringAt(current, 3, "CIO", "CIE", "CIA", NULL))
                MetaphAdd("S", "X");
              else
                MetaphAdd("S");
              current += 2;
              break;
            }

          //else
          MetaphAdd("K");

          //name sent in 'mac caffrey', 'mac gregor
          if (StringAt((current + 1), 2, " C", " Q", " G", NULL))
            current += 3;
          else
            if (StringAt((current + 1), 1, "C", "K", "Q", NULL)
                AND !StringAt((current + 1), 2, "CE", "CI", NULL))
              current += 2;
            else
              current += 1;
          break;

        case 'D':
          if (StringAt(current, 2, "DG", NULL))
            if (StringAt((current + 2), 1, "I", "E", "Y", NULL))
              {
                //e.g. 'edge'
                MetaphAdd("J");
                current += 3;
                break;
              }
            else
              {
                //e.g. 'edgar'
                MetaphAdd("TK");
                current += 2;
                break;
              }

          if (StringAt(current, 2, "DT", "DD", NULL))
            {
              MetaphAdd("T");
              current += 2;
              break;
            }

          //else
          MetaphAdd("T");
          current += 1;
          break;

        case 'F':
          if (GetAt(current + 1) == 'F')
            current += 2;
          else
            current += 1;
          MetaphAdd("F");
          break;

        case 'G':
          if (GetAt(current + 1) == 'H')
            {
              if ((current > 0) AND !IsVowel(current - 1))
                {
                  MetaphAdd("K");
                  current += 2;
                  break;
                }

              if (current < 3)
                {
                  //'ghislane', ghiradelli
                  if (current == 0)
                    {
                      if (GetAt(current + 2) == 'I')
                        MetaphAdd("J");
                      else
                        MetaphAdd("K");
                      current += 2;
                      break;
                    }
                }
              //Parker's rule (with some further refinements) - e.g., 'hugh'
              if (((current > 1) AND StringAt((current - 2), 1, "B", "H", "D", NULL) )
                  //e.g., 'bough'
                  OR ((current > 2) AND StringAt((current - 3), 1, "B", "H", "D", NULL) )
                  //e.g., 'broughton'
                  OR ((current > 3) AND StringAt((current - 4), 1, "B", "H", NULL) ) )
                {
                  current += 2;
                  break;
                }
              else
                {
                  //e.g., 'laugh', 'McLaughlin', 'cough', 'gough', 'rough', 'tough'
                  if ((current > 2)
                      AND (GetAt(current - 1) == 'U')
                      AND StringAt((current - 3), 1, "C", "G", "L", "R", "T", NULL) )
                    {
                      MetaphAdd("F");
                    }
                  else
                    if ((current > 0) AND GetAt(current - 1) != 'I')
                      MetaphAdd("K");

                  current += 2;
                  break;
                }
            }

          if (GetAt(current + 1) == 'N')
            {
              if ((current == 1) AND IsVowel(0) AND !SlavoGermanic())
                {
                  MetaphAdd("KN", "N");
                }
              else
                //not e.g. 'cagney'
                if (!StringAt((current + 2), 2, "EY", NULL)
                    AND (GetAt(current + 1) != 'Y') AND !SlavoGermanic())
                  {
                    MetaphAdd("N", "KN");
                  }
                else
                  MetaphAdd("KN");
              current += 2;
              break;
            }

          //'tagliaro'
          if (StringAt((current + 1), 2, "LI", NULL) AND !SlavoGermanic())
            {
              MetaphAdd("KL", "L");
              current += 2;
              break;
            }

          //-ges-,-gep-,-gel-, -gie- at beginning
          if ((current == 0)
              AND ((GetAt(current + 1) == 'Y')
                   OR StringAt((current + 1), 2, "ES", "EP", "EB", "EL", "EY", "IB", "IL", "IN", "IE", "EI", "ER", NULL)) )
            {
              MetaphAdd("K", "J");
              current += 2;
              break;
            }

          // -ger-,  -gy-
          if ((StringAt((current + 1), 2, "ER", NULL) OR (GetAt(current + 1) == 'Y'))
              AND !StringAt(0, 6, "DANGER", "RANGER", "MANGER", NULL)
              AND !StringAt((current - 1), 1, "E", "I", NULL)
              AND !StringAt((current - 1), 3, "RGY", "OGY", NULL) )
            {
              MetaphAdd("K", "J");
              current += 2;
              break;
            }

          // italian e.g, 'biaggi'
          if (StringAt((current + 1), 1, "E", "I", "Y", NULL) OR StringAt((current - 1), 4, "AGGI", "OGGI", NULL))
            {
              //obvious germanic
              if ((StringAt(0, 4, "VAN ", "VON ", "") OR StringAt(0, 3, "SCH", NULL))
                  OR StringAt((current + 1), 2, "ET", NULL))
                MetaphAdd("K");
              else
                //always soft if french ending
                if (StringAt((current + 1), 4, "IER ", NULL))
                  MetaphAdd("J");
                else
                  MetaphAdd("J", "K");
              current += 2;
              break;
            }

          if (GetAt(current + 1) == 'G')
            current += 2;
          else
            current += 1;
          MetaphAdd("K");
          break;

        case 'H':
          //only keep if first & before vowel or btw. 2 vowels
          if (((current == 0) OR IsVowel(current - 1))
              AND IsVowel(current + 1))
            {
              MetaphAdd("H");
              current += 2;
            }
          else//also takes care of 'HH'
            current += 1;
          break;

        case 'J':
          //obvious spanish, 'jose', 'san jacinto'
          if (StringAt(current, 4, "JOSE", NULL) OR StringAt(0, 4, "SAN ", NULL) )
            {
              if (((current == 0) AND (GetAt(current + 4) == ' ')) OR StringAt(0, 4, "SAN ", NULL) )
                MetaphAdd("H");
              else
                {
                  MetaphAdd("J", "H");
                }
              current +=1;
              break;
            }

          if ((current == 0) AND !StringAt(current, 4, "JOSE", NULL))
            MetaphAdd("J", "A");//Yankelovich/Jankelowicz
          else
            //spanish pron. of e.g. 'bajador'
            if (IsVowel(current - 1)
                AND !SlavoGermanic()
                AND ((GetAt(current + 1) == 'A') OR (GetAt(current + 1) == 'O')))
              MetaphAdd("J", "H");
            else
              if (current == last)
                MetaphAdd("J", " ");
              else
                if (!StringAt((current + 1), 1, "L", "T", "K", "S", "N", "M", "B", "Z", NULL)
                    AND !StringAt((current - 1), 1, "S", "K", "L", NULL))
                  MetaphAdd("J");

          if (GetAt(current + 1) == 'J')//it could happen!
            current += 2;
          else
            current += 1;
          break;

        case 'K':
          if (GetAt(current + 1) == 'K')
            current += 2;
          else
            current += 1;
          MetaphAdd("K");
          break;

        case 'L':
          if (GetAt(current + 1) == 'L')
            {
              //spanish e.g. 'cabrillo', 'gallegos'
              if (((current == (length - 3))
                   AND StringAt((current - 1), 4, "ILLO", "ILLA", "ALLE", NULL))
                  OR ((StringAt((last - 1), 2, "AS", "OS", "") OR StringAt(last, 1, "A", "O", NULL))
                      AND StringAt((current - 1), 4, "ALLE", NULL)) )
                {
                  MetaphAdd("L", " ");
                  current += 2;
                  break;
                }
              current += 2;
            }
          else
            current += 1;
          MetaphAdd("L");
          break;

        case 'M':
          if ((StringAt((current - 1), 3, "UMB", NULL)
               AND (((current + 1) == last) OR StringAt((current + 2), 2, "ER", NULL)))
              //'dumb','thumb'
              OR  (GetAt(current + 1) == 'M') )
            current += 2;
          else
            current += 1;
          MetaphAdd("M");
          break;

        case 'N':
          if (GetAt(current + 1) == 'N')
            current += 2;
          else
            current += 1;
          MetaphAdd("N");
          break;

        case 'Ñ':
          current += 1;
          MetaphAdd("N");
          break;

        case 'P':
          if (GetAt(current + 1) == 'H')
            {
              MetaphAdd("F");
              current += 2;
              break;
            }

          //also account for "campbell", "raspberry"
          if (StringAt((current + 1), 1, "P", "B", NULL))
            current += 2;
          else
            current += 1;
          MetaphAdd("P");
          break;

        case 'Q':
          if (GetAt(current + 1) == 'Q')
            current += 2;
          else
            current += 1;
          MetaphAdd("K");
          break;

        case 'R':
          //french e.g. 'rogier', but exclude 'hochmeier'
          if ((current == last)
              AND !SlavoGermanic()
              AND StringAt((current - 2), 2, "IE", NULL)
              AND !StringAt((current - 4), 2, "ME", "MA", NULL))
            MetaphAdd("", "R");
          else
            MetaphAdd("R");

          if (GetAt(current + 1) == 'R')
            current += 2;
          else
            current += 1;
          break;

//	case 'ß':
	case '\337':
	    // SZ Lig.
	  MetaphAdd("S");
	  current++;
	  break;
        case 'S':
          //special cases 'island', 'isle', 'carlisle', 'carlysle'
          if (StringAt((current - 1), 3, "ISL", "YSL", NULL))
            {
              current += 1;
              break;
            }

          //special case 'sugar-'
          if ((current == 0) AND StringAt(current, 5, "SUGAR", NULL))
            {
              MetaphAdd("X", "S");
              current += 1;
              break;
            }

          if (StringAt(current, 2, "SH", NULL))
            {
              //germanic
              if (StringAt((current + 1), 4, "HEIM", "HOEK", "HOLM", "HOLZ", NULL))
                MetaphAdd("S");
              else
                MetaphAdd("X");
              current += 2;
              break;
            }

          //italian & armenian
          if (StringAt(current, 3, "SIO", "SIA", NULL) OR StringAt(current, 4, "SIAN", NULL))
            {
              if (!SlavoGermanic())
                MetaphAdd("S", "X");
              else
                MetaphAdd("S");
              current += 3;
              break;
            }

          //german & anglicisations, e.g. 'smith' match 'schmidt', 'snider' match 'schneider'
          //also, -sz- in slavic language altho in hungarian it is pronounced 's'
          if (((current == 0)
               AND StringAt((current + 1), 1, "M", "N", "L", "W", NULL))
              OR StringAt((current + 1), 1, "Z", NULL))
            {
              MetaphAdd("S", "X");
              if (StringAt((current + 1), 1, "Z", NULL))
                current += 2;
              else
                current += 1;
              break;
            }

          if (StringAt(current, 2, "SC", NULL))
            {
              //Schlesinger's rule
              if (GetAt(current + 2) == 'H')
                //dutch origin, e.g. 'school', 'schooner'
                if (StringAt((current + 3), 2, "OO", "ER", "EN", "UY", "ED", "EM", NULL))
                  {
                    //'schermerhorn', 'schenker'
                    if (StringAt((current + 3), 2, "ER", "EN", NULL))
                      {
                        MetaphAdd("X", "SK");
                      }
                    else
                      MetaphAdd("SK");
                    current += 3;
                    break;
                  }
                else
                  {
                    if ((current == 0) AND !IsVowel(3) AND (GetAt(3) != 'W'))
                      MetaphAdd("X", "S");
                    else
                      MetaphAdd("X");
                    current += 3;
                    break;
                  }

              if (StringAt((current + 2), 1, "I", "E", "Y", NULL))
                {
                  MetaphAdd("S");
                  current += 3;
                  break;
                }
              //else
              MetaphAdd("SK");
              current += 3;
              break;
            }

          //french e.g. 'resnais', 'artois'
          if ((current == last) AND StringAt((current - 2), 2, "AI", "OI", NULL))
            MetaphAdd("", "S");
          else
            MetaphAdd("S");

          if (StringAt((current + 1), 1, "S", "Z", NULL))
            current += 2;
          else
            current += 1;
          break;

        case 'T':
          if (StringAt(current, 4, "TION", NULL))
            {
              MetaphAdd("X");
              current += 3;
              break;
            }

          if (StringAt(current, 3, "TIA", "TCH", NULL))
            {
              MetaphAdd("X");
              current += 3;
              break;
            }

          if (StringAt(current, 2, "TH", NULL)
              OR StringAt(current, 3, "TTH", NULL))
            {
              //special case 'thomas', 'thames' or germanic
              if (StringAt((current + 2), 2, "OM", "AM", NULL)
                  OR StringAt(0, 4, "VAN ", "VON ", NULL)
                  OR StringAt(0, 3, "SCH", NULL))
                {
                  MetaphAdd("T");
                }
              else
                {
                  MetaphAdd("0", "T");
                }
              current += 2;
              break;
            }

          if (StringAt((current + 1), 1, "T", "D", NULL))
            current += 2;
          else
            current += 1;
          MetaphAdd("T");
          break;

        case 'V':
          if (GetAt(current + 1) == 'V')
            current += 2;
          else
            current += 1;
          MetaphAdd("F");
          break;

        case 'W':
          //can also be in middle of word
          if (StringAt(current, 2, "WR", NULL))
            {
              MetaphAdd("R");
              current += 2;
              break;
            }

          if ((current == 0)
              AND (IsVowel(current + 1) OR StringAt(current, 2, "WH", NULL)))
            {
              //Wasserman should match Vasserman
              if (IsVowel(current + 1))
                MetaphAdd("A", "F");
              else
                //need Uomo to match Womo
                MetaphAdd("A");
            }

          //Arnow should match Arnoff
          if (((current == last) AND IsVowel(current - 1))
              OR StringAt((current - 1), 5, "EWSKI", "EWSKY", "OWSKI", "OWSKY", NULL)
              OR StringAt(0, 3, "SCH", NULL))
            {
              MetaphAdd("", "F");
              current +=1;
              break;
            }

          //polish e.g. 'filipowicz'
          if (StringAt(current, 4, "WICZ", "WITZ", NULL))
            {
              MetaphAdd("TS", "FX");
              current +=4;
              break;
            }

          //else skip it
          current +=1;
          break;

        case 'X':
          //french e.g. breaux
          if (!((current == last)
                AND (StringAt((current - 3), 3, "IAU", "EAU", NULL)
                     OR StringAt((current - 2), 2, "AU", "OU", NULL))) )
            MetaphAdd("KS");

          if (StringAt((current + 1), 1, "C", "X", NULL))
            current += 2;
          else
            current += 1;
          break;

        case 'Z':
          //chinese pinyin e.g. 'zhao'
          if (GetAt(current + 1) == 'H')
            {
              MetaphAdd("J");
              current += 2;
              break;
            }
          else
            if (StringAt((current + 1), 2, "ZO", "ZI", "ZA", NULL)
                OR (SlavoGermanic() AND ((current > 0) AND GetAt(current - 1) != 'T')))
              {
                MetaphAdd("S", "TS");
              }
            else
              MetaphAdd("S");

          if (GetAt(current + 1) == 'Z')
            current += 2;
          else
            current += 1;
          break;

        default:
          current += 1;
        }
    }

  metaph  = primary;
  //only give back 4 char metaph
  if (metaph.GetLength() > 4)
    metaph.SetAt(4,'\0');

  if (alternate)
    {
      metaph2 = secondary;
      if (metaph2.GetLength() > 4)
        metaph2.SetAt(4,'\0');
      num_high = _code(metaph2);
    }
  num_low = _code(metaph);

  UINT8 result = num_high;
  result <<= 32;
  result += num_low;

  return result;
}

GDT_BOOLEAN MetaphoneMatch(const UINT8 Hash1, const UINT8 Hash2)
{
  if (Hash1 == Hash2) return GDT_TRUE;

#define __Low32(_x)  (int)((_x) &  0x00000000FFFFFFFFLL)
#define __High32(_x) (int)(((_x) & 0xFFFFFFFF00000000LL) >> 32)
  int low1   = __Low32(Hash1);
  int high1  = __High32(Hash1);
  int low2   = __Low32(Hash2);
  int high2  = __High32(Hash2);

  if (low1 && ((low1 == low2) || (low1 == high2)))
    {
      return GDT_TRUE;
    }
  if (high1 && ((high1 == high2) || (high1 == low2)))
    {
      return GDT_TRUE;
    }
  return GDT_FALSE;
}


#ifdef TEST
int main(int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Usage: " << argv[0] << " string" << endl;
      return -1;
    }
  STRING input (argv[1]);
  MString Test(input);
  STRING  s1, s2;

  Test.DoubleMetaphone(s1, s2);

  cout << "Result \"" << input << "\" -> " << s1 << "+" << s2 << endl;
}


#endif
