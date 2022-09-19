#include <stdlib.h>
#include <iostream.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.hxx"
#include "index.hxx"
#include "mdthashtable.hxx"

#define STRLEN_T  UINT2 /* Limited to string 65536 bytes long */

sHASHTABLE::sHASHTABLE()
{
  tableSize   = 0;
  Table       = NULL; 
  fd          = -1;
  file_length = 0;
  lastOffset  = 0;
  loaded      = GDT_FALSE;

}


sHASHTABLE::sHASHTABLE(const STRING& fn, int size)
{
  tableSize   = 0;
  Table       = NULL; 
  fd          = -1;
  file_length = 0;
  lastOffset  = 0;
  loaded      = GDT_FALSE;
  Init (fn, size);
}


sHASHTABLE::~sHASHTABLE()
{
  if(fd != -1)
    close(fd);
  // free memory
  DeleteTable();
  logf (LOG_DEBUG, "sHASHTABLE deleted");
}


void sHASHTABLE::KillAll()
{
  if(fd != -1)
    close(fd);
  // free memory
  DeleteTable();
  unlink(filename);
  loaded = GDT_FALSE;

  lastStr.Clear();
  lastOffset = 0;
}


void sHASHTABLE::DeleteTable()
{
  if (Table) {
    logf (LOG_DEBUG, "Deleting sHASHTABLE with %d elements", tableSize);

    size_t i = 0;
    for(sHASHOBJ *node = Table[i]; i < tableSize; i++)
      {
	while (node != NULL)
	  {
	    sHASHOBJ *tmp = node->next;
	    delete node;
	    node = tmp;
	  }
      }
    delete[] Table;
    Table = NULL;
    tableSize = 0;
  }
  loaded = GDT_FALSE;
}

void sHASHTABLE::Init(const STRING& fn, int size)
{
  if (Table) DeleteTable();
  if (DiskTable.Ok())
    DiskTable.Unmap();
  if (fd != -1)
    {
      close(fd);
      fd = -1;

    }
  filename = fn;

  // some useful primes:
  // tableSize = 101, 1009, 10039, 100003, 500009, 1000003
  if ((tableSize = size) <= 0)
    tableSize = 0;
  else if (tableSize <= 101)
    tableSize = 101;
  else if (tableSize <= 1009)
    tableSize = 1009;
  else if (tableSize <= 7919)
    tableSize = 7919;
  else if (tableSize <= 8191)
    tableSize = 8191;
  else if (tableSize <= 10039)
    tableSize = 10039;
  else if (tableSize <= 16381)
    tableSize = 16381;
  else if (tableSize <= 17389)
    tableSize = 17389;
  else if (tableSize <= 32749)
    tableSize = 32749;
  else if (tableSize <= 65521)
    tableSize = 65521;
  else if (tableSize <= 100003)
    tableSize = 100003;
  else if (tableSize <= 131071) // 2^17 -1
    tableSize = 131071; 
  else if (tableSize <= 500009)
    tableSize = 500009;
  else if (tableSize <= 1000003)
    tableSize = 1000003;
  else
    logf (LOG_NOTICE, "Hashtable Init requested for %d>%d!", tableSize, 1000003);

  loaded = GDT_FALSE;
}


void sHASHTABLE::AllocTable()
{
  // allow for zero length table to disable hash table function
  if (tableSize > 0 && Table == NULL)
    Table = new sHASHOBJ *[tableSize];
}

