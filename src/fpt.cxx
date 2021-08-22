/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)fpt.cxx"


/*-@@@
File:		fpt.cxx
Description:	Class FPT - File Pointer Table
@@@*/

#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#include "common.hxx"
#include "fpt.hxx"

static int _global_otherOpens = 0; // Count of things opened without tracking....
static int _global_streams_count = 0; 
static int _global_opens_count  = 0;

#if defined(_WIN32) || defined(__i386)
#define	_DEFAULT_SLOTS	55	/* Intel x86 ABI */
#else
#define	_DEFAULT_SLOTS	15	/* SPARC ABI and default */
#endif

#ifndef ENFILE
# define ENFILE EMFILE
#endif


FPT::FPT ()
{
  Init ( _DEFAULT_SLOTS ); 
}

FPT::FPT (size_t TableSize)
{
  Init (TableSize);
}

void FPT::Init (size_t TableSize)
{
  const int kernel_max_streams = _IB_kernel_max_streams();
  TotalEntries = 0;

  // Init the mutex lock
  pthread_mutex_init(&mutex, NULL);

  if ( (_global_streams_count > (3*TableSize)) &&
	( (int)((_global_streams_count + TableSize+10)) >= kernel_max_streams) &&
	(_global_opens_count == 0 || _global_opens_count > (_global_streams_count/2)) )
    {
      message_log(LOG_NOTICE, "You MUST increase kernel soft/hard file stream limits (%d). Contact your sysadmin!!",
	kernel_max_streams);
      MaximumEntries = 0;
      Table = NULL;
    }
  else
    {
      // Test for the total table sizes and their usage (50% average)
      if (((kernel_max_streams - _global_streams_count) < (int)(3*TableSize+10)) &&
		_global_opens_count > (_global_streams_count/2))
	{
	  message_log (LOG_WARN, "You should increase kernel soft/hard file stream limits (%d).", kernel_max_streams);
	  if (TableSize > 10) TableSize /= 2;
	  if (TableSize > 20) TableSize = 16;
	  if (TableSize < 10) TableSize = 10;
	}
      try {
	Table = new FPREC[TableSize];
      } catch (...) {
	Table = NULL;
      }
      if (Table == NULL)
	{
	  message_log (LOG_PANIC|LOG_ERRNO, "FPT::Init() Allocation failure (wanted %d)", TableSize);
	  TableSize = 0;
	}
      _global_streams_count += (MaximumEntries = TableSize);
    }
}

static size_t LastId = 0;

INT FPT::ffclose(const STRING& Filename)
{
  size_t z = Lookup(Filename);
  if (z)
    {
      FILE *Fp;
      FPREC Fprec = Table[z - 1];
      GDT_BOOLEAN  Opened = Fprec.GetOpened ();
      if (Opened != GDT_TRUE)
	{
          pThreadLocker Lock(&mutex, "FPT::ffclose");
	  if (Table[z-1].GetFilePointer()) _global_opens_count--;
	  Table[z - 1].Dispose();
	  LowPriority (z);
	  return 0;
	}
      return (INT)z;
    }
  return -1;
}

GDT_BOOLEAN FPT::hasOpenHandle(const STRING& Filename) const
{
  size_t z  = Lookup(Filename);
  if (z)
    {
      FILE *Fp;
      if ((Fp = Table[--z].GetFilePointer ()) != NULL)
	return GDT_TRUE;
    }
  return GDT_FALSE;
}


size_t FPT::Lookup (const STRING& FileName) const
{
  if (Table == NULL) return 0;

  // Expand FileName
  const STRING Fn ( ExpandFileSpec(FileName) );
  // Loop through table
  size_t start = LastId > 2 ? LastId - 2 : LastId;
  for (size_t i = start; i < (TotalEntries + start); i++)
    {
      if (Table[i%TotalEntries].GetFileName () == Fn)
	return (LastId = (i%TotalEntries)) + 1;
    }
  return 0; // NOT FOUND
}

size_t FPT::Lookup (const FILE *FilePointer) const
{
  if (Table == NULL) return 0;

  // Loop through table backwards
  for (size_t i = TotalEntries + LastId; i > LastId; i--)
    {
      if (FilePointer == Table[ i%TotalEntries ].GetFilePointer ())
	return (LastId = (i%TotalEntries)) + 1;
    }
  return 0;
}

