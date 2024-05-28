/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)mdt.cxx"

//// TODO: htonll etc... Current implmentation is 32-bit!!!

const int MaxMDTInstances = 20; // 100000;

/************************************************************************
************************************************************************/

/*-@@@
File:		mdt.cxx
Version:	1.00
Description:	Class MDT - Multiple Document Table
Original Idea:	Nassib Nassar, nrn@cnidr.org
Author:		Edward C. Zimmermann
@@@-*/

#include <stdlib.h>
#include <string.h>

#if defined(_MSDOS) || defined(_WIN32)
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include "common.hxx"
#include "mdt.hxx"

#if defined(SOLARIS)
#include <sys/byteorder.h>
#include <iomanip.h>
#elif defined (BSD) || defined(LINUX)
#include <arpa/inet.h>
#else
#undef nthol
#undef htonl
#define  ntohl(x) (x)
#define  htonl(x) (x)
/*
extern   uint32_t htonl(uint32_t);
extern   uint32_t ntohl(uint32_t);
*/
#endif
#include <errno.h>
#include "mmap.hxx"
#include "index.hxx"
#include "fpt.hxx"

#ifdef _WIN32
# define EIDRM  82              /* Identifier removed */
#endif

#define PORTABLE_MDT 0

#include <unistd.h>

#if defined(_MSDOS) || defined(_WIN32)
# define ftruncate(_fd,_len) chsize(_fd, _len)
# include <io.h>
#endif

#if USE_MDTHASHTABLE
MDTHASHTABLE *_globalMDTHashTable = NULL;
#endif


#if PORTABLE_MDT
# ifdef O_BUILD_IB64
#  define NTOHL(_x) ntohll(_x)
#  define HTONL(_x) htonll(_x)
# else
#  define NTOHL(_x) ntohl(_x)
#  define HTONL(_x) htonl(_x)
# endif
#else
# undef  NTOHL
# define NTOHL(_x) (_x)
# undef  HTONL
# define HTONL(_x) (_x)
#endif





#define SIZEOF_MAGIC 16 /* See mdt.cxx */

#define _NextGlobal(_c) ((_c).GetGlobalFileStart()+(_c).GetLocalRecordEnd() + 1)

int _IB_MDT_SEED = 5039;
#define GROWTH_FACTOR(_x) ((_x)*3 + _IB_MDT_SEED)


#define DELETED_BITS        0xF0000000UL
#define INDEX_BITS          (0x0FFFFFFFUL | ((UINT8)(0xFFFFFFFFUL)) << 32)
#define DELETED_MASK(_x)    (((_x) & DELETED_BITS) ? true : false)
#define INDEX_MASK(_x)      ((_x) & INDEX_BITS)
#define KEYHASH_MASK(_x)    (UINT4)((_x) >> 32)
#define SET_DELETE_BITS(_x) ((_x) |= DELETED_BITS)
#define CLR_DELETE_BITS(_x) ((_x) &= INDEX_BITS) 
#define SET_DELETE_STATE(_x, _y)  ((_y) ? SET_DELETE_BITS(_x) : CLR_DELETE_BITS(_x))

// One at a time hash:
static UINT4 KeyHash32(const char *key, size_t len=0)
{
  UINT4      hash = 0;
  if (len == 0) len = strlen(key);
  // Max. key length is DocumentKeySize
  for (size_t i = 0; i < len && i<=DocumentKeySize; ++i)
    {
      hash += (BYTE)key[i]; hash += hash << 10; hash ^= hash >> 6;
    }
  hash += hash << 3; hash ^= hash >> 11; hash += hash << 15;
  return hash;
}

static inline UINT4 KeyHash32(const STRING& Key)
{
  return KeyHash32(Key.c_str(), Key.GetLength());
}

// Idx := which record in MDT (count)
// Key := Record Key
//
//  [4 bytes][4 bytes]
//    ^          \ the index where the top byte is deleted/not deleted
//    \ the 32-bit one_time_hash of the key.
//
static UINT8 IndexValueEncode(_index_id_t Idx, const STRING& Key)
{
  return (((UINT8)KeyHash32(Key)) << 32) | (UINT8)Idx;
}

//
// GpIndex *
// is a pointer to an array of GPRECs sorted by GpStart
//
/// NOTE: We want to move Hash to GPREC from KEYREC
//
class GPREC {
 public:
  GPREC ()       { Clear(); }
  void           Clear() {  GpStart = GpEnd = 0; Index = 0; }
  GPTYPE         GpStart;
  GPTYPE         GpEnd;
  SRCH_DATE      Date;
  _index_id_t    Index; // NOTE: DOUBLE of _index_id_t
};

// KeyIndex *
// is a pointer to an arrary of KEYRECs sorted by Key
//
class KEYREC {
 public:
  KEYREC()      { Clear(); }
  void  Clear() {  Hash = 0; Index=0; Key[0] = '\0'; }
  CHR            Key[DocumentKeySize];
  UINT4          Hash;
  _index_id_t    Index;
};

//
//
// Sorted by keys and then the list..
// This then can map Index-> Order if sorted by Key.
//
// KeySortTable
class KEYSORT {
  public:
   KEYSORT()      { Position=0;}
   _index_id_t      Position; // This is the order
};


bool MDT::BuildKeySortTable()
{
  size_t     entries = 0;

  if (useIndexMap)
    {
      message_log (LOG_ERROR, "MDT Can't build Key Sort Table while tables are mapped.");
      return false;
    }

  if (KeyIndexSorted == false)
    {
      SortKeyIndex ();
    }

  if (KeySortTable)
    {
      delete[] KeySortTable;
      KeySortTable = NULL;
    }

  if (TotalEntries)
    {
      size_t  index;

      KeySortTable = new KEYSORT[TotalEntries+1];
      for (_index_id_t i =0; i< TotalEntries; i++)
	{
	  if ((index = (size_t)INDEX_MASK(KeyIndex[i].Index)) == 0) {
	    message_log (LOG_WARN, "Undefined key index[%u]", (unsigned)i);
	  } else if (index > TotalEntries) {
	    message_log (LOG_PANIC, "MDT SortTable/KeyIndex[%u] grok (%u>%u)",
		(unsigned)i, (unsigned)index, (unsigned)TotalEntries);
	  } else {
	    KeySortTable[index-1].Position = i;
	    entries++;
	  }
	}
    }
  return entries == TotalEntries; // Any errors?
}


// Uses the KeySortTable
// Takes an index and returns the position in the sort
//
//
// KeySortPosition(idx1) > KeySortPosition(idx2)
// means that
// the Key of idx1 is lexi sorted > the key of idx2
//
size_t MDT::KeySortPosition(_index_id_t Idx) const
{
  size_t index = (size_t)INDEX_MASK(Idx);
  if (index > TotalEntries || KeySortTable == NULL || index <= 0) return 0;
  return KeySortTable[index-1].Position;
}

typedef MDTREC MDTRECORD;

static const GPTYPE CacheVersion = 1;

#if 0
class INDEXREC {
public:
  INDEXREC() {
    GpIndex      = NULL;
    KeyIndex     = NULL;
    KeySortTable = NULL;
  }

  int Clear()
    {
      if (!useIndexMap)
        {
          if (KeyIndex)     delete[] KeyIndex;
          if (GpIndex)      delete[] GpIndex;
          if (KeySortTable) delete[] KeySortTable;
        }
      else
        IndexMap.Unmap();
    }

  MMAP        IndexMap;
  bool useIndexMap;
#if USE_MDTHASHTABLE
  bool fastAdd;
#endif
  MMAP        MdtMap;
  bool useMdtMap;

  KEYREC     *KeyIndex;
  GPREC      *GpIndex;
  KEYSORT    *KeySortTable;

  MDTRECORD  *MdtIndex;

  size_t      lastIndex;
};
#endif

static int InstanceCount = 0;

int  MDT::Version() const
{
  return  (2*1000)+sizeof(MDTRECORD);
}