GDT_BOOLEAN sHASHTABLE::LoadOnDiskHashTable()
{
  // Read a pre-existing on-disk hashtable into memory.
  // This is needed if you need to do Add()'s to an
  // existing hashtable.
  if (fd == -1)
    {
      // need to open file 
      if ((fd = open(filename,O_RDONLY)) == -1)
        return GDT_FALSE;
    }

  // Memory Mapping disabled?
  if (tableSize == 0)
    return GDT_TRUE;
  AllocTable();

  DiskTable.CreateMap(fd, MapRandom);
  if (!DiskTable.Ok())
    {
      logf (LOG_ERRNO, "CreateMap (mmap) of '%s' FAILED!", filename.c_str());
      return GDT_FALSE;
    }
  file_length = DiskTable.Size();
  if (file_length > sizeof(STRINGData))
    {
      // first few bytes are left empty
      int           bytes = 0;
      STRING        temp_str;

      logf (LOG_DEBUG, "Loading Hash of names in '%s' (%ld bytes)", filename.c_str(), file_length);
      for (long newoffset = bytes; newoffset < file_length; newoffset += bytes)
	{
	  size_t   hash;

	  bytes = temp_str.RawRead( &DiskTable, (size_t)newoffset );
	  if (newoffset)
	    {
	      // insert into memory hashtable
	      hash = Hash(temp_str);
	      Table[hash] = new sHASHOBJ(temp_str, newoffset, Table[hash]);
	    }
	  else if (temp_str != NulString)
	    {
	      logf (LOG_PANIC, "'%s' is corrupt!", filename.c_str());
	      DiskTable.Unmap();
	      return GDT_FALSE;
	    }
	}
    }
  loaded = GDT_TRUE;
  return loaded;
}

