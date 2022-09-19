/*@@@
File:           lock.hxx
Version:        1.00
Description:    Locking functions
Author:         Edward C. Zimmermann, edz@bsn.com
@@@*/
#ifndef _LOCK_HXX
#define _LOCK_HXX

INT Lock (const char *dbFileStem, int Flags);
INT UnLock (const char *DbFileStem, int Flags);
INT LockWait(const char *DbFileStem, const int Flags = (L_READ|L_WRITE), const int secs = 60);

#endif
