/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*=======================================================
 *
 *  CLASS IO - manipulate generalized input/output objects
 *======================================================*/

/*  functions 

    open - open file or URL
    close - close file or URL
    read - read bytes from file or URL
    write - write bytes to file
    iseek - seek to position in file or URL
    top - reset current pointer in file or URL to 0
    remove - delete current file and shutdown IO channel
    itell - tell current pointer position
    igets - read x bytes up to newline into buffer
*/
#include "platform.h"


#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <iostream>
#include "common.hxx"
#include "io.hxx"


#ifndef NO_RLDCACHE

#include "rldcache.hxx"
extern RLDCACHE* Cache;

#endif


IO::IO()                    
{ 
  debug=false; 
  Buf = NULL;  
  ptr.fp=NULL;
  type = LFILE;
}


IO::IO(const STRING& Fname, const STRING& mmode)
{
  debug=false;
  Buf = NULL;
  ptr.fp=NULL;
  open(Fname, mmode);
}



IO::~IO()                   
{
}


/*==================================================================
 *   iseek:  seek to position in an IO channel, with optional offset
 *   
 *   returns: position after seek
 *=================================================================*/
off_t IO::iseek(off_t pos) 
{
  off_t curPos;
  
  if(debug) message_log (LOG_DEBUG, "Seek to position %ld, from top of file",pos);
  switch(type){
  case LFILE:
    fseek(ptr.fp,pos,0);
    curPos=ftell(ptr.fp);
    break;
  case URL:
    curPos = pos;
    cachePtr=curPos;
    break;
    
  }
  return(curPos);
  
}


char* IO::igets(char *b, int len)
{
  char *q;
  if(debug) message_log(LOG_DEBUG,"read %d bytes as fgets",len);
  switch(type){
  case LFILE:
    q=fgets(b,len,ptr.fp);
    if(q==NULL){
      return(NULL);
    }
    break;
  case URL:
    
    int i=0;
    
    for(;;){
      b[i]=*(Buf+cachePtr++);
      if(cachePtr==BufLength){
	b[++i]='\0';
	break;
      }
      if(b[i]=='\r' || b[i]=='\n')
	b[++i]='\0';
    }
    /* read from cache */
    break;
  }
  return(q);
  
  
}


off_t IO::iseek(off_t pos, int offset) 
{
  off_t curPos;
  
  if(debug) message_log(LOG_DEBUG,"Seek to position %ld, offset %d",pos,offset);
  switch(type){
  case LFILE:
    fseek(ptr.fp,pos,offset);
    curPos=ftell(ptr.fp);
    break;
  case URL:
    if(offset==0)
      curPos=pos;
    if(offset==2)
      curPos=BufLength-pos;
    if(offset==1)
      curPos=cachePtr+pos;
    cachePtr=curPos;
    break;
  }
  return(curPos);
  
}


/*==================================================================
 *   itell:  tell current IO channel pointer position
 *   returns: position
 *=================================================================*/
off_t IO::itell()
{
  off_t xPos;
  

  if(debug) message_log(LOG_DEBUG,"Tell position in %s",id.c_str());
  switch(type){
  case LFILE:
    xPos=ftell(ptr.fp);
    break;
  case URL:
    xPos=cachePtr;
    break;
  }
  return(xPos);
}


/*==================================================================
 *   open:  open an IO channel
 *   
 *   returns: true on success, false on failure
 *=================================================================*/
bool IO::open(const STRING& Fname, const STRING& mmode)
{
  id = Fname;
  mode = mmode;

  cachePtr=0;
  if(!id.Search("://"))
    {
      if((ptr.fp=fopen(id,mode)) != NULL)
	{
	  type=LFILE;
	  if(debug) message_log(LOG_DEBUG,"Open of '%s' Succeeded", id.c_str());
	  return true;
	}
    }
  else
    {
      /* it is a URL*/
      if(debug) message_log(LOG_DEBUG, "Open %s as %s",id.c_str(), "URL");
#ifndef NO_RLDCACHE
      if (Cache == NULL)
	message_log (LOG_ERROR, "IO:open() Failed, Cache is NULL");
      else if((Buf = Cache->GetFile(id, 0,-1,(size_t*)&BufLength, (time_t)360,0))==NULL)
	message_log(LOG_ERROR, "IO:open():Cache->GetFile() failed!");
      else
	{
	  type=URL;
	  if(debug) message_log(LOG_DEBUG,"Open of '%s' Succeeded", id.c_str());
	  return true; // Success
	}
#else
      message_log (LOG_ERROR, "IO:open() Failed, Cache is NULL");
#endif
     }

  if(debug) message_log(LOG_DEBUG, "Open Failed for '%s'",id.c_str());
  return false;
}


