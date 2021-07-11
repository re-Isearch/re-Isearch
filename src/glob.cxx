/*
// Unix Glob()
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
#include "gdt.h"
#include <ctype.h>

static inline GDT_BOOLEAN IsPathSep(const int c)
{
  return (c == '\\' || c == '/');
}


GDT_BOOLEAN Glob(const UCHR *pattern, const UCHR *str, GDT_BOOLEAN dot_special)
{
#ifdef _WIN32
# define CASE(_x) tolower(_x)
#else
# define CASE(_x) (_x)
#endif
  char c;
  const UCHR *cp;
  GDT_BOOLEAN done = GDT_FALSE, ret_code, ok;
  // Below is for vi fans
  const char OB = '{', CB = '}';

  // dot_special means '.' only matches '.'
  if (dot_special && !isalnum(*str) && *pattern != *str)
    return GDT_FALSE;

  while ((*pattern != '\0') && (!done) && (((*str == '\0') &&
	       ((*pattern == OB) || (*pattern == '*'))) || (*str != '\0')))
    {
      switch (*pattern)
	{
	case '\\':
	  pattern++;
	  if (*pattern != '\0')
	    pattern++;
	  break;
	case '*':
	  pattern++;
	  ret_code = GDT_FALSE;
	  while ((*str != '\0') && (!(ret_code = Glob (pattern, str++, GDT_FALSE))));
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
	  if (*pattern == '\\')
	    {
	      pattern++;
	      if (*pattern == '\0')
		{
		  done = GDT_TRUE;
		  break;
		}
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
	      if (*pattern == '\\')
		{
		  pattern++;
		  if (*pattern == '\0')
		    {
		      done = GDT_TRUE;
		      break;
		    }
		}
	      if ((*str < c) || (*str > *pattern))
		{
		  pattern++;
		  goto repeat;
		}
	    }
	  else if (CASE(*pattern) != CASE(*str) )
	    {
	      pattern++;
	      goto repeat;
	    }
#ifdef EBUG
	  else cerr << "Compare " << *pattern <<  " with " << *str << endl;
#endif
	  pattern++;
	  while ((*pattern != ']') && (*pattern != '\0'))
	    {
	      if ((*pattern == '\\') && (*(pattern + 1) != '\0'))
		pattern++;
	      pattern++;
	    }			// while()
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
		  if (*pattern == '\\')
		    pattern++;
		  ok = (*pattern++ == *cp++);
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
		    {
		      if (*++pattern == '\\')
			{
                      if (*++pattern == CB)
			    pattern++;
			}
		    }		// while()
		}
	      else
		{
                 while (*pattern != CB && *pattern != ',' && *pattern != '\0')
		    {
		      if (*++pattern == '\\')
			{
                            if (*++pattern == CB || *pattern == ',')
			    pattern++;
			}
		    }		// while()
		}
	      if (*pattern != '\0')
		pattern++;
	    }			// while()
	  break;
	default:
#if 0
	  // allow multiple //
	  if (IsPathSep(*str) && IsPathSep(*pattern))
	    {
	      while (IsPathSep(*str))    str++;
	      while (IsPathSep(*pattern))pattern++;
	    }
#endif
	  if (CASE(*str) == CASE(*pattern))
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


//-----------------------------------------------------------------------------

#ifdef EBUG
#include <stdio.h>
main(int argc, char **argv)
{
  if (Glob((const UCHR *)argv[1], (const UCHR *)argv[2], GDT_TRUE))
    printf("TRUE\n");
  else
    printf("FALSE\n");
}
#endif

//-----------------------------------------------------------------------------
