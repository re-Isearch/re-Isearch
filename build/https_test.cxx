#define CPPHTTPLIB_OPENSSL_SUPPORT

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


static const char *ipfs_gateway = "https://ipfs.io";


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
//
#if 0
static const STRING ThisDoctype("AUTOREMOTE");

static const STRING GatewaySection ("Gateways");

class AUTOREMOTE: public AUTODETECT {
public:
   AUTOREMOTE(PIDBOBJ DbParent, const STRING& Name) : AUTODETECT(DbParent, Name) {
      DocumentRoot = DOCTYPE::Getoption("BASE", NulString);
      if (DocumentRoot.IsEmpty()) {
	DocumentRoot = DbParent->ComposeDbFn(".url");
        DbParent->ProfileWriteString(Doctype, "BASE", DocumentRoot);
	message_log (LOG_INFO, "Setting crawl base to \"%s\"", DocumentRoot.c_str());
	IPFS_gateway = DbParent->ProfileGetString(GatewaySection, "IPFS", ipfs_gateway);
	if (IPFS_gateway.IsEmpty()) {
	  IPFS_gateway = ipfs_gateway;
	  message_log (LOG_DEBUG, "Setting IPFS default gateway to \"%s\"", ipfs_gateway);
	  DbParent->ProfileWriteString(GatewaySection, "IPFS", IPFS_gateway);
	}
      }
   };

   const char *Description(PSTRLIST List) const {
      if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
	List->AddEntry(Doctype);
      List->AddEntry (ThisDoctype);
      DOCTYPE::Description(List);
      return "\
Automatic Remote/Local file indexing Plugin (AUTODETECT)\n\
Index time Options:\n\
    BASE // Specifies the crawl base of the fetched URL tree (root)\n\
    files are downloaded into BASE/<protocol>/.. \n\
    This can also be specified in the doctype.ini as BASE=dir\n\
    in the [General] section. Once set and used it should never be changed\n\
    as the search depends upon the layout to reconstruct the URLs.\n\
    NOTE: if not set it uses <db>.url/ and sets the BASE accordingly.\n\n\
    In the [Gateways] sections one needs to define the gateways for the\n\
    non http/https protocols to access data via http/https\n";
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
   STRING        IPFS_Gateway;

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
  "Content-Type", 	// 0
  "Digest", 		// 1
  "ETag",		// 2
  "Expires",		// 3
  "Date",		// 4
  "Last-Modified",	// 5
  "Content-Language",   // 6
   NULL
} ;


bool fetch(const char *url, std::string *output, int *depth = NULL);

bool fetch(std::ostream& os, const std::string& site, const std::string& file, std::string *output, int *depth)
{

  httplib::Client cli(site);


  // Turn off SSL certificate verification
  cli.enable_server_certificate_verification(false);


  // We need to track depth so that we don't end up in a recursive
  // merry go round
  if (depth && *depth > 30) {
    std::cerr << "URL Recursive re-direction (" <<  *depth << ")" << std::endl;
    return false;
  }

  std::cerr << "SITE: " << site << std::endl;
  std::cerr << "Want file: " << file << std::endl;
  if (auto res = cli.Get(file)) {
    std::string meta = res->get_header_value("Content-location");

    if (!meta.empty()) {
std::cerr << "New location " << meta << std::endl;
      return fetch(meta.c_str(), output, depth);
    }

    for (size_t i=0; metatags[i]; i++) {
       meta = res->get_header_value(metatags[i]);
       if (meta.length()) {
#if 0
	const STRING value (meta.c_str());
	switch (i) {
	  case 6: r.SetLangauge(value); break;
	  case 5: r.SetDateModified(value); break;
	  case 4: r.SetDate(value); break;
	  case 3: r.SetDateExpires(value); break;
	  // Special case with ETag: we only use it if has not yet
	  // been used.
	  case 2: if (Db->MdtLookupKey (value) == 0) r.SetKey(meta);
		  break;
	}
#endif
     	 os << metatags[i] << ": " << meta << std::endl;
       }
    }
    os << std::endl << res->body << std::endl;
  } else {
    std::cerr << "error code: " << res.error() << std::endl;
    return false;
  }
  return true;
}

const char *base_path = "/tmp";


bool fetch(const char *url, std::string *output, int *depth)
{
   Uri u = Uri::Parse (url);

   if (depth) (*depth)++;

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
      std::string path = u.path;


      std::string depot = base_path;
      depot.append("/ipfs/");
      if (mkpath(depot.c_str(), 0777) != 0)
        return false;

      u.path = "/ipfs/";
      u.path.append(u.host);
      if (!path.empty() && path != "/") {
	u.path.append(path);
	if (mkpath(output_name.c_str(), 0777) != 0)
	   return false;
	output_name.append(path);
      }
   } else if (u.protocol == "http" || u.protocol == "https"){
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
   } else if (u.protocol == "file") {
     // file:///X, file:/X and file://localhost/X -> /x
     // while file://host/X we don't support now....
     if (u.host.empty() || u.host == "localhost") {
	if (output) *output = u.path;
	return true;
     }
     // We shall assume Andrew file system and map to /afs/hostname/path
     if (output) {
        *output = "/afs/";
	output->append(u.host);
	output->append(u.path) ;
        std::cerr << "Transfers via file:://" << u.host << " mapped to AFS " << *output <<  std::endl;
	return true;
     }
     return false;
   } else {
     std::cerr << "Protocol \"" << u.protocol << "\" is not (yet) supported!" << std::endl;
     return false;
   }

   std::cerr << "Output rewrite = " << output_name << std::endl;

   if (output) *output = output_name;

  std::cerr << "Fetching site: " << site << "  into " << output_name << std::endl;
  std::cerr  << "Path: " << u.path << std::endl;

  std::ofstream ofs;
  ofs.open (output_name, std::ofstream::out | std::ofstream::app);
  return  fetch(ofs, site, u.path, output, depth);
}

int main(int argc, char **argv)
{
  if (argc > 1) {
    std::string out;
    int depth = 0;
    bool result = fetch( argv[1], &out, &depth);
    cout << "Result: " << result << " --> " << out << endl;
  }
  else std::cerr << "Usage " << argv[0] << ": URL" << std::endl;
  return 0;
}
