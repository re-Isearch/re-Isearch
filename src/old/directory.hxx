
void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const char *pattern = NULL,
  const char *exclude = NULL,
  const char *inclDirpattern = NULL,
  const char *excludeDirpattern = NULL,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);


void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const STRING& pattern,
  const STRING& exclude = NulString,
  const STRING& inclDirpattern = NulString,
  const STRING& excludeDirpattern = NulString,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);

void do_directory(const STRING& dir,
  int (*do_file)(const STRING&),
  const STRLIST *filePatternList,
  const STRLIST *excludeList = NULL,
  const STRLIST *inclDirList = NULL,
  const STRLIST *excludeDirList = NULL,
  GDT_BOOLEAN recurse = GDT_TRUE,
  GDT_BOOLEAN follow = GDT_TRUE);


