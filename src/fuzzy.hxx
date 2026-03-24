/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef COMMON_HXX
# include "common.hxx"
# include "string.hxx"
#endif

int LevenshteinDistance(const STRING& source, const STRING& target);
int RatcliffCompare(const STRING &s1, const STRING& s2, int scale);

// Like a string compare, default is 75%
inline INT FuzzyCompare(const STRING &s1, const STRING& s2) {
  const int scale = 100; // 0-100 scale
  const int threshold = 75; // 75%
  int match = RatcliffCompare(s1, s2, scale);
  if (match > threshold) {
     match = 100; // Anything over threshold counts as a good match
  }
  return scale - match; // Over threshold returns 0 
}

// Compare string s1 with the first (max) len characters of str
inline INT FuzzyCompare(const STRING &s1, const UCHR *str, const size_t len) {
  return FuzzyCompare(s1, STRING(str,len)); 
}
