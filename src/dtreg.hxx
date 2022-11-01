/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/*--@@@
File:		dtreg.hxx
Description:	Class DTREG - Document Type Registry
@@@*/

#ifndef DTREG_HXX
#define DTREG_HXX

#include "../doctype/doctype.hxx"

class DoctypesClassRegistry;

#ifndef DOCTYPE_HXX
typedef DOCTYPE *PDOCTYPE;
#endif


typedef DOCTYPE* (*dt_constr_t) (IDBOBJ* parent, const STRING& Name);
typedef const char* (*dt_query_t) (void);


class           DTREG {
 public:
  DTREG(PIDBOBJ DbParent);
  DTREG(PIDBOBJ DbParent, const STRING& PluginsPath);

  bool     PluginExists(const STRING& Name) const;

  void            AddPluginPath(const STRING& Path);

  DOCTYPE        *GetDocTypePtr(UINT2 Id);
  DOCTYPE        *GetDocTypePtr(const STRING& Doctype);
  DOCTYPE        *GetDocTypePtr(const DOCTYPE_ID& Id);
  DOCTYPE        *GetDocTypePtr(const STRING& DocType, const STRING& DocTypeID);

  DOCTYPE        *RegisterDocType (const STRING& name, DOCTYPE* Ptr);
  bool     UnregisterDocType (const STRING& name);

  bool     ValidateDocType(const STRING& DocType);
  bool     ValidateDocType(const DOCTYPE_ID& Id);
  int             DoctypeId(const STRING& DocType);

  const           STRLIST& GetDocTypeList();
  void            PrintDoctypeList(ostream& os = cout) const;
  void            PrintDoctypeHelp(const STRING& Doctype, ostream& os = cout);
  int             Version() const;
  ~DTREG();

 private:
  DoctypesClassRegistry *DoctypesRegistry;
  STRING          LastDoctype;
  int             LastId;
  /* Document Handlers */
  PIDBOBJ         Db;
  STRLIST         DocTypeList;
  // Very private
  void BuildPluginList (const STRING& dir, STRLIST& dtlist, STRLIST *List = NULL) const;
  dt_constr_t get_doctype_constr (const STRING& dir, const STRING& dt_name) const;
  dt_constr_t open_doctype_constr(const STRING& path, const STRING& doctype) const;
  STRLIST PluginsSearchPath;
  int             pluginsLoaded;
};

typedef DTREG  *PDTREG;

// General Function

// General Read (via Doctype) Function
size_t ReadIndirect(FILE *Fp, char *Buffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t ReadIndirect(FILE *Fp, STRING *StringBuffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t ReadIndirect(const STRING& Filename, char *Buffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t ReadIndirect(const STRING& Filename, STRING *StringBuffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);

// General Read (via Doctype) for Presents Function
size_t GetRecordData(FILE *Fp, char *Buffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t GetRecordData(FILE *Fp, STRING *StringBuffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t GetRecordData(const STRING& Filename, char *Buffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);
size_t GetRecordData(const STRING& Filename, STRING *StringBuffer, off_t Start, size_t Length,
        const DOCTYPE *DoctypePtr = NULL);


// Print list of public doctypes (to cout)
void   PrintDoctypeList(ostream& os = cout);
// Print specific help
void   PrintDoctypeHelp(const STRING& Doctype, ostream& os = cout);

#endif
