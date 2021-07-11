#pragma ident  "@(#)htmlcat.cxx  1.6 12/18/00 01:03:04 BSN"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "common.hxx"
//
// WARNING: Special copyrighted code ahead!!!!
//

// Cat HTML stream 
// 1) Handles VT100 descriptive markup: Bold, Underlined and Overstriked
// 1) Some minimal support for VT100 Esc sequences
// 2) Handles locale (LC_CTYPE)--- now ISO 8859-x
// 3) Tries to identify valid URLs
// 4) Although also possible we don't handle man(1) type man page
//    references since this requires public use BSn's htmlman server.
//
// URL logic:
//   URL : = METHOD "://" host [ ":" PORT ] [ PATH ]
//   METHOD := ALPHA *( ALPHA | DIGIT | "." | "-" )
//
// NOTE: No check is made to see if the URL has an IANA registered method.
//       Does not markup HREF="/FOO"--- relative URLs in HTML elements-- at
//       this time.
//       For performance, the code depends upon the BSn STRING superset code
//       whence no provision has been made for the original CNIDR STRING class.
// TODO: Optimize!!
//
///*****************************************************************************
//*  HtmlCat and MailAnchor code are originally                                *
//*     (c) Copyright 1992,1993,1994,1995,1996 Basis Systeme netzwerk, Munich  *
//*  Released in 2020 into the public domain.                                  *
//*                                                                            *
//*****************************************************************************/
//
const STRING& CHARSET::HtmlCat (const CHR ch, PSTRING StringBufferPtr) const
{
/*
     Decimal - Character

     |  0 NUL|  1 SOH|  2 STX|  3 ETX|  4 EOT|  5 ENQ|  6 ACK|  7 BEL|
     |  8 BS |  9 HT | 10 NL | 11 VT | 12 NP | 13 CR | 14 SO | 15 SI |
     | 16 DLE| 17 DC1| 18 DC2| 19 DC3| 20 DC4| 21 NAK| 22 SYN| 23 ETB|
     | 24 CAN| 25 EM | 26 SUB| 27 ESC| 28 FS | 29 GS | 30 RS | 31 US |
     | 32 SP | 33  ! | 34  " | 35  # | 36  $ | 37  % | 38  & | 39  ' |
     | 40  ( | 41  ) | 42  * | 43  + | 44  , | 45  - | 46  . | 47  / |
     | 48  0 | 49  1 | 50  2 | 51  3 | 52  4 | 53  5 | 54  6 | 55  7 |
     | 56  8 | 57  9 | 58  : | 59  ; | 60  < | 61  = | 62  > | 63  ? |
     | 64  @ | 65  A | 66  B | 67  C | 68  D | 69  E | 70  F | 71  G |
     | 72  H | 73  I | 74  J | 75  K | 76  L | 77  M | 78  N | 79  O |
     | 80  P | 81  Q | 82  R | 83  S | 84  T | 85  U | 86  V | 87  W |
     | 88  X | 89  Y | 90  Z | 91  [ | 92  \ | 93  ] | 94  ^ | 95  _ |
     | 96  ` | 97  a | 98  b | 99  c |100  d |101  e |102  f |103  g |
     |104  h |105  i |106  j |107  k |108  l |109  m |110  n |111  o |
     |112  p |113  q |114  r |115  s |116  t |117  u |118  v |119  w |
     |120  x |121  y |122  z |123  { |124  | |125  } |126  ~ |127 DEL|
*/
  unsigned wch = (unsigned) UCS(ch);
  if (wch == 60)        StringBufferPtr->Cat ("&lt;");
  else if (wch == 62)   StringBufferPtr->Cat ("&gt;");
  else if (wch == 38)   StringBufferPtr->Cat ("&amp;");
  else if (wch == 34)   StringBufferPtr->Cat ("&quot;");
  else if (wch == 160)  StringBufferPtr->Cat ("&nbsp;");
  else if (wch == 173)   StringBufferPtr->Cat("&shy;");
  else if ( wch < 127 && (ib_isascii((UCHR)ch) ||  ib_isspace ((UCHR)ch)))
    StringBufferPtr->Cat (ch);
  else
    {
	char buf[9];
	// Non-ASCII character -- Map to UCS
	sprintf(buf, "&#%u;", wch); 
	StringBufferPtr-> Cat (buf);
    }
  return *StringBufferPtr;
}