void FPT::HighPriority (const size_t Index)
{
  if (Index == 0 || Index > TotalEntries || Table == NULL)
    return;

  const size_t IndexPriority = Table[Index - 1].GetPriority ();
  size_t Lowest = IndexPriority;
  for (size_t x = 0; x < TotalEntries; x++)
    {
      size_t OtherPriority;

      if ((OtherPriority = Table[x].GetPriority ()) <= IndexPriority)
	{
	  Table[x].SetPriority (OtherPriority + TotalEntries - x);
	  if (OtherPriority < Lowest)
	    Lowest = OtherPriority;
	}
    }
  Table[Index - 1].SetPriority (Lowest);
}

void FPT::LowPriority (const size_t Index)
{
  if (Index == 0 || Index > TotalEntries || Table == NULL)
    return;

  const size_t IndexPriority = Table[Index - 1].GetPriority ();
  size_t Highest = IndexPriority;
  for (size_t x = 0; x < TotalEntries; x++)
    {
      size_t OtherPriority;

      if ((OtherPriority = Table[x].GetPriority ()) > IndexPriority)
	{
	  Table[x].SetPriority (OtherPriority - TotalEntries + x);
	  if (OtherPriority > Highest)
	    Highest = OtherPriority;
	}
    }
  Table[Index - 1].SetPriority (Highest);
}

INT FPT::FreeSlot()
{
  if (Table == NULL) return -1;

  size_t NewEntry;
  if (TotalEntries >= MaximumEntries)
    {
      size_t freeEntry;
      // If table at maximum size, purge oldest element
      for (NewEntry = 0; NewEntry < TotalEntries; NewEntry++)
	{
	  freeEntry = NewEntry;
	  for (size_t x = 0; x < TotalEntries; x++)
	    {
	      if (NewEntry == 0)
		{
		  if (Table[x].GetFileName().IsEmpty() || Table[x].GetFilePointer () == NULL)
		    {
		      freeEntry = x;
		      break;
		    }
		} 
	      if (Table[x].GetPriority () > Table[NewEntry].GetPriority ())
		freeEntry = x;
	    }
	  if (Table[freeEntry].GetClosed ())
	    {
	      pThreadLocker Lock(&mutex, "FPT::FreeSlot");
	      if (Table[freeEntry].GetFilePointer()) _global_opens_count--;
	      Table[freeEntry].Dispose();
	      NewEntry = freeEntry;
	      break; // Got something
	    }
 	}
      if (NewEntry == TotalEntries)
	{
	  message_log (LOG_DEBUG, "Stream cache is filled, trying to collect garbage..");
	  for (freeEntry = 0; freeEntry < TotalEntries; freeEntry++)
	    {
	      if (Table[freeEntry].GetClosed())
		{
		  pThreadLocker Lock(&mutex, "FPT::FreeSlot");
		  if (Table[freeEntry].GetFilePointer()) _global_opens_count--;
		  Table[freeEntry].Dispose();
		  NewEntry = freeEntry;
		}
	    }
	  if (NewEntry == TotalEntries)
	    {
	      message_log (LOG_DEBUG, "Stream cache is filled and no room is available!");
	      return -1;
	    }
	}
    }
  else
    NewEntry = TotalEntries++;
  return NewEntry;
}

FILE *FPT::ffreopen(const STRING& filename, const CHR *mode, FILE *stream)
{
  FILE *Fp;
  size_t z = Lookup (filename);
  if (z)
    {
      if ((Fp = Table[--z].GetFilePointer ()) != NULL)
	{
	  fclose(Fp);
          _global_opens_count--;
	}
    }
  else
    z = FreeSlot();

  Fp = ::freopen(filename.c_str(), mode, stream);
  if ((INT)z != -1)
    {
      Table[z].SetOpenMode (mode);
      Table[z].SetFilePointer (Fp);
      Table[z].SetOpened();
      HighPriority (z + 1);
    }
  if (Fp)  _global_opens_count++;
  return Fp;
}

