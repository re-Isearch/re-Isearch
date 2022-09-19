// Private Header file
// The constants in this file are closely tied into
// the internal functioning of the Isearch fastload
// binary files..

#ifndef _MAGIC_H
#define _MAGIC_H 1

// Struct IDs
typedef enum OBJ_IDS  {
  objATTR = 1,  // 1
  objATTRLIST,  // 2
  objDF,        // 3
  objDFT,       // 4
  objDFD,	// 5
  objDFDT,	// 6
  objFC,        // 7
  objFCT,       // 8
  objIRESULT,   // 9
  objIRSET,     // 10
  objMDTREC,    // 11
  objRECLIST,   // 12
  objRECORD,    // 13
  objRESULT,    // 14
  objRSET,      // 15
  objSQUERY,    // 16
  objBCPL,      // BCPL String (max 255 chars)
  objSTRING,    // Our String (max 65536 chars) 
  objSTRLIST,   // 19
  objDATE,      // 20
  objDATERANGE, // 21
  objRCACHE,    // 22 
  objGPOLYFLD,  // 23
  objDATEFLD,   // 24
  objNUMFLD,    // 25
  objGPOLYLIST, // 26
  objNLIST,     // 27
  objDLIST,     // 28
  objINTLIST,   // 29
  objBBOXLIST,  // 30
  objRESOURCE,  // 31
  objSCANLIST,  // 32
  objGEOSCORE,  // 33
  objINODE,     // 34
  objQUERY,     // 35

  // .inx magic
  objINDEXm = 0x49, // MSB Index
  objINDEXl = 0x69 // LSB Index
} obj_t;

inline obj_t getObjID(PFILE Fp)
{
  return (obj_t)( fgetc(Fp) & '\377');
}

inline void putObjID(obj_t c, PFILE Fp)
{
  fputc(c, Fp);
}

inline void PushBackObjID(obj_t c, PFILE Fp)
{
  ungetc(c, Fp);
}

#endif
