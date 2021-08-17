/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <iostream>

using namespace std;

class Z3950_ERROR {
public:
  Z3950_ERROR();
  ~Z3950_ERROR();

  Z3950_ERROR& operator=(const Z3950_ERROR& OtherError);

  friend ostream& operator <<(ostream& os, const Z3950_ERROR& Error);

  int         SetErrorCode(int Error);
  int         GetErrorCode() const;
  const char *ErrorMessage() const;
  const char *ErrorMessage(int ErrorCode) const;
private:
  int errorCode;
};
