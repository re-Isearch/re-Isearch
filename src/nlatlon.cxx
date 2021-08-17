/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "nlatlon.hxx"

/*
NAME
       ParseLatToNum - convert a string to a latitude

SYNOPSIS
       #include "numlatlon.h"

       float ParseLatToNum(const char *nptr);

DESCRIPTION
       The  ParseLatToNum() function converts  the initial portion of
       the string pointed to by  nptr to float.  North  latitudes are
       returned as positive numbers between 0 and 90, south latitudes
       are returned as negative numbers between -90 and 0.

       Errors are returned as -99.
*/

float ParseLatToNum(char *Term)
{
  int i;
  float sign, value;
  char *accum,tmpnum[2];

  //  if (!(accum=(char*)malloc(strlen(Term)+1)))
  accum= new char [strlen(Term)+1];
  if (!accum)
    return(LatERROR);
  i=0;
  sign=1.0;
  strcpy(accum,"");
  tmpnum[1]='\0';

  while (Term[i] != '\0') {
    if (Term[i] == '-')
      sign = -1.0;
    else if (isdigit(Term[i])) {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if (Term[i] == '.') {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if ((Term[i] == 'N') || (Term[i] == 'n'))
      sign = 1.0;
    else if ((Term[i] == 'S') || (Term[i] == 's'))
      sign = -1.0;
    else
      return(LatERROR);
    i++;
  }
  value = atof(accum);
  //  free(accum);
  delete [] accum;
  if (value > 90.0)
    return(LatERROR);
  value = sign * value;
  return(value);
}

/*
NAME
       ParseLonToNum - convert a string to a latitude

SYNOPSIS
       #include "numlatlon.h"

       float ParseLonToNum(const char *nptr);

DESCRIPTION
       The  ParseLonToNum() function converts  the initial portion of
       the string pointed to by nptr to float.  Longitude is returned
       as a floating point number between 0 and 360, increasing towards
       the east, so western longitudes will be values between 180 and
       360.  The routine will handle either signed numbers or numbers
       suffixed with "E" and "W".  Negative longitudes will be assumed
       to be the same as west longitudes.

       Errors are returned as -999.
*/

float ParseLonToNum(char *Term)
{
  int i;
  float sign, value;
  char *accum,tmpnum[2];

  //  if (!(accum=(char*)malloc(strlen(Term)+1)))
  accum= new char [strlen(Term)+1];
  if (!accum)
    return(LonERROR);
  i=0;
  sign=1.0;
  strcpy(accum,"");
  tmpnum[1]='\0';

  while (Term[i] != '\0') {
    if (Term[i] == '-')
      sign = -1.0;
    else if (isdigit(Term[i])) {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if (Term[i] == '.') {
      tmpnum[0] = Term[i];
      strcat(accum,tmpnum);
    }
    else if ((Term[i] == 'E') || (Term[i] == 'e'))
      sign = 1.0;
    else if ((Term[i] == 'W') || (Term[i] == 'w'))
      sign = -1.0;
    else
      return(LonERROR);
    i++;
  }
  value = atof(accum);
  //  free(accum);
  delete [] accum;
  if (value > 360.0)
    return(LonERROR);
  if (sign < 0.0)
    value = 360.0 - value;
  return(value);
}

