// $Id: gpolyfield.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $
/*@@@
File:		gpolyfield.cxx
Version:	1.0
$Revision: 1.1 $
Description:	Class GPOLYFIELD - Spatial polygon data object
Author:		Archie Warnock (warnock@awcubed.com), A/WWW Enterprises
@@@*/

#include <math.h> /* for fabs */
#undef logf

#include "gpolyfield.hxx"
#include "magic.hxx"

// Constructor - defaults to bounding rectangle
GPOLYFLD::GPOLYFLD()
{
  GlobalStart = 0;
  nVertices = 0;
  Vertices = NULL;
  Extent   = 0;
}

void  GPOLYFLD::SetVertexCount(size_t x)
{
  if (x > nVertices)
    {
      DOUBLE    *new_Vertices = NULL;
      DOUBLE    *old_Vertices = Vertices;
      try {
         new_Vertices = new DOUBLE[x];
      } catch (...) {
	logf (LOG_PANIC|LOG_ERRNO, "Could not allocate Spatial Polygon object %d", x);
	return;
      }
      size_t  i;
      for (i=0; i<nVertices; i++)
	new_Vertices[i] = old_Vertices[i];
      for (; i < x; i++)
	new_Vertices[i] = 0;
      Vertices = new_Vertices;
      if (old_Vertices) delete[] old_Vertices;
    }
  nVertices = x;
  setExtent();
}



// Constructor for n vertices
GPOLYFLD::GPOLYFLD(INT n)
{
  GlobalStart = 0;
  nVertices = n;
  try {
    Vertices = new DOUBLE[nVertices];
  } catch (...){
    nVertices = 0;
    Vertices = NULL;
  }
  for (size_t i = 0; i< nVertices; i++)
    Vertices[i] = 0;
  Extent = 0;
}


// Copy Constructor
GPOLYFLD::GPOLYFLD(const GPOLYFLD& OtherField)
{
  GlobalStart = OtherField.GlobalStart;
  nVertices = OtherField.nVertices;
  Vertices = new DOUBLE[nVertices];
  for (size_t i = 0; i < nVertices; i++)
    Vertices[i] = OtherField.Vertices[i];
  Extent = OtherField.Extent;
}

DOUBLE GPOLYFLD::setExtent(DOUBLE Val)
{
  if (nVertices == 4 && Val <= 0.0)
    Extent = fabs((Vertices[BBOXFLD::east] - Vertices[BBOXFLD::west]) *
        (Vertices[BBOXFLD::north] - Vertices[BBOXFLD::south]));
  else
    Extent = Val;
  return Extent;
}

GPOLYFLD::GPOLYFLD(const BBOXFLD& OtherField)
{
  GlobalStart = OtherField.GetGlobalStart();
  nVertices   = 4;
  Vertices    = new DOUBLE[nVertices];
  Vertices[BBOXFLD::north] = OtherField.getNorth();
  Vertices[BBOXFLD::east]  = OtherField.getEast();
  Vertices[BBOXFLD::west]  = OtherField.getWest();
  Vertices[BBOXFLD::south] = OtherField.getSouth();
  setExtent();
}


GPOLYFLD GPOLYFLD::operator=(const GPOLYFLD& OtherField)
{
  const size_t count =  OtherField.nVertices;

  GlobalStart  = OtherField.GlobalStart;
  Extent       = OtherField.Extent;

  if (count == 0)
    {
      if (Vertices) delete[] Vertices;
      nVertices = 0;
      Vertices = NULL;
    }
  else
    {
      if (count != nVertices)
	{
	  if (Vertices) delete[] Vertices;
	  Vertices = new DOUBLE[nVertices=count];
	}
      for (size_t i = 0; i < nVertices; i++)
	Vertices[i] = OtherField.Vertices[i];
    }

  return *this;
}

DOUBLE GPOLYFLD::operator[](size_t n) const
{
  if (n >= 0 && n < nVertices) return Vertices[n];
  return 0;
}


