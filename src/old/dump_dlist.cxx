#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <errno.h>
#include "common.hxx"
#include "date.hxx"
#include "dlist.hxx"
#include "magic.hxx"

typedef UINT4  _Count_t;

int
main (int argc, char **argv) {

  FILE*       Fp;
  _Count_t    Total;
  DATEFLD     Value;
  const char *index;
  int         id;

  if (argc < 2) {
    cout << "You forgot the file name" << endl;
    exit(1);
  }
  index = argv[1];

  if ((Fp = fopen(index, "rb")) == NULL) {
    cout << "ERROR: Can't open \"" << index << "\": " << strerror(errno) << "!" << endl;
    exit(1);
  }

  if ((id = getObjID(Fp)) != objDLIST)
    {
      const char *s = NULL;
      const char *t = "";
      fclose(Fp);
      cout << "ERROR: " << index << " is not a numerical ";
      switch (id) {
	case objNLIST:     t="number", s = "nlist"; break;
	case objGPOLYLIST: t="GPoly"; s = "gpolylist"; break;
	case objDLIST:     t="date"; s = "dlist"; break;
	case objINTLIST:   t="integer"; s = "intlist"; break;
	case objBBOXLIST:  t="bounded box"; s = "bboxlist"; break;
      }
      if (s) cout << "but a " << t << " index file. Use dump_" << s << " instead!" << endl;
      else if (id > 0) cout << "index ? File is type: " << id << endl;
      else cout << "but a standard index file!" << endl;

      exit(1);
    }

  cout << "First block - sorted by numeric value" << endl;
  Read(&Total, Fp);

  cout << "Total = " << Total << endl;

  for (int x=0; x<Total; x++) {
    
    ::Read(&Value, Fp);     cout << "Value = " << Value << endl;;
  }

  cout << endl << endl;
  cout << "Second block - sorted by GP" << endl;
  for (int x=0; x<Total; x++) {
    ::Read(&Value, Fp);    cout << "Value = " << Value << endl; 
  }

  cout << endl;
  cout << "At end of file? " ;
  if (ftell(Fp) < GetFileSize(Fp))
    cout << "Not at end of file Pos=" << ftell(Fp) << " of " << GetFileSize(Fp) << endl;

  exit(0);
}
