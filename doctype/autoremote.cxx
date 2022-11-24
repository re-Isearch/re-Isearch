#define CPPHTTPLIB_OPENSSL_SUPPORT


/*@@@
File:           autoremote.cxx
Version:        1.00
Description:    Plugin AUTOREMOTE to index http/https/ipfs files 
Author:         Edward Zimmermann
Comments:       May need a newer version of OpenSSL library than often shipped with the O/S.

		This plugin uses BOTH std::string and STRING classes. The later to interop with
		the httplib.h (cpp-httplib) which depends upon it.

		This code uses cpp-httplib
		https://github.com/yhirose/cpp-httplib.git

                The default ipfs_gateway should be configured below.
		
		Gateways used the translation: protocol://X -> gateway/protocol/X
		e.g. ipfs://QmXoypizjW3WknFiJnKLwHCnL72vedxjQkDDP1mXWo6uco/wiki
		maps to https://ipfs.io/ipfs/QmXoypizjW3WknFiJnKLwHCnL72vedxjQkDDP1mXWo6uco/wiki
		where https://ipfs.io/ is the defined IPFS gateway.
@@@*/


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


// Configuration secion
static const char *ipfs_gateway = "https://ipfs.io";
static const char *btfs_gateway = "http://gateway.btfs.io/";
// End configuration section


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
static const STRING ThisDoctype    ("AUTOREMOTE");
static const STRING GatewaySection ("Gateways");
static const STRING RootSection    ("BASE");

static const char *myDescription = "Automatic Remote/Local file indexing Plugin (AUTODETECT)";

static const char *dir_extension = ".url";



class  IBDOC_AUTOREMOTE: public AUTODETECT {
public:
   IBDOC_AUTOREMOTE(PIDBOBJ DbParent, const STRING& Name) : AUTODETECT(DbParent, Name) {
      DocumentRoot = DOCTYPE::Getoption(RootSection, NulString);

      if (Db) {
	if (DocumentRoot.IsEmpty()) {
	  DocumentRoot = Db->ComposeDbFn( dir_extension );
          Db->ProfileWriteString(Doctype, RootSection, DocumentRoot);
	  message_log (LOG_INFO, "Setting crawl base to \"%s\"", DocumentRoot.c_str());
	}
	Db->SetMirrorBaseDirectory(DocumentRoot);

	IPFS_gateway = DbParent->ProfileGetString(GatewaySection, "IPFS");
	if (IPFS_gateway.IsEmpty()) {
	  IPFS_gateway = ipfs_gateway;
	  message_log (LOG_DEBUG, "Setting IPFS default gateway to \"%s\"", ipfs_gateway);
	  Db->ProfileWriteString(GatewaySection, "IPFS", IPFS_gateway);
	}
	BTFS_gateway = DbParent->ProfileGetString(GatewaySection, "BTFS");
        if (BTFS_gateway.IsEmpty()) {
          IPFS_gateway = ipfs_gateway;
          message_log (LOG_DEBUG, "Setting BTFS default gateway to \"%s\"", btfs_gateway);
          Db->ProfileWriteString(GatewaySection, "BTFS", BTFS_gateway);
        }
	CA_CERT_FILE =  DOCTYPE::Getoption("CERT", NulString);
      } else {
	// Set defaults
	IPFS_gateway = ipfs_gateway;
	BTFS_gateway = btfs_gateway;
      }

     desc.form("%s\nThis allows one to index remote URLs such as https://www.exodao.net/\n\n\
Index time Options:\n\
CERT\t// Specifies the path to the ca_cert_file should it be used\n\
%s\t// Specifies the crawl base of the fetched URL tree (root) files are downloaded into\n\
\tBASE/<protocol>/..  This can also be specified in the doctype.ini as BASE=dir in the\n\
\t[General] section. Once set and used it should never be changed as the search depends upon\n\
\tthe layout to reconstruct the URLs.\n\
NOTE: if not set it uses <db>%s/ and sets the %s accordingly.\n\n\
In the [%s] sections one needs to define the gateways for the non http/https protocols (such\n\
as IPFS and BTFS) to access data via http/https.\n\
NOTE: For remote file://host/path it assumes AFS under /afs/host/path",
    myDescription,  RootSection.c_str(), dir_extension,  RootSection.c_str(),
    GatewaySection.c_str());

   };

