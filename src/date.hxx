/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		dates.hxx
Description:	Class DATE - BSN Date data structures class
@@@*/

#ifndef DATE_HXX
#define DATE_HXX

// DATE Routines
enum Date_Precision {
  UNKNOWN_DATE=0, BAD_DATE, CURRENT_DATE, MILENIUM_PREC, CENTURY_PREC, YEAR_PREC, MONTH_PREC, DAY_PREC, HOUR_PREC, MIN_PREC, SEC_PREC
};
enum Date_Match {
  MATCH_ERROR=-1, BEFORE, BEFORE_DURING, DURING_EQUALS, DURING_AFTER, AFTER
};


class SRCH_DATE {
  friend class INDEX;
  friend class DATERANGE;
  friend class MDTREC;
  friend class DATE_BOOST;
public:
  SRCH_DATE();
  SRCH_DATE(const SRCH_DATE& OtherDate);
  SRCH_DATE(const time_t *Time);
  SRCH_DATE(struct tm *time_str);
  SRCH_DATE(const DOUBLE FloatVal);
  SRCH_DATE(const STRING& String);
  SRCH_DATE(const CHR* CString);
  SRCH_DATE(const long LongVal);
  SRCH_DATE(const int IntVal);
  SRCH_DATE(FILE *Fp);

  operator       DOUBLE () const;
  operator       long () const;
  operator       STRING () const;

  SRCH_DATE&     operator  =(const SRCH_DATE& OtherDate);

  // Arithmetic
  SRCH_DATE&     operator +=(const SRCH_DATE& OtherDate);
  SRCH_DATE&     operator +=(INT Add);
  SRCH_DATE&     operator +=(DOUBLE Add);
  SRCH_DATE&     operator -=(const SRCH_DATE& OtherDate);
  SRCH_DATE&     operator -=(INT Minus);
  SRCH_DATE&     operator -=(DOUBLE Minus);
  SRCH_DATE&     operator ++() {return PlusNdays(1); }
  SRCH_DATE&     operator --() {return MinusNdays(1); }

  // Comparison operators
#if 0
  bool operator ==(const SRCH_DATE& OtherDate) const {
	return Equals(OtherDate);};
  bool operator !=(const SRCH_DATE& OtherDate) const {
	return !Equals(OtherDate);};
  bool operator >(const SRCH_DATE& OtherDate) const {
	return IsAfter(OtherDate);};
  bool operator <(const SRCH_DATE& OtherDate) const {
	return IsBefore(OtherDate);};
  bool operator >=(const SRCH_DATE& OtherDate) const;
  bool operator <=(const SRCH_DATE& OtherDate) const;
#endif
  friend int operator <  (const SRCH_DATE &dt1, const SRCH_DATE &dt2);
  friend int operator <= (const SRCH_DATE &dt1, const SRCH_DATE &dt2);
  friend int operator >  (const SRCH_DATE &dt1, const SRCH_DATE &dt2);
  friend int operator >= (const SRCH_DATE &dt1, const SRCH_DATE &dt2);
  friend int operator == (const SRCH_DATE &dt1, const SRCH_DATE &dt2);
  friend int operator != (const SRCH_DATE &dt1, const SRCH_DATE &dt2);

  // Add/Subract dates..
  SRCH_DATE& Plus(const SRCH_DATE& OtherDate);
  SRCH_DATE& Minus(const SRCH_DATE& OtherDate);
  // Add N seconds, minutes, hours, days, weeks, months or years..
  SRCH_DATE& PlusNseconds(int seconds);
  SRCH_DATE& PlusNminutes(int minutes);
  SRCH_DATE& PlusNhours(int hours);
  SRCH_DATE& PlusNdays(int days);
  SRCH_DATE& PlusNweeks(int weeks)      {return PlusNdays(weeks*7); };
  SRCH_DATE& PlusNmonths(int months);
  SRCH_DATE& PlusNyears(int years);
  // Substract..
  SRCH_DATE& MinusNseconds(int seconds) {return PlusNseconds(-seconds);};
  SRCH_DATE& MinusNminutes(int minutes) {return PlusNminutes(-minutes);};
  SRCH_DATE& MinusNhours(int hours)     {return PlusNhours(-hours);};
  SRCH_DATE& MinusNdays(int days)       {return PlusNdays(-days);};
  SRCH_DATE& MinusNweeks(int weeks)     {return PlusNweeks(-weeks); };
  SRCH_DATE& MinusNmonths(int months)   {return PlusNmonths(-months);};
  SRCH_DATE& MinusNyears(int years)     {return PlusNyears(-years);};
  // Some cases
  SRCH_DATE& Tommorrow()                { return PlusNdays(1); };
  SRCH_DATE& Yesterday()                { return MinusNdays(1); };

  SRCH_DATE& NextWeek()                 { return PlusNweeks(1); };
  SRCH_DATE& LastWeek()                 { return MinusNweeks(1); };

  SRCH_DATE& NextMonth()                { return PlusNmonths(1); };
  SRCH_DATE& LastMonth()                { return MinusNmonths(1); };

