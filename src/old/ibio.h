#ifndef IBIO

#ifdef USE_SFIO
#include <sfio.h>

extern Sfio_t*	_stdopen(int, const char*);
extern int	_stdprintf(const char*, ...);

#define IBIO                            Sfio_t
#define IBIO_stderr()			sfstderr
#define IBIO_stdout()			sfstdout
#define IBIO_stdin()			sfstdin

#define IBIO_printf			sfprintf
#define IBIO_stdoutf			_stdprintf
#define IBIO_vprintf(f,fmt,a)		sfvprintf(f,fmt,a)          
#define IBIO_read(f,buf,count)	sfread(f,buf,count)
#define IBIO_write(f,buf,count)	sfwrite(f,buf,count)
#define IBIO_open(path,mode)		sfopen(NULL,path,mode)
#define IBIO_fdopen(fd,mode)		_stdopen(fd,mode)
#define IBIO_reopen(path,mode,f)	sfopen(f,path,mode)
#define IBIO_close(f)			sfclose(f)
#define IBIO_puts(f,s)		sfputr(f,s,-1)
#define IBIO_putc(f,c)		sfputc(f,c)
#define IBIO_ungetc(f,c)		sfungetc(f,c)
#define IBIO_sprintf			sfsprintf
#define IBIO_getc(f)			sfgetc(f)
#define IBIO_eof(f)			sfeof(f)
#define IBIO_error(f)			sferror(f)
#define IBIO_fileno(f)		sffileno(f)
#define IBIO_clearerr(f)		sfclrerr(f)
#define IBIO_flush(f)			sfsync(f)
#define IBIO_tell(f)			sftell(f)
#define IBIO_seek(f,o,w)		sfseek(f,o,w)
#define IBIO_rewind(f)		(void) sfseek((f),0L,0)
#define IBIO_tmpfile()		sftmp(0)


#else		/* ! USE_SFIO */

#define IBIO                            FILE
#define IBIO_stderr()                   stderr
#define IBIO_stdout()                   stdout
#define IBIO_stdin()                    stdin

#define IBIO_printf                     fprintf
#define IBIO_stdoutf                    printf
#define IBIO_vprintf(f,fmt,a)           vprintf(f,fmt,a)
#define IBIO_read(f,buf,count)          fread(f,buf,count)
#define IBIO_write(f,buf,count)         fwrite(f,buf,count)
#define IBIO_open(path,mode)            fopen(NULL,path,mode)
#define IBIO_fdopen(fd,mode)            fdopen(fd,mode)
#define IBIO_reopen(path,mode,f)        reopen(f,path,mode)
#define IBIO_close(f)                   fclose(f)
#define IBIO_puts(f,s)                  puts(f,s)
#define IBIO_putc(f,c)                  putc(f,c)
#define IBIO_ungetc(f,c)                ungetc(f,c)
#define IBIO_sprintf                    sprintf
#define IBIO_getc(f)                    getc(f)
#define IBIO_eof(f)                     feof(f)
#define IBIO_error(f)                   ferror(f)
#define IBIO_fileno(f)                  fileno(f)
#define IBIO_clearerr(f)                clearerr(f)
#define IBIO_flush(f)                   flish(f)
#define IBIO_tell(f)                    ftell(f)
#define IBIO_seek(f,o,w)                fseek(f,o,w)
#define IBIO_rewind(f)                 (void) fseek((f),0L,0)
#define IBIO_tmpfile()                 tmpfile()

#endif
#endif
