/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <stdlib.h>
//#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "common.hxx"
#include "index.hxx"
#include "mdthashtable.hxx"

#define STRLEN_T  UINT2 /* Limited to string 65536 bytes long */

#ifndef _WIN32
# define O_BINARY  0
#endif

#ifdef _WIN32
#define DEF_PERMIT (S_IRUSR | S_IWUSR)
#else
#define DEF_PERMIT (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#endif

sHASHTABLE::sHASHTABLE()
{
  tableSize   = 0;
  Table       = NULL; 
  fd          = -1;
  file_length = 0;
  loaded      = false;
  mmap_reads  = false;

}


sHASHTABLE::sHASHTABLE(const STRING& fn, int size)
{
  tableSize   = 0;
  Table       = NULL; 
  fd          = -1;
  file_length = 0;
  loaded      = false;
  mmap_reads  = false;

  Init (fn, size);
}


sHASHTABLE::~sHASHTABLE()
{
  if(fd > 0)
    {
      close(fd);
      fd = -1;
    }
  // free memory
  DeleteTable();
  message_log (LOG_DEBUG, "sHASHTABLE deleted");
}


bool sHASHTABLE::KillAll()
{
  bool result = true;

  if(fd > 0)
    {
      close(fd);
      fd = -1;
    }
  // free memory
  DeleteTable();
  if (UnlinkFile(filename) != 0)
    {
      if (EraseFileContents(filename) != 0)
	result = false;
    }
  lastCache.Clear();
  loaded = false;
  return result;
}


void sHASHTABLE::DeleteTable()
{
  message_log (LOG_DEBUG, "delete sHASHTABLE table");

  if (Table) {
    message_log (LOG_DEBUG, "Deleting sHASHTABLE with %d elements", tableSize);

#if 0 /* How it was */
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
#else
    for(size_t i=0; i < tableSize; i++)
      {
	sHASHOBJ *node = Table[i];
        while (node != NULL)
          {
            sHASHOBJ *tmp = node->next;
            delete node;
            node = tmp;
          }
      }
#endif
    delete[] Table;
    Table = NULL;
    tableSize = 0;
  }
  else
     message_log (LOG_DEBUG, "sHASHTABLE table was empty (nothing to delete)");
  loaded = false;
}

void sHASHTABLE::Init(const STRING& fn, int size)
{
  if (Table) DeleteTable();
  if (DiskTable.Ok())
    DiskTable.Unmap();

  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
  filename = fn;

/*
static long s_polys[] =  {
  4+3, 8+3, 16+3, 32+5, 64+3, 128+3, 256+29, 512+17, 1024+9, 2048+5, 4096+83,
    8192+27, 16384+43, 32768+3, 65536+45, 131072+9, 262144+39, 524288+39,
    1048576+9, 2097152+5, 4194304+3, 8388608+33, 16777216+27, 33554432+9,
    67108864+71, 134217728+39, 268435456+9, 536870912+5, 1073741824+83, 0
};   
*/


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
    message_log (LOG_NOTICE, "Hashtable requested for %d>%d!", tableSize, 1000003);

  message_log (LOG_DEBUG, "Hashtable '%s': %d slots", fn.c_str(), tableSize);
  loaded = false;
}


void sHASHTABLE::AllocTable()
{
  // allow for zero length table to disable hash table function
  if (tableSize > 0 && Table == NULL)
    {
      Table = new sHASHOBJ *[tableSize];
      // Fill the slots
      for (size_t i=0; i<tableSize; i++) Table[i] = NULL;
   }
}

