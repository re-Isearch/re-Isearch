/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>

#include "dirent.hxx"
#include "common.hxx"
#include "idb.hxx"
#include "string.hxx"
#include "record.hxx"


RECORD r;

int makeloop()
{
    STRING sIDX = "/tmp/index/IDXNAME_";
    STRING sDOCS= "/usr/local/www/data/nonmonotonic.net/Z39.50/.";
    for(int i=0; i < 999; i++)
    {
        IDB* pIDB = new IDB(STRING().form("%s-%d", sIDX.c_str(), i), false);
        pIDB->SetIndexingMemory(1000);
        int iLoops=0;
        {
            struct dirent *pdirentry = NULL;
            DIR* pDir = opendir(sDOCS);
	    if (pDir == NULL)
	     printf("Can't open '%s'\n", sDOCS.c_str());
            else while((pdirentry = readdir(pDir)) != NULL)
            {
                if(strcmp(pdirentry->d_name, ".") == 0) continue;
                if(strcmp(pdirentry->d_name, "..")== 0) continue;
                iLoops++;
                if(iLoops > 900)break;
                char abspathname[1024];
                strcpy(abspathname, sDOCS);
                abspathname[strlen(sDOCS)] =  '/';
                strcpy(abspathname + strlen(sDOCS) + 1 , pdirentry->d_name);
        
                struct stat fstt;
                if(!lstat(abspathname, &fstt) == 0)
                {
                    printf("EEEE1");
                    continue;
                }
                if(S_ISDIR(fstt.st_mode) != 0) 
                {
		    printf("%s Is a dir... ",  abspathname);
                    continue;
                }
		//printf("Add %s\n", abspathname);
                //RECORD r(abspathname);
                r.SetFullFileName(abspathname);
                r.SetDocumentType("AUTODETECT");
                if(!pIDB->AddRecord(r))
                    printf("EEEE2");
                
            }
            pIDB->Index(true);
        }
        pIDB->ffGC();
        delete pIDB;
        pIDB = NULL;
        printf("loop:%d",i);
    }
    return 0;
}

main() {
  makeloop();

}
