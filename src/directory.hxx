/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */

void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const char *pattern = NULL,
  const char *exclude = NULL,
  const char *inclDirpattern = NULL,
  const char *excludeDirpattern = NULL,
  bool recurse = true,
  bool follow = true);


void do_directory( const STRING& dir,
  int (*do_file)(const STRING&),
  const STRING& pattern,
  const STRING& exclude = NulString,
  const STRING& inclDirpattern = NulString,
  const STRING& excludeDirpattern = NulString,
  bool recurse = true,
  bool follow = true);

void do_directory(const STRING& dir,
  int (*do_file)(const STRING&),
  const STRLIST *filePatternList,
  const STRLIST *excludeList = NULL,
  const STRLIST *inclDirList = NULL,
  const STRLIST *excludeDirList = NULL,
  bool recurse = true,
  bool follow = true);