MDT::MDT (INDEX *Index)
{
  MdtFp = NULL;
  MDTHashTable = NULL;

  if (Index)
    {
      static const STRING MdtInfo("_MdtControl");

      const char *def = (InstanceCount < MaxMDTInstances) ? "1" : "0";
      // Do we use an Index Map??
      useIndexMap    = Index-> ProfileGetString(MdtInfo, "UseIndexMap", def).GetBool();
#if USE_MDTHASHTABLE
      fastAdd        = Index->GetMergeStatus() == iNothing;
#endif
      useMdtMap      = Index-> ProfileGetString(MdtInfo, "UseMdtMap", def).GetBool();
      MdtWrongEndian = Index->IsWrongEndian();
      FileStem       = Index->GetDbFileStem();
      Fpt            = Index->GetMainFpt();
    }
  else
    {
      useIndexMap    = true;
      useMdtMap      = true;
#if USE_MDTHASHTABLE
      fastAdd        = false;
#endif
      MdtWrongEndian = !IsBigEndian ();
      FileStem       = __IB_DefaultDbName;
      Fpt            = NULL;
    }

  GpIndex   = NULL;
  KeyIndex  = NULL;
  KeySortTable = NULL; 
  KeyIndexSorted = false;

  ReadOnly = true;

  if (FileStem.GetLength())
    Init();
}

MDT::MDT (const STRING& DbFileStem, const bool WrongEndian)
{
  MdtFp          = NULL;
  MDTHashTable   = NULL;
  useIndexMap    = (InstanceCount < MaxMDTInstances);
  useMdtMap      = (InstanceCount < MaxMDTInstances);
#if USE_MDTHASHTABLE
  fastAdd        = false;
#endif
  FileStem       = DbFileStem;
  MdtWrongEndian = WrongEndian;

  GpIndex   = NULL;
  KeyIndex  = NULL;
  KeySortTable = NULL; 
  KeyIndexSorted = false;

  ReadOnly = true;

  if (DbFileStem.GetLength())
    Init();
}

void MDT::Init()
{
  GPTYPE realTotal = 0;

  if (MdtFp)
    {
      fclose(MdtFp);
      MdtFp = NULL;
    }
  InstanceCount++;
  message_log (LOG_DEBUG, "MDT::Init() instance %d", InstanceCount);

  if (IsBigEndian ())
    {
      if (MdtWrongEndian) Magic = "!MDT"; // Wrong endian on a little endian system
     else                 Magic = "<MDT";
    }
  else
    {
      if (MdtWrongEndian) Magic = "!mdt";
      else                Magic = "!MDT";
    }

  if (GpIndex || KeyIndex || KeySortTable)
    {
      if (!useIndexMap)
	{
	  if (KeyIndex)     delete[] KeyIndex;
	  if (GpIndex)      delete[] GpIndex;
	  if (KeySortTable) delete[] KeySortTable;
	}
      else
	IndexMap.Unmap();
      GpIndex   = NULL;
      KeyIndex  = NULL;
      KeySortTable = NULL; 
    }

  TotalEntries = 0;
  NextGlobalGp = 0;
#if 0
  GPTYPE totalDeleted = 0;
#endif
  MdtName = FileStem+DbExtMdt;
  MdtIndexName = FileStem+DbExtMdtIndex;

  if (useMdtMap)
    {
      MdtMap.CreateMap(MdtName, MapRandom);
      if (MdtMap.Ok())
	{
	  // Here we map it in...
	  MdtIndex = (MDTRECORD *)(MdtMap.Ptr() + SIZEOF_MAGIC);
	  realTotal = TotalEntries = (size_t)((MdtMap.Size() - SIZEOF_MAGIC)/ sizeof (MDTRECORD));
	  if (TotalEntries == 0)
	    {
	      useIndexMap = useMdtMap = false;
	      MdtMap.Unmap();
	    }
	}
      else
	{
	  useMdtMap = false;
	}
    }

  bool reBuildIndexCache = false;

  bool try_open_stream = true;
  if (useIndexMap)
    {
      IndexMap.CreateMap(MdtIndexName, MapRandom);
      if (IndexMap.Ok())
	{
	  const BYTE *ptr = IndexMap.Ptr();
	  const GPTYPE cache_version = *((GPTYPE *)ptr);
	  if (cache_version != CacheVersion || IndexMap.Size() < (2*sizeof(GPTYPE)))
	    {
	      message_log (LOG_ERROR, "Key/Index Cache version mis-match (!%d)!", CacheVersion);
	      IndexMap.Unmap();
	      GpIndex = NULL;
	      KeyIndex = NULL;
	      KeySortTable = NULL; 
	      useIndexMap = false;
	      reBuildIndexCache = true;
	      goto done;
	    }
	  else
	    {
	      GPTYPE GpTotal;
	      memcpy((void *)&GpTotal, ptr+sizeof(GPTYPE), sizeof(GPTYPE));
	      if (IndexMap.Size() >= (2*sizeof(GPTYPE)+GpTotal*(sizeof(GPREC)+sizeof(KEYREC))))
		{
		  GpIndex  =(GPREC *) (2*sizeof(GPTYPE) + ptr);
		  KeyIndex =(KEYREC *)(2*sizeof(GPTYPE) + ptr + GpTotal*sizeof(GPREC));
		  KeySortTable = (KEYSORT *)((BYTE *)KeyIndex + GpTotal*sizeof(KEYREC));
		  useIndexMap = ((TotalEntries = GpTotal) != 0);
		}
	      else // ERROR
		{
		  if (GpTotal || TotalEntries)
		    message_log (LOG_WARN, "Key/Index cache defective");
		  IndexMap.Unmap();
		  GpIndex   = NULL;
		  KeyIndex  = NULL;
		  KeySortTable = NULL;
		  useIndexMap = false;
		}
	  }
	}
      else
	{
	  useIndexMap = false;
	}
    }
  message_log (LOG_DEBUG, "Index %sMapped, MDT %sMapped", useIndexMap ? "" : "not ", useMdtMap ? "" : "not ");

if (!useIndexMap) {
  // Load Gp Index
  int fd;

  if ((fd = open (MdtIndexName, O_RDONLY)) != -1)
    {
      struct stat s;
      GPTYPE version;
#ifdef _WIN32
      setmode(fd, O_BINARY);
#endif

#pragma GCC diagnostic ignored "-Wunused-result"
      read(fd, &version, sizeof(GPTYPE));
      read(fd, &realTotal, sizeof(GPTYPE));
      realTotal = HTONL(realTotal);

      if (fstat(fd, &s) != -1)
	{
	  TotalEntries = (size_t)((s.st_size - 2*sizeof(GPTYPE))/ ( sizeof(GPREC)+sizeof(KEYREC) + sizeof(KEYSORT)));
	}
      if (version != CacheVersion)
	message_log (LOG_WARN, "MDT: Version of '%s' %d!=%d!", MdtIndexName.c_str(),
		version, CacheVersion);

      if (TotalEntries != (size_t)realTotal)
	{
	  long diff = realTotal - TotalEntries;
	  if (diff < 0) diff *= -1;
	  message_log (LOG_ERROR, "MDT: Format error on Gp/Key cache file '%s' %ld!=%ld [%ld %s] (recoverable)",
		MdtIndexName.c_str(), TotalEntries, realTotal, diff, realTotal > TotalEntries ? "missing" : "excess");
	  TotalEntries = 0;
	}
      else
	TotalEntries = (size_t)realTotal;

      if (TotalEntries)
	{
	  try {
	    GpIndex = new GPREC[TotalEntries];
	  } catch (...) {
	    message_log (LOG_PANIC, "Can't allocate %ld bytes for Gp cache (recoverable)", TotalEntries*sizeof(GPREC) );
	    GpIndex = NULL;
	    TotalEntries = 0;
	  }

	  size_t bytes = sizeof(GPREC)*TotalEntries;
	  if (GpIndex && (bytes != (size_t)_sys_read (fd, GpIndex, bytes)))
	    {
	      // Error
	      TotalEntries = 0;
	      message_log (LOG_ERROR, "Read error on Gp cache file (recoverable)");
	    }
	  // Load Key Index
	  if (TotalEntries)
	    {
	      try {
		KeyIndex = new KEYREC[TotalEntries];
		KeySortTable = new KEYSORT[TotalEntries];
	      } catch (...) {
		message_log (LOG_PANIC, "Can't allocate memory for %ld element key cache and/or key sort table (recoverable)", TotalEntries);
		KeyIndex = NULL;
		KeySortTable = NULL;
		TotalEntries = 0;
	      }
	      bytes = sizeof(KEYREC)*TotalEntries;

	      if (KeyIndex && (bytes != (size_t)_sys_read (fd, (void *)KeyIndex, bytes)))
		{
		  // Error
		  message_log (LOG_ERROR, "Read error on key cache file (recoverable)");
		  TotalEntries = 0;
		}
	     bytes = sizeof(KEYSORT)*TotalEntries;
	      if (KeySortTable && (bytes != (size_t)_sys_read (fd, (void *)KeyIndex, bytes)))
		{
		  message_log (LOG_ERROR, "Read error on key sort table");
		}
	    }
	  close(fd);
	  if (TotalEntries == 0)
	    {
	      if (GpIndex)
		{
		  delete[] GpIndex; GpIndex = NULL;
		}
	      if (KeyIndex)
		{
		  delete[] KeyIndex; KeyIndex = NULL;
		}
	      if (KeySortTable)
		{
		  delete[] KeySortTable; KeySortTable = NULL;
		}
	    }
	}
      else
	{
	  close (fd);
	}
    }

  if (TotalEntries == 0)
    reBuildIndexCache = true;
}

done:
  // Open on-disk MDT Stream
  ReadOnly = false;
  if ((MdtFp = fopen (MdtName, "r+b")) == NULL)
    {
      if ((MdtFp = fopen (MdtName, "w+b")) != NULL)
	{
	  WriteHeader();
	  fclose (MdtFp);
	  MdtFp = fopen (MdtName, "r+b");
	}
      else
	{
	  MdtFp = fopen (MdtName, "rb");
	  ReadOnly = true;
	}
    }
  if (MdtFp != NULL)
    {
      message_log (LOG_DEBUG, "MDT %s -> %d", MdtName.c_str(), fileno(MdtFp));
      ReadTimestamp(); // Get timestamp
    }
  else
    {
#if 1 
     // Need to define in the init and a member FPT * = Index->Parent->MainFpt
      if (errno == EMFILE && try_open_stream) 
	{
	  if (Fpt != NULL)
	    {
	      message_log (LOG_INFO, "Insufficient file/stream handles in O/S");
	      Fpt->CloseAll();
	      try_open_stream = false;
	      goto done;
	    }
	}
#endif
      message_log (LOG_FATAL|LOG_ERRNO, "Could not create/open %s (MDT)", MdtName.c_str());
    }

  if (reBuildIndexCache)
    {
      TotalEntries = (size_t)realTotal;
      RebuildIndex();
    }

#if USE_MDTHASHTABLE
  if (MDTHashTable != NULL)
    {
      message_log (LOG_ERROR, "MDT Hash Table was already inited in '%s'", MDTHashTable->Filename().c_str());
      delete MDTHashTable;
    }
  try {
    MDTHashTable = new MDTHASHTABLE( FileStem, fastAdd ); 
  } catch (...) {
    message_log (LOG_PANIC|LOG_ERRNO, "Can't allocate multiple document strings table!");
    MDTHashTable = NULL;
  }
  if (_globalMDTHashTable == NULL)
    _globalMDTHashTable = MDTHashTable;
#endif

  Changed = false;
  KeyIndexSorted = true;
  GpIndexSorted = true;
  MaxEntries = TotalEntries;
  lastKeyIndex = lastIndex = TotalEntries/2;
}

