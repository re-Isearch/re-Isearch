/* Ths file contains ranking and misc. callback functions */

/* -------------------------------------------------------------------------- */
/*

  RANKING TABLES

  These ranking function ONLY effect runtime behavior and may be changed at
  any time without effecting indexes.


*/
/* -------------------------------------------------------------------------- */

/* Newrank added here as a place to configure */

float _ib_Newsrank_weight_factor_hours(int hours)
{
  int   days;
  float weight = 1;

  if (hours <0 && hours > -18) hours = 2; // Correction for time zone glitches

  days = hours/24;

  if (days == -1)       weight = -7.00; // Slight penalty
  else if (days < 0)    weight = days * 6.0; // heavier penalty
  else if (days == 0)   weight = 10.00;
  if (days <= 1)   weight += 5.0;
  if (days <= 2)   weight += 4.0;
  if (days <= 3)   weight += 1.75;
  if (days <= 7)   weight += 0.75;
  if (days <= 14)  weight += 0.55;
  if (days <= 24)  weight += 0.43;
  if (days <= 60)  weight += 0.32;
  if (days <= 100) weight += 0.21;
  if (days <= 300) weight += 0.10;

  if (hours <= 2)
    weight *= 12;
  else if (hours <= 3)
    weight *= 9;
  else if (hours <= 4)
    weight *= 6.2;
  else if (hours <= 8)
    weight *= 4.6;
  else if (hours <= 12)
    weight *= 3.2;
  else if (hours <= 16)
    weight *= 2.6;
  else if (hours <= 23)
    weight *= 2.2;
  return weight;
}

float _ib_Newsrank_weight_factor(int days)
{
  return _ib_Newsrank_weight_factor_hours(days*24);
}



float _ib_Distrank_weight_factor(const int distance)
{
  if (distance <= 4)
    return 20;
  if (distance <= 10)
    return 8;
  if (distance <= 100)
    return 1.5;
  if (distance > 1000)
    return 0.98;
  return 1;
}

int (* __Private_IRSET_Sort) (void *, int, void *, int, void *) = 0;
int (* __Private_RSET_Sort)  (void *, int, int) = 0;

