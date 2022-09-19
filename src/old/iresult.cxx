#pragma ident  "@(#)iresult.cxx  1.31 02/25/01 00:23:40 BSN"

/************************************************************************
************************************************************************/


/*-@@@
File:		iresult.cxx
Version:	1.00
Description:	Class IRESULT - Internal Search Result
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <math.h>
#include "common.hxx"
#include "iresult.hxx"
#include "magic.hxx"


#include <float.h>
#ifndef MAXFLOAT
# ifndef FLT_MAX
#  define FLT_MAX 3.402823466E+18
# endif
#define MAXFLOAT FLT_MAX
#endif



#if USE_GEOSCORE

void GEOSCORE::Write(FILE *fp) const
{
  putObjID(objGEOSCORE, fp);
  ::Write(t, fp);
  ::Write(q, fp);
}

GDT_BOOLEAN GEOSCORE::Read(FILE *fp)
{
  obj_t obj = getObjID(fp); // It it really GEOSCORE? 

  if (obj != objGEOSCORE)
    {
      PushBackObjID (obj, fp);
      return GDT_FALSE;
    }
  ::Read(&t, fp);
  ::Read(&q, fp);
  return GDT_TRUE;
}
#endif


#ifdef DEBUG_MEMORY
long __IB_IRESULT_allocated_count = 0; // Used to track stray IRESULTs
#endif

// Index contains:
// vIndex and MdtIndex packed into a 4 byte word
// 3 bytes for the MdtIndex and 1 byte for the vIndex
// FFFFFFFF
//  ^ 
//  |____ this is the vIndex

IRESULT::IRESULT()
{
  Index          = 0;
  HitCount       = 0;
  AuxCount       = 0;
//  Date.Clear();
  Score          = 0;
  Mdt            = NULL;
  maxTermHitsVector = 0;
  TermHitsVector    = NULL;
#ifdef DEBUG_MEMORY
  __IB_IRESULT_allocated_count++;
#endif
}

IRESULT::IRESULT(MDT *p)
{
  Index          = 0;
  HitCount       = 0;
  AuxCount       = 0;
//Date.Clear();
  Score          = 0;
  Mdt            = p;
  maxTermHitsVector = 0;
  TermHitsVector    = NULL;
#ifdef DEBUG_MEMORY
  __IB_IRESULT_allocated_count++;
#endif
}


IRESULT::IRESULT(const IRESULT& OtherIresult)
{
#ifdef DEBUG_MEMORY
  __IB_IRESULT_allocated_count++;
#endif
  *this = OtherIresult;
}


IRESULT& IRESULT::operator=(const IRESULT& OtherIresult)
{
  Index     = OtherIresult.Index;
  SortIndex = OtherIresult.SortIndex;
  HitCount  = OtherIresult.HitCount;
  AuxCount  = OtherIresult.AuxCount;
  Date      = OtherIresult.Date;
  Score     = OtherIresult.Score;
#if USE_GEOSCORE
  gScore    = OtherIresult.gScore;
#endif
  HitTable =  OtherIresult.HitTable;
  Mdt       = OtherIresult.Mdt;

  // Re-allocate ?
  if (AuxCount <= 0)
    {
      if (TermHitsVector) free (TermHitsVector);
      TermHitsVector = NULL;
      maxTermHitsVector = 0;
    }
  else if (AuxCount > maxTermHitsVector)
    {
      // Free the old one and allocate since values are new
      if (TermHitsVector) free (TermHitsVector);
      if ((TermHitsVector = (UINT4 *)calloc (AuxCount+1, sizeof(UINT4))) == NULL)
	maxTermHitsVector = 0;
      else
	maxTermHitsVector = AuxCount+1;
    }
  if (TermHitsVector && OtherIresult.TermHitsVector)
    memcpy((void *)TermHitsVector, (void *)(OtherIresult.TermHitsVector), AuxCount*sizeof(UINT4));

  return *this;
}

/*
void IRESULT::SetHitTable(const FC& Fc)
{
  HitTable.Clear();
  HitTable.AddEntry(Fc);
}

void IRESULT::SetHitTable(const IRESULT& ResultRecord)
{
  HitTable = ResultRecord.HitTable;
}


void IRESULT::SetHitTable(const FCLIST& NewHitTable)
{
  HitTable = NewHitTable;
}
*/

const FCT IRESULT::GetHitTable(FCLIST *Table) const
{
  if (Table)
    *Table = HitTable.GetFCLIST();
  return HitTable;
}

SRCH_DATE IRESULT::GetDate()
{
  if (Date.IsNotSet())
    {
      if ( Mdt )
	Date = Mdt->GetDate( Index.GetMdtIndex()) ;
    }
  return Date;
}