#if 1 /* EXPERIMENTAL (NOT EXHAUSTIVELY TESTED) */
static inline STRINGINDEX URNAnchor(const STRING& Input, STRINGINDEX Start = 1)
{
  STRINGINDEX pos;
  UCHR ch, lastCh = 0;
  // UCHR cl;
  size_t sawC;
  STRINGINDEX len = Input.GetLength();

//  URN ::= "urn:" NID ":" NSS
//	NID           ::= let-num [ 1*31let-num-hyp ]
//	let-num-hyp ::=  letter / number / "-"
//	let-num     ::= letter / number
// 	letter       ::= %x41..5A / %x61..7A
//	number      ::= %x30..39
//
//  NSS         ::= 1*URN_chars
//	URN_chars   ::= trans / ("%" hex hex)
//	trans       ::= letter / number / other / reserved
//	hex         ::= number / %x41..46 / %x61..66
//	other       ::= "(" / ")" / "+" / "," / "-" / "." /
//			":" / "=" / "@" / ";" / "$" / "_" /
//			"!" / "*" / "'"

  while ((pos = Input.SearchAny("urn:", Start)) > 0)
    {
      Start = pos + 1;
      // cl = 0;
      sawC = 0;
      if (pos > 1)
	{
	  ch = (UCHR)Input.GetChr(pos-1);
	  if (! isspace(ch) && strchr("\\\"<>[]{}`|", ch) == NULL) 
	    continue;
	}
      if (! isalnum((UCHR)Input.GetChr(pos+4)))
	continue;
      do {
	ch = (UCHR)Input.GetChr(Start++);
	if (ch == ':')
	  {
	    sawC++;
	    if (lastCh == ch)
	      break; // :: in URN?
	  }
	else if (lastCh == '%' && !isxdigit(ch))
	  break; // Back use of '%'
	lastCh = ch;
      } while ((Start <= len) && 
	( isalnum(ch) || ch == ':' ||
	(sawC>1 && strchr("%;:.,/-+_'=@!$?#*()", ch)) ) );
      // Was it a real URN?
      if (sawC > 1 && ch != ':' && lastCh != '%')
	break; // URN ::= "urn:" NID ":" NSS
    }
  return pos;
}

// Find the first Internet email anchor after Start in Input
//
// Internet Mail Address: path@host
// host can contain Alphanumeric, '-' or '.'
// path can contain Alphanumberic, '-', '.', '!', '%', '/', '=' or '_'
// X400 gateways use also '/' and '='
//
// examples:
//	user@host.dom
//	foo.bar@host.dom
//	Z3950IW%NERVM.BITNET@vm.gmd.de
//	joe!sam@foo.bar
//
// Note: X.400 and "pure" UUCP mail addresses are not handled.
static inline STRINGINDEX MailAnchor(const STRING& Input, STRINGINDEX Start=1)
{
  // Try to find a "legal" email address reference.
  UCHR ch, last_ch;
  STRINGINDEX add_pos, len;
  INT dots;
  while ((add_pos = Input.Search('@', Start)) > 0)
    {
      Start = add_pos+1;
      last_ch = 0;
      // Mail paths can contain [a-z],[0-9],'-','_','%', '!' and '.'
      while (--add_pos > 0 /* was 1 */)
        {
          ch = Input.GetChr(add_pos);
	  if (ch == '/' && last_ch && !isalpha(last_ch))
	    {
	      break; // Bad X.400
	    }
          if (!isascii(ch) ||
		(! isalnum(ch) && ch != '-' && ch != '.' &&
		ch != '%' && ch != '!' && ch != '_' && ch != '/' && ch != '='))
	    {
              break;
	    }
	  if (! isalnum(ch) && last_ch && ! isalnum(last_ch))
	    break; // No good
	  last_ch = ch;
        }
      add_pos++;

      // See if we are looking at an email address
      if (last_ch == '/')
	{
	  // X.400 mail gateway?
	  for (STRINGINDEX x=add_pos+1; x < Start; x++)
	    {
	      last_ch = Input.GetChr(x);
	      if (!isalpha(last_ch))
		break;
	    }
	  if (last_ch != '=')
	    continue; // Nope
	}
      else if (!isalpha(last_ch)) continue; // Nope
      last_ch = Input.GetChr(Start);
      if (! isalnum(last_ch)) continue; // First character must be alphanumeric

      GDT_BOOLEAN ipnum = isdigit(last_ch);
      len = Input.GetLength();
      dots = 0;
      for (STRINGINDEX x = Start+1; x <= len; x++)
	{
	  ch = Input.GetChr(x);
	  // Email address delimiters (punctuation)
	  if ( isspace(ch) ||
		ch == '>' || ch == '<' ||
		ch == ')' || ch == '(' ||
		ch == ']' || ch == '[' ||
		ch == '}' || ch == '{' || 
		ch == ',' || ch == ';')
	    {
	      if (! isalnum(last_ch) || dots == 0 || len - Start <= 4)
		{
		  if (last_ch != '.') // Allow tailing .
		    break; // Nope!
		}
	      if (isdigit(last_ch))
		{
		  if (!ipnum || dots != 3)
		    break; // To many digits
		}
	      else if (ipnum)
		{
		  break; // Not an internet name
		}
	      return add_pos; // Looking at email address
	    }
	  if (! isalnum(ch) && ch != '.' && ch != '-')
	    break; // Not an email address
	  if (ch == '.')
	    {
	      if (last_ch == '.')
		break; // No .. allowed
	      dots++;
	    }
	  last_ch = ch;
#if 1
	  if (x == len)
	    {
	      return add_pos; // Looks good
	    }
#endif
	}
    }
  return 0; // Nothing
}

