#include <iostream>
#include <math.h>
#include <time.h>

using namespace std;


//STANDARD CONSTANTS
double pi = 3.1415926535;   // Pi
double solarConst = 1367;           // solar constant W.m-2


// Function to convert radian to hours
double RadToHours (double tmp)
{
    //double pi = 3.1415926535; // Pi
    return (tmp * 12 / pi);
}
// Function to convert hours to radians
double HoursToRads (double tmp)
{
    //double pi = 3.1415926535; // Pi
    return (tmp * pi / 12);
}




// Function to calculate the angle of the day
double AngleOfDay (int day,     // number of the day
                   int month,   // number of the month
                   int year // year
                 )

{   // local vars
    int i, leap;
    int numOfDays = 0;                                              // number of Day 13 Nov=317
    int numOfDaysofMonths[12] = {0,31,28,31,30,31,30,31,31,30,31,30};   // Number of days per month
    int AllYearDays;                                                // Total number of days in a year 365 or 366
    double DayAngle;                                                // angle of the day (radian)
    //double pi = 3.1415926535; // Pi

    // leap year ??
    leap = 0;
    if ((year % 400)==0)
    {   AllYearDays = 366;
        leap = 1;
    }
    else if ((year % 100)==0) AllYearDays = 365;
         else if ((year % 4)==0)
            {   AllYearDays = 366;
                leap = 1;
            }
             else AllYearDays = 365;

    // calculate number of day
    for (i=0;i<month;i++) numOfDays += numOfDaysofMonths[i];
    if ( (month > 2) && leap) numOfDays++;
    numOfDays += day;

    // calculate angle of day
    DayAngle = (2*pi*(numOfDays-1)) / AllYearDays;
    return DayAngle;

}


// Function to calculate declination - in radian
double Declination (double DayAngle     // angle day in radian
                    )
{
    double SolarDeclination;
    // Solar declination (radian)
    SolarDeclination = 0.006918
            - 0.399912 * cos (DayAngle)
            + 0.070257 * sin (DayAngle)
            - 0.006758 * cos (2*DayAngle)
            + 0.000907 * sin (2*DayAngle)
            - 0.002697 * cos (3*DayAngle)
            + 0.00148 * sin (3*DayAngle);
    return SolarDeclination;
}



// Function to calculate Equation of time ( et = TSV - TU )
double EqOfTime (double DayAngle        // angle day (radian)
                      )
{
    double et;
    // Equation of time (radian)
    et = 0.000075
         + 0.001868 * cos (DayAngle)
         - 0.032077 * sin (DayAngle)
         - 0.014615 * cos (2*DayAngle)
         - 0.04089 * sin (2*DayAngle);
    // Equation of time in hours
    et = RadToHours(et);

    return et;
}

// Calculation of the duration of the day in radian
double DayDurationRadian (double _declination,      // _declination in radian
                          double lat                // latitude in radian
                         )
{
    double dayDurationj;

    dayDurationj = 2 * acos( -tan(lat) * tan(_declination) );
    return dayDurationj;
}

// Function to calculate Day duration in Hours
double DayDuratInHours (double _declination     // _declination in radian
                  , double lat              // latitude in radian
                  )
{
    double dayDurationj;

    dayDurationj = DayDurationRadian(_declination, lat);
    dayDurationj = RadToHours(dayDurationj);
    return dayDurationj;
}


// Function to calculate the times TSV-UTC
double Tsv_Tu (double rlong             // longitude en radian positive a l est.
               ,double eqOfTime         // Equation of times en heure
              )
{
    double diffUTC_TSV; // double pi = 3.1415926535;   // Pi

    // diffUTC_TSV Solar time as a function of longitude and the eqation of time
    diffUTC_TSV = rlong * (12 / pi) + eqOfTime;

    // difference with local time
    return diffUTC_TSV;
}


// Calculations of the orbital excentricity
double Excentricity(int day,
                    int month,
                    int year)
{

    double dayAngleRad, E0;

    // calculate the angle of day in radian
    dayAngleRad = AngleOfDay(day, month, year);

    // calculate the excentricity
    E0 = 1.000110 + 0.034221 * cos(dayAngleRad)
            + 0.001280 * sin(dayAngleRad)
            +0.000719 * cos(2*dayAngleRad)
            +0.000077 * sin(2*dayAngleRad);

    return E0;
}