#if 0
void IRESULT::AddToHitTable(const FC& Fc)
{
  HitTable.AddEntry(Fc);
}

// edz: Check this?
void IRESULT::AddToHitTable(const IRESULT& ResultRecord)
{
  HitTable.AddEntry( ResultRecord.HitTable );
}
#endif

// NOTE: We don't write SortIndex since its really
// volatile
void IRESULT::Write(FILE *fp) const
{
  putObjID(objIRESULT, fp);
  ::Write(Index, fp);

  ::Write(HitCount, fp);
  ::Write(AuxCount, fp);
  ::Write(Score, fp);
  ::Write(Date, fp);
  HitTable.Write(fp);
#if USE_GEOSCORE
  ::Write(gScore, fp);
#endif

#if 0 // July 15 .. Not for now!!
  for (size_t i=0; i < AuxCount; i++)
    ::Write((UINT4)(TermHitsVector ? TermHitsVector[i] : 0), fp);
#endif
}

GDT_BOOLEAN IRESULT::Read(FILE *fp)
{
  obj_t obj = getObjID(fp); // It it really an IRESULT?

  if (obj != objIRESULT)
    {
      PushBackObjID (obj, fp);
      return GDT_FALSE;
    }
  ::Read(&Index, fp);

  ::Read(&HitCount, fp);
  ::Read(&AuxCount, fp);
  ::Read(&Score, fp);
  ::Read(&Date, fp);
  HitTable.Read(fp);
#if USE_GEOSCORE
  ::Read(&gScore, fp);
#endif
  Mdt = NULL;
  SortIndex = 0;
  return GDT_TRUE; // TODO: Sanity check!
}

GDT_BOOLEAN IRESULT::SetVectorTermHits (const UINT count)
{
//cerr << "AuxCount = " << AuxCount << "  hits=" << count << endl;
  if (AuxCount >= maxTermHitsVector)
    {
      void *ptr;
      maxTermHitsVector += 64; // Expand for 64 more terms
      if (TermHitsVector == NULL)
	ptr = calloc(maxTermHitsVector + 1, sizeof(UINT4));
      else
        ptr = realloc(TermHitsVector, (maxTermHitsVector + 1)*sizeof(UINT4));
      if (ptr)
	TermHitsVector = (UINT4 *)ptr;
      else if (maxTermHitsVector)
	{
	  free(TermHitsVector);
	  TermHitsVector = NULL;
	  maxTermHitsVector = 0;
	  return GDT_FALSE; // Can't, no memory
	}
    }
  else if (AuxCount) // We con't want to count more than 0xFFFFFFFF hits
    TermHitsVector[AuxCount-1] = (count <= 0xFFFFFFFF ? (UINT4)count :  0xFFFFFFFF);
  else
    return GDT_FALSE; // AuxCount can't be zero.. 
  return GDT_TRUE;
}


IRESULT::~IRESULT()
{
  if (TermHitsVector)
    {
// for (size_t i=0; i < AuxCount; i++) cerr << " [" << i << "] = " << TermHitsVector[i]; cerr << endl;
      free (TermHitsVector);
    }
#ifdef DEBUG_MEMORY
  if (--__IB_IRESULT_allocated_count < 0)
    logf (LOG_PANIC, "IRESULT global allocated count %ld < 0!", (long)__IB_IRESULT_allocated_count);
#endif
}

// Common Functions
void Write(const IRESULT& Iresult, PFILE Fp)
{
  Iresult.Write(Fp); 
}

GDT_BOOLEAN Read(PIRESULT IresultPtr, PFILE Fp)
{
  return IresultPtr->Read(Fp);
}

struct IRESULTData
{
  signed   int  nRefs;        // reference count
  unsigned int  nTotalEntries;
  unsigned int  nMaxEntries;

  // mimics declaration 'IRESULT data[nAllocLength]'
  IRESULT * data() const { return (IRESULT*)(this + 1); }

  int RefCount() const { return nRefs; }

  // empty string has a special ref count so it's never deleted
  GDT_BOOLEAN  IsEmpty()   const { return nRefs == -1; }
  GDT_BOOLEAN  IsShared()  const { return nRefs > 1;   }
  GDT_BOOLEAN  IsValid()   const { return nRefs != 0;  }

  // lock/unlock
  void  Lock()   { if ( nRefs >= 0 ) nRefs++;                        }
  void  Unlock() { if ( nRefs > 0  ) if (--nRefs == 0 ) delete[] this; }
};

static const int tEmpty[] = { -1, 0, 0, 0 };

static IRESULTData *g_Nul = (IRESULTData*)&tEmpty;
static IRESULT     *t_Nul = (IRESULT *)(&g_Nul[3]);

#define INCREMENT 4096