bool MDT::RebuildIndex()
{
  if (!Ok())
    {
      if (useIndexMap)
	{
          IndexMap.Unmap();
	  useIndexMap = false;
	  KeySortTable = NULL;
        }
      return false;
    }
  else if (TotalEntries)
    {
      message_log (LOG_NOTICE, "Rebuilding Key/Gp indexes..");
      // Rebuild by reading MDT
      MDTREC Mdtrec;
      if (useIndexMap)
	{
	  IndexMap.Unmap();
	  useIndexMap = false;
	  KeySortTable = NULL;
	}
      else
	{
	  if (KeyIndex)     { delete[]KeyIndex;     KeyIndex = NULL; }
	  if (GpIndex)      { delete[]GpIndex;      GpIndex  = NULL; }
          if (KeySortTable) { delete[]KeySortTable; KeySortTable = NULL; }
	}
     
      try {
	GpIndex = new GPREC[TotalEntries];
	KeyIndex = new KEYREC[TotalEntries];
      } catch (...) {
	message_log (LOG_PANIC|LOG_ERRNO,
		"MDT: Gp Index memory allocation for %ld elements failed.", TotalEntries) ;
	return false;
      }

      // The following loop is safe for running in threads.
#pragma omp parallel for 
      for (_index_id_t i = 0; i < TotalEntries; i++)
	{
	  // Gp Index
	  GetEntry(i+1, &Mdtrec);
	  GpIndex[i].GpStart = HTONL(Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordStart ());
	  GpIndex[i].GpEnd   = HTONL(Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordEnd ());
	  GpIndex[i].Date    = Mdtrec.GetDate();	
	  _index_id_t  index = i+1; // INDEX Increment
	  if (Mdtrec.GetDeleted())
	    SET_DELETE_BITS(index);
	  GpIndex[i].Index   = HTONL(index);

#define SET_KEYINDEX_KEY(_slot, _key) \
	KeyIndex[_slot].Hash = KeyHash32 ((const char *)memcpy(KeyIndex[_slot].Key, _key, DocumentKeySize));

	  // Key Index: Set the key, caculate its hash
	  SET_KEYINDEX_KEY(i, Mdtrec.Key);
	  KeyIndex[i].Index  = index;
	}
    }

  return BuildKeySortTable() ;
}


INT MDT::GetIndexNum() const
{
  INT2 num = 0;

  if (TotalEntries > 0 && MdtFp)
    {
      if (fseek(MdtFp, 4, SEEK_SET) != -1)
	{
	  ::Read(&num, MdtFp);
	}
    }
  return num >= 0 ? num : 0;
}

bool MDT::SetIndexNum(INT Num) const
{
  if (MdtFp== NULL)
    {
      message_log(LOG_ERROR, "MDT:: Can't set index number, stream NIL?");
    }
  else if (Num >= 0)
    {
      if (fseek(MdtFp, 4, SEEK_SET) != -1)
	{
	  const INT2 number = (INT2)(Num & 0xFFFF);
	  ::Write(number, MdtFp);
	  fflush(MdtFp);
	  return true;
	}
      else
	message_log (LOG_ERRNO, "Could not seek to index byte in MDT (fd=%d)", fileno(MdtFp));
    }
  return false;
}

void MDT::WriteTimestamp()
{
  if (Changed)
    {
      time_t Now = time((time_t *)NULL);
      Timestamp.Set(&Now);
      message_log (LOG_DEBUG, "Set timestamp to: %s", Timestamp.ISOdate().c_str());
      if (MdtFp && !ReadOnly)
	{
	  if (fseek(MdtFp, 12, SEEK_SET) != -1)
	    ::Write((UINT4)Now, MdtFp);
	  else
	    message_log (LOG_ERRNO, "Could not write timestamp (%s)", Timestamp.ISOdate().c_str());
	}
    }
}


void MDT::ReadTimestamp()
{
  if (MdtFp)
    {
      // Seek 12 bytes
      if (fseek(MdtFp, 12, SEEK_SET) != -1)
	{
	  SRCH_DATE newTimestamp;
	  UINT4 x = 0;
	  ::Read(&x, MdtFp);
	  time_t timeval = x;
	  if (timeval == 0) /* Old Indexes */
	    newTimestamp.SetTimeOfFile(MdtFp);
	  else
	    newTimestamp.Set(&timeval);
	  if (!Timestamp.Ok() || (newTimestamp > Timestamp))
	    Timestamp = newTimestamp;
	  return;
	}
    }
  Timestamp.SetNow(); // Set the timestamp to now..
}

/*
Byte 0-1  is the number of .idx and .sis
Byte 2-12 are Magic header information
Byte 13-16 are reserved
*/

