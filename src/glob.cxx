/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
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

static inline bool IsPathSep(const int c)
{
  return (c == '\\' || c == '/');
}


bool Glob(const UCHR *pattern, const UCHR *str, bool dot_special)
{
#ifdef _WIN32
# define CASE(_x) tolower(_x)
#else
# define CASE(_x) (_x)
#endif
  char c;
  const UCHR *cp;
  bool done = false, ret_code, ok;
  // Below is for vi fans
  const char OB = '{', CB = '}';

  // dot_special means '.' only matches '.'
  if (dot_special && !isalnum(*str) && *pattern != *str)
    return false;

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
	  ret_code = false;
	  while ((*str != '\0') && (!(ret_code = Glob (pattern, str++, false))));
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
	      done = true;
	      break;
	    }
	  if (*pattern == '\\')
	    {
	      pattern++;
	      if (*pattern == '\0')
		{
		  done = true;
		  break;
		}
	    }
	  if (*(pattern + 1) == '-')
	    {
	      c = *pattern;
	      pattern += 2;
	      if (*pattern == ']')
		{
		  done = true;
		  break;
		}
	      if (*pattern == '\\')
		{
		  pattern++;
		  if (*pattern == '\0')
		    {
		      done = true;
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
	      ok = true;
	      while (ok && (*cp != '\0') && (*pattern != '\0') &&
                 (*pattern != ',') && (*pattern != CB))
		{
		  if (*pattern == '\\')
		    pattern++;
		  ok = (*pattern++ == *cp++);
		}		// while()
	      if (*pattern == '\0')
		{
		  ok = false;
		  done = true;
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
	      done = true;
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
  if (Glob((const UCHR *)argv[1], (const UCHR *)argv[2], true))
    printf("TRUE\n");
  else
    printf("FALSE\n");
}
#endif

//-----------------------------------------------------------------------------
