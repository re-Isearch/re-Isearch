

void change_encoding_from_UTF8_with_BOM_to_ANSI(const char* filename)
{
    ifstream infile;
    string strLine="";
    string strResult="";
    infile.open(filename);
    if (infile)
    {
        // the first 3 bytes (ef bb bf) is UTF-8 header flags
        // all the others are single byte ASCII code.
        // should delete these 3 when output
        getline(infile, strLine);
        strResult += strLine.substr(3)+"\n";

        while(!infile.eof())
        {
            getline(infile, strLine);
            strResult += strLine+"\n";
        }
    }
    infile.close();

    char* changeTemp=new char[strResult.length()];
    strcpy(changeTemp, strResult.c_str());
    char* changeResult = change_encoding_from_UTF8_to_ANSI(changeTemp);
    strResult=changeResult;

    ofstream outfile;
    outfile.open(filename);
    outfile.write(strResult.c_str(),strResult.length());
    outfile.flush();
    outfile.close();
}

// change a char's encoding from UTF8 to ANSI
char* change_encoding_from_UTF8_to_ANSI(char* szU8)
{ 
    int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);
    wchar_t* wszString = new wchar_t[wcsLen + 1];
    ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);
    wszString[wcsLen] = '\0';

    int ansiLen = ::WideCharToMultiByte(CP_ACP, NULL, wszString, wcslen(wszString), NULL, 0, NULL, NULL);
    char* szAnsi = new char[ansiLen + 1];
    ::WideCharToMultiByte(CP_ACP, NULL, wszString, wcslen(wszString), szAnsi, ansiLen, NULL, NULL);
    szAnsi[ansiLen] = '\0';

    return szAnsi;
}





void change_encoding_from_UTF8_without_BOM_to_ANSI(const char* filename)
{
    ifstream infile;
    string strLine="";
    string strResult="";
    infile.open(filename);
    if (infile)
    {
        while(!infile.eof())
        {
            getline(infile, strLine);
            strResult += strLine+"\n";
        }
    }
    infile.close();

    char* changeTemp=new char[strResult.length()];
    strcpy(changeTemp, strResult.c_str());
    char* changeResult = change_encoding_from_UTF8_to_ANSI(changeTemp);
    strResult=changeResult;

    ofstream outfile;
    outfile.open(filename);
    outfile.write(strResult.c_str(),strResult.length());
    outfile.flush();
    outfile.close();
}
