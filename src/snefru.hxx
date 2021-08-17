/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
STRING SnefruHash(const STRING& Input);
STRING SnefruHash(const char *CString);
STRING SnefruHash(const char *ptr, size_t length);

STRING OneWayHash(const STRING& Input);
STRING OneWayHash(const char *CString);
STRING OneWayHash(const char *ptr, size_t length);