void MDT::WriteHeader() const
{
  if (MdtFp)
    {
      int num;

      if ((num = (TotalEntries ? GetIndexNum() : 0)) < 0)
	num = 0;
      
      if (fseek(MdtFp, 0, SEEK_SET) != -1)
	{
	  fwrite(Magic, 4, sizeof(char),  MdtFp);
/* 4 */	  ::Write((INT2)num,              MdtFp);
/* 6 */	  ::Write((UINT2)(sizeof(MDTRECORD)),MdtFp);
/* 8 */	  ::Write((UCHR)DocumentKeySize,  MdtFp);
/* 9 */	  ::Write((UCHR)DocumentTypeSize, MdtFp);
#if USE_MDTHASHTABLE
/*10 */   ::Write((UINT2)0,               MdtFp); // Reserved
#else
/*10 */	  ::Write((UINT2)MaxDocPathNameSize, MdtFp);
#endif
/*12 */	  ::Write((UINT4)time((time_t *)NULL), MdtFp); // time 
	}
      else
	message_log (LOG_ERRNO, "Can't write MDT Header");
    }
  else message_log (LOG_ERROR, "Can't write MDT Header: MDT not opened!");
}

bool MDT::Ok() const
{
  if (MdtFp && TotalEntries)
    {
      if (fseek(MdtFp, 0, SEEK_SET) != -1)
	{
	  UINT2 val;
 	  UCHR ch;
	  char  tmp[5];
#pragma GCC diagnostic ignored "-Wunused-result"
	  fread(tmp, 4, sizeof(char), MdtFp);
	  if (memcmp(tmp, Magic, 4) != 0)
	    return false;
	  ::Read(&val, MdtFp); // Number of indexes
	  ::Read(&val, MdtFp);
	  if (val != sizeof(MDTRECORD))
	    {
	      message_log (LOG_NOTICE, "MDT's records are not compatible with this version.");
	      return false;
	    }
	  ::Read(&ch, MdtFp);
	  if (ch != (UCHR)DocumentKeySize)
	    return false;
	  ::Read(&ch, MdtFp);
	  if (ch != (UCHR)DocumentTypeSize)
	    return false;
#if! USE_MDTHASHTABLE
	  ::Read(&val, MdtFp);
	  if (val != (UINT2)MaxDocPathNameSize)
	    return false;
#endif
	}
    }
  return true;
}

bool MDT::IsSystemFile (const STRING& Filename)
{
  return ((MdtName == Filename) || (MdtIndexName == Filename));
}

size_t MDT::AddEntry (const MDTREC& MdtRecord)
{
  if (ReadOnly == true)
    {
      return 0;
    }
  if (TotalEntries >=  MdtIndexCapacity)
    {
      message_log (LOG_PANIC, "MDT Capacity of %lu records has been exceeded!",  MdtIndexCapacity);
      return 0;
    }
//if (TotalEntries == 0) WriteHeader();
  if (TotalEntries == MaxEntries)
    {
      Resize (GROWTH_FACTOR(MaxEntries));
    }

  size_t Slot = TotalEntries;
  bool unique = true;
  // Add to Key Index
  const char *key =  MdtRecord.Key;

  if (KeyIndexSorted && TotalEntries)
    {
      if (strncmp(key, KeyIndex[TotalEntries-1].Key, DocumentKeySize) < 0)
        {
	  size_t left = TotalEntries;
	  if (TotalEntries == 1)
	    {
	      left = 0;
	    }
          else for (size_t i = (left-1)/2, oip, low = 0, high = left-1;;)
            {
	      int res;
              /* binary search */
	      oip = i;
              if ((res  = strncmp(key, KeyIndex[i].Key, DocumentKeySize)) == 0)
		{
		  const STRING Key(key, DocumentKeySize);
		  // Change Key....
		  STRING newKey (Key);
		  MDTREC mdtrec (MdtRecord);
		  GetUniqueKey (&newKey, false);
		  mdtrec.SetKey(newKey);
		  message_log (LOG_ERROR, "Duplicate Key \"%s\" (found at %d). Setting key to \"%s\".",
			Key.c_str(), i, newKey.c_str());
		  return AddEntry (mdtrec);
		  // left = i; break;
		}
	      else if (res < 0)
                high = i;
              else
                low = i;
              if (high - low <= 0 || ( i= (high + low) / 2) == oip)
                {
                  left = high;
                  break;
                }
            }
	 size_t rest = TotalEntries - left;
	 Slot = left;
         if (rest)
            memmove((void *)&KeyIndex[Slot+1], (void *)&KeyIndex[Slot], rest*sizeof (KEYREC));
        }
    }
  if (unique)
    {
      // Set the key
      SET_KEYINDEX_KEY(Slot, key);
    }

  // Set the index..
  GPTYPE index = TotalEntries + 1; // INDEX Increment 
  if (MdtRecord.GetDeleted())
    SET_DELETE_BITS(index);
  KeyIndex[Slot].Index  = HTONL ( index );

  const GPTYPE oldGlobalGp = NextGlobalGp;
  // Add to Gp Index
  NextGlobalGp = MdtRecord.GetGlobalFileStart ();

  // Here is where the GpIndex gets its additional value:
  GpIndex[TotalEntries].Date    = MdtRecord.GetDate();
  GpIndex[TotalEntries].GpStart = HTONL(NextGlobalGp + MdtRecord.GetLocalRecordStart ());

  NextGlobalGp += MdtRecord.GetLocalRecordEnd ();
  GpIndex[TotalEntries].GpEnd   = HTONL(NextGlobalGp);
  NextGlobalGp++; // Increment

  // Have we exceeded the address space of GPTYPE?
  if (NextGlobalGp < oldGlobalGp)
    {
      message_log (LOG_FATAL|LOG_PANIC, "Physical database capacity exceeded (max %lu MB).",
	MAX_GPTYPE/(1024L*1024L));
    }
  else if (NextGlobalGp > (MAX_GPTYPE -  1048576 /*(2^20)*/))
    {
      message_log (LOG_WARN, "Physical database capacity nearly reached (%luK, max %lu MB).",
	NextGlobalGp/1024L, MAX_GPTYPE/(1024L*1024L));
    }

  GpIndex[TotalEntries].Index   = HTONL(index);
  if (GpIndexSorted && TotalEntries > 1)
    {
      if (GpIndex[TotalEntries-1].GpStart < GpIndex[TotalEntries-2].GpStart)
        {
          GpIndexSorted = false;
        }
    }
  // Add to on-disk MDT
  if (MdtWrongEndian)
    {
      MDTREC TempMdtrec;
      TempMdtrec = MdtRecord;
      TempMdtrec.FlipBytes ();
      TempMdtrec.Write(MdtFp, ++TotalEntries);
    }
  else
    {
      MdtRecord.Write(MdtFp, ++TotalEntries);
    }
  Changed = true;
  return TotalEntries;
}


STRING MDT::GetKey (const size_t Index, int *Hash) const
{
  STRING Key;
  if (KeyIndex)
    {
      size_t Position = KeySortPosition(Index);
      if (KeyIndex[Position].Index != Index)
	message_log (LOG_PANIC, "MDT::GetKey() table glitch"); 
      Key = STRING(KeyIndex[Position].Key, DocumentKeySize);
      if (Hash) *Hash = KeyIndex[Position].Hash;
#if 0
      MDTREC Mdtrec;
      GetEntry(Index, &Mdtrec);
      if (Key != Mdtrec.GetKey())
	cerr << "Ooooooops!!!!!!" << endl;
#endif
    }
  return Key;
}



static int MdtCompareKeysByIndex (const void *KeyRecPtr1, const void *KeyRecPtr2)
{
  const GPTYPE index1 = KeyRecPtr1 ? NTOHL(((KEYREC *) KeyRecPtr1)->Index) : 0;
  const GPTYPE index2 = KeyRecPtr2 ? NTOHL(((KEYREC *) KeyRecPtr2)->Index) : 0;
  return  INDEX_MASK(index1) -   INDEX_MASK(index2);
}

