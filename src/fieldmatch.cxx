/*
//
// Pattern        Function
// -----------------------------------------------------
//  '*'         = match 0 or more occurances of anything
// "[abc]"      = match anyof "abc" (ranges supported)
// "{xx,yy,zz}" = match anyof "xx", "yy", or "zz"
// '?'          = match any character
//
//  '\'  is used to "escape" special characters
// Recursive
*/
#include "common.hxx"
#include "string.hxx"

#define SP_EQ(_x, _y) ((*(_x) == '\\' || *(_x) == '/' || *(_x) == '|') \
	&& (*(_y) == '\\' || *(_y) == '/' || *(_y) == '|'))

static GDT_BOOLEAN FieldPathGlob(const UCHR *pattern, const UCHR *str)
{
# define CASE(_x) toupper(_x)
  char c;
  const UCHR *cp;
  GDT_BOOLEAN done = GDT_FALSE, ret_code, ok;
  // Below is for vi fans
  const char OB = '{', CB = '}';

  while ((*pattern != '\0') && (!done) && (((*str == '\0') &&
	       ((*pattern == OB) || (*pattern == '*'))) || (*str != '\0')))
    {
      switch (*pattern)
	{
	case '*':
	  pattern++;
	  ret_code = GDT_FALSE;
	  while ((*str != '\0') && (!(ret_code = FieldPathGlob (pattern, str++))));
	  if (ret_code)
	    {
	      while (*str != '\0')
		str++;
	      while (*pattern != '\0')
		pattern++;
	    }
	  break;
	case '[':
	  pattern++;
	repeat:
	  if ((*pattern == '\0') || (*pattern == ']'))
	    {
	      done = GDT_TRUE;
	      break;
	    }
	  if (*(pattern + 1) == '-')
	    {
	      c = *pattern;
	      pattern += 2;
	      if (*pattern == ']')
		{
		  done = GDT_TRUE;
		  break;
		}
	      if ((*str < c) || (*str > *pattern))
		{
		  pattern++;
		  goto repeat;
		}
	    }
	  else if (CASE(*pattern) != CASE(*str) && !SP_EQ(pattern, str))
	    {
	      pattern++;
	      goto repeat;
	    }
#ifdef EBUG
	  else cerr << "Compare " << *pattern <<  " with " << *str << endl;
#endif
	  pattern++;
	  while ((*pattern != ']') && (*pattern != '\0'))
	    pattern++;
	  if (*pattern != '\0')
	    {
	      pattern++, str++;
	    }
	  break;
	case '?':
	  pattern++;
	  str++;
	  break;
	case OB:
	  pattern++;
         while ((*pattern != CB) && (*pattern != '\0'))
	    {
	      cp = str;
	      ok = GDT_TRUE;
	      while (ok && (*cp != '\0') && (*pattern != '\0') &&
                 (*pattern != ',') && (*pattern != CB))
		{
		  ok = ((CASE(*pattern) == CASE(*cp)) || SP_EQ(pattern, cp));
		  pattern++;
		  cp++;
		}		// while()
	      if (*pattern == '\0')
		{
		  ok = GDT_FALSE;
		  done = GDT_TRUE;
		  break;
		}
	      else if (ok)
		{
		  str = cp;
                  while ((*pattern != CB) && (*pattern != '\0'))
		    pattern++;
		}
	      else
		{
                 while (*pattern != CB && *pattern != ',' && *pattern != '\0')
		   pattern++;
		}
	      if (*pattern != '\0')
		pattern++;
	    }			// while()
	  break;
	default:
	  if (CASE(*str) == CASE(*pattern) || SP_EQ(str, pattern) )
	    {
	      str++, pattern++;
	    }
	  else
	    {
#ifdef EBUG
	      cerr << "*str = " << *str << " != " << *pattern << endl;
#endif
	      done = GDT_TRUE;
	    }
	}			// switch()
    }				// while()
  while (*pattern == '*')
    pattern++;
  return ((*str == '\0') && (*pattern == '\0'));
}

GDT_BOOLEAN STRING::FieldMatch(const STRING& OtherString) const
{
  return FieldPathGlob((const UCHR *)m_pchData, (const UCHR *)OtherString.m_pchData);
}


GDT_BOOLEAN STRING::FieldMatch(const CHR * CString) const
{
  return FieldPathGlob((const UCHR *)m_pchData, (const UCHR *)CString);
}
  

//-----------------------------------------------------------------------------

#ifdef EBUG
#include <stdio.h>
main(int argc, char **argv)
{
  if (FieldPathGlob((const UCHR *)argv[1], (const UCHR *)argv[2]))
    printf("TRUE\n");
  else
    printf("FALSE\n");
}
#endif

//-----------------------------------------------------------------------------
