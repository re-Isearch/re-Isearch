This is the build direectory for re-Isearch.

There are a few Makefiles to choose from. 
The default Makefile calls, depending upon the OS, a number of other Makefiles.
Out of the box on Linux we are using the Makefile.ubuntu and it should be fine for other Linux distros.
The Makefile.MacOS is for Darwin (Apple) and has been tested on Apple Silicon using both gcc and clang compilers.

The repository also contains optional document handlers (doctypes) that are supplied as plugins. This includes doctypes for a number of office formats as well as "autoremote" which allows one to index remote files (http/https/ipfs etc). Some of these have either 3rd party libs as prequisites (autoremote requires, for example, OpenSSL) or require (3rd party) external filters (availabel seperately) to function. By default they are NOT built. In order to have them build one must "make plugins".


> External Base Classes ("Plugin Doctypes"):
  ODT2:              // "OASIS Open Document Format Text" (ODT) Plugin (MEMODOC)
  ADOBE_PDF:         // Adobe PDF Plugin
  USPAT:             // US Patents (Green Book)
  ODT_J:             // "OASIS Open Document Format Text" (ODT) Plugin (MEMODOC)
  MSWORD:            // M$ Word Plugin
  RTF:               // "Rich Text Format" (RTF) Plugin
  PDFDOC:            // OLD Adobe PDF Plugin
  AUTOREMOTE:        // Automatic Remote/Local file indexing Plugin (AUTODETECT)
  NULL:              // Empty plugin
  MSRTF:             // M$ RTF (Rich Text Format) Plugin [XML]
  TEXT:              // Plain Text Plugin
  DJVU:              // "DjVu eBooks" (DjVu) Plugin
  MSOLE:             // M$ OLE type detector Plugin
  MSEXCEL:           // M$ Excel (XLS) Plugin
  ODT:               // "OASIS Open Document Format Text" (ODT) Plugin (TEXTDOC)
  ESTAT:             // EUROSTAT CSL Plugin
  ISOTEIA:           // ISOTEIA project (GILS Metadata) XML format locator records
  MSOFFICE:          // M$ Office OOXML Plugin


For addtional information on building, installing, developing and using the system please consult the handbook in [docs/](https://github.com/re-Isearch/re-Isearch/blob/master/docs/re-Isearch-Handbook.pdf).

A basic cheat-sheet is in [INSTALATION](../INSTALATION)

This project was funded through the NGI0 Discovery Fund, a fund established by NLnet with financial support from the European Commission's Next Generation Internet programme, under the aegis of DG Communications Networks, Content and Technology under grant agreement No 825322.

<IMG SRC="https://nlnet.nl/image/logo_nlnet.svg" ALT="NLnet Foundation" height=100> <IMG SRC="https://nlnet.nl/logo/NGI/NGIZero-green.hex.svg" ALT="NGI0 Search" height=100> &nbsp; &nbsp; <IMG SRC="https://ngi.eu/wp-content/uploads/sites/77/2017/10/bandiera_stelle.png" ALT="EU" height=100>

