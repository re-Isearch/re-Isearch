/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <ctype.h>

/* The following constants are used to determine the category of a Unicode character.  */
#define UNICODE_CATEGORY_MASK 0X1F

#define REGISTER

enum {
    UNASSIGNED,
    UPPERCASE_LETTER,
    LOWERCASE_LETTER,
    TITLECASE_LETTER,
    MODIFIER_LETTER,
    OTHER_LETTER,
    NON_SPACING_MARK,
    ENCLOSING_MARK,
    COMBINING_SPACING_MARK,
    DECIMAL_DIGIT_NUMBER,
    LETTER_NUMBER,
    OTHER_NUMBER,
    SPACE_SEPARATOR,
    LINE_SEPARATOR,
    PARAGRAPH_SEPARATOR,
    CONTROL,
    FORMAT,
    PRIVATE_USE,
    SURROGATE,
    CONNECTOR_PUNCTUATION,
    DASH_PUNCTUATION,
    OPEN_PUNCTUATION,
    CLOSE_PUNCTUATION,
    INITIAL_QUOTE_PUNCTUATION,
    FINAL_QUOTE_PUNCTUATION,
    OTHER_PUNCTUATION,
    MATH_SYMBOL,
    CURRENCY_SYMBOL,
    MODIFIER_SYMBOL,
    OTHER_SYMBOL
};

/*
 * The following macros are used for fast character category tests.  The
 * x_BITS values are shifted right by the category value to determine whether
 * the given category is included in the set.
 */ 

#define ALPHA_BITS ((1 << UPPERCASE_LETTER) | (1 << LOWERCASE_LETTER) \
    | (1 << TITLECASE_LETTER) | (1 << MODIFIER_LETTER) | (1 << OTHER_LETTER))

#define DIGIT_BITS (1 << DECIMAL_DIGIT_NUMBER)

#define SPACE_BITS ((1 << SPACE_SEPARATOR) | (1 << LINE_SEPARATOR) \
    | (1 << PARAGRAPH_SEPARATOR))

#define CONNECTOR_BITS (1 << CONNECTOR_PUNCTUATION)

#define PRINT_BITS (ALPHA_BITS | DIGIT_BITS | SPACE_BITS | \
       (1 << NON_SPACING_MARK) | (1 << ENCLOSING_MARK) | \
       (1 << COMBINING_SPACING_MARK) | (1 << LETTER_NUMBER) | \
       (1 << OTHER_NUMBER) | (1 << CONNECTOR_PUNCTUATION) | \
       (1 << DASH_PUNCTUATION) | (1 << OPEN_PUNCTUATION) | \
       (1 << CLOSE_PUNCTUATION) | (1 << INITIAL_QUOTE_PUNCTUATION) | \
       (1 << FINAL_QUOTE_PUNCTUATION) | (1 << OTHER_PUNCTUATION) | \
       (1 << MATH_SYMBOL) | (1 << CURRENCY_SYMBOL) | \
       (1 << MODIFIER_SYMBOL) | (1 << OTHER_SYMBOL))

#define PUNCT_BITS ((1 << CONNECTOR_PUNCTUATION) | \
       (1 << DASH_PUNCTUATION) | (1 << OPEN_PUNCTUATION) | \
       (1 << CLOSE_PUNCTUATION) | (1 << INITIAL_QUOTE_PUNCTUATION) | \
       (1 << FINAL_QUOTE_PUNCTUATION) | (1 << OTHER_PUNCTUATION))

/* Unicode characters less than this value are represented by themselves */
#define UNICODE_SELF 0x80


/* A 16-bit Unicode character is split into two parts in order to index
   into the following tables.  The lower OFFSET_BITS comprise an offset
   into a page of characters.  The upper bits comprise the page number. */

#define OFFSET_BITS 5


/* The following macros extract the fields of the character info.  The
   GetDelta() macro is complicated because we can't rely on the C compiler
   to do sign extension on right shifts.  */

#define GetCaseType(info) (((info) & 0xE0) >> OFFSET_BITS)
#define GetCategory(info) ((info) & 0x1F)
#define GetDelta(info) (((info) > 0) ? ((info) >> 22) : (~(~((info)) >> 22)))

/* Extract the information about a character from the Unicode character tables.  */
extern const unsigned char __ib_unicode_pageMap[];
extern const unsigned char __ib_unicode_groupMap[];
extern const int           __ib_unicode_groups[];

#define GetUniCharInfo(ch) ( __ib_unicode_groups[ __ib_unicode_groupMap[( __ib_unicode_pageMap[(((int)(ch)) & 0xffff) >> OFFSET_BITS] << OFFSET_BITS) | ((ch) & ((1 << OFFSET_BITS)-1))]])


// Functions......

inline int _ib_UniCharToLower(int ch)
{
  const int info = GetUniCharInfo(ch);
  return GetCaseType(info) & 0x02 ? ch + GetDelta(info) : ch;
}

inline int _ib_UniCharToUpper(int ch)
{
  const int info = GetUniCharInfo(ch);
  return GetCaseType(info) & 0x04 ? ch - GetDelta(info) : ch;
}


// Ctype style things...

inline int _ib_UniCharIsAlnum(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return (((ALPHA_BITS | DIGIT_BITS) >> category) & 1);
}

inline int _ib_UniCharIsAlpha(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return ((ALPHA_BITS >> category) & 1);
}

inline int _ib_UniCharIsLower(int ch)
{
  return ((GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK) == LOWERCASE_LETTER);
}

inline int _ib_UniCharIsUpper(int ch)
{
  return ((GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK) == UPPERCASE_LETTER);
}

inline int _ib_UniCharIsControl(int ch)
{
  return ((GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK) == CONTROL);
}

inline int _ib_UniCharIsDigit(int ch)
{
  return ((GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK) == DECIMAL_DIGIT_NUMBER);
}

inline int _ib_UniCharIsGraph(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return (((PRINT_BITS >> category) & 1) && ((unsigned char) ch != ' '));
}


inline int _ib_UniCharIsPrint(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return ((PRINT_BITS >> category) & 1);
}


inline int _ib_UniCharIsPunct(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return ((PUNCT_BITS >> category) & 1);
}

inline int _ib_UniCharIsSpace(int ch)
{
  if (ch < UNICODE_SELF)
   return isspace((unsigned char)ch); /* INTL: ISO space */

  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return ((SPACE_BITS >> category) & 1);
}


/*
 *----------------------------------------------------------------------
 *
 * Test if a character is alphanumeric or a connector punctuation
 * mark.
 *
 * Results:
 * Returns 1 if character is a word character.
 *
 * Side effects:
 * None.
 *
 *----------------------------------------------------------------------
 */

inline int _ib_UniCharIsWordChar(int ch)
{
  REGISTER int category = (GetUniCharInfo(ch) & UNICODE_CATEGORY_MASK);
  return (((ALPHA_BITS | DIGIT_BITS | CONNECTOR_BITS) >> category) & 1);
}