  SRCH_DATE& NextYear()                 { return PlusNyears(1); };
  SRCH_DATE& LastYear()                 { return MinusNyears(1); };

  DOUBLE         GetValue() const;
  DOUBLE         GetWholeDayValue() const; // to day 

  bool IsYearDate() const;
  bool IsMonthDate() const;
  bool IsDayDate() const;
  bool IsBogusDate() const;
  bool IsValidDate() const;
  bool IsLeapYear() const;

  bool Ok() const { return IsValidDate();}

  void        Clear()    { d_date = d_rest = 0;}
  bool IsNotSet() const { return d_date == 0; }

  bool TrimToMonth();
  bool TrimToYear();

  bool SetToYearStart();	// To 1 Jan
  bool SetToYearEnd();	// To 31 Dec
  bool SetToMonthStart();// To 1 XXX
  bool SetToMonthEnd();	// To ? XXX (depends on month)
  bool SetToDayStart();  // To 00:00:00
  bool SetToDayEnd();    // To 23:59:59

  bool PromoteToMonthStart();
  bool PromoteToMonthEnd();
  bool PromoteToDayStart();
  bool PromoteToDayEnd();

  void        GetTodaysDate();
  void        SetNow();

  // Set Year, Month, Day
  bool SetYear(int nYear);   // YYYY
  bool SetMonth(int nMonth); // MM
  bool SetDay(int nDay);     // DD

  // Get Year, Month, Day
  int         Year() const;  // Year of date YYYY
  int         Month() const; // Month of date MM (1-12)
  int         Day() const;   // Day of date YY (1-31)

  int         DayOfWeek() const; // 1 = Sunday, ... 7 = Saturday
  int         DayOfYear() const;
  int         GetFirstDayOfMonth() const; // 1 = Sunday, ... 7 = Saturday

  int         GetWeekOfMonth() const;
  int         WeekOfYear() const;

  int         GetDaysInMonth() const;
  long        GetJulianDate() const; // days since 1/1/4713 B.C.
  long        GetTimeSeconds() const; // Seconds of Time since Midnight

  bool SetTimeOfFile(int fd);
  bool SetTimeOfFile(FILE *fp);
  bool SetTimeOfFile(const STRING& Pathname);

  bool SetTimeOfFileCreation(int fd);
  bool SetTimeOfFileCreation(FILE *fp);
  bool SetTimeOfFileCreation(const STRING& Pathname);

  SRCH_DATE& GetTimeOfFile(int fd)
     { SetTimeOfFile(fd); return *this;       };
  SRCH_DATE& GetTimeOfFile(FILE *fp)
     { SetTimeOfFile(fp); return *this;       };
  SRCH_DATE& GetTimeOfFile(const STRING& Pathname)
     { SetTimeOfFile(Pathname); return *this; };

  SRCH_DATE& GetTimeOfFileCreation(int fd)
     { SetTimeOfFileCreation(fd); return *this;       };
  SRCH_DATE& GetTimeOfFileCreation(FILE *fp)
     { SetTimeOfFileCreation(fp); return *this;       };
  SRCH_DATE& GetTimeOfFileCreation(const STRING& Pathname)
     { SetTimeOfFileCreation(Pathname); return *this; };


  bool Set(const SRCH_DATE& OtherDate);
  bool Set(const DOUBLE DoubleVal);
  bool Set(const int Val) { return Set((long)Val);}
  bool Set(const long LongVal);
  bool Set(const CHR *CStringVal);
  bool Set(const STRING& StringVal);
  bool Set(struct tm *time_str);
  bool Set(const time_t *time = NULL);
  bool Set(FILE *Fp) { return SetTimeOfFile(Fp); }
#ifdef WIN32
  bool Set(const SYSTEMTIME *SystemTime);
#endif
  bool Set(int year, int mon, int day, int hour, int min, int sec);
  bool SetTime(int hour, int min, int sec);
  bool SetTime(DOUBLE Fraction);
  bool SetDate(int year, int mon, int day);
  bool SetDayOfYear(int Day);
  bool SetDayOfYear(int year, int Day);
  bool SetWeekOfYear(int Week, int Day=1);
  bool SetWeekOfYear(int Year, int Week, int Day);
  
  bool IsBefore(const SRCH_DATE& OtherDate) const;
  bool Equals(const SRCH_DATE& OtherDate) const;
  bool IsDuring(const SRCH_DATE& OtherDate) const;
  bool IsAfter(const SRCH_DATE& OtherDate) const;

  time_t MkTime() const;
  time_t MkTime(const SRCH_DATE& Date) const;
  time_t MkTime(struct tm *time_str) const;
  time_t MkTime(struct tm *time_str, const SRCH_DATE& Date) const;

  // Human Readable Copy
  STRING ISOdate() const;
  STRING RFCdate() const; 
  STRING ANSIdate() const;
  STRING LCdate() const; 
  char *ISOdate(char *buf, size_t maxsize=25) const; // if null will allocate
  char *RFCdate(char *buf, size_t maxsize=40) const; // if null will allocate
  char *ANSIdate(char *buf, size_t maxsize=27) const; // if null will allocate
  char *LCdate(char *buf, size_t maxsize=60) const;

