/************************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery and
Retrieval, 1995. 

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby granted
without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of noteworthy
uses of this software. 

3. The names of MCNC and Clearinghouse for Networked Information Discovery
and Retrieval may not be used in any advertising or publicity relating to
the software without the specific, prior written permission of MCNC/CNIDR. 

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. 

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE
POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
************************************************************************/

/*@@@
File:		dtconf.cxx
Version:	1.00
Description:	Document Type configuration utility for Isearch
Author:		Nassib Nassar, nrn@cnidr.org
@@@*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXDT 500
#define MAXSTR 80

main() {
	printf("\n");
	printf("Configuring Isearch for the following document types (see dtconf.inf):\n");

	// Read configuration
	int x, y;
	char DtName[MAXDT][MAXSTR];
	char DtFn[MAXDT][MAXSTR];
	int TotalDt = 0;
	char s[MAXSTR], t[MAXSTR], u[MAXSTR], v[MAXSTR];
	char* p;
	char* pp;
	FILE* fp;
	FILE* fpi;
	fp = fopen("dtconf.inf", "r");
	if (fp) {
		while ( fgets(s, MAXSTR, fp) ) {
			p = s;
			while (isalnum(*p)) {	// truncate after the first word
				p++;
			}
			*p = '\0';
			if (*s != '\0') {
				strcpy(DtFn[TotalDt], s);
				sprintf(t, "%s.hxx", DtFn[TotalDt]);	// append .hxx
				fpi = fopen(t, "r");
				if (fpi) {
					x = 0;
					while ( (fgets(u, MAXSTR, fpi)) && (!x) ) {
						if (!strncmp(u, "class ", 6)) {
							x = 1;
							strcpy(v, u);
							p = v + 6;
							while (*p == ' ') {
								p++;
							}
							pp = p;
							while (isalnum(*pp)) {
								pp++;
							}
							*pp = '\0';
							strcpy(DtName[TotalDt], p);
						}
					}
					fclose(fpi);
						printf("\t%s", DtName[TotalDt]);
						fpi = fopen(t, "r");
					if (fpi) {
						x = 0;
						while ( (fgets(u, MAXSTR, fpi)) && (!x) ) {
							char *tp = u;
							while (isspace(*tp))
								tp++;
							if (!strncmp(tp, "Description:", 12)) {
								x = 1;
								if ( (p=strchr(tp, '-')) ) {
									printf(" %s", p);
								}
							}
						}
						fclose(fpi);
					} else {
						printf("\n");
					}
						TotalDt++;
					} else {
					printf("\t(File %s not found.)\n", t);
				}
			}
		}
		fclose(fp);
	}

	// Generate dtreg.hxx
	fp = fopen("../src/dtreg.hxx", "w");
	fprintf(fp, "/*@@@\n");
	fprintf(fp, "File:\t\tdtreg.hxx\n");
	fprintf(fp, "Version:\t1.00\n");
	fprintf(fp, "Description:\tClass DTREG - Document Type Registry\n");
	fprintf(fp, "Author:\t\tNassib Nassar, nrn@cnidr.org\n");
	fprintf(fp, "@@@*/\n");
	fprintf(fp, "\n");
	fprintf(fp, "#ifndef DTREG_HXX\n");
	fprintf(fp, "#define DTREG_HXX\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"defs.hxx\"\n");
	fprintf(fp, "#include \"../doctype/doctype.hxx\"\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "#include \"../doctype/%s.hxx\"\n", DtFn[x]);
	}
	fprintf(fp, "\n");
	fprintf(fp, "class DTREG {\n");
	fprintf(fp, "public:\n");
	fprintf(fp, "\tDTREG(PIDBOBJ DbParent);\n");
	fprintf(fp, "\tPDOCTYPE GetDocTypePtr(const STRING& DocType);\n");
	fprintf(fp, "\tvoid GetDocTypeList(PSTRLIST StringListBuffer) const;\n");
	fprintf(fp, "\t~DTREG();\n");
	fprintf(fp, "private:\n");
	fprintf(fp, "\tPIDBOBJ Db;\n");
	fprintf(fp, "\tPDOCTYPE DtDocType;\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "\tP%s Dt%s;\n", DtName[x], DtName[x]);
	}
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "typedef DTREG* PDTREG;\n");
	fprintf(fp, "\n");
	fprintf(fp, "#endif\n");
	fclose(fp);

	// Generate dtreg.cxx
	fp = fopen("../src/dtreg.cxx", "w");
	fprintf(fp, "/*@@@\n");
	fprintf(fp, "File:\t\tdtreg.cxx\n");
	fprintf(fp, "Version:\t1.00\n");
	fprintf(fp, "Description:\tClass DTREG - Document Type Registry\n");
	fprintf(fp, "Author:\t\tNassib Nassar, nrn@cnidr.org\n");
	fprintf(fp, "@@@*/\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"dtreg.hxx\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "DTREG::DTREG(PIDBOBJ DbParent) {\n");
	fprintf(fp, "\tDb = DbParent;\n");
	fprintf(fp, "\tDtDocType = new DOCTYPE(Db);\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "\tDt%s = 0;\n", DtName[x]);
	}
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "PDOCTYPE DTREG::GetDocTypePtr(const STRING& DocType) {\n");
	fprintf(fp, "\tif (DocType.Equals(\"\")) {\n");
	fprintf(fp, "\t\treturn DtDocType;\n");
	fprintf(fp, "\t}\n");
	fprintf(fp, "\tSTRING DocTypeID;\n");
	fprintf(fp, "\tDocTypeID = DocType;\n");
	fprintf(fp, "\tDocTypeID.UpperCase();\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "\tif (DocTypeID.Equals(\"%s\")) {\n", DtName[x]);
		fprintf(fp, "\t\tif (!Dt%s) {\n", DtName[x]);
		fprintf(fp, "\t\t\tDt%s = new %s(Db);\n", DtName[x], DtName[x]);
		fprintf(fp, "\t\t}\n");
		fprintf(fp, "\t\treturn Dt%s;\n", DtName[x]);
		fprintf(fp, "\t}\n");
	}
	fprintf(fp, "\treturn 0;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "void DTREG::GetDocTypeList(PSTRLIST StringListBuffer) const {\n");
	fprintf(fp, "\tSTRING s;\n");
	fprintf(fp, "\tSTRLIST DocTypeList;\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "\ts = \"%s\";\n", DtName[x]);
		fprintf(fp, "\tDocTypeList.AddEntry(s);\n");
	}
	fprintf(fp, "\t*StringListBuffer = DocTypeList;\n");
	fprintf(fp, "}\n");
	fprintf(fp, "\n");
	fprintf(fp, "DTREG::~DTREG() {\n");
	fprintf(fp, "\tdelete DtDocType;\n");
	for (x=0; x<TotalDt; x++) {
		fprintf(fp, "\tif (Dt%s) {\n", DtName[x]);
		fprintf(fp, "\t\tdelete Dt%s;\n", DtName[x]);
		fprintf(fp, "\t}\n");
	}
	fprintf(fp, "}\n");
	fclose(fp);

	// Generate Makefile
	fpi = fopen("../src/Makefile.org", "r");
	fp = fopen("../src/Makefile", "w");
	fprintf(fp, "#############################################################################\n");
	fprintf(fp, "#############################################################################\n");
	fprintf(fp, "#############################################################################\n");
	fprintf(fp, "#####                                                                   #####\n");
	fprintf(fp, "##### NOTE: This Makefile was generated by dtconf.  If you need to      #####\n");
	fprintf(fp, "#####       make changes to the Makefile, modify the file Makefile.org  #####\n");
	fprintf(fp, "#####       instead of this file.  The dtconf utility uses Makefile.org #####\n");
	fprintf(fp, "#####       to generate this file.                                      #####\n");
	fprintf(fp, "#####                                                                   #####\n");
	fprintf(fp, "#############################################################################\n");
	fprintf(fp, "#############################################################################\n");
	fprintf(fp, "#############################################################################\n\n");
	if (fpi) {
		while ( fgets(s, MAXSTR, fpi) ) {
			y = 0;
			if (!strncmp(s, "###DTOBJ###", 11)) {
				y = 1;
				fprintf(fp, "\tdoctype.o");
				for (x=0; x<TotalDt; x++) {
					fprintf(fp, " \\\n\t%s.o", DtFn[x]);
				}
				fprintf(fp, "\n\n");
			}
			if (!strncmp(s, "###DTHXX###", 11)) {
				y = 1;
				fprintf(fp, "\t$(DOCTYPE_DIR)/doctype.hxx");
				for (x=0; x<TotalDt; x++) {
					fprintf(fp, " \\\n\t$(DOCTYPE_DIR)/%s.hxx", DtFn[x]);
				}
				fprintf(fp, "\n\n");
			}
			if (!strncmp(s, "###DTMAKE###", 12)) {
				y = 1;
				for (x=0; x<TotalDt; x++) {
					fprintf(fp, "%s.o:$(H) $(DOCTYPE_DIR)/%s.cxx\n", DtFn[x], DtFn[x]);
					fprintf(fp, "\t$(CC) $(CFLAGS) $(INC) -c $(DOCTYPE_DIR)/%s.cxx\n", DtFn[x]);
					fprintf(fp, "\n");
				}
			}
			if (!y) {
				fprintf(fp, "%s", s);
			}
		}
		fclose(fpi);
	}
	fclose(fp);

	printf("\n");
}