static int MdtCompareGpByIndex (const void *GpRecPtr1, const void *GpRecPtr2)
{
  const GPTYPE index1 =  GpRecPtr1 ? NTOHL(((GPREC *) GpRecPtr1)->Index) : 0;
  const GPTYPE index2 =  GpRecPtr2 ? NTOHL(((GPREC *) GpRecPtr2)->Index) : 0;
  return  INDEX_MASK(index1) -  INDEX_MASK(index2);
}

void MDT::IndexSortByIndex ()
{
#if 1 /* 05.2024 tweaks */
  if (KeyIndex && !KeyIndexSorted && useIndexMap)
    {
      QSORT (KeyIndex, TotalEntries, sizeof (KEYREC), MdtCompareKeysByIndex);
      KeyIndexSorted = Changed = true;
    }
  if (GpIndex && !GpIndexSorted)
    {
      QSORT (GpIndex, TotalEntries, sizeof (GPREC), MdtCompareGpByIndex);
      GpIndexSorted = Changed = true;
    }
#else
  if (KeyIndex)
    {
      if (useIndexMap)
        qsort (KeyIndex, TotalEntries, sizeof (KEYREC), MdtCompareKeysByIndex);
      KeyIndexSorted = false;
    }
  if (GpIndex)
    {
      qsort (GpIndex, TotalEntries, sizeof (GPREC), MdtCompareGpByIndex);
      GpIndexSorted = false;
    }
  Changed = true;
#endif
}



size_t MDT::GetTotalDeleted() const
{
  size_t count = 0;
  for (size_t x=1; x<=TotalEntries; x++)
    {
      if (IsDeleted(x))
	count++;
    } 
  return count;
}

size_t MDT::RemoveDeleted ()
{
  size_t delcount = 0;

#if 1
  if (ReadOnly == false)
    {
      int    ready_to_modify = 0;
      size_t n = 1;
      MDTREC Mdtrec;

      for (size_t x = 1; x <= TotalEntries; x++)
	{
	  if (GetEntry (x, &Mdtrec) && Mdtrec.GetDeleted () == false)
	    {
	      if (x != n)
		{
		  if (!ready_to_modify)
		    {
		      if (useIndexMap) RebuildIndex();
		      else IndexSortByIndex ();
		      ready_to_modify = 1;
		    }
		  if (MdtWrongEndian) Mdtrec.FlipBytes ();
		  if (Mdtrec.Write(MdtFp, n) == false)
		    message_log (LOG_ERROR|LOG_ERRNO, "RemoveDeleted: Could not write to MDT");
		  KeyIndex[n - 1] = KeyIndex[x - 1];
		  KeyIndex[n - 1].Index = n;
		  GpIndex[n - 1] = GpIndex[x - 1];
		  GpIndex[n - 1].Index = n;
		}
	      n++;
	    }
	  else
	    delcount++;
	}
      if (delcount)
	{
	  // Clear the rest memory
	  for (size_t x=n-1; x < TotalEntries; x++)
	    {
	      KeyIndex[x].Clear();
	      GpIndex[x].Clear();
	    }
	  if (ftruncate (fileno (MdtFp), (TotalEntries = n - 1) * sizeof (MDTRECORD) + SIZEOF_MAGIC) == -1)
	    {
	      message_log (LOG_ERROR|LOG_ERRNO, "Could not truncated MDT to %d entries", TotalEntries);
	    }
	  NextGlobalGp = 0;
	  Changed = true;
	}
    }
#endif
  return delcount;
}

bool MDT::GetEntry (const size_t Index, MDTREC* MdtrecPtr) const
{
  if (MdtFp == NULL)
    message_log (LOG_PANIC, "MdtFp is NULL!");

  if ((Index > 0) && (Index <= TotalEntries))
    {
      message_log (LOG_DEBUG, "GetEntry(%d,..) from fd=%d", fileno(MdtFp));
      if (useMdtMap)
	{
	  *MdtrecPtr = MdtIndex[Index - 1];
	}
      else if (MdtrecPtr->Read(MdtFp, Index) == false)
	{
	  return false;
	}
      if (MdtWrongEndian)
	MdtrecPtr->FlipBytes ();
      MdtrecPtr->HashTable = MDTHashTable;
      return true;
    }
  message_log (LOG_PANIC, "MDT::GetEntry Index=%ld : Out of bounds (1,%ld)", (long)Index, (long)TotalEntries);
  return false;
}

MDTREC *MDT::GetEntry(const size_t Index)
{
  if (GetEntry(Index, &tmpMdtrec) == true)
    return &tmpMdtrec;
  return NULL;
}


bool MDT::SetDeleted(const size_t Index, bool Delete)
{
  if (!ReadOnly && Index <= TotalEntries)
    {
      MDTREC mdtrec;

      if (GetEntry(Index, &mdtrec))
	{
	  // A request to change anything?
	  if (mdtrec.GetDeleted() != Delete)
	    {
	      // YES..
	      mdtrec.SetDeleted(Delete);
	      mdtrec.Write(MdtFp, Index);
	      message_log (LOG_INFO, "MDT::SetDeleted: Entry #%d %sdeleted", Index, Delete ? "" : "un");
#if 1 /* EXPERIMENTAL April 2008 */
	      if (useIndexMap || useMdtMap)
		Resize(TotalEntries + 1); // To unmap
	      if (GpIndex)
		{
		  _index_id_t  index = Index;
		  if (Delete) SET_DELETE_BITS(index);
		  GpIndex[Index].Index = HTONL(index);
		}
	      else
#endif
	      // Don't want to build KeySortTable
	      Changed = true; // ADDED 2006 August 18
	    }
	  else
	    message_log (LOG_INFO, "MDT::SetDeleted: Entry #%d already %sdeleted", Index, Delete ? "" : "un");
	  return true;
	}
      message_log (LOG_ERROR, "MDT::SetDeleted failed: Index %d not available", Index);
    }
  else if (!ReadOnly)
    message_log (LOG_ERROR, "MDT::SetDeleted failed: MDT Index %d OUT-OF-RANGE (>%d)!", Index, TotalEntries);
  return false;
}

bool MDT::IsDeleted(const size_t Index) const
{
  if ((Index > 0) && (Index <= TotalEntries))
    {
      if (!useMdtMap)
	{
	  MDTREC Mdtrec;
	  return Mdtrec.IsDeleted(MdtFp, (INT)Index);
	}
      return MdtIndex[Index - 1].GetDeleted();
   }
  return true; // Non-existant records are considered deleted....
}


void MDT::SetEntry (const size_t Index, const MDTREC& MdtRecord)
{
  // TODO:
  //    Handle when MdtRecord is deleted!
  //
  if (ReadOnly == true)
    {
      return;
    }
  if ((Index > 0) && (Index <= TotalEntries))
    {
      // Save on-disk record
      if (MdtWrongEndian)
	{
	  MDTREC TempMdtrec;
	  TempMdtrec = MdtRecord;
	  TempMdtrec.FlipBytes ();
	  TempMdtrec.Write(MdtFp, Index);
	}
      else
	{
	  MdtRecord.Write(MdtFp, Index);
	}
      size_t x;
      // Update Key Index
      for (x = 0; x < TotalEntries; x++)
	{
	  if ( INDEX_MASK( NTOHL(KeyIndex[x].Index) ) == (GPTYPE)Index )
	    {
	      // New value for key?
	      if (strncmp(KeyIndex[x].Key, MdtRecord.Key, DocumentKeySize) != 0)
		{
		  // Yes.. Set the key..
		  SET_KEYINDEX_KEY(x, MdtRecord.Key)
		  Changed = true;
		  if (KeyIndexSorted)
		    {
		      // Check if the sort order is still OK..
		      if (x > 1 && (strncmp(KeyIndex[x].Key, KeyIndex[x-1].Key, DocumentKeySize) < 0)) {
			KeyIndexSorted = false;
		      } else if (x < (TotalEntries-1) && (strncmp(KeyIndex[x+1].Key, KeyIndex[x].Key, DocumentKeySize) < 0)) {
			KeyIndexSorted = false;
		      }
		    }
		}
	      break;
	    }
	}
      // Update Gp Index
      for (x = 0; x < TotalEntries; x++)
	{
	  if ( INDEX_MASK(GpIndex[x].Index) == (GPTYPE)Index)
	    {
	      const GPTYPE GpStart = MdtRecord.GetGlobalFileStart () + MdtRecord.GetLocalRecordStart ();
	      const GPTYPE GpEnd   = MdtRecord.GetGlobalFileStart () + MdtRecord.GetLocalRecordEnd ();
	      if (GpIndexSorted)
		{
		  if (x > 1 && (GpStart < NTOHL(GpIndex[x-1].GpStart)))
		    GpIndexSorted = false;
		  else if (x < (TotalEntries-1) && (NTOHL(GpIndex[x+1].GpStart) < GpStart))
		    GpIndexSorted = false;
		}
	      // Same numbers?
	      const GPTYPE host_start = HTONL(GpStart);
              const GPTYPE host_end   = HTONL(GpEnd);

	      if (Changed == false)
		{
		  Changed = (GpIndex[x].GpStart != host_start || GpIndex[x].GpEnd != host_end);
		}
	      if (Changed)
		{
		  GpIndex[x].GpStart = host_start;
		  GpIndex[x].GpEnd   = host_end; 
		  GpIndex[x].Date    = MdtRecord.GetDate();
		  SET_DELETE_STATE(GpIndex[x].Index, MdtRecord.GetDeleted());
		}
	      break;
	    }
	}
    }
  if (Index == TotalEntries)
    {
      NextGlobalGp = _NextGlobal(MdtRecord);
    }
}