PFILE FPT::ffopen (const STRING& FileName, const CHR* Type)
{
  if (Table == NULL)
    {
      FILE *fp =  ::fopen(FileName.c_str(), Type);
      if (fp) _global_otherOpens++;
      return fp;
    }

  size_t z;
  PFILE Fp = NULL;
  errno = 0;

  if (FileName.IsEmpty())
    return NULL;

  // Check if file is already open
  if ((z = Lookup (FileName)) != 0)
    {
      // If found, check OpenMode
      FPREC Fprec = Table[z - 1];
      const STRING Fn ( Fprec.GetFileName () );
      const STRING Om ( Fprec.GetOpenMode () );
      GDT_BOOLEAN  Opened = Fprec.GetOpened ();

      if ((Fp = Fprec.GetFilePointer ()) == NULL) {
	message_log (LOG_ERROR, "Stream cache of '%s' is bonked! Contact bugs@nonmonotonic.com", FileName.c_str());
	if ((Fp = ::fopen(FileName, Type)) != NULL)
	  {
	    Table[z - 1].SetFilePointer (Fp);
	    Table[z - 1].SetOpened();
	    Table[z - 1].SetOpenMode (Type);
	     _global_opens_count++;
	  }
      } else if (Opened && Fp)
	{
	  if (Om[0] != 'r')
	    message_log (LOG_WARN, "FPT: %s (%s) is already opened %s.", Fn.c_str(), Type, Om.c_str());
	  else
	    message_log (LOG_DEBUG, "FPT: %s (%s) is already opened %s.", Fn.c_str(), Type, Om.c_str());
	}
      if (Om == Type)
	{
	  // If same OpenMode, use the cached information
	  if (Om[0] == 'r')
	    {
	      if (Fp)
		fseek (Fp, 0, SEEK_SET);
	    } 
	  else
	    {
	      if (Fp) { fclose (Fp); _global_opens_count--; }
	      Fp = fopen(FileName, Type);
	      Table[z - 1].SetFilePointer (Fp);
	      if (Fp) _global_opens_count++;
	    }
	}
      else
	{
	  // If different OpenMode, update table
	  if (Fp) { fclose (Fp); _global_opens_count--; }
	  Fp = fopen(FileName, Type);
	  Table[z - 1].SetOpenMode (Type);
	  Table[z - 1].SetFilePointer (Fp);
	  if (Fp)  _global_opens_count++;
	}
      // Is the stream open?
      if (Fp == NULL)
	{
	  // Oops, could not change mode!
	  Table[z - 1].SetClosed (); 
	  message_log (LOG_ERRNO, "Could not change mode of %s to %s", FileName.c_str(), Type);
	  LowPriority(z); 
	}
      else
	{
	  // Good pointer, set priority high
	  Table[z - 1].SetOpened();
	  HighPriority (z);
	}
      return Fp;
    }
  else
    {
      // If not found, open and cache the FilePointer
      // Note: We don't cache the file stream if it could
      // not be opened...
      errno = 0; // Reset..
      if ((Fp = fopen(FileName, Type)) != NULL)
	{ 
opened:
	   _global_opens_count++;
	  // We now have an open stream..
	  INT NewEntry = FreeSlot();
	  if (NewEntry != -1)
	    {
	      Table[NewEntry].SetFileName (FileName);
	      Table[NewEntry].SetOpenMode (Type);
	      Table[NewEntry].SetPriority (MaximumEntries + 1);
	      HighPriority (NewEntry + 1);
	      Table[NewEntry].SetOpened();
	      Table[NewEntry].SetFilePointer (Fp);
	    }
	}
      else if (errno == EMFILE || errno == 0 || errno == ENFILE)
	{
	  Sync (); // Clean out garbage...
	  errno = 0; // Reset error
	  if ((Fp = ::fopen(FileName, Type)) != NULL)
	    goto opened; // Yea!!
	  if (errno == 0 || errno == EMFILE || errno == ENFILE)
	    message_log (LOG_ERROR, "Too many open streams: %u active / %u allocated / %u open.",
		(unsigned)TotalEntries, _global_streams_count, _global_opens_count);
        }
    }
  return Fp;
}


INT FPT::ffdelete(const STRING& Filename)
{
  size_t z = Lookup (Filename);
  if (z != 0)
    {
      pThreadLocker Lock(&mutex, "FPT::ffdelete");
      // Force all close
      if (Table[z-1].GetFilePointer()) _global_opens_count--;
      Table[z - 1].Dispose();
      LowPriority (z);
    }
  int result = unlink(Filename);
  if (result != 0)
    {
      AddtoGarbageFileList(Filename);
    }
  return result;
}


