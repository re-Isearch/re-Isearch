#ifndef MARC_HXX
#define MARC_HXX

#include "gdt.h"
#include "marclib.hxx"
#include "string.hxx"

//
// Display formats:
//
#define MARC_FORMAT_DEFAULT 0
#define MARC_FORMAT_SHORT 1
#define MARC_FORMAT_MARC 2
#define MARC_FORMAT_EVALUATION 3
#define MARC_FORMAT_TITLE 4

class MARC {
	MARC_REC	*c_rec;
	char		*c_data;
	INT4		c_len;
	INT		c_format,
			c_maxlen;
public:
	MARC(STRING & Data);
	~MARC();

	void SetDisplayFormat(INT Format) { c_format = Format; }
	void SetDisplayWidth(INT Width) { c_maxlen = Width; }
	void Print(FILE *fp=stdout);
};

#endif