static int MdtCompareKeys (const void *KeyRecPtr1, const void *KeyRecPtr2)
{
  return strncmp ((((KEYREC *) KeyRecPtr1)->Key), (((KEYREC *) KeyRecPtr2)->Key), DocumentKeySize);
}

void MDT::SortKeyIndex ()
{
  if (KeyIndexSorted == false && KeyIndex)
    {
      if (TotalEntries > 1)
	{
	  QSORT(KeyIndex, TotalEntries, sizeof (KEYREC), MdtCompareKeys);
	  Changed = true;
	}
      KeyIndexSorted = true;
    }
}

void MDT::Resize (const size_t Entries)
{
  if (Entries > TotalEntries)
    {
      if (Entries > MdtIndexCapacity)
	message_log (LOG_WARN, "MDT Capacity is %lu records in this version.",  MdtIndexCapacity);
      // Resize Key Index
      KEYREC *OldKeyIndex = KeyIndex;
      try {
	KeyIndex = new KEYREC[Entries];
      } catch (...) {
	message_log (LOG_ERRNO, "Memory re-alloc failed for %ld KEYRECS", Entries);
	KeyIndex = NULL;
      }
      if (OldKeyIndex)
	{
	  if (KeyIndex)
	    memcpy (KeyIndex, OldKeyIndex, TotalEntries * sizeof (KEYREC));
	  if (!useIndexMap) delete[]OldKeyIndex;
	}
      // Add to Gp Index
      GPREC *OldGpIndex = GpIndex;
      try {
	GpIndex = new GPREC[Entries];
      } catch(...) {
	message_log (LOG_ERRNO, "Memory re-alloc failed for %ld GPRECS", Entries);
	if (KeyIndex)
	  {
	    delete[] KeyIndex;
	    KeyIndex = NULL;
	  }
	GpIndex = NULL;
      }
      if (OldGpIndex)
	{
	  if (GpIndex)
	    memcpy (GpIndex, OldGpIndex, TotalEntries * sizeof (GPREC));
	  if (!useIndexMap) delete[]OldGpIndex;
	}
      if (GpIndex == NULL)
	MaxEntries = TotalEntries = 0;
      MaxEntries = Entries;
      if (useIndexMap || useMdtMap)
	{
	  useIndexMap = false;
	  useMdtMap = false;
	  IndexMap.Unmap();
	  MdtMap.Unmap();
	  KeySortTable = NULL; // This one is important!
	}
    }
  else if (Entries == 0)
    {
      MaxEntries = TotalEntries = 0;
      if (useIndexMap)
	{
	  useIndexMap = false;
	  IndexMap.Unmap();
	  KeyIndex = NULL;
	  GpIndex = NULL;
	  KeySortTable = NULL;
	}
      if (useMdtMap)
	{
	  useMdtMap = false;
	  MdtMap.Unmap();
	}
    }
}

#if 0


static int MdtCompareKeys (const void *KeyRecPtr1, const void *KeyRecPtr2)
{
  STRING Ref (((KEYREC *) KeyRecPtr1)->Key);
  return strncmp (Ref, (((KEYREC *) KeyRecPtr2)->Key), Ref->GetLength());
}

size_t* MDT::LookupByKeys (const STRING& Key)
{
  const size_t kLen = Key.Length();
  if (!Key.IsWild() || (kLen >  DocumentKeySize) || kLen == 0)
    {
       size_t i = LookupByKey(Key);
       return i;
    }
  STRINGINDEX i = Key.Search("?");
  if (i > 0) Key.EraseAfter(i);
  if ((i = Key.Search("*")) > 0) Key.EraseAfter(i);

  if (TotalEntries)
    {
      if (KeyIndexSorted == false)
        {
          diff = 0;
          SortKeyIndex ();
        }
      const size_t offset = diff > 0 ? lastKeyIndex : 0;
      if (offset != TotalEntries)
        {
          KEYREC KeyRec;
          Key.GetCString (KeyRec.Key, DocumentKeySize);
          const KEYREC *KeyRecPtr= (const KEYREC *)bsearch (&KeyRec, KeyIndex+offset,
                TotalEntries-offset, sizeof (KEYREC), MdtCompareFirstKeys);
          if (KeyRecPtr)
            {
              lastKeyIndex = KeyRecPtr - KeyIndex; // 2007.10 @@@ not /sizeof(KEYREC)
              return INDEX_MASK ( NTOHL(KeyRecPtr->Index) );
            }
        }
    }
  return 0; // Not Found
}



#endif

size_t MDT::LookupByKey (const STRING& Key)
{
  if (TotalEntries)
    {
      INT diff = lastKeyIndex < TotalEntries ? Key.Compare(KeyIndex[lastKeyIndex].Key, DocumentKeySize) : 0;
      if (diff == 0)
	return INDEX_MASK ( NTOHL(KeyIndex[lastKeyIndex].Index) );

      const size_t kLen = Key.Length();
      if (kLen == 0)
	{
	  message_log (LOG_DEBUG, "MDT::LookupByKey: Zero-length Key lookup?");
	  return 0;
	}
      if (kLen > DocumentKeySize)
	{
#if 1
	  // Search using the Right side of Key
	  return LookupByKey(Key.Right(DocumentKeySize));
#else
          // Search using a truncated key
	  return LookupByKey(STRING(Key).EraseAfterNul(DocumentKeySize));
#endif
	}

      if (KeyIndexSorted == false)
	{
	  diff = 0;
          SortKeyIndex ();
	}
      const size_t offset = diff > 0 ? lastKeyIndex : 0;
      if (offset != TotalEntries)
	{
	  KEYREC KeyRec;
	  Key.GetCString (KeyRec.Key, DocumentKeySize);
	  const KEYREC *KeyRecPtr= (const KEYREC *)bsearch (&KeyRec, KeyIndex+offset,
		TotalEntries-offset, sizeof (KEYREC), MdtCompareKeys);
	  if (KeyRecPtr)
	    {
	      lastKeyIndex = (KeyRecPtr - KeyIndex); // @@@@ 2007.10 not /sizeof(KEYREC)
	      return INDEX_MASK ( NTOHL(KeyRecPtr->Index) );
	    }
	}
    }
  return 0; // Not Found
}