/*==================================================================
 *   close:  close an open IO channel
 *=================================================================*/
void IO::close()
{
  if (debug && !id.IsEmpty()) message_log(LOG_DEBUG,"Close %s",id.c_str());
  switch (type) {
  case LFILE:
    if(ptr.fp){
      fclose(ptr.fp);
      ptr.fp=NULL;
      id.Clear();
    }else{
      if(debug) message_log(LOG_DEBUG,"Attempt to close closed LFILE");
    }
    break;
  case URL:
    if(Buf != NULL) {
      free(Buf);
      Buf = NULL;
    }
#ifndef NO_RLDCACHE
   RLDENTRY *NewEntry = Cache ? Cache->GetEntryByName(id) : NULL;
   if (NewEntry) {
cerr << "DELETE ENTRY" << endl;
      Cache->DeleteEntry(NewEntry);
   }
   ptr.p=0;
#endif
    break;
  }
}


/*==================================================================
 *   remove:  destroy object accessed by IO channel
 *   returns - true on success, false on failure
 *
 *   side effects - closes IO channel if open.
 *=================================================================*/
int IO::remove()
{
  int status = -1;
  
  if(debug) message_log(LOG_DEBUG,"Close and unlink %s",id.c_str());
  switch(type){
  case LFILE:
    close();
    status=unlink(id);
    status++;			// unlink returns 0 on success, -1 on failure
    id = NulString;
    break;
  case URL:
    close();
    status=1;
    break;
  }
  return(status);
}


/*==================================================================
 *   read:  read length items of size bytes (length*size bytes) from
 *   IO channel.
 *
 *   returns - items read, 0 for EOF
 *=================================================================*/
size_t IO::read(char *buffer, size_t length, size_t size) 
{
  size_t readSize;
  
  if(debug){
    message_log(LOG_DEBUG,"Read %d items of %d bytes from %s\n",length, size,id.c_str());
  }
  switch(type){
  case LFILE:
    if(ptr.fp){
      readSize=fread((void *)buffer,length,size,ptr.fp);
      if(debug) message_log(LOG_DEBUG,"Read  %d items",readSize);
    }
    else{
      if(debug) message_log(LOG_DEBUG,"Attempt to read from closed file %s",id.c_str());
      readSize=0;
    }
    break;
  case URL:
    size_t i=0;
    off_t startPoint;
    
    for(startPoint=cachePtr;cachePtr<(startPoint+(length*size)); cachePtr++){
      buffer[i++]=*(Buf+cachePtr);
      if(i==BufLength)
	break;
    }
    readSize=i;
    break;
  }
  return(readSize);
}


/*==================================================================
 *   write:  write length items of size bytes (length*size bytes) to
 *   IO channel.
 *
 *   returns - items written
 *=================================================================*/
size_t IO::write(const char *buffer, size_t length, size_t size)
{
  size_t writeSize;
  
  if(debug) message_log(LOG_DEBUG,"Write %d items of %d bytes to %s",length, size,id.c_str());
  switch(type){
  case LFILE:
    if(ptr.fp)
      writeSize=fwrite(buffer,size,length,ptr.fp);
    else{
      if(debug) message_log(LOG_DEBUG,"Attempt to write to closed file %s",id.c_str());
      writeSize=0;
    }
    break;
  case URL:			// write operation not permitted
    writeSize=0;
    if(debug) message_log(LOG_DEBUG,"Write operation not permitted to URL %s",id.c_str());
    break;
  }
  return(writeSize);
}


void IO::top()
{
  if(debug) message_log(LOG_DEBUG,"set file %s pointer to 0",id.c_str());
  switch(type){
  case LFILE:
    if(ptr.fp)
      rewind(ptr.fp);
    else
      if(debug) message_log(LOG_DEBUG,"File %s not open for rewind",id.c_str());
    break;
  case URL:
    cachePtr=0;
    break;
  }
}
