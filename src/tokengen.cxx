/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)tokengen.cxx  1.7 06/27/00 20:31:59 BSN"

#include <ctype.h>
#include "common.hxx"
#include "tokengen.hxx"

size_t TOKENGEN::GetTotalEntries ()
{
  DoParse ();
  return TokenList.GetTotalEntries ();
}

void TOKENGEN::SetQuoteStripping (GDT_BOOLEAN DoItOrNot)
{
  DoStripQuotes = DoItOrNot;
  HaveParsed = GDT_FALSE;
  DoParse ();
}

TOKENGEN::TOKENGEN (const STRING& InString) : DoStripQuotes (GDT_FALSE), 
HaveParsed (GDT_FALSE)
{
  InCharP = InString.NewCString ();
}

TOKENGEN::~TOKENGEN ()
{
  if (InCharP) delete[] InCharP;
}

void TOKENGEN::DoParse (void)
{
  if (!HaveParsed)
    {
      char *Next = InCharP;
      STRING TokenStr;

      while ((Next = nexttoken (Next, &TokenStr)))
	{
	  if (TokenStr != "\\")
	    {
	      TokenList.AddEntry (TokenStr);
	    }
	}
      HaveParsed = GDT_TRUE;
    }
}


GDT_BOOLEAN TOKENGEN::GetEntry (const size_t Index, STRING *StringEntry)
{
  DoParse ();
  return TokenList.GetEntry (Index, StringEntry);
}

char *TOKENGEN::nexttoken (char *input, STRING *token)
{
  GDT_BOOLEAN istoken = GDT_FALSE;

  token->Clear();
  while (*input)
    {
      if (isspace ((unsigned char)*input))
	{
	  input++; // go past
	  if (istoken)
	    {
	      return input;
	    }
	  continue;
	}
      if ((*input == '(') || (*input == ')') || (*input == '!'))
	{
	  if (!istoken)
	    {
	      //NOTE: STRING = char. doesn't work.
	      token->Cat(*input++);
#if 1
	      if (*input == ':' && *(input+1) && !isspace(*(input+1)))
		{
		  token->Cat(*input++);
		  if (*input == '-' || *input == '+')
		    token->Cat(*input++);
		  do {
		  token->Cat(*input++);
		  } while (isdigit(*input) || *input == '.') ;
		}
//cerr << "Token=" << *token << endl;
#endif
	      return input;
	    }
	  else
	    {
	      return input;
	    }
	}

      if (*input == '"')
	{
	  //quoted strings are literals and should be returned as one token
	  CHR *BeginQuote;
	  if (!DoStripQuotes)
	    BeginQuote = input;
	  else
	    BeginQuote = ++input;

	  do
	    {
	      token->Cat( *input++ );
	    }
	  while (*input != '"' && *input);

	  if (*input == '"')
	    {
	      if (!DoStripQuotes)
		token->Cat(*input++);
	      istoken = GDT_TRUE;

	      // special case for weighted terms (BUGFIX)
	      if ( *input == ':' && isdigit(*(input+1)) )
		{
		  do {
		    token->Cat(*input++);
		  } while (isdigit(*input));
		}
	      continue;
	    }
	  else
	    {
	      //if quotes aren't matched, parse it
	      //as part of a single term.
	      input = ++BeginQuote;
	      token->EraseAfter (token->SearchReverse ('"'));
	      continue;
	    }
	}

      if ( *input == '{') //}
	{
	  char *BeginQuote;
	  BeginQuote = input;
	  // We've found a grouping (special case - RECT{...})
	  do {
	    *token += *input;
	    input++;
	  } while (/*{*/ *input != '}' && *input);
	  if (/*{*/ *input == '}')
	    {
	      input++;
	      istoken = 1;
	    }
	  else
	    {
	      //if braces aren't matched, parse it
	      //as part of a single term.
	      input = ++BeginQuote;
	      token->EraseAfter(token->SearchReverse(/*{*/ '}'));
	    }
	  continue;
	}

      if (*input == '&' || *input == '|')
	{
	  //looks like a C-style operator
	  if ((*input == *(input + 1)) ||
	      (*input == '&' && *input == '!'))
	    {
	      // it IS a C-style operator
	      if (!istoken)
		{
		  //if we're looking for a token, grab it.
		  //again, STRING = char, doesn't work.
		  token->Cat(*input++);
		  token->Cat(*input++);
		  return input;
		}
	      else
		{
		  return input;
		}
	    }
	  else
	    {
	      //must be part of something else
	      istoken = GDT_TRUE;
	      token->Cat( *input++ );
	      continue;
	    }
	}

      istoken = GDT_TRUE;
      token->Cat( *input++ );
      continue;
    }

  return (istoken) ? input : NULL;
}