// copy constructor
IRESULT_TABLE::IRESULT_TABLE()
{
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
  Allocate(0);
}

#define GetIRESULTData() ((IRESULTData*)t_Data-1)

IRESULT_TABLE::IRESULT_TABLE(const IRESULT_TABLE& Src)
{
  t_Data = Src.t_Data;           // share same data
  Sort   = Src.Sort;
  GetIRESULTData()->Lock();             // => one more copy
}

IRESULT_TABLE::IRESULT_TABLE(const IRESULT& Record)
{
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
  Allocate(5);
  AddEntry(Record);
}

IRESULT_TABLE::IRESULT_TABLE(const IRESULT *TablePtr, size_t TotalEntries)
{
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
  Allocate(TotalEntries+1);
  for (size_t i=0; i< TotalEntries; i++)
    AddEntry(TablePtr[i]);
}


IRESULT_TABLE& IRESULT_TABLE::operator=(const IRESULT_TABLE& OtherTable)
{
  GetIRESULTData()->Unlock();
  t_Data   = OtherTable.t_Data;           // share same data
  Sort     = OtherTable.Sort;
  MinScore = OtherTable.MinScore;
  MaxScore = OtherTable.MaxScore;
  GetIRESULTData()->Lock();      // => one more copy
  return *this;
}


static IRESULTData *TableAllocate(size_t Len)
{
  if (Len == 0)
    return g_Nul;

  // allocate memory:
  // 1) one extra character for '\0' termination
  // 2) sizeof(STRINGData) for housekeeping info
  IRESULTData* pData;
  try {
    pData = (IRESULTData*)new char[sizeof(IRESULTData) + (Len+1)*sizeof(IRESULT)];
  } catch(...) {
    pData = NULL;
  }
  if (pData == NULL)
    {
      logf (LOG_PANIC|LOG_ERRNO, "Could not allocate iresult table space (%d)", Len);
      Len = 0;
      throw;
    }
  pData->nRefs         = 1;
  pData->nMaxEntries   = Len;
  pData->nTotalEntries = 0;
  return pData;
}

void IRESULT_TABLE::Allocate(size_t Len)
{
  IRESULTData* pData = TableAllocate(Len);
  if (pData)
    t_Data           = pData->data();  // data starts after STRINGData
}

size_t IRESULT_TABLE::GetTotalEntries() const
{
  return GetIRESULTData()->nTotalEntries;
}

void IRESULT_TABLE::Reinit()
{
  GetIRESULTData()->Unlock();
  Allocate(0);
  MinScore=MAXFLOAT;
  MaxScore=0.0;
  Sort= ByIndex;
}

void IRESULT_TABLE::CopyBeforeWrite()
{
  const size_t Total = GetTotalEntries();
  if (GetIRESULTData()->IsShared())
    {
      IRESULTData* pData = TableAllocate(Total);
      IRESULT *NewTable = pData->data(); 
      for (size_t i=0; i< Total; i++)
	NewTable[i] = t_Data[i];
      GetIRESULTData()->Unlock();
      t_Data = pData->data();
    }
}

void IRESULT_TABLE::SetTotalEntries(size_t Size)
{
  size_t Total    = GetTotalEntries();
  size_t NewTotal = (Size > Total ? Total : Size);

  if (Size == 0)
    {
      GetIRESULTData()->Unlock();
      t_Data = t_Nul;
      MinScore=MAXFLOAT;
      MaxScore=0.0;
      Sort= ByIndex;
    }
  else if (GetIRESULTData()->IsShared())
    {
      IRESULTData* pData = TableAllocate(NewTotal);
      IRESULT *NewTable = pData->data();
      for (size_t i=0; i< NewTotal; i++)
        NewTable[i] = t_Data[i];
      GetIRESULTData()->Unlock();
      t_Data = pData->data();
    }
  else
    GetIRESULTData()->nTotalEntries = NewTotal;
}

void IRESULT_TABLE::Expand(size_t Add)
{
  const size_t Total = GetTotalEntries();
  const size_t Max   = GetIRESULTData()->nMaxEntries + Add;
  IRESULTData* pData = TableAllocate(Max);
  IRESULT *NewTable = pData->data();
  for (size_t i=0; i< Total; i++)
    NewTable[i] = t_Data[i];
  GetIRESULTData()->Unlock();
  t_Data = pData->data();
}

void IRESULT_TABLE::AddEntry(const IRESULT& Record)
{
  const size_t NewTotal = GetTotalEntries()+1;
  if (NewTotal >= GetIRESULTData()->nMaxEntries)
    {
      Expand( NewTotal*3 );
    }
  SetEntry(NewTotal, Record);
  Sort = Unsorted;
}


