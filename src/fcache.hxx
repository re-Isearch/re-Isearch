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
   FCACHE(const PIDBOBJ parent, const STRING& fieldName, GDT_BOOLEAN useDisk = GDT_FALSE);


   GDT_BOOLEAN Ok() const { return FieldTotal != 0; }

   // Normal entry points...
   GDT_BOOLEAN ValidateInField(const GPTYPE HitGp);
   GDT_BOOLEAN ValidateInField(const FC& HitGp);

   GDT_BOOLEAN ValidateInField(const GPTYPE HitGp, const STRING& fieldName, GDT_BOOLEAN Disk);
   GDT_BOOLEAN ValidateInField(const FC& HitGp, const STRING& fieldName, GDT_BOOLEAN Disk);

   GDT_BOOLEAN ValidateInField(const GPTYPE HitGp, STRSTACK& Stack, GDT_BOOLEAN Disk);
   GDT_BOOLEAN ValidateInField(const FC& HitFc, STRSTACK Stack, GDT_BOOLEAN Disk);

   FC FcInField (const GPTYPE HitGp, FILE *fp) const;

   // The workhorses..
   GDT_BOOLEAN ValidateInField (const GPTYPE HitGp, const STRING& FieldName) const;
   GDT_BOOLEAN ValidateInField (const GPTYPE HitGp, FILE *Fp,  const size_t Total = 0) const;
   GDT_BOOLEAN ValidateInField (const GPTYPE HitGp, const void *Buffer, const size_t Length) const;
   GDT_BOOLEAN ValidateInField (const GPTYPE HitGp, const GPTYPE *Buffer, const size_t Total) const;

   GDT_BOOLEAN ValidateInField (const FC& HitFc, const STRING& FieldName) const;
   GDT_BOOLEAN ValidateInField (const FC& HitFc, FILE *Fp, const size_t Total = 0) const;
   GDT_BOOLEAN ValidateInField (const FC& HitFc, const void *Buffer, const size_t Length) const;
   GDT_BOOLEAN ValidateInField (const FC& HitFc, const GPTYPE *Buffer, const size_t Total) const;

   size_t GetTotal() const { return FieldTotal; }

   GDT_BOOLEAN SetFieldName(const STRING& fieldName, GDT_BOOLEAN Disk=GDT_FALSE);
   GDT_BOOLEAN GetFieldName(STRING *fieldNamePtr) const;
   STRING      GetFieldName() const { return FieldName; }

   ~FCACHE();
private:
   size_t LoadFieldCache(const STRING& fieldName, GDT_BOOLEAN useDisk);
   size_t GetZones (const GPTYPE HitGp, const STRING& fieldName, FCT *Zones);

/* Private data */
   MMAP        Cache;    	// Memory Maped 
   FILE       *Fp;		// Stream
   GDT_BOOLEAN Disk;     	// Use disk or memory?
   FC          Range;    	// Start and End points
   size_t      FieldTotal;	// How many Fields?
   STRING      FieldName;	// The name of the loaded field
   PIDBOBJ     Parent;  	// Parent class
};