// Calculate the theoretical energy flux for the day radiation
double TheoreticRadiation(int day, int month, int year,
                            double lat          // Latitude in radian !
                            )
{
    double RGth;        // Theoretical radiation
    double decli;       // Declination
    double E0;
    double sunriseHourAngle;            // Hour angle of sunset



    // Calculation of the declination in radian
    decli = Declination (AngleOfDay(day, month, year));

    // Calcuate excentricity
    E0 = Excentricity(day, month, year);

    // Calculate hour angle in radian
    sunriseHourAngle = DayDurationRadian(decli, lat) / 2;

    // Calculate Theoretical radiation en W.m-2
    RGth = solarConst * E0 * (cos(decli)*cos(lat)*sin(sunriseHourAngle)/sunriseHourAngle + sin(decli)*sin(lat));

    return RGth;

}

// Function to calculate decimal hour of sunrise: result in local hour
double CalclulateSunriseLocalTime(int day,
                        int month,
                        int year,
                        double rlong,
                        double rlat)

{
    // local variables
    int h1, h2;
    time_t hour_machine;
    struct tm *local_hour, *gmt_hour;


    double result;
    // Calculate the angle of the day
    double DayAngle = AngleOfDay(day, month, year);
    // Declination
    double SolarDeclination = Declination(DayAngle);
    // Equation of times
    double eth = EqOfTime(DayAngle);
    // True solar time
    double diffUTC_TSV = Tsv_Tu(rlong,eth);
    // Day duration
    double dayDurationj = DayDuratInHours(SolarDeclination,rlat);

    // local time adjust
    time( &hour_machine );      // Get time as long integer.
    gmt_hour =  gmtime( &hour_machine );
    h1 = gmt_hour->tm_hour;
    local_hour = localtime( &hour_machine );    // local time.
    h2 = local_hour->tm_hour;

    // final result
    result = 12 - fabs(dayDurationj / 2) - diffUTC_TSV + h2-h1;

    return result;

}

// Function to calculate decimal hour of sunset: result in local hour
double CalculateSunsetLocalTime(int day,
                          int month,
                          int year,
                          double rlong,
                          double rlat)

{
    // local variables
    int h1, h2;
    time_t hour_machine;
    struct tm *local_hour, *gmt_hour;
    double result;

    // Calculate the angle of the day
    double DayAngle = AngleOfDay(day, month, year);
    // Declination
    double SolarDeclination = Declination(DayAngle);
    // Equation of times
    double eth = EqOfTime(DayAngle);
    // True solar time
    double diffUTC_TSV = Tsv_Tu(rlong,eth);
    // Day duration
    double dayDurationj = DayDuratInHours(SolarDeclination,rlat);

    // local time adjust
    time( &hour_machine );      // Get time as long integer.
    gmt_hour =  gmtime( &hour_machine );
    h1 = gmt_hour->tm_hour;
    local_hour = localtime( &hour_machine );    // local time.
    h2 = local_hour->tm_hour;

    // resultat
    result = 12 + fabs(dayDurationj / 2) - diffUTC_TSV + h2-h1;

    return result;

}



// Function to calculate decimal hour of sunrise: result universal time
double CalculateSunriseUniversalTime(int day,
                        int month,
                        int year,
                        double rlong,
                        double rlat)

{
    double result;
    // Calculate the angle of the day
    double DayAngle = AngleOfDay(day, month, year);
    // Declination
    double SolarDeclination = Declination(DayAngle);
    // Equation of times
    double eth = EqOfTime(DayAngle);
    // True solar time
    double diffUTC_TSV = Tsv_Tu(rlong,eth);
    // Day duration
    double dayDurationj = DayDuratInHours(SolarDeclination,rlat);
    // resultat
    result = 12 - fabs(dayDurationj / 2) - diffUTC_TSV;

    return result;

}