bool sHASHTABLE::LoadOnDiskHashTable()
{
  loaded = true;
  // Read a pre-existing on-disk hashtable into memory.
  // This is needed if you need to do Add()'s to an
  // existing hashtable.
  if (fd == -1)
    {
      // need to open file 
      if ((fd = open(filename,O_RDONLY | O_BINARY)) == -1)
	{
	  if (errno == ENOENT)
	    {
	      // Does not yet exist!
	      return true;
	    }
	  return false;
	}
    }

  // Memory Mapping disabled?
  if (tableSize == 0)
    {
      return true;
    }

  AllocTable();
  DiskTable.CreateMap(fd, MapRandom);
  close(fd);
  fd *= -1;


  if (!DiskTable.Ok())
    {
      message_log (LOG_ERRNO, "CreateMap (mmap) of '%s'[%d] FAILED!", filename.c_str(), -fd);
      fd = -1;
      return false;
    }

  file_length = DiskTable.Size();
  if ((size_t)file_length > sizeof(STRINGData))
    {
      // first few bytes are left empty
      int           bytes = 0;
      STRING        temp_str;

      message_log (LOG_DEBUG, "Loading Hash of names in '%s' (%ld bytes)", filename.c_str(), file_length);
      for (off_t newoffset = bytes; newoffset < file_length; newoffset += bytes)
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
	      message_log (LOG_PANIC, "'%s' is corrupt!", filename.c_str());
	      DiskTable.Unmap();
	      return false;
	    }
	}
    }
  return loaded;
}

off_t sHASHTABLE::Add(const STRING& str)
{
  // Add a string to our hashtable and return the unique id. (really the
  // index to the strings location in the on disk table). We keep a
  // in memory hashtable to do fast lookups for repeated Add()'s of the
  // same string.
  size_t       hash; // We init this when we use it!
  off_t        newoffset = 0;
  sHASHOBJ    *node = NULL;

  errno = 0;

  if (str == lastCache.str) return lastCache.offset;

  AllocTable();
check:

  if (tableSize)
    {
      // check to see if this str/offet pair already exists in the hash table
      hash = Hash(str);
      if (hash > tableSize)
	message_log (LOG_PANIC, "Hash table overflow (%d >%d)!", (int)hash, (int)tableSize);
      else if (Table)
	{
	  node = Table[hash];
	  while (node)
	    {
	      if (::PathCompare(str.c_str(), node->str.c_str()) == 0)
		break; // Found it
	      node = node->next;
	    }
	}
      else
	message_log (LOG_PANIC, "Hash table went puff! Should have %u elements.", (int)tableSize);
      // Did we find it?
      if (node)
       return (node->offset); // YES!
  }
  // Not found!
  // check to see if we had and Get() operations that
  // would have mmaped the file...
  if (DiskTable.Ok())
    {
      DiskTable.Unmap();
      // If file is open close it...
      if (fd > 0)
	{
	  close(fd);
	  fd = -1;
	}
    }
  // we have a new entry, add it.
open_again:
  if (fd <= 0)
    {
      // need to open file 
      if ((fd = open(filename,  O_RDWR | O_APPEND | O_CREAT | O_BINARY, DEF_PERMIT)) == -1)
	{
	  message_log (LOG_ERRNO, "Can't open '%s' to add '%s'!", filename.c_str(), str.c_str());
	  return false;
	}
    }
  // append new string to the end of the file
  if ((newoffset = lseek(fd, 0, SEEK_END)) < 0)
    {
      message_log (LOG_ERRNO, "Can't lseek to end on handle %d. Can't add '%s'!", fd, str.c_str());
      return false;
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
	  message_log (LOG_ERRNO, "Create '%s'.. Write error on handle %d.", filename.c_str(), fd);
	  return false;
	}
    }

  if (tableSize)
    {
      // insert the string/offset pair into the hash in front
      node = new sHASHOBJ (str, newoffset, Table[hash]);
      Table[hash] = node;
    }

  offset = newoffset;

  // write out the string to disk
  int  res = str.RawWrite(fd);
  if (res != -1)
    {
      offset += res;
      file_length = offset;
      if ((unsigned)res < (str.GetLength() + sizeof(STRINGData) + 1))
	message_log (LOG_ERRNO, "Write error on handle %d. Wrote %d < %d bytes.", fd,
	res, str.GetLength() + sizeof(STRINGData) + 1);
    }
  else if (errno == EBADF) // Handle open only for reading
    {
      close(fd);
      fd = -1;
      goto open_again;
    }
  else
    message_log (LOG_ERRNO, "Write error on handle %d. Could not write '%s'", fd, str.c_str());

  lastCache.str = str;
  return (lastCache.offset = newoffset);
}

