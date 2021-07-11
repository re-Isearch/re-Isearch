#ifndef _IB_STDIO_H
# define _IB_STDIO_H
#include <stdio.h>

extern FILE*  _IB_Extern_fopen(const char *, const char*);
extern FILE*  _IB_Extern_freopen(const char *, const char*, FILE *);
extern int    _IB_Extern_fclose(FILE *);
extern int    _IB_Extern_fseek(FILE *, long, int);
extern long   _IB_Extern_ftell(FILE *);
extern int    _IB_Extern_fseeko(FILE *, off_t, int);
extern off_t  _IB_Extern_ftello(FILE *);
extern void   _IB_Extern_rewind(FILE *);
extern int    _IB_Extern_fgetpos(FILE *, fpos_t *);
extern int    _IB_Extern_fsetpos(FILE *, const fpos_t *);
extern char*  _IB_Extern_fgets(char *, int, FILE *);
extern size_t _IB_Extern_fread(void *, size_t, size_t, FILE *);
extern size_t _IB_Extern_fwrite(const void *, size_t, size_t, FILE *);
extern int    _IB_Extern_open(const char *path, int flags, mode_t mode);
extern int    _IB_Extern_close(int fd);

#define fopen   _IB_Extern_fopen
#define freopen _IB_Extern_freopen
#define fclose  _IB_Extern_fclose
#define fseek   _IB_Extern_fseek
#define ftell   _IB_Extern_ftell
#define fseeko  _IB_Extern_fseeko
#define ftello  _IB_Extern_ftello
#define rewind  _IB_Extern_rewind
#define fgetpos _IB_Extern_fgetpos
#define fsetpos _IB_Extern_fsetpos
#define fgets   _IB_Extern_fgets
#define fread   _IB_Extern_fread
#define fwrite  _IB_Extern_fwrite

#endif
