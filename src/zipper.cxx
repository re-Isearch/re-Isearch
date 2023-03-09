// Reads .zip files and extracts list of files as an XML
// Record
#include <iostream>
#include <fstream>
#include <libgen.h>


bool printNextFileName(std::ifstream &fs)
{

    if (fs.eof())       return false;

    fs.ignore(6);
    char buf[2];
    fs.read(buf,2);
    int flags=buf[0]|buf[1]<<8;
    bool hasEncryptionHeader=1&flags;
    bool hasDataDescriptor=(1<<3)&flags;

    fs.ignore(10);
    unsigned char buf2[4];
    fs.read((char *)buf2,4);
    long size=buf2[0]|buf2[1]<<8|buf2[2]<<16|buf2[3]<<24;

    fs.ignore(4);
    unsigned char buf3[2];
    fs.read((char *)buf3,2);
    int fileNameLength=buf3[0]|buf3[1]<<8;

    unsigned char buf4[2];
    fs.read((char *)buf4,2);
    int extraFieldLength=buf4[0]|buf4[1]<<8;

    char buf5[fileNameLength];
    fs.read(buf5,fileNameLength);

    if (fileNameLength) {
      if (buf5[0] == 24) return false;
      std::cout << "<item>";
      for(int i=0;i<fileNameLength;i++)
        std::cout<<buf5[i];
      std::cout << "</item>" << std::endl;
    }

    // std::cout<<" "<<hasEncryptionHeader<<" "<<hasDataDescriptor; //testing

    fs.ignore(extraFieldLength+(hasEncryptionHeader?8:0)+size+(hasDataDescriptor?18:0)); //some of these values are for testing

    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
       std::cerr << "Usage is: zipper zip_file" << std::endl;
       return -1;
    }
    // zipper path_to_zip  URL_for_zip
    std::ifstream fs(argv[1],  std::ios::binary);

    const char *name = basename(argv[1]);
    std::cout << "<?xml version=\"1.0\"?>\n\
<!DOCTYPE ARCHIVE SYSTEM \"archive.dtd\"> " << std::endl;

    std::cout << "<archive type=\"zip\">" << std::endl;
    std::cout << "<filename>" << name << "</filename>" << std::endl;
    std::cout << "<contents>" << std::endl;
    while ( printNextFileName(fs));
    fs.close();
    std::cout << "</contents></archive>" << std::endl;
    return 0;
}