long sHASHTABLE::Add(const STRING& str)
{
  // Add a string to our hashtable and return the unique id. (really the
  // index to the strings location in the on disk table). We keep a
  // in memory hashtable to do fast lookups for repeated Add()'s of the
  // same string.

  // This the last one?
  if (str == lastStr)
    return lastOffset; // OK..

  size_t       hash;
  long         newoffset = 0;
  sHASHOBJ    *node = NULL;

  AllocTable();
check:
  if (tableSize)
    {
      // check to see if this str/offet pair already exists in the hash table
      hash = Hash(str);
      for (node = Table[hash]; node && (str != node->str); node = node->next)
	/* loop */;
      // Did we find it?
      if (node)
       return (node->offset); // YES!
  }
  // Not found!

  // check to see if we had and Get() operations that
  // would have mmaped the file...
  if (DiskTable.Ok())
    {
      close(fd);
      fd = -1;
      DiskTable.Unmap();
    }
  // we have a new entry, add it.
  if (fd == -1)
    {
      // need to open file 
      if ((fd = open(filename,  O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
	{
	  logf (LOG_ERRNO, "Can't open '%s' to add '%s'!", filename.c_str(), str.c_str());
	  return GDT_FALSE;
	}
    }
  // append new string to the end of the file
  if ((newoffset = lseek(fd, 0, SEEK_END)) < 0)
    {
      logf (LOG_ERRNO, "Can't lseek to end on handle %d. Can't add '%s'!", fd, str.c_str());
      return GDT_FALSE;
    }
  else if (newoffset > 0)
    {
      if (tableSize != 0 && !loaded)
	{
	  LoadOnDiskHashTable();
	  goto check;
	}
    }
  else if (newoffset == 0)
    {
      // First time opening this file
      // Zero is the error code, so we fill the first few bytes
      // with zeros, so that the offset zero is never used.
      if ((newoffset = NulString.RawWrite(fd)) == -1)
	{
	  logf (LOG_ERRNO, "Create '%s'.. Write error on handle %d.", filename.c_str(), fd);
	  return GDT_FALSE;
	}
    }

  if (tableSize)
    {
      // insert the string/offset pair into the hash in front
      node = new sHASHOBJ (str, newoffset, Table[hash]);
      Table[hash] = node;
    }

  lastOffset = offset = newoffset;
  lastStr    = str;

  // write out the string to disk
  int  res = str.RawWrite(fd);
  if (res != -1)
    {
      offset += res;
      file_length = offset;
      if (res < (str.GetLength() + sizeof(STRINGData) + 1))
	logf (LOG_ERRNO, "Write error on handle %d. Wrote %d < %d bytes.", fd,
	res, str.GetLength() + sizeof(STRINGData) + 1);
    }
  return (newoffset);
}

STRING sHASHTABLE::Get(long i)
{
  if (lastOffset == i)
    return lastStr;

  const char msg[] = "Can't lookup string in %s: %s";
  // Take the unique id (really the index into the on-disk file) and
  // return the corresponding string.
  
  // The file has to be mmap()'d for this to work.
  STRING         str;

  if (!DiskTable.Ok() && fd >= 0)
    {
      // the file is opened, but not mmapped. We must close() and re-open()
      close(fd);
      fd = -1;
    }

  if (fd == -1) {
    // need to open file 
    if ((fd = open(filename,O_RDONLY)) == -1)
      {
	logf (LOG_ERRNO, msg, filename.c_str(), "file open failure");
	return str;
      }
    // mmap file for fast easy access
    DiskTable.CreateMap(fd, MapRandom);
    if (!DiskTable.Ok())
      {
	logf (LOG_PANIC, msg, filename.c_str(), "memory map failure");
	return str;
      }
    file_length = DiskTable.Size();
  }

  if (file_length > sizeof(STRINGData) && i > 0)
    {
      if (str.RawRead( &DiskTable, (size_t)i) == -1)
	logf (LOG_PANIC, "Can't lookup string %ld in '%s'! File Truncated to %ld?",
		i, filename.c_str(), file_length);
    }
  return str;
}

size_t sHASHTABLE::Hash(const STRING& Str) const
{
#if 1
  size_t       len  = Str.GetLength();
  size_t       hash =  len; // Seed hash with strlen()
  const char  *ptr  = Str.c_str();
  for(size_t i=0 ; i < len; i++)
    hash = ((hash << 7) + (unsigned int)(*ptr++)) % tableSize;
#else
  unsigned int stop = Str.GetLength();
  // Only calculate hash on the last 30 characters
  // for speed. Some exerimentation showed that this was
  // enough.
  unsigned int start = (stop > 30) ? (stop - 30) : 0;

  // seed hash with strlen() 
  unsigned int hash = stop;  
  for(unsigned i = start; i < stop; i++)
    hash = ((hash << 7) + ((unsigned int) Str[i])) % tableSize;
#endif
  return hash;
}


MDTHASHTABLE::MDTHASHTABLE(const IDBOBJ *Idb)
{
  if (Idb)
    {
      if (Idb->GetMergeStatus() == iNothing)
	Init(Idb->GetDbFileStem(), 0, 0);
      else
	Init(Idb->GetDbFileStem());
    }
  else
    Init (NulString);
}


MDTHASHTABLE::MDTHASHTABLE(const INDEX *index)
{
  if (index)
    {
      if (index->GetMergeStatus() == iNothing)
	Init(index->GetDbFileStem(), 0, 0);
      else
	Init(index->GetDbFileStem());
    }
  else
    Init (NulString);
}

MDTHASHTABLE::MDTHASHTABLE(const STRING& Stem)
{
  Init (Stem);
}


MDTHASHTABLE::MDTHASHTABLE(const STRING& Stem, GDT_BOOLEAN Fast)
{
  if (Fast)
    Init(Stem, 0, 0);
  else
    Init (Stem);
}



void MDTHASHTABLE::Init(const STRING& FileStem)
{
  Init(FileStem, 10039, 65521);  // was 10039, 500009
}


void MDTHASHTABLE::Init(const STRING& FileStem, size_t FileTableSize, size_t PathTableSize)
{
  fileNameTable.Init(FileStem + ".fn", FileTableSize);
  pathNameTable.Init(FileStem + ".pn", PathTableSize);
}


void  MDTHASHTABLE::KillAll()
{
  fileNameTable.KillAll();
  pathNameTable.KillAll();
}


MDTHASHTABLE::~MDTHASHTABLE()
{
  logf (LOG_DEBUG, "MDTHASHTABLE deleted");
}