INT FPT::ffdelete(FILE *Fp)
{
  int    result = -1;
  if (Fp != NULL)
    {
      size_t z;
      if ((z = Lookup (Fp)) != 0)
	{
	  STRING Filename = Table[z-1].GetFileName();
	  if (Fp) { fclose(Fp); _global_opens_count--; }
	  Table[z - 1].SetClosed ();
	  if ((result = unlink(Filename)) != 0)
	    AddtoGarbageFileList(Filename);
	  Table[z - 1].SetFileName(NulString);
	  Table[z - 1].SetFilePointer (NULL);
	  LowPriority (z);
	}
      else
	{
	  fclose(Fp); // Can't delete, only close 
	  _global_otherOpens--; // ????
	}
  }
  return result; 
}

INT FPT::ffdispose(const STRING& Filename)
{
  size_t z = Lookup (Filename);
  if (z != 0)
    {
      pThreadLocker Lock(&mutex, "FPT::ffdispose");
      if (Table[z-1].GetFilePointer()) _global_opens_count--;
      Table[z - 1].Dispose();
      LowPriority (z);
      return 0;
    }
  return -1;
}

INT FPT::ffdispose(FILE *Fp)
{
  size_t z;
  if (Fp == NULL)
   return -1;
  if ((z = Lookup (Fp)) != 0)
    {
      pThreadLocker Lock(&mutex, "FPT::ffdispose");
      if (Fp) {fclose(Fp); _global_opens_count--; }
      Table[z - 1].SetClosed ();
      Table[z - 1].SetFileName(NulString);
      Table[z - 1].SetFilePointer (NULL);
      LowPriority (z);
    }
  else if (Fp)
    {
      _global_otherOpens--;
      fclose (Fp);
    }
  return 0;
}

INT FPT::ffclose (PFILE FilePointer)
{
  size_t z;
  if (FilePointer == NULL)
   return -1;
  if ((z = Lookup (FilePointer)) != 0)
    {
      if (FilePointer) fflush(FilePointer);
      Table[z - 1].SetClosed ();
      LowPriority (z);
    }
  else if (FilePointer)
    {
      _global_otherOpens--;
      fclose (FilePointer);
    }
  return 0;
}

void FPT::Sync ()
{
  size_t i = 0;
  PFPREC NewTable = NULL;
  pThreadLocker Lock(&mutex, "FPT::Sync");
  for (size_t x = 0; x < TotalEntries; x++)
    {
      if (Table[x].GetClosed ())
        {
	  if (Table[x].GetFilePointer()) _global_opens_count--;
	  Table[x].Dispose();
        }
       else
	{
	  if (i == 0)
	    {
	      try { NewTable = new FPREC[ MaximumEntries ]; } catch (...) { NewTable  = NULL; }
	      if (NewTable  == NULL)
		{
		  message_log (LOG_PANIC, "Can't sync FPT tables, allocation failed");
		}
	    }
	  if (NewTable)
	    NewTable[i++] = Table[x];
	  else
	    message_log (LOG_PANIC, "FPREC is NULL");
	}
    }
  if (i)
    {
      if (Table) delete[]Table;
      Table = NewTable;
    }
  TotalEntries = i;
//  Dump (cerr);
}

void FPT::Dump(ostream& os) const
{
  os << "== FPT::Dump ==" << endl;
  for (size_t i = 0; i < TotalEntries ; i++)
    os << "  [" << i << "] " <<  (STRING)Table[i] << endl;
  os << "===============" << endl;
}

void FPT::CloseAll ()
{
  if (TotalEntries == 0) return; // Do nothing

  pThreadLocker Lock(&mutex, "FPT::CloseAll");
  for (size_t x = 0; x < TotalEntries; x++)
    {
      if (Table[x].GetClosed ())
	{
	  if (Table[x].GetFilePointer()) _global_opens_count--;
	  Table[x].Dispose();
	}
      else
        {
	  message_log (LOG_ERROR, "Dangling open '%s'", Table[x].GetFileName().c_str());
//	  Table[x].SetClosed (); // Mark as closed
	}
    }
  TotalEntries = 0;
#ifndef _WIN32
  sync(); // Sync file system
#endif
}

FPT::~FPT ()
{
  CloseAll ();
  if (_global_otherOpens > 0)
    message_log (LOG_ERROR, "Potential %d dangling opened streams.", _global_otherOpens);

 _global_streams_count -= MaximumEntries;

  if (Table) delete[]Table;
  if ( _global_streams_count == 0 &&  _global_opens_count)
  message_log (LOG_WARN, "FPT: Closed last FPT but %d file streams still open.", _global_opens_count);
  else message_log (LOG_DEBUG, "Deleted FPT (%d global streams left, %d open)", _global_streams_count, _global_opens_count);
}