GPTYPE MDT::GetNameByGlobal(GPTYPE gp, STRING *Path, GPTYPE *Size, GPTYPE *LS, DOCTYPE_ID *Doctype)
{
   GPTYPE          Start = 0;
   MDTREC          Mdtrec;

  if (GetMdtRecord (gp, &Mdtrec))
    {
      if (Path)    *Path    = Mdtrec.GetFullFileName();
      if (Doctype) *Doctype = Mdtrec.GetDocumentType();
      if (Size)    *Size    = Mdtrec.GetLocalRecordEnd() - Mdtrec.GetLocalRecordStart() + 1;
      if (LS)      *LS      = Mdtrec.GetLocalRecordStart();
      Start = Mdtrec.GetGlobalFileStart() + Mdtrec.GetLocalRecordStart();
   } else
     message_log(LOG_ERROR, "Lookup failed for %ld", (long) gp);
   return (Start);
}

size_t MDT::GetMdtRecord (const STRING& Key, MDTREC *MdtrecPtr)
{
  const size_t x = LookupByKey (Key);
  if (x == 0)
    *MdtrecPtr = MDTREC(this);
  else if (GetEntry (x, MdtrecPtr))
    return x;
  return 0; // Nope
}

static int MdtCompareGpStarts (const void *GpRecPtr1, const void *GpRecPtr2)
{
  return NTOHL(((GPREC *) GpRecPtr1)->GpStart) - NTOHL(((GPREC *) GpRecPtr2)->GpStart);
}

static int MdtCompareGps (const void *GpPtr, const void *GpRecPtr)
{
  const GPTYPE Gp = NTOHL(*((GPTYPE *) GpPtr));

  if (Gp < NTOHL(((GPREC *) GpRecPtr)->GpStart))
    return -1;
  if (Gp <= NTOHL(((GPREC *) GpRecPtr)->GpEnd))
    return 0;
  return 1;
}


void MDT::SortGpIndex ()
{
  if (GpIndexSorted == false)
    {
      if (GpIndex)
	{
	  QSORT (GpIndex, TotalEntries, sizeof (GPREC), MdtCompareGpStarts);
	}
      GpIndexSorted = true;
      Changed = true;
    }
}

#if 0
size_t MDT::LookupByGp (const GPTYPE Gp, SRCH_DATE *Date)
{

}
#endif


size_t MDT::LookupByGp (const GPTYPE Gp, FC *Fc)
{
  errno = 0;
  if (TotalEntries)
    {
      size_t s_offset = 0;
      size_t e_offset = 0;

      if (lastIndex < TotalEntries)
	{
	  GPTYPE start = NTOHL(GpIndex[lastIndex].GpStart);
	  if (Gp >= start)
	    {
	      GPTYPE end = NTOHL(GpIndex[lastIndex].GpEnd);
	      if (Gp <= end)
		{
		  if (Fc)
		    {
		      Fc->SetFieldStart( start );
		      Fc->SetFieldEnd ( end );
		    }
		  if (DELETED_MASK( GpIndex[lastIndex].Index ))
		    {
		      errno = ENOENT;
		      return 0; // DELETED
		    }
		  return INDEX_MASK( GpIndex[lastIndex].Index );
		}
	      else if (lastIndex == TotalEntries)
		return 0; // Not found
#if 0
	      if (Gp <= (end = NTOHL(GpIndex[lastIndex+1].GpEnd)))
		{
		  if (Gp >= (start = NTOHL(GpIndex[lastIndex+1].GpStart)))
		    {
		      if (Fc)
			{
			  Fc->SetFieldStart( start );
			  Fc->SetFieldEnd ( end );
			}
		      return INDEX_MASK ( GpIndex[++lastIndex].Index );
		    }
		}
#endif
	      s_offset = e_offset = lastIndex; 
	    }
	  else if (lastIndex == 0)
	    {
	      return 0; // Before the first?
	    }
	  else
	    {
	      e_offset = TotalEntries - lastIndex; // Experimental
//cerr << "e_offset=" << e_offset << endl;
	    }
	}
      if (GpIndexSorted == false)
	{
          SortGpIndex ();
	  s_offset = e_offset = 0;
	}
      GPREC *GpRecPtr= (GPREC *) bsearch (&Gp, GpIndex+s_offset, TotalEntries-e_offset, sizeof (GPREC), MdtCompareGps);
      if (GpRecPtr)
	{
	  lastIndex = (GpRecPtr - GpIndex); // @@@ 2007.10:  not /sizeof(GPREC)
//cerr << "lastIndex=" << lastIndex << endl;
	  if (Fc)
	    {
	      Fc->SetFieldStart(NTOHL(GpRecPtr->GpStart));
	      Fc->SetFieldEnd (NTOHL(GpRecPtr->GpEnd));
	    }
	  if (DELETED_MASK( NTOHL(GpRecPtr->Index) ))
	    {
	      errno = ENOENT;
	      return 0; // DELETED
	    }
	  return INDEX_MASK ( NTOHL(GpRecPtr->Index) );
	}
#if 0
     cerr << "Could NOT FIND " << Gp << " Debug Dump..." << endl;
     Dump (0, cerr);
#endif
    }
  errno = EIDRM;
  return 0; // Not Found
}

size_t MDT::GetMdtRecord (const GPTYPE gp, MDTREC *MdtrecPtr)
{
  const size_t x = LookupByGp (gp);
  if (x == 0)
    {
      if (errno == EIDRM) message_log (LOG_PANIC, "Could not find GP %ld!", gp);
      *MdtrecPtr = MDTREC(this);
    }
  else if (GetEntry (x, MdtrecPtr))
    {
      return x;
    }
  return 0; // Nope
}

GPTYPE MDT::GetNextGlobal ()
{
  if (TotalEntries && NextGlobalGp == 0)
    {
      // Note: Could also use GPREC index
      MDTREC Mdtrec;
      if (GetEntry (TotalEntries, &Mdtrec))
        NextGlobalGp = _NextGlobal(Mdtrec);
    }
  return NextGlobalGp;
}

STRING& MDT::GetUniqueKey (PSTRING StringPtr, bool Override)
{
  STRING OldKey;
  size_t Index = 0;
  int    cut_len = 4;

  // Is it already Unique?
  if (!StringPtr->IsEmpty())
    {
      OldKey = *StringPtr;
      if ((Index = LookupByKey(*StringPtr)) == 0)
	return *StringPtr;
      STRINGINDEX x = StringPtr->SearchReverse(',');
      if (x > (StringPtr->GetLength() - 6) )
	StringPtr->EraseAfterNul(x-1);
      if ((StringPtr->GetLength() + cut_len) > DocumentKeySize)
	StringPtr->EraseAfterNul(DocumentKeySize-4);
    }

  // Not Unique

  STRING NewKey;
  INT x = 0, y = 0;
  // bit encoding..
  static const char digits[] ="zyxwvutsrqponmlkjihgfedcba_ZYXWVUTSRQPONMLKJIHGFEDCBA@9876543210";
  const int cut = (1<<(4*(cut_len-1))) - 1;
  do {
      if (y)
	{
	  if (y < (INT)(sizeof(digits)/sizeof(char)))
	    NewKey.form("%s,,%c%c", StringPtr->c_str(), digits[y], digits[x]);
	  else
	    NewKey.form("%x%s%c", y, StringPtr->c_str(), digits[x]);
	}
      else
	NewKey.form("%s,%03x", StringPtr->c_str(), cut-x); // lower case
      if (NewKey.GetLength() > DocumentKeySize)
	{
	  if (StringPtr->IsEmpty())
	    {
	      message_log (LOG_PANIC, "Can't caculate a unique key. Contact software support.");
	      break;
	    }
	  y = StringPtr->GetInt();
	  StringPtr->Clear();
	  continue;
	}
      if (y == 0 && x < cut)
	{
	  x++;
	}
      else if (++x >= (INT)(sizeof(digits)/sizeof(char)))
	{
	  x = 0;
	  y++;
	}
    } while (LookupByKey(NewKey));
  if (Override && Index)
    {
      MDTREC mdtrec;
      if (GetEntry (Index, &mdtrec))
	{
	  message_log (LOG_INFO, "Previous duplicate key '%s'%s set to '%s' and marking record deleted.",
		OldKey.c_str(),
		(OldKey.GetLength() > DocumentKeySize) ? "(truncated)" : "",
		NewKey.c_str());
	  mdtrec.SetKey(NewKey);
	  mdtrec.SetDeleted(true);
	  SetEntry (Index, mdtrec);
	  return *StringPtr;
	}
    }
  return *StringPtr = NewKey;  
}

