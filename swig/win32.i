/*******************************************************************************/
%{

#ifndef _T
# ifdef  _UNICODE
#  define __T(x)      L ## x
# else
#  define __T(x)      x
# endif
# define _T(x)       __T(x)
#endif

HINSTANCE __hInstance = NULL;

double getTimeDiff(time_t &t1, bool bResetTimer)
{
    time_t t2;
    time (&t2);
    double dif = difftime (t2,t1);
    if(bResetTimer)
    {
        time (&t1);
    }
    return dif;
}
STRING GetMyDir(HINSTANCE h)
{
    static STRING sMyDir = "";
    if(sMyDir.GetLength() > 0)
        return sMyDir;

    char p[1024];
    ZeroMemory(p, 1024);
    GetModuleFileName(h, p, 1024);
    sMyDir = RemoveFileName(p);

    if((sMyDir.GetChar(sMyDir.GetLength() -1) == '\\') ||
       (sMyDir.GetChar(sMyDir.GetLength() -1) == '/'))
    {
        sMyDir = sMyDir.Left(sMyDir.GetLength()-1);
    }
    if(h == NULL)
    {
        STRING sMyDir2 = sMyDir;
        sMyDir = "";
        return sMyDir2;
    }
    return sMyDir;
}
STRING GetMyFN(BOOL bWithExt, HINSTANCE h)
{
    static STRING sMyFNWithExt      = "";
    static STRING sMyFNWithoutExt   = "";
    if(bWithExt  && sMyFNWithExt.GetLength() > 0)
        return sMyFNWithExt;
    if(!bWithExt && sMyFNWithoutExt.GetLength() > 0)
        return sMyFNWithoutExt;

    char p[1024];
    ZeroMemory(p, 1024);
    GetModuleFileName(h, p, 1024);
    sMyFNWithExt = p;
    sMyFNWithExt = RemovePath(sMyFNWithExt);

    int iLastDot = sMyFNWithExt.Find('.',TRUE/*bFromEnd*/);
    if (iLastDot < 1)
        sMyFNWithoutExt = sMyFNWithExt;
    else
        sMyFNWithoutExt = sMyFNWithExt.Before('.');

    return sMyFNWithoutExt;
}
STRING __GetCFG(HINSTANCE h, STRING sEntry, STRING sSection="CFG", STRING sFN="");
STRING __GetCFG(HINSTANCE h, STRING sEntry, STRING sSection/*="CFG"*/, STRING sFN/*=""*/)
{
    if (sFN.IsEmpty())
        sFN = GetMyDir(h)+"/"+GetMyFN(FALSE,h)+".cfg";  //"ipc" for engine-driver,, "mod" for ipview, "properties" for web, "cfg" for all other
    STRING sTitle = "dummy";
    REGISTRY r(sTitle);
    r.ProfileAddFromFile(sFN);
    STRING sDefault = "";
    STRING sEntryValue = "";
    r.ProfileGetString(sSection, sEntry,sDefault,&sEntryValue);
    return sEntryValue;
}
unsigned int GetDebug(HINSTANCE h)
{
    static unsigned int iDebug = 99999;
    if(iDebug != 99999)
        return iDebug;
    STRING sDebug = __GetCFG(h, "debug");
    sDebug.Trim(false);
    sDebug.Trim(true);
    if(sDebug == "")
    {
        if(h == NULL)   return 0;
        iDebug = 0;
        return iDebug;
    }
    if(sDebug == "1")
    {
        iDebug = 1;
        return iDebug;
    }
    iDebug = atoi(sDebug);
    return iDebug;
}
BOOL __Log(HINSTANCE h, STRING s2log)
{
    if(GetDebug(h) < 1) return TRUE;

    static STRING sfn2open = GetMyDir(h)+"/"+GetMyFN(FALSE,h)+".log";
    //static STRING sfn2open = "c:\\temp\\os.independent.c.log";
    s2log = _T(s2log+"\n");
    FILE *fp = fopen(sfn2open, "a+"/*append*/);
    if (fp == NULL)
        fp = fopen("c:\\temp\\os.independent.c.log", "a+"/*append*/);
    if (fp != NULL)
    {
        if(GetDebug(h) > 1)
        {
            static time_t t1;
            static BOOL b_time_called = FALSE;
            if(!b_time_called)
            {
                time (&t1);
                b_time_called = TRUE;
            }
            STRING sdif;            //sdif.form("%lf, t1:'%d', b_time_called:'%d'",getTimeDiff(t1, true), &t1, b_time_called);
            sdif.form("%.0lf",getTimeDiff(t1, true));
            s2log = _T(sdif + ":" + s2log);
        }
        s2log.WriteFile(fp);
        fclose(fp);
        return TRUE;
    }
    return FALSE;
}
void i2s(STRING &s, int i)
{
    s.form("%d", i);
}
STRING i2s(int i)
{
    STRING s;
    i2s(s,i);
    return s;
}
int __ib__output(int iLevel, const char* sMsg)
{
    if(GetDebug(__hInstance) < 1)  return 0;

    STRING s;
    switch(iLevel)
    {
        case LOG_PANIC: s = "PANIC";    break;
        case LOG_FATAL: s = "FATAL";    break;
        case LOG_ERROR: s = "ERR";      break;
        case LOG_ERRNO: s = "ERRNO";    break;
        case LOG_WARN:  s = "WRN";      break;
        case LOG_NOTICE:s = "NOTICE";   break;
        case LOG_INFO:  s = "INF";      break;
        case LOG_DEBUG: s = "DBG";      break;
        case LOG_ALL:   s = "ALL";      break;
        case LOG_ANY:   s = "ANY";      break;
        default:        s = "?"+i2s(iLevel)+"?";        break;
    }
    __Log(__hInstance, "__ib__output\t"+s+":"+STRING(sMsg));
    return 0;
}

int my_ib_ResolveConfigPath(const char *Filename, char *buffer, int length)
{
    if(__hInstance == NULL) return 0;

    static STRING sMyDir(GetMyDir(__hInstance)+"\\");

    STRING sFPN(sMyDir + Filename);
    if(_access(sFPN,0) != 0) return 0;

    if (((int)sFPN.GetLength()) >= length)
    {
        __Log(__hInstance, "my_ib_ResolveConfigPath\tERR:fn.too.long:'"+sFPN+"'");
        return 0;
    }
    ::lstrcpy(buffer, sFPN);
    return sFPN.GetLength();
}
int my_ib_ResolveBinPath(const char *Filename, char *buffer, int length)
{
    //__Log(__hInstance, "my_ib_ResolveBinPath\tINF:fn:'"+CSHISTRING(Filename)+"'");
    if(__hInstance == NULL) return 0;

    static STRING sMyDir(GetMyDir(__hInstance)+"\\");

    STRING sFPN(sMyDir + Filename);

    if(_access(sFPN,0) != 0) return 0;

    if ((int)sFPN.GetLength() >= length)
    {
        __Log(__hInstance, "my_ib_ResolveBinPath\tERR:fn.too.long:'"+sFPN+"'");
        return 0;
    }
    ::lstrcpy(buffer, sFPN);
    return sFPN.GetLength();
}


extern "C" int WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    __hInstance = hInstance;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            log_init((GetDebug(__hInstance) > 0) ? LOG_ALL : 0);
            log_init(LOG_ALL, __ib__output);

            _ib_ResolveConfigPath = my_ib_ResolveConfigPath;
        _ib_ResolveBinPath = my_ib_ResolveBinPath;

            return TRUE;

        case DLL_PROCESS_DETACH:
            return TRUE;
        default:
            return TRUE;
    }
    return TRUE;
}
/*********************************************************************************/
%}
