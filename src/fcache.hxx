/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/* ########################################################################

		 FCACHE - Field Coordinate Lookup Cache

   ######################################################################## */

#include "strstack.hxx"
#include "mmap.hxx"
#include "idbobj.hxx"

class FCACHE {
public:
   FCACHE(const PIDBOBJ parent);
   FCACHE(const PIDBOBJ parent, const STRING& fieldName, bool useDisk = false);


   bool Ok() const { return FieldTotal != 0; }

   // Normal entry points...
   bool ValidateInField(const GPTYPE HitGp);
   bool ValidateInField(const FC& HitGp);

   bool ValidateInField(const GPTYPE HitGp, const STRING& fieldName, bool Disk);
   bool ValidateInField(const FC& HitGp, const STRING& fieldName, bool Disk);

   bool ValidateInField(const GPTYPE HitGp, STRSTACK& Stack, bool Disk);
   bool ValidateInField(const FC& HitFc, STRSTACK Stack, bool Disk);

   FC FcInField (const GPTYPE HitGp, FILE *fp) const;

   // The workhorses..
   bool ValidateInField (const GPTYPE HitGp, const STRING& FieldName) const;
   bool ValidateInField (const GPTYPE HitGp, FILE *Fp,  const size_t Total = 0) const;
   bool ValidateInField (const GPTYPE HitGp, const void *Buffer, const size_t Length) const;
   bool ValidateInField (const GPTYPE HitGp, const GPTYPE *Buffer, const size_t Total) const;

   bool ValidateInField (const FC& HitFc, const STRING& FieldName) const;
   bool ValidateInField (const FC& HitFc, FILE *Fp, const size_t Total = 0) const;
   bool ValidateInField (const FC& HitFc, const void *Buffer, const size_t Length) const;
   bool ValidateInField (const FC& HitFc, const GPTYPE *Buffer, const size_t Total) const;

   size_t GetTotal() const { return FieldTotal; }

   bool SetFieldName(const STRING& fieldName, bool Disk=false);
   bool GetFieldName(STRING *fieldNamePtr) const;
   STRING      GetFieldName() const { return FieldName; }

   ~FCACHE();
private:
   size_t LoadFieldCache(const STRING& fieldName, bool useDisk);
   size_t GetZones (const GPTYPE HitGp, const STRING& fieldName, FCT *Zones);

/* Private data */
   MMAP        Cache;    	// Memory Maped 
   FILE       *Fp;		// Stream
   bool Disk;     	// Use disk or memory?
   FC          Range;    	// Start and End points
   size_t      FieldTotal;	// How many Fields?
   STRING      FieldName;	// The name of the loaded field
   PIDBOBJ     Parent;  	// Parent class
};
