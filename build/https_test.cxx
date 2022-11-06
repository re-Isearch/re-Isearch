//#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>
#include <iostream>

#include <string>
#include <algorithm>
#include <cctype>
#include <functional>

//using namespace std;

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

bool fetch(const char *url)
{
   Uri u = Uri::Parse (url);

   std::string output_name = base_path;
  
   output_name.append("/");
   output_name.append(u.protocol);
   if (!u.port.empty() && u.port != "80") {
     output_name.append("_");
     output_name.append(u.port);
   }
   output_name.append("/");
   output_name.append(u.host);
   output_name.append("/");
   if (u.path.empty() || u.path == "/")
     output_name.append("index.html");
   else 
     output_name.append(u.path);


   std::cerr << "Output rewrite = " << output_name << std::endl;
   
   std::string site = u.protocol;
   if (!site.empty()) site.append("://");
   site.append(u.host);
   if (!u.port.empty()) {
     site.append(":");
     site.append(u.port);
   }
  std::cerr << "Fetching site: " << site << std::endl;
  std::cerr  << "Path: " << u.path << std::endl;

  std::ofstream ofs;
  ofs.open (output_name, std::ofstream::out | std::ofstream::app);
  return  fetch(ofs, site, u.path);
}

int main(int argc, char **argv)
{
  if (argc > 1) {
    fetch( argv[1] );
  }
  else std::cerr << "Usage " << argv[0] << ": URL" << std::endl;
  return 0;
}