STRING sHASHTABLE::Get(off_t i)
{
  // Take the unique id (really the index into the on-disk file) and
  // return the corresponding string.
  message_log (LOG_DEBUG, "Looking up string #0x%lx", i);
  
  // The file has to be mmap()'d for this to work.
  STRING         str;

  if (fd == -1 && mmap_reads == false)
    {
      if ((fd = open(filename, O_RDWR|O_APPEND|O_BINARY /* was O_RDONLY */)) == -1)
	fd = open(filename, O_RDONLY);
    }

  if ((!DiskTable.Ok() || mmap_reads == false) && fd >= 0)
    {
      message_log (LOG_DEBUG, "Seeking in '%s'(%ld).", filename.c_str(), i);
      // the file is opened, but not mmapped.
#if 1	/* Do normal reads */
      if (str.RawRead(fd, i) <= 0)
	message_log (LOG_ERROR|LOG_ERRNO, "Zero string in MDT??? // '%s'(%ld:%ld)",
		filename.c_str(), i, GetFileSize(fd));
      else
	return str;
#endif
      close(fd);
      fd = -1;
    }

  static const char *msg = "Can't lookup string %ld in %s: %s";
  if (fd == -1) {
    // need to open file 
    errno = 0;
    if ((fd = open(filename,O_RDONLY|O_BINARY)) == -1)
      {
	message_log (LOG_ERRNO, msg, i, filename.c_str(), "file open failure");
	return str;
      }
    // mmap file for fast easy access
    message_log (LOG_DEBUG, "Memory mapping '%s'", filename.c_str());
    DiskTable.CreateMap(fd, MapRandom);
    close(fd);
    fd *= -1;

    if (!DiskTable.Ok())
      {
	message_log (LOG_PANIC, msg, i, filename.c_str(), "memory map failure");
	fd = -1;
	return str;
      }
    file_length = DiskTable.Size();
  }

  if ((size_t)file_length > sizeof(STRINGData) && i > 0)
    {
      if (str.RawRead( &DiskTable, (size_t)i) == -1)
	message_log (LOG_PANIC, "Can't lookup string %ld in '%s'! File Truncated to %ld?",
		i, filename.c_str(), file_length);
    }
  return str;
}


size_t sHASHTABLE::Hash(const STRING& Str) const
{
  if (tableSize > 0)
    {
      size_t       start = 0;
      size_t       len  = Str.GetLength();
      size_t       hash =  len; // Seed hash with strlen()
      const char  *ptr  = Str.c_str();

#ifdef _WIN32
      int          Ch;

      // If the path is longer then 1024 do just the first bits
      if (len > 1024 ) len=1024; 

      // Character independent
      for(size_t i=start ; i < len; i++)
	{
	  Ch = *ptr++;
	  if (Ch == '\0') break;
	  if (Ch == '/') Ch = '\\';
	  else Ch = toupper(Ch);
	  hash = ((hash << 7) + Ch) % tableSize;
	}

#else
     // If the path is longer then 1024 do just the last bits
      if (len > 1024 ) start = len - 1024;

      for(size_t i=start ; i < len; i++)
	hash = ((hash << 7) + (unsigned int)(*ptr++)) % tableSize;
#endif
      return hash;
    }
  return 0;
}


MDTHASHTABLE::MDTHASHTABLE(const IDBOBJ *Idb)
{
  if (Idb)
    {
      if (Idb->GetMergeStatus() == iNothing)
	Init(Idb->GetDbFileStem(), 0);
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
	Init(index->GetDbFileStem(), 0);
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


MDTHASHTABLE::MDTHASHTABLE(const STRING& Stem, bool Fast)
{
  if (Fast)
    Init(Stem, 0);
  else
    Init (Stem);
}



void MDTHASHTABLE::Init(const STRING& FileStem)
{
  Init(FileStem, 65521);  // was 10039, 500009
}


void MDTHASHTABLE::Init(const STRING& FileStem, size_t TableSize)
{
  message_log (LOG_DEBUG, "Init of '%s' MDT filename table (%d)", FileStem.c_str(), TableSize);

  oldFileOffset = 0;
  oldPathOffset = 0;
  oldFileName.Clear();
  oldPath.Clear();

  StringTable.Init(FileStem + DbExtMdtStrings, TableSize);
}


bool MDTHASHTABLE::KillAll()
{
  return StringTable.KillAll();
}


MDTHASHTABLE::~MDTHASHTABLE()
{
  message_log (LOG_DEBUG, "MDTHASHTABLE deleted");
}