   const char *Description(PSTRLIST List) const {
      if ( List->IsEmpty() && !(Doctype ^= ThisDoctype))
	List->AddEntry(Doctype);
      List->AddEntry (ThisDoctype);
      DOCTYPE::Description(List);
      return desc.c_str();
   }

   void ParseRecords(const RECORD& FileRecord) {
     // const STRING s (FileRecord.GetFileName());
     STRING s (FileRecord.GetFullFileName());

     message_log(LOG_DEBUG, "AUTOREMOTE::ParseRecords got \"%s\"", s.c_str());

     if (s.Search("://") == 0) {
	// Not URL
	AUTODETECT::ParseRecords(FileRecord);
	return;
     }

     // Strip trailing /.
     if (s.Last() == '.' && s.Last(1) == '/')
	s.RemoveLast(2);


     // Now we need to fetch and push into queue 
     int depth = 0;
     RECORD r(FileRecord);

     if (fetch(s.c_str(), r, &depth))
       {
        // Got it
	r.SetDocumentType("AUTODETECT");
	AUTODETECT::ParseRecords(r);
       }
     }

   ~ IBDOC_AUTOREMOTE() { }
private:
   bool          fetch(const char *url, RECORD& record, int *depth = NULL);
   bool          fetch(const std::string &filepath, const std::string& site, const std::string& file, RECORD& record, int *depth);

   STRING        DocumentRoot;
   STRING        IPFS_gateway; // the IPFS Gateway
   STRING        BTFS_gateway; // the BTFS Gateway
   STRING        CA_CERT_FILE;
   STRING        desc; // the description

};


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



#if 0
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
#endif


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


// Note IPFS keys can be long as 
// Qme7ss3ARVgxv6rXqVPiiikMJ8u2NLgimgszg13pYriDKEoiu


bool  IBDOC_AUTOREMOTE::fetch(const std::string& filepath,	
	const std::string& site, const std::string& file, RECORD& record, int *depth)
{
  httplib::Client cli(site);

  // Timeout in seconds
  // cli.set_connection_timeout(20);


  if (!CA_CERT_FILE.IsEmpty()) {
    cli.set_ca_cert_path(CA_CERT_FILE.c_str());
    cli.enable_server_certificate_verification(true);
  } else // Turn off SSL certificate verification
    cli.enable_server_certificate_verification(false);


  // We need to track depth so that we don't end up in a recursive
  // merry go round
  if (depth && *depth > 30) {
    message_log (LOG_ERROR, "URL Recursive re-direction exceeded (%d). Skipping.", *depth);
    return false;
  }

  // std::cerr << "SITE: " << site << std::endl;
  // std::cerr << "Want file: " << file << std::endl;
  if (auto res = cli.Get(file)) {
    std::string meta = res->get_header_value("Content-location");
    if (!meta.empty()) {
      message_log (LOG_DEBUG, "New location %s", meta.c_str());
      return fetch(meta.c_str(), record, depth);
    }
    meta = res->get_header_value("Location");
    if (!meta.empty()) {
      message_log (LOG_DEBUG, "New location %s%s", site.c_str(), meta.c_str());
      return fetch((site + meta).c_str(), record, depth);
    }


    for (size_t i=0; metatags[i]; i++) {
       meta = res->get_header_value(metatags[i]);
       if (meta.length()) {
	const STRING value (meta.c_str());
	switch (i) {
	  case 6: record.SetLanguage(value); break;
	  case 5: record.SetDateModified(value); break;
	  case 4: record.SetDate(value); break;
	  case 3: record.SetDateExpires(value); break;
	  // Special case with ETag: we only use it if has not yet
	  // been used.
	  case 2: // ETag may be a good place to build a key...
		  // unsigned k = site.Hash();
		  if (Db->MdtLookupKey (value) == 0) record.SetKey(value);
		  break;
	}
     	 // os << metatags[i] << ": " << meta << std::endl;
       }
    }

    std::ofstream os;
    os.open (filepath, std::ofstream::out | std::ofstream::app);
    os << res->body << std::endl; // Make sure we have a trailing new line
  } else {
    message_log (LOG_ERROR, "http/https protocol failure. Error code %d",  res.error());
    return false;
  }
  return true;
}