GDT_BOOLEAN GPOLYFLD::GetVertexN(size_t x, DOUBLE* Value) const
{
  if (x >= 0 && x < nVertices)
    {
      if (Value) *Value = Vertices[x];
      return GDT_TRUE;
    }
  return GDT_FALSE;
}

// Store the Nth vertex
void GPOLYFLD::SetVertexN(size_t x, DOUBLE value)
{
  if (x > nVertices) Resize(nVertices); // Added Oct 2005

  if ((x >= 0) && (x < nVertices))
    {
      Vertices[x] = value;
      setExtent();
    }
}


// Read the polygon from disk
int GPOLYFLD::Read(FILE *fp)
{
  size_t oldSize = nVertices;
  obj_t  obj = getObjID(fp); // It it really a GPOLY Field?

  if (obj != objGPOLYFLD)
    {
      if (obj == -1 && feof(fp))
	return -1;
      logf (LOG_WARN, "GPOLYFLD::Read. Not a GPOLY Field (%d!=%d @%ld)?",
	(int)obj, (int)objGPOLYFLD, (long)(ftell(fp)-1));
      PushBackObjID (obj, fp);
      GlobalStart = 0;
      nVertices   = 0;
      if (Vertices)
	{
	  delete[] Vertices;
	  Vertices = NULL;
	}
      return -1;
    }

  ::Read(&GlobalStart, fp);
  ::Read(&nVertices,fp);

//cerr << "GlobalStart=" << GlobalStart << endl;
//cerr << "nVerticecs =" << nVertices << endl;

  if (nVertices > oldSize)
    {
      DOUBLE* newArray = new DOUBLE[nVertices];
      if (Vertices) delete[] Vertices;
      Vertices = newArray;

    }
  DOUBLE value;
  size_t i = 0;
  while (i< nVertices && !feof(fp))
    {
      ::Read(&value, fp);
      Vertices[i++] = value;
    }
  if (i<nVertices)
    nVertices = i;
  ::Read(&value, fp);
  Extent = value;
  return nVertices;
}


// Write the polygon out to disk
void GPOLYFLD::Write(FILE *fp) const
{
//cerr << "Write GPLYFIELD" << endl;
  if (nVertices)
    {
      putObjID(objGPOLYFLD, fp);
      ::Write(GlobalStart, fp);
      ::Write(nVertices,fp);
      for (size_t i=0; i< nVertices; i++)
	{
	  ::Write(Vertices[i], fp);
	}
      ::Write((DOUBLE)Extent, fp);
    }
}


void Write(const GPOLYFLD& s, FILE *Fp) { s.Write(Fp); }
void Read(GPOLYFLD *s, FILE *Fp)        { if (s) s->Read(Fp);  }


void GPOLYFLD::Dump(ostream& os) const
{
  os << " ptr: " << GlobalStart << "(" << nVertices << ")" << endl << "\t[";
  for (INT i=0; i<nVertices; i++)
    os << Vertices[i] << " ";
  os << "]" << endl;
}

// This needs/should be the same format taken as input to the
// squery engine
//
// RECT{c0 c1 c2 c3}
//
GPOLYFLD::operator STRING() const
{
 // { "West", "North", "East", "South" } see geosearch
  if (nVertices == 0) return NulString;

  STRING val;

  // At this time nVertices should probably ALWAYS be 4
  // so we should expect to see always RECT, even if at this
  // stage its not needed.
  if (nVertices == 4) val << "RECT";
  val << "{";
  for (int i=0; i<nVertices;i++)
    {
      if (i) val << ",";
      val << Vertices[i];
    }
  val << "}";
  return val;
}

void GPOLYFLD::Resize(size_t x)
{
  if (x == 0)
    {
      if (Vertices)
	{
	  delete[] Vertices;
	  Vertices = NULL;
	}
      nVertices = 0;
    }
  else
    SetVertexCount(x);
}


GPOLYFLD::~GPOLYFLD()
{
  if (Vertices) delete [] Vertices;
}
