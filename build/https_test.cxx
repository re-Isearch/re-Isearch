//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "common.hxx"
#include "record.hxx"
#include "doc_conf.hxx"
#include "autodetect.hxx"

#include <httplib.h>
#include <iostream>

#include <string>
#include <algorithm>
#include <cctype>
#include <functional>

//using namespace std;
//
//
//
// This is the start of development for a DOCTYPE plug-in called
// AUTOREMOTE
//
// It is like AUTODETECT except it handles both local file paths as
// well as URLs, passing the object then to AUTODETECT
//
//
#if 0
static const STRING ThisDoctype("AUTOREMOTE");



class AUTOREMOTE: public AUTODETECT {
public:
   AUTOREMOTE(PIDBOBJ DbParent, const STRING& Name) : AUTODETECT(DbParent, Name) {
      DocumentRoot = DOCTYPE::Getoption("BASE", NulString);
      if (DocumentRoot.IsEmpty())
	DocumentRoot = DbParent->ComposeDbFn(".cat");

   };

   const char *Description(PSTRLIST List) const {
      if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
	List->AddEntry(Doctype);
      List->AddEntry (ThisDoctype);
      DOCTYPE::Description(List);
      return "Automatic Remote/Local file indexing Plugin (AUTODETECT)";
   }

   void ParseRecords(const RECORD& FileRecord) {
     const STRING s (FileRecord.GetFullFileName());
     if (s.Search("://") == 0) {
	// Not URL
	AUTODETECT::ParseRecords(FileRecord);
	return;
     }
     // Now we need to fetch and put in DocumentRoot
     RECORD r (FileRecord);
     std::string   output;

     if (fetch(s.c_str(), &output))
       {
        // Got it
	r.SetDocumentType("AUTODETECT");
	r.SetFullFileName( output.c_str()); // Set its new path
	AUTODETECT::ParseRecords(r);
       }
     }

   ~AUTOREMOTE() { }
private:
   STRING        DocumentRoot;

};
#endif


struct Uri
{
public:
std::string QueryString, path, protocol, host, port;

static Uri Parse(const std::string &uri)
{
    Uri result;

    result.path = "/";
    result.protocol = "http";

    typedef std::string::const_iterator iterator_t;

    if (uri.length() == 0)
        return result;

    iterator_t uriEnd = uri.end();

    // get query start
    iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');

    // protocol
    iterator_t protocolStart = uri.begin();
    iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

    if (protocolEnd != uriEnd)
    {
        std::string prot = &*(protocolEnd);
        if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
        {
            result.protocol = std::string(protocolStart, protocolEnd);
            protocolEnd += 3;   //      ://
        }
        else
            protocolEnd = uri.begin();  // no protocol
    }
    else
        protocolEnd = uri.begin();  // no protocol

    // host
    iterator_t hostStart = protocolEnd;
    iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

    iterator_t hostEnd = std::find(protocolEnd,
        (pathStart != uriEnd) ? pathStart : queryStart,
        ':');  // check for port

    result.host = std::string(hostStart, hostEnd);

    // port
    if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
    {
        hostEnd++;
        iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
        result.port = std::string(hostEnd, portEnd);
    }

    // path
    if (pathStart != uriEnd)
        result.path = std::string(pathStart, queryStart);

    // query
    if (queryStart != uriEnd)
        result.QueryString = std::string(queryStart, uri.end());

    return result;

}   // Parse
};  // uri




static int do_mkdir(const char *path, mode_t mode)
{
    struct stat     st;
    int             status = 0;

    if (stat(path, &st) != 0)
    {
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

static int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}


static const char *metatags[] = {
  "Content-Type",
  "Digest",
  "ETag",
  "Expires",
  "Date",
  "Last-Modified",
   NULL
} ;

bool fetch(std::ostream& os, const std::string& site, const std::string& file)
{
  httplib::Client cli(site);

  std::cerr << "SITE: " << site << std::endl;
  std::cerr << "Want file: " << file << std::endl;
  if (auto res = cli.Get(file)) {
    std::string meta = res->get_header_value("Content-location");

    if (!meta.empty()) os << "Location: " << meta << std::endl;

    for (size_t i=0; metatags[i]; i++) {
       meta = res->get_header_value(metatags[i]);
       if (meta.length())
     	 os << metatags[i] << ": " << meta << std::endl;
    }
    os << std::endl << res->body << std::endl;
  } else {
    std::cerr << "error code: " << res.error() << std::endl;
    return false;
  }
  return true;
}

const char *base_path = "/tmp";

const char *ipfs_gateway = "https://ipfs.io";

bool fetch(const char *url, std::string *output)
{
   Uri u = Uri::Parse (url);

   std::string output_name = base_path;
  
   output_name.append("/");
   output_name.append(u.protocol);
   if (!u.port.empty() && (u.protocol == "http" && u.port != "80")) {
     output_name.append("_");
     output_name.append(u.port);
   }
   output_name.append("/");
   output_name.append(u.host);

   std::string site;

   // IPFS is a special case which we rewrite to go through
   // a gateway to fetch the resource BUT store on the disk
   // as IPFS.
   if (u.protocol == "ipfs") {
      site = ipfs_gateway;
      u.path = "/ipfs/";
      u.path.append(u.host);

      std::string depot = base_path;
      depot.append("/ipfs");
      if (mkpath(depot.c_str(), 0777) != 0)
	return false;
   } else {
      site = u.protocol;
     // make sure we can write to the path
      if (mkpath(output_name.c_str(), 0777) != 0)
	return false; // could not create path
      output_name.append("/");
      if (u.path.empty() || u.path == "/")
       output_name.append("_._.html"); // default instead of index.html as they may differ
      else
       output_name.append(u.path);
      if (!site.empty()) site.append("://");
      site.append(u.host);
       if (!u.port.empty()) {
         site.append(":");
         site.append(u.port);
       }
   }

   std::cerr << "Output rewrite = " << output_name << std::endl;

   if (output) *output = output_name;

  std::cerr << "Fetching site: " << site << "  into " << output_name << std::endl;
  std::cerr  << "Path: " << u.path << std::endl;

  std::ofstream ofs;
  ofs.open (output_name, std::ofstream::out | std::ofstream::app);
  return  fetch(ofs, site, u.path);
}

int main(int argc, char **argv)
{
  if (argc > 1) {
    std::string out;
    bool result = fetch( argv[1], &out );
    cout << "Result: " << result << " --> " << out << endl;
  }
  else std::cerr << "Usage " << argv[0] << ": URL" << std::endl;
  return 0;
}