bool  IBDOC_AUTOREMOTE::fetch(const char *url, RECORD& record, int *depth)
{
   Uri u = Uri::Parse (url);

   if (depth) (*depth)++;

   std::string output_name = DocumentRoot.c_str();
  
   output_name.append("/");
   output_name.append(u.protocol);
   if (!u.port.empty() && (u.protocol == "http" && u.port != "80")) {
     output_name.append("_");
     output_name.append(u.port);
   }
   output_name.append("/");
   output_name.append(u.host);

   std::string site;

   // std::cerr << "u.protocol = \"" << u.protocol << "\"" << std::endl;   

   // IPFS and BTFS are special cases which we rewrite to go through
   // a gateway to fetch the resource BUT store on the disk 
   if ((u.protocol == "ipfs") || (u.protocol == "btfs") || (u.protocol == "ipns")) {
      site = (u.protocol == "btfs" ? BTFS_gateway.c_str() : IPFS_gateway.c_str() );
      std::string path = u.path;


      // std::string depot = DocumentRoot.c_str();
      // // such as  depot.append("/ipfs/");
      // depot.append("/");
      // depot.append(u.protocol);
      // depot.append("/");
      // if (mkpath(depot.c_str(), 0777) != 0)
      //   return false;

      // such as u.path = "/ipfs/";
      u.path = "/";
      u.path.append(u.protocol);
      u.path.append("/");

      u.path.append(u.host);
      if (!path.empty() && path != "/") {
	u.path.append(path);
	output_name.append(path);
      }
      if (!MkDirs(output_name.c_str(), 0777)) {
	  message_log (LOG_ERROR, "Could not create dirs for %s. %s", output_name.c_str(),
			  errno == EEXIST ? "File in the way." : strerror(errno));
	  return false; // could not make ; 
      }
   } else if (u.protocol == "http" || u.protocol == "https"){
      site = u.protocol;
	// // make sure we can write to the path
	// if (mkpath(output_name.c_str(), 0777) != 0)
	// 	return false; // could not create path
      output_name.append("/");
      if (u.path.empty() || u.path == "/")
       output_name.append("_._.html"); // default instead of index.html as they may differ
      else
       output_name.append(u.path);

      if (!MkDirs(output_name.c_str(), 0777))
	return false; // could not make ; 

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
	record.SetFullFileName (u.path.c_str());
	return true;
     }
     // We shall assume Andrew file system and map to /afs/hostname/path
     if (Exists("/afs")) {
	STRING path("/afs/");		     
	path << u.host.c_str() << u.path.c_str();
	message_log (LOG_DEBUG, "Transfers via file:://%s mapped to AFS %s", u.host.c_str(), path.c_str());
	record.SetFullFileName (path);
	return true;
     }
     return false;
   } else {
#if 0
     STRING gateway = DbParent->ProfileGetString(GatewaySection, u.protocol);
     if (gateway.IsEmpty()) {
	message_log(LOG_ERROR,"Protocol \"%s\" is not (yet) supported! Skipping.", u.protocol.c_str());
	return false;
     }

#endif
     message_log(LOG_ERROR,"Protocol \"%s\" is not (yet) supported! Skipping.", u.protocol.c_str());
     return false;
   }

   // std::cerr << "Output rewrite = " << output_name << std::endl;

   record.SetFullFileName (output_name.c_str());

   // std::cerr << "Fetching site: " << site << "  into " << output_name << std::endl;
   // std::cerr  << "Path: " << u.path << std::endl;

  return  fetch(output_name, site, u.path, record, depth);
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_AUTOREMOTE *  __plugin_autoremote_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_AUTOREMOTE (parent, Name);
  }
  int          __plugin_autoremote_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_autoremote_query (void) { return myDescription; }
}


