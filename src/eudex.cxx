/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "eudex.hxx"

//
// Based on
// Eudex: A blazingly fast phonetic reduction/hashing algorithm.
//
// Eudex ([juːˈdɛks]) is a Soundex-esque phonetic reduction/hashing algorithm,
// providing locality sensitive "hashes" of words, based on the spelling and pronunciation.



static const uint64_t PHONES[] = {
    0b00000000L, // a
    0b01001000L, // b
    0b00001100L, // c
    0b00011000L, // d
    0b00000000L, // e
    0b01000100L, // f
    0b00001000L, // g
    0b00000100L, // h
    0b00000001L, // i
    0b00000101L, // j
    0b00001001L, // k
    0b10100000L, // l
    0b00000010L, // m
    0b00010010L, // n
    0b00000000L, // o
    0b01001001L, // p
    0b10101000L, // q
    0b10100001L, // r
    0b00010100L, // s
    0b00011101L, // t
    0b00000001L, // u
    0b01000101L, // v
    0b00000000L, // w
    0b10000100L, // x
    0b00000001L, // y
    0b10010100L  // z
    };

static const uint64_t PHONES_C1[] = {
    0b00010101L, // ß
    0b00000000L, // à
    0b00000000L, // á
    0b00000000L, // â
    0b00000000L, // ã
    0b00000000L, // ä [æ]
    0b00000001L, // å [oː]
    0b00000000L, // æ [æ]
    0b10010101L, // ç [t͡ʃ]
    0b00000001L, // è
    0b00000001L, // é
    0b00000001L, // ê
    0b00000001L, // ë
    0b00000001L, // ì
    0b00000001L, // í
    0b00000001L, // î
    0b00000001L, // ï
    0b00010101L, // ð [ð̠] (represented as a non-plosive T)
    0b00010111L, // ñ [nj] (represented as a combination of n and j)
    0b00000000L, // ò
    0b00000000L, // ó
    0b00000000L, // ô
    0b00000000L, // õ
    0b00000001L, // ö [ø]
    0b11111111L, // ÷
    0b00000001L, // ø [ø]
    0b00000001L, // ù
    0b00000001L, // ú
    0b00000001L, // û
    0b00000001L, // ü
    0b00000001L, // ý
    0b00010101L, // þ [ð̠] (represented as a non-plosive T)
    0b00000001L, // ÿ
    };

static const uint64_t INJECTIVE_PHONES[] = {
    0b10000100L, // a*
    0b00100100L, // b
    0b00000110L, // c
    0b00001100L, // d
    0b11011000L, // e*
    0b00100010L, // f
    0b00000100L, // g
    0b00000010L, // h
    0b11111000L, // i*
    0b00000011L, // j
    0b00000101L, // k
    0b01010000L, // l
    0b00000001L, // m
    0b00001001L, // n
    0b10010100L, // o*
    0b00100101L, // p
    0b01010100L, // q
    0b01010001L, // r
    0b00001010L, // s
    0b00001110L, // t
    0b11100000L, // u*
    0b00100011L, // v
    0b00000000L, // w
    0b01000010L, // x
    0b11100100L, // y*
    0b01001010L // z
    };

static const uint64_t INJECTIVE_PHONES_C1[] = {
    0b00001011L, // ß
    0b10000101L, // à
    0b10000101L, // á
    0b10000000L, // â
    0b10000110L, // ã
    0b10100110L, // ä [æ]
    0b11000010L, // å [oː]
    0b10100111L, // æ [æ]
    0b01010100L, // ç [t͡ʃ]
    0b11011001L, // è
    0b11011001L, // é
    0b11011001L, // ê
    0b11000110L, // ë [ə] or [œ]
    0b11111001L, // ì
    0b11111001L, // í
    0b11111001L, // î
    0b11111001L, // ï
    0b00001011L, // ð [ð̠] (represented as a non-plosive T)
    0b00001011L, // ñ [nj] (represented as a combination of n and j)
    0b10010101L, // ò
    0b10010101L, // ó
    0b10010101L, // ô
    0b10010101L, // õ
    0b11011100L, // ö [œ] or [ø]
    0b11111111L, // ÷
    0b11011101L, // ø [œ] or [ø]
    0b11100001L, // ù
    0b11100001L, // ú
    0b11100001L, // û
    0b11100101L, // ü
    0b11100101L, // ý
    0b00001011L, // þ [ð̠] (represented as a non-plosive T)
    0b11100101L // ÿ
    };

static const int num_letters = (int)(sizeof(PHONES)/sizeof(uint64_t));
static const wchar_t SKIP = L'0';

static uint64_t first_phonetic(wchar_t letter);
static uint64_t next_phonetic(eudex_t prev, wchar_t  letter, wchar_t *result);


