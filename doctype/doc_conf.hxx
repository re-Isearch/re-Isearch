/*** Local Configurations for BSn doctypes ****/
#ifndef DOC_CONF_HXX
#define DOC_CONF_HXX

#ifdef _WIN32
#undef WWW_SUPPORT
#else
#ifndef WWW_SUPPORT
# define WWW_SUPPORT 1
#endif
#endif

// Generic Doctypes (HTML Presentation)
#ifdef WWW_SUPPORT
# ifndef CGI_IINFO
#  define CGI_IINFO "iinfo"
# endif
# ifndef CGI_FETCH
#  define CGI_FETCH "ifetch"
# endif
# ifndef CGI_FORM
#  define CGI_FORM "iform"
# endif
# ifndef CGI_DIR
#  define CGI_DIR "cgi-bin"
# endif
#endif

#define SHOW_DATE 1

// SGML
#define ACCEPT_SGML_EMPTY_TAGS	1 /* 1 ==> Accept Empty tags per Annex C.1.1.1 SGML Handbook */

// HTML
#define DEFAULT_HTML_LEVEL	3 /* 0 all, 1 Ignore some, 2 take real, 3 a bit firmer */
#define STORE_SCHEMAS		1 /* 0 ==> No, 1==> Yes, handle (Schema=)(Type=) */

// Mail
#define RESTRICT_MAIL_FIELDS	1 /* 0 ==> Accept anything, 1==> only those in list */
#define SHOW_MAIL_DATE   	0 /* 0 ==> Headline "From: Subject" 1==> "From dd/mm/yy: Subject" */ 

// Bibliographic Formats

// Use Native or Unified names by default?
#define USE_UNIFIED_NAMES 1

// Define Below to individualy modify above behaviour
//#define REFER_UNIFIED_NAMES 1 
//#define MEDLINE_UNIFIED_NAMES 1
//#define FILMLINE_UNIFIED_NAMES 1

// General 
#define USE_UNIFIED_NAMES 1

// --------- End User Configurable Options

// Specific
#ifndef REFER_UNIFIED_NAMES
#define REFER_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif
#ifndef MEDLINE_UNIFIED_NAMES
#define MEDLINE_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif
#ifndef FILMLINE_UNIFIED_NAMES
#define FILMLINE_UNIFIED_NAMES USE_UNIFIED_NAMES
#endif

// General
#ifndef BSN_EXTENSIONS
# define BSN_EXTENSIONS	0 /* 0 ==> CNIDR's Isearch, 1==> BSn's version */
#endif

#endif