#endif

const STRING& CHARSET::HtmlCat (const STRING& Input, GDT_BOOLEAN Anchor) const
{
  STRING tmp;
  return HtmlCat(Input, &tmp, Anchor);
}



const STRING& CHARSET::HtmlCat (const STRING& Input, PSTRING StringBufferPtr, GDT_BOOLEAN Anchor) const
{
  if (Anchor)
    {
      HtmlCat(Input, StringBufferPtr);
    }
  else
    {
      for (register const CHR *tp = (const CHR *)Input; *tp; tp++)
	HtmlCat(*tp, StringBufferPtr);
    }
  return *StringBufferPtr;
}


// Warning: Very tangled web ahead...
const STRING& CHARSET::HtmlCat (const STRING& Input, PSTRING StringBufferPtr) const
{
  char   buf[9];
  STRING sbuf;
  UCHR ch, nextCh;
  const char *underline = NULL;
  const char *font = NULL;
  const char *color = NULL;
  const char *fg_color = NULL, *bg_color = NULL;
  STRINGINDEX Len = Input.GetLength ();
  STRINGINDEX anchor_pos = Input.Search ("://", 1);
  const char *Font_Close = "</font>";
  if (anchor_pos)
    {
      // Find URL start
      STRINGINDEX old_pos;
      UCHR lastCh = 0;
      do
	{
	  old_pos = anchor_pos;
	  if ( ib_isalnum (Input.GetChr (anchor_pos + 3)))
	    {
	      while (anchor_pos > 1)
		{
		  ch = Input.GetChr (--anchor_pos - 1);
		  if (! ib_isalnum (ch) && ch != '-')
		    break;
		  lastCh = ch;
		}
	      // Is it really a URL?
	      if (isalpha (lastCh))
		break;		// Simple test for now..

	    }
	}
      while ((anchor_pos = Input.Search ("://", old_pos + 3)) != 0);
    }
#if 1 /* EXPERIMENTAL */
  STRINGINDEX add_pos = MailAnchor(Input);
  STRINGINDEX urn_pos = URNAnchor(Input);
#endif
  for (STRINGINDEX idx = 1; idx <= Len; idx++)
    {
#if 1 /* EXPERIMENTAL */
      if (idx == urn_pos)
	{
	  if (idx != anchor_pos)
	    {
	      STRING Anchor;
	      for(;;)
		{
		  ch = (UCHR) Input.GetChr(idx++);
/*
		  A URN ends with an excluded char:
		  excluded ::= octets 1-32 (1-20 hex) / "\" / """ /
			"&" / "<" / ">" / "[" / "]" / "^" /
			"`" / "{" / "|" / "}" / "~" /
			octets 127-255 (7F-FF hex)
*/
		  if (ch >= 127 || ch <= 32 ||
			ch == '\\' || ch == '\"' || ch == '&' || 
			ch == '<' || ch == '>' ||
			ch == '[' || ch == ']' ||
			ch == '^' || ch == '`' ||
			ch == '{' || ch == '|' ||
			ch == '}' || ch == '~')
		    {
		      idx--;
		      break;
		    }
		  Anchor.Cat(ch);
		}
#if 1 /* New try to catch \033[7m", "\033[m */
	      if (Anchor.Search("\033["))
		{
		  STRING link (Anchor);
		  STRINGINDEX ipos = link.Search("://");
		  if (ipos)
		    {
		      STRING tmp;
		      link.SetChr(ipos, '\v');
		      HtmlCat(link, &tmp); 
		      if ((ipos = tmp.Search("\v//")) != 0)
			tmp.SetChr(ipos, ':');
		      link = tmp;
		    }
		  *StringBufferPtr << "<A HREF=\"" <<
		    Anchor.VT100Strip() << "\">" << link << "</A>";
		}
	      else
#endif
	      *StringBufferPtr << "<A HREF=\"" << Anchor <<
		"\">" << Anchor << "</A>";
	    }
	  urn_pos = URNAnchor(Input, idx+1);
      } else if (idx == add_pos) {
	STRING Anchor, What;
	const char HEX[] = "0123456789ABCDEF";
	do {
	  ch = (UCHR) Input.GetChr(idx);
	  if ( ib_isspace(ch) ||
		ch == '>' || ch == '<' ||
		ch == ')' || ch == '(' ||
		ch == ']' || ch == '[' ||
		ch == '{' || ch == '}' ||
		ch == ',' || ch == ';') {
	    idx--;
	    break;
	  }
	  What.Cat(ch);
	  if ( ib_isascii(ch) && ( ib_isalnum(ch) || ch == '.' || ch == '@' || ch == '-' ||
		ch == '_' || ch == '/' || ch == '='))
	    {
	      // BUGFIX: Handle trailing '.'
	      if (ch == '.' &&  ib_isspace(Input.GetChr(idx+1)))
		{
		  idx--;
		  break;
		}
	      Anchor.Cat(ch);
	    }
	  else
	    {
	      Anchor << '%' << HEX[(ch & '\377') >> 4] << HEX[(ch & '\377') % 16];
	    }
	} while (++idx <= Len);	
	ch = StringBufferPtr->GetChr( StringBufferPtr->GetLength() );
        // Fri Oct 25 00:11:13 MET DST 1996 check if ch is != 0 
	if (ch && ! ib_isspace(ch) && !ispunct(ch)) // Added !ispunct(ch)
	  {
	    StringBufferPtr->Cat ( Anchor );
	  }
	else
	  {
	    *StringBufferPtr << "<A HREF=\"mailto:" << Anchor << "\" \
onMouseOver=\"self.status='eMail " << What << "'; return true\">" << What << "</A>";
	  }
	add_pos = MailAnchor(Input, idx);

      } else
#endif
      if (idx == anchor_pos)
	{
	  // Handle http:// embeded in messages
	  STRING Anchor;
	  do
	    {
	      ch = (UCHR) Input.GetChr (idx);
	      if ( ib_isspace (ch) || ch == '>' || ch == '<')
		break;
	      // Strip trailing punctuation---
	      // but URL can have trailing '/' or '*' (z39.50 URL extension)
	      if (ispunct (ch) && ch != '/' && ch != '*')
		{
		  if (idx < Len &&  ib_isspace (Input.GetChr (idx + 1)))
		    break;
		}
	      Anchor.Cat (ch);
	    }
	  while (++idx <= Len);
	  // Note: we don't check if we have a registered
	  // protocol at this time
#if 1 /* New try to catch \033[7m", "\033[m */
	  if (Anchor.Search("\033["))
	    {
	      STRING link (Anchor);
	      STRINGINDEX ipos = link.Search("://");
	      if (ipos)
		{
		  STRING tmp;
		  link.SetChr(ipos, '\v');
		  HtmlCat(link, &tmp);
		  if ((ipos = tmp.Search("\v//")) != 0)
		    tmp.SetChr(ipos, ':');
		  link = tmp;
		}
	      *StringBufferPtr << "<A HREF=\"" <<
		Anchor.VT100Strip() << "\">" << link << "</A>";
	    }
	  else
#endif
	  *StringBufferPtr << "<A HREF=\"" << Anchor << "\">"
	    << Anchor << "</A>";
	  if ((anchor_pos = Input.Search ("://", idx)) > 0)
	    {
	      // Find URL start
	      STRINGINDEX old_pos;
	      UCHR lastCh = 0;
	      do
		{
		  old_pos = anchor_pos;
		  if ( ib_isalnum (Input.GetChr (anchor_pos + 3)))
		    {

		      while (anchor_pos > 1)
			{
			  ch = Input.GetChr (--anchor_pos - 1);
			  if (! ib_isalnum (ch) && ch != '-')
			    break;
			  lastCh = ch;
			}
		      // Is it really a URL?
		      if (isalpha (lastCh))
			break;	// Simple test for now..

		    }
		}
	      while ((anchor_pos = Input.Search ("://", old_pos + 3)) != 0);
	    }
	  idx--;		// Backup
	}
      else
	{
	  ch = (UCHR) Input.GetChr (idx);
	  nextCh = Input.GetChr (idx + 1); 
	  if (ch == '\033' && nextCh == '[' )
	    {
	      // Minimal support for VT100 ESC sequences
/*
                 The ECMA-48 SGR sequence ESC [
                 <parameters> m sets display attributes.
                 Several attributes can be set in the same
                 sequence.

                  0 reset all attributes to their defaults
                  1 set bold
                  2 set half-bright
                  4 set underscore
                  5 set blink
                  7 set reverse video
                  21 set normal intensity (this is not compatible with ECMA-48)
                  22 set normal intensity
                  24 underline off
                  25 blink off
                  27 reverse video off
                  30 set black foreground
                  31 set red foreground
                  32 set green foreground
                  33 set brown foreground
                  34 set blue foreground
                  35 set magenta foreground
                  36 set cyan foreground
                  37 set white foreground
                  38 set underscore on, set default foreground color
                  39 set underscore off, set default foreground color
                  40 set black background
                  41 set red background
                  42 set green background
                  43 set brown background
                  44 set blue background
                  45 set magenta background
                  46 set cyan background
                  47 set white background
                  49 set default background color
*/
  
	      idx += 2;
	      ch = Input.GetChr (idx);
	      nextCh = Input.GetChr (idx + 1);
	      if (nextCh == 'm')
		{
		  if (font)
		    {
		      *StringBufferPtr << '<' << '/' << font << '>';
		      font = NULL;
		    }
		  if (color)
		    {
		      *StringBufferPtr << Font_Close;
		      color = NULL;
		    }
		  if      (ch == '1')	font = "B";
		  else if (ch == '2')   color = "#999999"; // Half bright
		  else if (ch == '4')	underline = "U";
		  else if (ch == '5')	font = "BLINK";
		  else if (ch == '6')   font = "STRONG";
		  else if (ch == '7')   {
		    font = "B";
		    color = "white;background-color:black";
		  } else
		    font = NULL;
		  if (font || underline)
		    *StringBufferPtr << '<' << (font ? font : underline) << '>';
		  if (color)
		    *StringBufferPtr << "<font style=\"color:" << color << "\">";
		  idx++; // Skip
		}
	      else if (isdigit(nextCh) && (ch == '2' || ch == '3' || ch == '4'))
		{
		  idx++;
		  switch (ch) {
		    case '2':
		      switch (nextCh) {
			case '1': // 21 set normal intensity (this is not compatible with ECMA-48)
			case '2': // 22 set normal intensity
			case '7': // // 27 reverse video off
			  if (font)
			    *StringBufferPtr << "</" << font << ">";
			  font = NULL;
			  break;
			case '4': // 24 underline off
			  if (underline)
			    *StringBufferPtr << "</" << underline << ">";
			  underline = NULL;
			  break;
		      }
		      break;
		    case '3':
		      if (fg_color) *StringBufferPtr << Font_Close; 
		      switch (nextCh) {
			case '0': fg_color = "black"; break;
			case '1': fg_color = "red"; break;
			case '2': fg_color = "green"; break;
			case '3': fg_color = "brown"; break;
			case '4': fg_color = "blue"; break;
			case '5': fg_color = "magenta"; break;
			case '6': fg_color = "cyan"; break;
			case '7': fg_color = "white"; break;
			default:
			  fg_color = "black";
		      }
		      *StringBufferPtr << "<font style=\"color:" << fg_color << ">";
		      break;
		    case '4':
		      if (bg_color) *StringBufferPtr << Font_Close;
		      switch (nextCh) {
			case '0': bg_color = "black"; break;
			case '1': bg_color = "red"; break;
			case '2': bg_color = "green"; break;
			case '3': bg_color = "brown"; break;
			case '4': bg_color = "blue"; break;
			case '5': bg_color = "magenta"; break;
			case '6': bg_color = "cyan"; break;
			case '7': bg_color = "white"; break;
			default:
			   bg_color="white";
		      }
		      break;
		     *StringBufferPtr << "<font style=\"background-color:" << bg_color << ">";

		  }
		}
	      else if (ch == 'm')
		{
		  if (bg_color)
		    {
		      *StringBufferPtr << Font_Close;
		      bg_color = NULL;
		    }
		  if (fg_color)
		    {
		      *StringBufferPtr << Font_Close;
		      fg_color = NULL;
		    }
		  if (underline)
		    {
		      *StringBufferPtr << '<' << '/' << underline << '>';
		      underline = NULL;
		    }
		  if (font)
		    {
		      *StringBufferPtr << '<' << '/' << font << '>';
		      font = NULL;
		    }
		  if (color)
		    {
		      *StringBufferPtr << Font_Close;
		      color = NULL;
		    }
		}
	    }
	  else if (nextCh == '\b')
	    {
	      if (ch == '_' || ch == '-')
		{
		  // Handle _U_n_d_e_r_l_i_n_e_d
		  // and -o-v-e-r-s-t-r-i-k-e-d text
		  // Note: <S> was <STRIKE> in draft-ietf-html-spec-04.txt 
		  // We adopt the tags from the latest DTDs...
		  // <U> is for underline and <S> for overstrike
		  GDT_BOOLEAN Underlined = (ch == '_');
		  StringBufferPtr->Cat (Underlined ? "<U>" : "<S>");
		  do
		    {
		      idx += 2;
		      ch = (UCHR) Input.GetChr (idx);
		      StringBufferPtr->Cat (ch);
		      ch = (UCHR) Input.GetChr (++idx);
		    }
		  while (ch == '_');
		  StringBufferPtr->Cat (Underlined ? "</U>" : "</S>");
		  idx--;	// Decrement

		}
	      else if (ch == Input.GetChr (idx + 2))
		{
		  // Handle BBoolldd overstrike (Use B for Bold)
		  sbuf.Clear();
		  do
		    {
		      sbuf.Cat (ch);
		      idx += 3;
		      ch = Input.GetChr (idx);
		    }
		  while (Input.GetChr (idx + 1) == '\b' && ch == Input.GetChr (idx + 2));
		  idx--;	// Decrement

		  StringBufferPtr->Cat ("<B>");
		  HtmlCat (sbuf, StringBufferPtr); // Recursion
		  StringBufferPtr->Cat ("</B>");
		}
	      else
		{
		  idx++;	// Rubout (Note multiple Rubouts loose sync...)
		}
	    }
	  else if (ch == '<')
	    StringBufferPtr->Cat ("&lt;");
	  else if (ch == '>')
	    StringBufferPtr->Cat ("&gt;");
	  else if (ch == '&')
	    StringBufferPtr->Cat ("&amp;");
	  else if (ch == '"')
	    StringBufferPtr->Cat ("&quot;");
	  else if (( ib_isascii (ch) &&  ib_isgraph(ch))  ||  ib_isspace (ch))
	    StringBufferPtr->Cat (ch);
	  else
	    {
	      // Non-printable character: Map to UCS
	      sprintf (buf, "&#%d;",  (unsigned)UCS(ch));
	      StringBufferPtr->Cat (buf);
	    }
	}

      // Should not need these but I'm paranoid
      if (add_pos && idx >= add_pos)
	add_pos = MailAnchor(Input, idx);
      if (urn_pos && idx >= urn_pos)
	urn_pos = URNAnchor(Input, idx);
    }
  // Close dangling VT100 Esc sequence
  if (underline) *StringBufferPtr << '<' << '/' << underline << '>';
  if (font) *StringBufferPtr << '<' << '/' << font << '>';
  if (color) *StringBufferPtr << Font_Close;
  if (bg_color) *StringBufferPtr << Font_Close;
  if (fg_color) *StringBufferPtr << Font_Close;
  return *StringBufferPtr;
}


///*****************************************************************************
//*  End Special code                                                          *
//*     (c) Copyright 1992,1993,1994,1995,1996 Basis Systeme netzwerk, Munich  *
//*                        All rights reserved.                                *
//*                                                                            *
//*  Released 2020 in the public domain                                        *
//*                                                                            *
//*****************************************************************************/