/* main functions */
eudex_t EUDEX::hash(const wchar_t * input) {
    uint64_t first_byte = *input != '\0' ? first_phonetic(*input) : 0;
    ++input; // advance to next character

    char n = 1; // limit to first 8 bytes of string (same as in rust impl)
    eudex_t res = 0L;
    while (1) {
        if ((n == 0) || (*input == L'\0')) {
            break;
        }

        wchar_t r = L'1';
        uint64_t phonetic = next_phonetic(res, *input, &r);
        if (r != SKIP) {
            res <<= 8;
            res |= phonetic;
            n <<= 1;
        }
        ++input;
    }
    return res | (first_byte << 56L);
}

/* internal implementation */

static inline uint64_t popcount64(eudex_t x) {

#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcount(x);
#else
    const uint64_t m1 = 0x5555555555555555;
    const uint64_t m2 = 0x3333333333333333;
    const uint64_t m4 = 0x0f0f0f0f0f0f0f0f;
    const uint64_t h01 = 0x0101010101010101;
    x -= (x >> 1) & m1;
    x = (x & m2) + ((x >> 2) & m2);
    x = (x + (x >> 4)) & m4;
    return (x * h01) >> 56;
#endif

}

static inline int get_letter_index(wchar_t letter) { return ((letter | 32) - 'a') & 0xFF; }

static uint64_t next_phonetic(eudex_t prev, wchar_t  letter, wchar_t *result) {
    int index = get_letter_index(letter);
    uint64_t c = 0L;
    if (index < num_letters) {
        c = PHONES[index];
    } else if (index >= 0xDF && index < 0xFF) {
        c = PHONES_C1[index - 0xDF];
    } else {
        *result = SKIP;
    }

    if ((c & 1) != (prev & 1)) {
        return c;
    }
    *result = SKIP;
    return c;
}

static uint64_t first_phonetic(wchar_t letter) {
    int index;

    // Letter must be in Latin-1 space
    if (letter <= 0xFF) {
      if ((index = get_letter_index(letter)) < num_letters)
        return INJECTIVE_PHONES[index];
      if (index >= 0xDF && index < 0xFF)
        return INJECTIVE_PHONES_C1[index - 0xDF];
    }
    return 0;
}

int64_t EUDEX::distance(eudex_t a, eudex_t b) const {
    eudex_t dist = a ^ b;
    int     sign = a > b ? 1: -1 ;
    return sign * (popcount64(dist & 0xFF) + popcount64((dist >> 8) & 0xFF) * 2
        + popcount64((dist >> 16) & 0xFF) * 3
        + popcount64((dist >> 24) & 0xFF) * 5
        + popcount64((dist >> 32) & 0xFF) * 8
        + popcount64((dist >> 40) & 0xFF) * 13
        + popcount64((dist >> 48) & 0xFF) * 21
        + popcount64((dist >> 56) & 0xFF) * 34 );
}


EUDEX::EUDEX(NUMBER number)
{
//   if (sizeof(NUMBER) >= sizeof(eudex_t))
//     memcpy(&eudex, &number, sizeof(NUMBER));
//   else
     eudex = (eudex_t)number;
}

NUMBER EUDEX::conv_to_number() const
{
   NUMBER value = eudex;
 
   EUDEX other (value);
   if (other.eudex != eudex) std::cerr << "Conversion error " <<  eudex << "  to  " << other.eudex <<  "   // " << value << std::endl;
   return value;
}


#ifdef EBUG

#include <codecvt>
#include <locale>

int main(int argc, char **argv)
{
  const wchar_t *s1 =  L"Hans Stras \x388";
  const wchar_t *s2 = L"Hands Stra\xdf e";

//  if (argc > 1) s1 = argv[1];
//  if (argc > 2) s2 = argv[2];
  EUDEX  e1 (s1);
  EUDEX  e2 (s2);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
    std::wstring str1 = s1;
    std::wstring str2 = s2;

    std::cout << utf8_conv.to_bytes(str1) << '\n';
    std::cout << utf8_conv.to_bytes(str2) << '\n';


  std::cout << " --> " << e1 << " x " << e2 << " --> " << (e1 - e2) << std::endl;

  NUMBER n1  = e1;
  NUMBER n2  = e2;

  auto h1 = e1.value();
  auto h2 = e2.value();

  // Just to make sure that out internal conversion to NUMBER does not break the
  // the eudex difference.. Needs to remain stable

  e1 = n1;
  e2 = n2;

  std::cerr << "NUMBER " << n1  << "  - " << n2 <<  "  = " << (n1 - n2) <<  "   versus " 
	<< e1 << " x " << e2 << " diff " << (e1-e2) << std::endl;

  if (h1 != e1.value() || h2 != e2.value())
    std::cerr << "!! LOST PRECISION !!" << std::endl;

  return 0;

}



#endif
