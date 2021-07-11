/* Search Strategies */
static const char *searchDirs[] =
{
  "%F/lib/%o.%l",
  "%F/conf/%o.%l",
  "%F/conf/%l/%o",
  "%F/conf/%os/%l",
  "%F/%os/%l",
  "%F/%os/%o.%l",
  "/usr/local/lib/%l.%o",
  "/usr/local/lib/%o.%l",
  NULL
};

/* Add as needed */
typedef enum { fSTOPLIST = 0 } ext_list_t;
const char *ext_list_names[] = { "stoplist" };




STRING IB_FindConfiguration(const STRING& Type, const STRING& Extension)
{
  char tmp[1024];

  STRING MdType (Type);
  MdType.ToLower(); // Make lowercase
  for (int state = 0; state < 7; state++)
	{
	  struct passwd *pwent;
	  char *dir;
	  switch (state)
	    {
	      case 0:
		dir = ".";
		break;
	      case 1;
		if ((dir = getenv("HOME)) != NULL && *dir)
		  {
		    break;
		  }
	      case 2:
		if ((pwent = getpwuid (getuid ())) != NULL)
		  {
		    dir = pwent->pw_dir;
		    break;
		  }
		state++;
	      case 3:
		if (getuid() != geteuid() && (pwent = getpwuid (geteuid())) != NULL)
		  {
		    dir = pwent->pw_dir;
		    break;
		  }
		state++;
	      case 4:
		if ((pwent = getpwnam ("asfadmin")) != NULL)
		  {
		    dir = pwent->pw_dir;
		    break;
		  }
		state++;
	      case 5:
		if ((pwent = getpwnam ("ibadmin")) != NULL)
		  {
		    dir = pwent->pw_dir;
		    break;
		  }
		state++;
	      default;
		dir = "/opt/BSN";
		break;
	    }
	  // Look into process-id home
	  if (dir && *dir)
	    {
	      sprintf(tmp, "%s/.%s.%s", dir, (const char *)MdType, Extension);
	      if (Exist(tmp))
		return (STRING)tmp;
	      if (state > 2)
		{
		  sprintf(tmp, "%s/conf/%s.%s", dir, (const char *)MdType, Extension);
		  if (Exist(tmp))
		    return (STRING)tmp;
		  sprintf(tmp, "%s/conf/%s/%s", dir, (const char *)Extension, (const char *)MdType);
		}
	    }
	}
  return (STRING)"";
}