  friend ostream &operator << (ostream &os, const SRCH_DATE &dt); // ISOdate
  friend STRING  &operator << (STRING &String, const SRCH_DATE &dt); // ISOdate

  bool getdate(const char *string, struct tm *tm);

  bool Strftime(const char *format, PSTRING StringBuffer) const;
  bool RFC(PSTRING StringBuffer) const;
  bool ISO(PSTRING StringBuffer) const;
  bool ANSI(PSTRING StringBuffer) const;
  bool Locale(PSTRING StringBuffer) const;

  bool Read(PFILE Fp);
  void Write(PFILE Fp) const;

  ~SRCH_DATE();

  long        MinutesDiff(SRCH_DATE Other) const;
  long        DaysDifference(const SRCH_DATE& Other) const;
  long        DaysAgo() const;
  long        HoursAgo() const;
  long        MinutesAgo() const { return -MinutesDiff("Now"); }

  long        DaysAgoHours(int Days = 0) const; // Days ago in hours, starting at 'Days' days measures only days

  friend INT Compare(const SRCH_DATE& Date1, const SRCH_DATE& Date2);
private:
  // Methods
  bool ParseTm(struct tm *time_str) const;
  bool ParseTm(struct tm *time_str, const SRCH_DATE& time,
	bool Validate = false) const;
  void        SetPrecision();
  void        SetPrecision(enum Date_Precision prec);
  enum Date_Precision GetPrecision() const;
  void        SetTime(UINT4 Time);
  UINT4       GetTime() const;
  bool date_parse(const char *str);
  Date_Match  DateCompare(const SRCH_DATE& OtherDate) const;

  // Data
  UINT4             d_date; // YYYY, YYYYMM or YYYYMMDD
  UINT4             d_rest; // HHMMSS, in FFFFF We store the d_prec in the upper FFF
			    // d_prec = (d_rest & 0xFFF00000) >> 20;
			    // d_time = (d_rest & 0x000FFFFF);
};

class DATERANGE
{
public:
  DATERANGE();
  DATERANGE(const SRCH_DATE& NewStart, const SRCH_DATE& NewEnd);
  DATERANGE(const SRCH_DATE& NewDate);
  DATERANGE(time_t *NewStart, time_t *NewEnd);
  DATERANGE(const STRING& DateString);
  DATERANGE(const CHR* DateString);

  // Arithmetic
  DATERANGE& operator  =(const DATERANGE& OtherRange);
  DATERANGE& operator +=(const INT Days);
  DATERANGE& operator +=(const DOUBLE Offset);
  DATERANGE& operator +=(const DATERANGE& DateRange);
  DATERANGE& operator -=(const INT Days);
  DATERANGE& operator -=(const DOUBLE Offset);
  DATERANGE& operator -=(const DATERANGE& DateRange);
  DATERANGE& operator ++() {return *this += 1; }
  DATERANGE& operator --() {return *this -= 1; }

  // typecasts
  operator    STRING () const;

  friend ostream &operator << (ostream &os, const DATERANGE &dt); // ISOdate
  friend STRING  &operator << (STRING &String, const DATERANGE &dt); // ISOdate

  SRCH_DATE  GetStart() const { return d_start; }
  SRCH_DATE  GetEnd()   const { return d_end; }

  void  SetStart(const SRCH_DATE& NewStart) { d_start = NewStart; }
  void  SetEnd(const SRCH_DATE& NewEnd)  { d_end = NewEnd; }

  bool Ok() const;
  bool Defined() const;
  bool Contains(const SRCH_DATE& TestDate) const;
  bool Contains(const DATERANGE& OtherRange) const;

  void        Clear()  { d_start.Clear(); d_end.Clear(); }

  // Comparison operators
  bool operator ==(const DATERANGE& OtherRange) const;
  bool operator !=(const DATERANGE& OtherRange) const;
  bool operator  >(const DATERANGE& OtherRange) const;
  bool operator  <(const DATERANGE& OtherRange) const;
  bool operator >=(const DATERANGE& OtherRange) const;
  bool operator <=(const DATERANGE& OtherRange) const;

  // Formating...
  bool ISO(STRING *String) const;
  bool ISO(STRING *From, STRING *To) const;
  bool RFC(STRING *From, STRING *To) const;
  bool Locale(STRING *From, STRING *To) const;
  bool Strftime(const char *fmt, STRING *From, STRING *To) const;

  // I/O
  bool Read(PFILE Fp);
  void Write(PFILE Fp) const;

  ~DATERANGE();

private:
  SRCH_DATE d_start;
  SRCH_DATE d_end;
};


// Common functions....
inline void Write(const SRCH_DATE& date, FILE *Fp)  { date.Write(Fp);         }
inline bool Read(SRCH_DATE *date, FILE *Fp)  { return date->Read(Fp);  }
inline void Write(const DATERANGE& range, FILE *Fp) { range.Write(Fp);        }
inline bool Read(DATERANGE *range, FILE *Fp) { return range->Read(Fp); }


#endif
