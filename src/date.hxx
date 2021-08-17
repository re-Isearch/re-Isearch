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
  UNKNOWN_DATE=0, BAD_DATE, CURRENT_DATE, YEAR_PREC, MONTH_PREC, DAY_PREC, HOUR_PREC, MIN_PREC, SEC_PREC
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
  GDT_BOOLEAN operator ==(const SRCH_DATE& OtherDate) const {
	return Equals(OtherDate);};
  GDT_BOOLEAN operator !=(const SRCH_DATE& OtherDate) const {
	return !Equals(OtherDate);};
  GDT_BOOLEAN operator >(const SRCH_DATE& OtherDate) const {
	return IsAfter(OtherDate);};
  GDT_BOOLEAN operator <(const SRCH_DATE& OtherDate) const {
	return IsBefore(OtherDate);};
  GDT_BOOLEAN operator >=(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN operator <=(const SRCH_DATE& OtherDate) const;
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

  GDT_BOOLEAN IsYearDate() const;
  GDT_BOOLEAN IsMonthDate() const;
  GDT_BOOLEAN IsDayDate() const;
  GDT_BOOLEAN IsBogusDate() const;
  GDT_BOOLEAN IsValidDate() const;
  GDT_BOOLEAN IsLeapYear() const;

  GDT_BOOLEAN Ok() const { return IsValidDate();}

  void        Clear()    { d_date = d_rest = 0;}
  GDT_BOOLEAN IsNotSet() const { return d_date == 0; }

  GDT_BOOLEAN TrimToMonth();
  GDT_BOOLEAN TrimToYear();

  GDT_BOOLEAN SetToYearStart();	// To 1 Jan
  GDT_BOOLEAN SetToYearEnd();	// To 31 Dec
  GDT_BOOLEAN SetToMonthStart();// To 1 XXX
  GDT_BOOLEAN SetToMonthEnd();	// To ? XXX (depends on month)
  GDT_BOOLEAN SetToDayStart();  // To 00:00:00
  GDT_BOOLEAN SetToDayEnd();    // To 23:59:59

  GDT_BOOLEAN PromoteToMonthStart();
  GDT_BOOLEAN PromoteToMonthEnd();
  GDT_BOOLEAN PromoteToDayStart();
  GDT_BOOLEAN PromoteToDayEnd();

  void        GetTodaysDate();
  void        SetNow();

  // Set Year, Month, Day
  GDT_BOOLEAN SetYear(int nYear);   // YYYY
  GDT_BOOLEAN SetMonth(int nMonth); // MM
  GDT_BOOLEAN SetDay(int nDay);     // DD

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

  GDT_BOOLEAN SetTimeOfFile(int fd);
  GDT_BOOLEAN SetTimeOfFile(FILE *fp);
  GDT_BOOLEAN SetTimeOfFile(const STRING& Pathname);

  GDT_BOOLEAN SetTimeOfFileCreation(int fd);
  GDT_BOOLEAN SetTimeOfFileCreation(FILE *fp);
  GDT_BOOLEAN SetTimeOfFileCreation(const STRING& Pathname);

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


  GDT_BOOLEAN Set(const SRCH_DATE& OtherDate);
  GDT_BOOLEAN Set(const DOUBLE DoubleVal);
  GDT_BOOLEAN Set(const int Val) { return Set((long)Val);}
  GDT_BOOLEAN Set(const long LongVal);
  GDT_BOOLEAN Set(const CHR *CStringVal);
  GDT_BOOLEAN Set(const STRING& StringVal);
  GDT_BOOLEAN Set(struct tm *time_str);
  GDT_BOOLEAN Set(const time_t *time = NULL);
  GDT_BOOLEAN Set(FILE *Fp) { return SetTimeOfFile(Fp); }
#ifdef WIN32
  GDT_BOOLEAN Set(const SYSTEMTIME *SystemTime);
#endif
  GDT_BOOLEAN Set(int year, int mon, int day, int hour, int min, int sec);
  GDT_BOOLEAN SetTime(int hour, int min, int sec);
  GDT_BOOLEAN SetTime(DOUBLE Fraction);
  GDT_BOOLEAN SetDate(int year, int mon, int day);
  GDT_BOOLEAN SetDayOfYear(int Day);
  GDT_BOOLEAN SetDayOfYear(int year, int Day);
  GDT_BOOLEAN SetWeekOfYear(int Week, int Day=1);
  GDT_BOOLEAN SetWeekOfYear(int Year, int Week, int Day);
  
  GDT_BOOLEAN IsBefore(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN Equals(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN IsDuring(const SRCH_DATE& OtherDate) const;
  GDT_BOOLEAN IsAfter(const SRCH_DATE& OtherDate) const;

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

  GDT_BOOLEAN getdate(const char *string, struct tm *tm);

  GDT_BOOLEAN Strftime(const char *format, PSTRING StringBuffer) const;
  GDT_BOOLEAN RFC(PSTRING StringBuffer) const;
  GDT_BOOLEAN ISO(PSTRING StringBuffer) const;
  GDT_BOOLEAN ANSI(PSTRING StringBuffer) const;
  GDT_BOOLEAN Locale(PSTRING StringBuffer) const;

  GDT_BOOLEAN Read(PFILE Fp);
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
  GDT_BOOLEAN ParseTm(struct tm *time_str) const;
  GDT_BOOLEAN ParseTm(struct tm *time_str, const SRCH_DATE& time,
	GDT_BOOLEAN Validate = GDT_FALSE) const;
  void        SetPrecision();
  void        SetPrecision(enum Date_Precision prec);
  enum Date_Precision GetPrecision() const;
  void        SetTime(UINT4 Time);
  UINT4       GetTime() const;
  GDT_BOOLEAN date_parse(const char *str);
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

  GDT_BOOLEAN Ok() const;
  GDT_BOOLEAN Defined() const;
  GDT_BOOLEAN Contains(const SRCH_DATE& TestDate) const;
  GDT_BOOLEAN Contains(const DATERANGE& OtherRange) const;

  void        Clear()  { d_start.Clear(); d_end.Clear(); }

  // Comparison operators
  GDT_BOOLEAN operator ==(const DATERANGE& OtherRange) const;
  GDT_BOOLEAN operator !=(const DATERANGE& OtherRange) const;
  GDT_BOOLEAN operator  >(const DATERANGE& OtherRange) const;
  GDT_BOOLEAN operator  <(const DATERANGE& OtherRange) const;
  GDT_BOOLEAN operator >=(const DATERANGE& OtherRange) const;
  GDT_BOOLEAN operator <=(const DATERANGE& OtherRange) const;

  // Formating...
  GDT_BOOLEAN ISO(STRING *String) const;
  GDT_BOOLEAN ISO(STRING *From, STRING *To) const;
  GDT_BOOLEAN RFC(STRING *From, STRING *To) const;
  GDT_BOOLEAN Locale(STRING *From, STRING *To) const;
  GDT_BOOLEAN Strftime(const char *fmt, STRING *From, STRING *To) const;

  // I/O
  GDT_BOOLEAN Read(PFILE Fp);
  void Write(PFILE Fp) const;

  ~DATERANGE();

private:
  SRCH_DATE d_start;
  SRCH_DATE d_end;
};


// Common functions....
inline void Write(const SRCH_DATE& date, FILE *Fp)  { date.Write(Fp);         }
inline GDT_BOOLEAN Read(SRCH_DATE *date, FILE *Fp)  { return date->Read(Fp);  }
inline void Write(const DATERANGE& range, FILE *Fp) { range.Write(Fp);        }
inline GDT_BOOLEAN Read(DATERANGE *range, FILE *Fp) { return range->Read(Fp); }


#endif