void MDT::Dump (INT Skip, ostream& os) const
{
  STRING key;
  MDTREC Mdtrec;

  os << endl << "Total Entries in MDT: " << TotalEntries << endl;
  if (TotalEntries > (size_t)Skip)
    {
#ifdef SOLARIS
      os << "                  Record Key                     Global          Local      \tFile" << endl;
#else
      os << "          \tRecordKey\tGlobal\tLocal\tFile" << endl;
#endif
    }
  char tmp[64];
  for (size_t x = 1 + Skip; x <= TotalEntries; x++)
    {
      STRING  s0, s1;
      GetEntry (x, &Mdtrec);
      Mdtrec.GetKey (&key);
      sprintf(tmp, "%ld-%ld ",
	(long)(Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordStart ()),
	(long)(Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordEnd ()) );
#ifdef SOLARIS
      os << setw(16) << tmp <<  
	setw(DocumentKeySize - key.GetLength()) << setfill(' ') << key <<
	"0x" << setw(6) << setfill('0') << hex << Mdtrec.GetGlobalFileStart () <<
	"   0x" << setw(6) << setfill('0') << hex << Mdtrec.GetLocalRecordStart () <<
	"-0x" << setw(6) << setfill('0') << hex << Mdtrec.GetLocalRecordEnd () <<
	"\t\"" << '\"' << (s0 = Mdtrec.GetFileName ()) << '\"';
      if ((s1 = Mdtrec.GetOrigFileName()) != s0)
        os << " <orig: \"" << s1 << "\" >";

      if (Mdtrec.GetDeleted())
	os << " <deleted> ";
      os << setfill(' ') << endl;
#else
      os << tmp << '\t' << key << '\t' << Mdtrec.GetGlobalFileStart () << '\t' <<
	Mdtrec.GetLocalRecordStart () << "\t" << Mdtrec.GetLocalRecordEnd () << "\t\"" <<
	 (s0 = Mdtrec.GetFileName ()) << '\"';

      if ((s1 = Mdtrec.GetOrigFileName()) != s0)
	os << " <orig: \"" << s1 << "\" >";
      if (Mdtrec.GetDeleted())
	os << " <deleted> ";
      os << endl;
#endif
    }
}

bool MDT::KillAll()
{
  bool result = true;

  message_log (LOG_DEBUG, "MDT KillAll()");
  Resize(0);
  if (!ReadOnly && MdtFp) {
    if (EraseFileContents (MdtFp) == -1)
      {
        message_log (LOG_ERRNO, "KillAll: Could not truncate MDT to ZERO entries");
	result = false;
      }
  }
  Changed = false;
  if (UnlinkFile(MdtIndexName) != 0)
    {
      const STRING bak ( MdtIndexName+"~");
      if (RenameFile(MdtIndexName, bak) == 0)
	AddtoGarbageFileList(bak);
      else if ( EraseFileContents(MdtIndexName) != 0 && result)
	result = false;
    }
#if USE_MDTHASHTABLE
  if (MDTHashTable && MDTHashTable->KillAll() == false && result)
    result = false;
#endif

  // Added 26 Feb 2004 . Is this needed?????
  if (!useIndexMap)
    {
      if (KeyIndex)     delete[] KeyIndex;
      if (GpIndex)      delete[] GpIndex;
      if (KeySortTable) delete[] KeySortTable;
    }
  else
    {
      // Make sure!
      MdtMap.Unmap(); IndexMap.Unmap(); 
      useIndexMap = useMdtMap = false;
    }
  KeyIndex = NULL;
  GpIndex = NULL;
  KeySortTable = NULL;

  NextGlobalGp = 0; // Reset Count
  TotalEntries = 0;

  WriteTimestamp(); // New time
  return result;
}

void MDT::FlushMDTIndexes()
{
  message_log (LOG_DEBUG, "Flushing MDT...");

  if ((Changed == true) && (ReadOnly == false))
    {
/*
      if (MdtFp)
	{
	  fclose(MdtFp);
	  MdtFp = NULL;
	}
*/
#if 0
      if (KeyIndexSorted)
	{
	  message_log (LOG_DEBUG, "Debug Code: Checking KeyIndex Sort.");
	  for (size_t i=1; i < TotalEntries; i++)
	    {
	      if (strncmp(KeyIndex[i-1].Key, KeyIndex[i].Key, DocumentKeySize) > 0)
		{
		  message_log (LOG_ERROR, "KEY SORT ERROR. Contact edz@nonmonotonic.com!!!!");
		  KeyIndexSorted = false;
		}
	    }
	}
#endif
      // Sort indices
      if (GpIndexSorted == false) SortGpIndex ();
      if (KeyIndexSorted == false) SortKeyIndex ();

      BuildKeySortTable();

      int fd;
      // Save MDT lookup cache...
      if ((fd = open (MdtIndexName, O_WRONLY|O_CREAT, 0666)) != -1)
	{
	  // static_cast<void>(write(int, const void *, size_t));
#ifdef _WIN32
	  setmode(fd, O_BINARY);
#endif
	  message_log (LOG_DEBUG, "Writing MDT '%s' lookup cache v%ld %lu elements.",
			MdtIndexName.c_str(), (long)CacheVersion, (long)TotalEntries);
	  const GPTYPE total = (GPTYPE)HTONL(TotalEntries);
	  // Save Gp Index
#pragma GCC diagnostic ignored "-Wunused-result"
	  write(fd, (const void *)&CacheVersion, sizeof(GPTYPE));
	  write(fd, (const void *)&total, sizeof(GPTYPE));
	  write (fd, GpIndex, sizeof(GPREC)*TotalEntries);
	  // Save Key Index
	  write (fd, KeyIndex, sizeof(KEYREC)*TotalEntries);
	  // Now write the Key Sort
	  write (fd, KeySortTable, sizeof(KEYSORT)*TotalEntries);
	  close (fd);
	}
      WriteTimestamp();
      Changed = false;
    }
  else if ((MaxEntries == 0) && (Changed == false) && (ReadOnly == false))
    {
      message_log (LOG_DEBUG, "Removing MDT rests.");
      // did nothing so zap junk
      UnlinkFile(MdtName);
      UnlinkFile(MdtIndexName);
    }
}


MDT::~MDT ()
{
  WriteTimestamp();

  // Close MdtFp
  if (MdtFp)
    {
      fclose (MdtFp);
      MdtFp = NULL;
    }

  // Flush
  FlushMDTIndexes();

  // Free resource
  if (!useIndexMap)
    {
      if (KeyIndex)     delete[] KeyIndex;
      if (GpIndex)      delete[] GpIndex;
      if (KeySortTable) delete[] KeySortTable;
    }
  InstanceCount--;
#if USE_MDTHASHTABLE
  if (MDTHashTable)
    {
      if (InstanceCount == 0)
	{
	  // Last instance so we can
	  if (MDTHashTable != _globalMDTHashTable)
	    {
	      message_log (LOG_DEBUG, "Deleting (delayed) _globalMDTHashTable");
	      delete _globalMDTHashTable;
	    }
	  _globalMDTHashTable = NULL; // Remove global pressence
	  delete MDTHashTable;
	  message_log (LOG_DEBUG, "Deleting current MDTHashTable");
	}
      else if (MDTHashTable != _globalMDTHashTable) // Other instances still being used
	{
	  message_log (LOG_DEBUG, "Deleting current MDTHashTable (not _global)");
	  delete MDTHashTable;
	}
      MDTHashTable = NULL;
    }
#endif
  if (InstanceCount)
    message_log (LOG_DEBUG, "Deleted MDT Instance, %d instances still exist", InstanceCount);
  else
    message_log (LOG_DEBUG, "Disposed of all MDT Instances");
}


// Search MDT..

/*
     #define PEEKC      (*sp)
     #define UNGETC(c)    (--sp)
     #define RETURN(*c)    return;
     #define ERROR(c)     regerr
     #include <regexp.h>
      . . .
           (void) compile(*argv, expbuf, &expbuf[ESIZE],'\0');
      . . .
           if (step(linebuf, expbuf))
                             succeed;
*/
