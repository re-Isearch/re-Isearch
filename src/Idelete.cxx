/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*@@@
File:		Idelete.cxx
Description:	Command-line delete utility for re-Isearch databases
@@@*/

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include "common.hxx"
#include "idb.hxx"
#include "record.hxx"
#include "strlist.hxx"
#include "dtreg.hxx"

class IDBC : public IDB {
public:
  IDBC(const STRING& NewPathName, const STRING& NewFileName, const STRLIST& NewDocTypeOptions) :
	IDB(NewPathName, NewFileName, NewDocTypeOptions) { };
protected:
  void IndexingStatus(const t_IndexingStatus StatusMessage, const STRING& FileName,
	const INT WordCount) { };
};

typedef IDBC* PIDBC;

#include "registry.hxx"

static const char *wastebasket = ".wastebasket";

static int Link(const STRING& from, const STRING& to)
{
  if (Exists(to)) return -1;
  if (!Exists(from)) return 0; 

  if (FileLink (from, to) < 0)
    {
      (void) unlink (to);
      return -1;
    }
  return 0;
}

int main (int argc, char **argv)
{
  if (argc < 2)
    {
      cout << "Idelete -- Program to delete files from the index.\n\
Usage: Idelete [options] file ...\n\
  -d (X)      // Use (X) as the root name for database files.\n\
  -o (X)      // Document type specific option.\n\
  -sub (X)    // Default=\"" << wastebasket << "\"\n\
  -pre (X)]   // Prepend base to path (default=\"\")\n\
  -quiet      // Quiet\n\
  -f [(X)|-]  // Read list from a file (X) or - for stdin\n\
  -mark       // Just mark as Deleted and don't move or remove\n\
              // like Iutil -del but by file name and not by key.\n\
  -shredder   // WARNING: Dangerous option!!\n\
              // Don't move things to wastebasket but REMOVE/DELETE!\n\
              // WARNING: THIS REMOVES AND DESTROYS THE SOURCE!!!!!!!\n\
  -debug      // debug...\n\n\
Example: " << argv[0] << " -d DATABASE foo\n\
  // this marks the records in foo as deleted and moves\n\
  // path/foo to path/" << wastebasket << "/foo.\n\n\
Note: In the datebase.ini [Idelete] .wasterbasket= and .prepend= \n\
override the command line arguments!" << endl;
      return 0;
    }
  STRLIST DocTypeOptions;
  STRING Flag;
  STRING DBName;
  STRLIST WordList;
  STRING  prepend;
  GDT_BOOLEAN DebugFlag = GDT_FALSE;
  GDT_BOOLEAN Shredder  = GDT_FALSE;
  GDT_BOOLEAN Dont_delete = GDT_FALSE;
  GDT_BOOLEAN QuietFlag = GDT_FALSE;
  GDT_BOOLEAN VerboseFlag = GDT_FALSE;
  INT x = 0;
  INT LastUsed = 0;
  while (x < argc)
    {
      if (argv[x][0] == '-')
	{
	  Flag = argv[x];
	  if (Flag.Equals ("-o"))
	    {
	      if (++x >= argc)
		{
		  message_log(LOG_ERROR, "Usage Error: No option specified after -o.");
		  return 0;
		}
	      STRING S;
	      S = argv[x];
	      DocTypeOptions.AddEntry (S);
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-q") || Flag.Equals("-quiet"))
	    {
	      QuietFlag = 1;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-d"))
	    {
	      if (++x >= argc)
		{
		  message_log(LOG_ERROR, "Usage Error: No database specified after -d.");
		  return 0;
		}
	      DBName = argv[x];
	      LastUsed = x;
	    }
          else if (Flag.Equals ("-pre") || Flag.Equals("-base"))
            {
              if (++x >= argc)
                {
		  message_log(LOG_ERROR, "Usage Error: No prepend directory specified after -%s.", Flag.c_str());
                  return 0;
                }
              prepend = argv[x];
              LastUsed = x;
            }
	  else if (Flag.Equals ("-f") )
	    {
              if (++x >= argc)
                {
		  message_log(LOG_ERROR, "Usage Error: Nothing specified after -%s.", Flag.c_str());
                  return 0;
                }
	      char *arg = argv[x];
	      LastUsed = x;
	      FILE *fp;
	      if (arg[0] == '-' && arg[1] == '\0')
		fp = stdin;
	      else if ((fp = fopen(arg, "rt")) == NULL)
		{
		  message_log (LOG_ERRNO, "ERROR: can't open filelist %s!", arg);
		  return -1;
		}
	      char buf[BUFSIZ+1];
	      STRING S;
	      while ( fgets(buf, BUFSIZ, fp) != NULL)
		{
		  if (*buf)
		    {
		      (S = buf).Strip();
		      if (!S.IsEmpty())
			WordList.AddEntry(S);
		    }	
		}
	      if (fp != stdin)
		fclose(fp);
	    }
	  else if (Flag.Equals ("-sub"))
	    {
              if (++x >= argc)
                {
		  message_log(LOG_ERROR, "Usage Error: No wastedir specified after -sub.");
                  return 0;
                }
              wastebasket = argv[x];
              LastUsed = x;
	    }
	  else if (Flag.Equals ("-verbose"))
	    {
	      QuietFlag = 0;
	      LastUsed = x;
	    }
	  else if (Flag.Equals ("-debug"))
	    {
	      DebugFlag = 1;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-mark"))
	    {
	      Dont_delete = GDT_TRUE;
	      LastUsed = x;
	    }
	  else if (Flag.Equals("-shredder"))
	    {
	      Shredder = GDT_TRUE;
	      LastUsed = 1;
	    }
	}
      x++;
    }

  if (DBName.Equals (""))
    {
      DBName = __IB_DefaultDbName;
    }

  x = LastUsed + 1;

//      RECLIST reclist;
  //      RECORD record;
  //      STRING PathName, FileName;

  STRING S;
  for (int z = x; z < argc; z++)
    WordList.AddEntry ( argv[z] );

  // we need to prevent bad combinations of options, such as -erase and -del together

  PIDBC pdb;
  STRING DBPathName, DBFileName;

  DBPathName = DBName;
  DBFileName = DBName;
  RemovePath (&DBFileName);
  RemoveFileName (&DBPathName);

#if 0
  // Check if database exits
  STRING DbInfo = DBPathName + DBFileName + ".dbi";
  if (Exists (DbInfo) == GDT_FALSE)
    {
      cout << "The specified database \"" << DbInfo << "\" does not exist." << endl;
      return 0;
    }
#endif

  pdb = new IDBC (DBPathName, DBFileName, DocTypeOptions);

  if (DebugFlag)
    {
      pdb->DebugModeOn ();
    }

  if (!pdb->IsDbCompatible ())
    {
      cout << "The specified database is not compatible with this version of Iutil." << endl;
      cout << "Please use matching versions of Iindex, Isearch, and Iutil." << endl;
      delete pdb;
      return 0;
    }

  STRING myWastebasket;
  pdb->ProfileGetString("Idelete", ".wastebasket", wastebasket, &myWastebasket);
  pdb->ProfileGetString("Idelete", ".prepend", prepend, &prepend);

  RECORD Record;
  const INT y = pdb->GetTotalRecords ();
  STRING Fullname, Filename, Path, NewPath;
  STRING last, NewName, LastName;
  STRING Key;
  STRLIST delKeys;
  INT TotalDeleted = 0;

  INT zTotal = WordList.GetTotalEntries();
  for (x = 1; x <= y; x++)
    {
      if (!pdb->GetDocumentDeleted (x))
	{
	  pdb->GetDocumentInfo (x, &Record);
	  Record.GetKey (&Key);
	  if (!QuietFlag)
	    cout << Key;
	  Record.GetFileName (&Filename);
	  if (!QuietFlag)
	    cout << '\t' << Filename;
	  if (!QuietFlag)
	    cout << "\t(" << Record.GetRecordStart () << '-' << Record.GetRecordEnd () << ")\t";
	  Record.GetFullFileName (&Fullname);
	  if (!QuietFlag)
	    cout << Fullname;
	  if (!QuietFlag)
	    cout << endl;
	  for (INT w = 1; w <= zTotal; w++)
	    {
	      S = WordList.GetEntry (w);
	      if ((S == Fullname) || (S == Filename))
		{
		  if (Shredder || Dont_delete)
		    {
		      if (!QuietFlag)
			cout << "\t" << Key << " ---------> (Nil)";
                      TotalDeleted += pdb->DeleteByKey (Key);   // Delete record

		      if (last != Fullname)
			{
			  if (Shredder)
			    {
			      unlink ( Fullname );
			      if (!QuietFlag) cout << "\tUNLINKED";

			    }
			  last = Fullname;
			}
                      Record.SetFullFileName(NulString);
		      Record.SetBadRecord(GDT_TRUE);
                      pdb->SetDocumentInfo (x, Record);
		      if (!QuietFlag)
			cout << endl;
		      continue;
                    }

		  Path = prepend;
		  Path += Record.GetPath ();
		  Path += myWastebasket;
		  // Move file to wastebasket
#ifdef _WIN32
#define mask (S_IRUSR | S_IWUSR)
#else
#define mask (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif
		  if (-1 == MkDir (Path, mask))
		    message_log (LOG_ERRNO, "Error: could not make directory '%s'", Path.c_str());
		  INT count = 0;
		  do
		    {
		      NewName = Filename;
		      NewName += ".";
		      NewPath = count++;	//scrach

		      NewName += NewPath;
		      NewPath = Path;
		      NewPath += "/";
		      NewPath += NewName;
		    }
		  while (Exists (NewPath) && (NewName != LastName));
		  LastName = NewName;

		  if ((last != Fullname) && Link (Fullname, NewPath) == -1)
		    {
		      message_log (LOG_ERRNO, "Error: could not link '%s' to '%s'", Fullname.c_str(),
			NewPath.c_str());
		      if (Exists (NewPath))
			message_log (LOG_ERROR, "! Only 1 (one) revision currently supported!");
		    }
		  else
		    {
		      last = Fullname;	// Cache to to inform if rename already done

		      if (!QuietFlag)
			cout << '\t' << Fullname << " --> " << NewPath << endl;
		      TotalDeleted += pdb->DeleteByKey (Key);	// Delete record

		      Record.SetBadRecord();

		      Record.SetPath (Path);	// Change path

		      Record.SetFileName (NewName);	// Change name

		      pdb->SetDocumentInfo (x, Record);

		      message_log (LOG_DEBUG, "Unlinking '%s'", Fullname.c_str());
		      if (unlink((const char *)Fullname) < 0)
			message_log (LOG_ERRNO, "Could not unlink '%s'", Fullname.c_str());
		    }
		}
	    }
	}
    }
  if (TotalDeleted)
    cout << TotalDeleted << " record(s) deleted." << endl;
  else
    cout << "Nothing Found" << endl;
  delete pdb;
  return 0;
}
