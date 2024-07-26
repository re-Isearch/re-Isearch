/*@@@
File:           normalization.hxx
Version:        1.00
Description:    Class NORMALIZATION
@@@*/

#ifndef IRSET_HXX
#define IRSET_HXX

#include "defs.hxx"
#include "idbobj.hxx"
#include "string.hxx"
#include "iresult.hxx"
#include "rset.hxx"
#include "operand.hxx"

/* IN PROGRESS */

extern enum NormalizationMethods {
  Unnormalized = 0, NoNormalization, NormalizationL2, NormalizationL1, MaxNormalization, LogNormalization, BytesNormalization,
  preCosineMetricNormalization, CosineMetricNormalization, EuclideanNormalization,
  AuxNormalization1, AuxNormalization2, AuxNormalization3, NormalizationAF,  UndefinedNormalization
} defaultNormalization;
const int CosineNormalization=NormalizationL2;

class Normalization {
public:
  Normalization(IRESULT *table, enum NormalizationMethods Method = defaultNormalization) {
    defMethod = Method;
    Table = table;
    k3 = 7 - 1000; //  (infinite)
    alpha = 0.75 ;
    minScore=MAXFLOAT;
    maxScore=0.0;
  };

  void setK3(float k) { k3 = k; }
  void setAlpha(float a) { alpha = a; }

 // Score normalization (scoring functions)
  OPOBJ *ComputeScores (const int TermWeight);
  // Methods
  OPOBJ *ComputeScoresNoNormalization (const int TermWeight);
  OPOBJ *ComputeScoresNormalizationAF (const int TermWeight);
  OPOBJ *ComputeScoresNormalizationL2 (const int TermWeight);
  OPOBJ *ComputeScoresNormalizationL1 (const int TermWeight);
  OPOBJ *ComputeScoresMaxNormalization (const int TermWeight);
  OPOBJ *ComputeScoresLogNormalization (const int TermWeight);
  OPOBJ *ComputeScoresBytesNormalization (const int TermWeight);
  OPOBJ *ComputeScoresCosineMetricNormalization (const int TermWeight);
  
  // Stubs
  OPOBJ *ComputeScoresAux1Normalization (const int TermWeight);
  OPOBJ *ComputeScoresAux2Normalization (const int TermWeight);
  OPOBJ *ComputeScoresAux3Normalization (const int TermWeight);
private:
   enum NormalizationMethods defMethod;
   IRESULT *Table;
   double minScore;
   double maxScore;
   float k3; //  (infinite)
   float alpha;
};