// Function to calculate decimal hour of sunset: result in universal time
double CalculateSunsetUniversalTime(int day,
                          int month,
                          int year,
                          double rlong,
                          double rlat)

{
    double result;

    // Calculate the angle of the day
    double DayAngle = AngleOfDay(day, month, year);
    // Declination
    double SolarDeclination = Declination(DayAngle);
    // Equation of times
    double eth = EqOfTime(DayAngle);
    // True solar time
    double diffUTC_TSV = Tsv_Tu(rlong,eth);
    // Day duration
    double dayDurationj = DayDuratInHours(SolarDeclination,rlat);
    // resultat
    result = 12 + fabs(dayDurationj / 2) - diffUTC_TSV;

    return result;

}


// Function to calculate the height of the sun in radians the day to day j and hour TU
double SolarHeight (int tu,     // universal times (0,1,2,.....,23)
                      int day,
                      int month,
                      int year,
                      double lat,   // latitude in radian
                      double rlong  // longitude in radian
                     )
{
    // local variables
    //  double pi = 3.1415926535;   // Pi
    double result, tsvh;

    // angle of the day
    double DayAngle = AngleOfDay(day, month, year);
    // _declination
    double decli = Declination(DayAngle);
    // eq of time
    double eq = EqOfTime(DayAngle);
    // calculate the tsvh with rlong positiv for the east and negative for the west
    tsvh = tu + rlong*180/(15*pi) + eq;
    // hour angle per hour
    double ah = acos( -cos((pi/12)*tsvh) );
    // final result
    result = asin( sin(lat)*sin(decli) + cos(lat)*cos(decli)*cos(ah) );


    return result;
}



///////////EXTRA FUNCTIONS/////////////////////////////

//Julian day conversion for days calculations
//Explanation for this sick formula...for the curious guys...
//http://www.cs.utsa.edu/~cs1063/projects/Spring2011/Project1/jdn-explanation.html

int julian(int year, int month, int day) {
  int a = (14 - month) / 12;
  int y = year + 4800 - a;
  int m = month + 12 * a - 3;
  if (year > 1582 || (year == 1582 && month > 10) || (year == 1582 && month == 10 && day >= 15))
    return day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  else
    return day + (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;
}

void print_time(double t)
{
  const int hour = (long)t;
  const double fract = 60.0 * (t - hour);
  const long min = (long) fract;
  cout << hour << ":" << min << endl;
}


int main(int argc, char* argv[])
{
    int day = 1;
    int month = 3;
    int year = -1764;


    // double lat = 31.7767;
    // double lon = 35.2345;

    // Los Angeles
     double lat = 34.052235;
     double lon = -118.243683 ;

    // Western Wall / Kotel: 31.7767° N, 35.2345° E

    // Munich: 48.1351° N, 11.5820° E
    const double rlat = lat * pi/180;
    const double rlong = lon * pi/180;

    double _AngleOfDay = AngleOfDay ( day , month , year );
    cout << "Angle of day: " << _AngleOfDay << "\n";

    double _Declinaison = Declination (_AngleOfDay);
    cout << "Declination (Delta): " << _Declinaison << "\n";

    double _EqOfTime = EqOfTime (_AngleOfDay);
    cout << "Declination (Delta): " << _EqOfTime << "\n";

    double _DayDuratInHours = DayDuratInHours (_Declinaison, rlat);
    cout << "Day duration: " << _DayDuratInHours << "\n";

    double _Excentricity = Excentricity(day, month, year);
    cout << "Excentricity: " << _Excentricity << "\n";

    double _TheoreticRadiation = TheoreticRadiation(day, month, year, rlat);
    cout << "Theoretical radiation: " << _TheoreticRadiation << "\n";

    double _CalclulateSunriseLocalTime = CalclulateSunriseLocalTime
                    (day, month, year, rlong, rlat);
    cout << "Sunrise Local Time: " << _CalclulateSunriseLocalTime << "\n";

    double _CalculateSunsetLocalTime = CalculateSunsetLocalTime
        (day, month, year, rlong, rlat);
    cout << "Sunset Local Time: " << _CalculateSunsetLocalTime << "\n";
    print_time(_CalculateSunsetLocalTime);

    return 0;
}