GDT_BOOLEAN IRESULT_TABLE::SetEntry(size_t n, const IRESULT& Record)
{
  if (n > 0 && n <= GetIRESULTData()->nMaxEntries)
    {
      CopyBeforeWrite();
      if (n > 1)
	{
	  INT index = Record.GetMdtIndex ();
	  if (t_Data[n-2].GetMdtIndex () == index)
	    {
	      t_Data[n-2].IncHitCount (Record.GetHitCount ());
	      t_Data[n-2].AddToHitTable (Record);
	      const DOUBLE score = t_Data[n-2].IncScore (Record.GetScore() );
	      if (score < MinScore) MinScore = score;
	      if (score > MaxScore) MaxScore = score;
	      if (Sort == ByScore)
		Sort = Unsorted;
	      return GDT_TRUE;
	    }
	  if (Sort == ByIndex && t_Data[n-2].GetMdtIndex() > index)
	    Sort = Unsorted;
	}
      t_Data[n-1] = Record;
      if (n > GetIRESULTData()->nTotalEntries)
	GetIRESULTData()->nTotalEntries = n;
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

static int iIndexCompare(const void *x, const void *y)
{
  return (int)(*((size_t *)x) -  ((IRESULT *)y)->GetMdtIndex() );
}

size_t IRESULT_TABLE::FindByMdtIndex(size_t Index) const
{
  const size_t TotalEntries = GetTotalEntries();
  if (Sort == ByIndex && TotalEntries > 3)
    {
      // Binary search...
      IRESULT *ptr = (IRESULT *)bsearch(&Index, (const void *)t_Data, TotalEntries, sizeof(IRESULT), iIndexCompare);
      if (ptr)
	return (size_t)(ptr - t_Data) + 1;
    }
  // linear
  for (size_t i = 0; i < TotalEntries; i++)
    {
      if (t_Data[i].GetMdtIndex () == Index)
	return i + 1;
    }
  return 0; // NOT FOUND
}


static int IrsetIndexCompare(const void* x, const void* y)
{
  return (int)( (*((PIRESULT)x)).GetMdtIndex() - (*((PIRESULT)y)).GetMdtIndex() );
}  

static int IrsetScoreCompare (const void *x, const void *y)
{
  const DOUBLE Difference = ((*((const IRESULT *) y)).GetScore () -
        (*((const IRESULT *) x)).GetScore ());
  if (Difference < 0)
    return (-1);
  else if (Difference > 0)
    return (1);
  // Difference == 0
  return IrsetIndexCompare(y, x); // Reverse order in index
}

void IRESULT_TABLE::SortByScore ()
{
  if (Sort != ByScore)
    {
      CopyBeforeWrite();
      const size_t TotalEntries = GetTotalEntries();
      if (TotalEntries > 1)
	QSORT (t_Data, TotalEntries, sizeof (IRESULT), IrsetScoreCompare);
      Sort = ByScore;
    }
}

void IRESULT_TABLE::SortByIndex ()
{
  if (Sort != ByIndex)
    {
      CopyBeforeWrite();
      const size_t TotalEntries = GetTotalEntries();
      if (TotalEntries > 1)
	QSORT (t_Data, TotalEntries, sizeof(IRESULT), IrsetIndexCompare);
      Sort = ByIndex;
    }
}

void IRESULT_TABLE::NormalizeScores (const off_t TotalDocs, const INT TermWeight)
{
  const size_t TotalEntries = GetTotalEntries();

  if (TotalEntries)
    {
      // Get Freq of <Total Docs in Db>/<Total Docs in Result Set> 
      const DOUBLE InvDocFreq = (DOUBLE)(TotalDocs) / (DOUBLE)TotalEntries;
      DOUBLE SumSqScores = 0;
      // Get sum of squares
      for (size_t i = 0; i < TotalEntries; i++)
	{
	  DOUBLE Score = t_Data[i].GetHitCount () * InvDocFreq;

	  SumSqScores += Score * Score; // Running sum of squares
	  Score *= TermWeight;
	  t_Data[i].SetScore ( Score );
	  if ((Score - MaxScore) > 0.0) MaxScore=Score;
	  if ((Score - MinScore) < 0.0) MinScore=Score;
	}
      // The Root of Squares...
      DOUBLE SqrtSum= sqrt (SumSqScores);
      if (SqrtSum == 0.0)
	{
	  SqrtSum = 1.0;
	}
      else
	{
	  MaxScore /= SqrtSum;
	  MinScore /= SqrtSum;
	}
      // Now caculate the Factor
      for (size_t x = 0; x < TotalEntries; x++)
	{
          t_Data[x].SetScore ( t_Data[x].GetScore () / SqrtSum );
	}
    }
}

IRESULT_TABLE::~IRESULT_TABLE()
{
  GetIRESULTData()->Unlock();
}
