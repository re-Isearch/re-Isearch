#ifndef _BOOST_H
# define _BOOST_H 1 

extern "C" {
 extern double  log(double);
 extern double  exp(double);
 extern double  pow(double, double);
};

class BOOST {
friend class DATE_BOOST;
public:
  enum Progression { Linear, Inversion, LogLinear, LogInversion, Exponential,
	ExpInversion, Power, PowInversion} Progression;
  BOOST() {
    Factor = 0;
    Weight = 1;
    Scalar = 1;
    Progression = Linear;
  }
  BOOST& operator = (const BOOST& Other) {
    Factor      = Other.Factor;
    Weight      = Other.Weight;
    Scalar      = Other.Scalar;
    Progression = Other.Progression;
    return *this;
  }
  void   SetWeight(const DOUBLE x) { Weight = x;    }
  DOUBLE GetWeight() const         { return Weight; }
  void   SetFactor(const DOUBLE x) { Factor = x;    }
  DOUBLE GetFactor() const         { return Factor; }
  void   SetScalar(const DOUBLE x) { Scalar = x;    }
  DOUBLE GetScalar() const         { return Scalar; }
  DOUBLE Score(const DOUBLE score, const DOUBLE y) const {
    if (y+Factor == 0) switch(Progression) {
      default:
      case Linear:       return Weight*score + Scalar*(y+Factor);
      case Inversion:    return Weight*score + Scalar/(y+Factor);
      case LogLinear:    return Weight*score + Scalar*log(y+Factor);
      case LogInversion: return Weight*score + Scalar/log(y+Factor);
      case Exponential:  return Weight*score + Scalar*exp(y+Factor);
      case ExpInversion: return Weight*score + Scalar/exp(y+Factor);
      case Power:        return Weight*score + Scalar*pow(y, Factor);
      case PowInversion: return Weight*score + Scalar/pow(y, Factor);
    }
    return Weight*score;
  }
private:
  DOUBLE Weight;
  DOUBLE Factor;
  DOUBLE Scalar;
//  Progression Progression;
};


class DATE_BOOST : public BOOST {
public:
  enum Resolution { Minutes, Hours, Days };
  DATE_BOOST() {
    Resolution = Minutes;
    BaseDateLine.SetNow();
  }
  DATE_BOOST(const BOOST& x) : BOOST(x) {
    BaseDateLine.SetNow();
  }

  DATE_BOOST(const SRCH_DATE& Date) {
    if (SetBaseDateLine(Date))
      {
	enum Date_Precision r = BaseDateLine.GetPrecision();
	if      (r == HOUR_PREC) Resolution = Hours;
	else if (r >= MIN_PREC)  Resolution = Minutes;
        else                     Resolution = Days;
      }
    else
      BaseDateLine.SetNow();
    
  }
  DATE_BOOST& operator = (const DATE_BOOST& Other) {
    Factor      = Other.Factor;
    Weight      = Other.Weight;
    Scalar      = Other.Scalar;
    BaseDateLine= Other.BaseDateLine;
    Progression = Other.Progression;
    return *this;
  }
  GDT_BOOLEAN  SetBaseDateLine(const SRCH_DATE& Date) {
    if (Date.Ok()) return (BaseDateLine = Date).Ok();
    return GDT_FALSE;
  }
  SRCH_DATE GetBaseDateLine() const {
    return BaseDateLine;
  }
  void      SetResolution(enum Resolution x) {
    Resolution = x;
  }
  enum Resolution GetResolution() const {
    return Resolution;
  }
  DOUBLE     Score(const DOUBLE score, const SRCH_DATE& Date) const {
    switch (Resolution) {
      default:
      case Minutes: return BOOST::Score(score, BaseDateLine.MinutesDiff(Date));
      case Hours:   return BOOST::Score(score, (BaseDateLine.MinutesDiff(Date))/60.0);
      case Days:    return BOOST::Score(score, BaseDateLine.DaysDifference(Date));
    }
  }
private:
  SRCH_DATE       BaseDateLine;
  enum Resolution Resolution;
};

class RANKING_BOOST {
public:
  RANKING_BOOST() {
    PriorityFactor  = 0;
  }
  RANKING_BOOST& operator =(const RANKING_BOOST& Other) {
    PriorityFactor  = Other.PriorityFactor;
    Index           = Other.Index;
    Freshness       = Other.Freshness;
    Longevity       = Other.Longevity;
    return *this;
  }
  void        SetPriority(DOUBLE x)             { PriorityFactor = x;    }
  DOUBLE      GetPriority() const               { return PriorityFactor; }
  void        SetIndex(const BOOST& x)          { Index = x;             }
  BOOST       GetIndex() const                  { return Index;          }
  void        SetFreshness(const DATE_BOOST& x) { Freshness = x;         }
  DATE_BOOST  GetFreshness() const              { return Freshness;      }
  void        SetLongevity(const BOOST& x)      { Longevity = x;         }
  BOOST       GetLongevity() const              { return Longevity;      }
private:
  DOUBLE      PriorityFactor;
  BOOST       Index;
  BOOST       Longevity;
  DATE_BOOST  Freshness;
};

#endif
