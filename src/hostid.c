/* Determine the acurate hostid */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/systeminfo.h>

#define UINT4 unsigned int
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 144
#endif

long _IB_Hostid()
{
  static UINT4 hostid =  0x7f000001U; /* Loopback */
  if (hostid ==          0x7f000001U)
    {
#ifdef SI_HW_SERIAL
      char serial[14];
      if (-1 != sysinfo (SI_HW_SERIAL, serial, sizeof(serial)))
        hostid = (UINT4)atol(serial);
#else
#ifdef HAVE_GETHOSTID
      hostid = gethostid();
#endif
#endif
      if (hostid == 0)
	{
	  fprintf (stderr, "\
Platform seems to be missing a unique hostid! PC Platform? \
Set hostid to the Ethernet MAC address of its main networking card.\n");
	}
      if (hostid == 0 || hostid == ((UINT4)-1) || hostid == 0x7f000001U)
	{
	  /*Use the internet address as the hostid */
	  char name[MAXHOSTNAMELEN+1];

	  fprintf(stderr, "Hostid call returned: 0x%lx\n", hostid);
	  hostid = 0xFF;
	  if (gethostname(name, sizeof(name)/sizeof(char) -1) != -1)
	    {
	      struct hostent *h = gethostbyname (name);
	      if (h)
		{
		  const char *iptr;
		  size_t i;
		  hostid = 0;
		  for (i = 0; hostid == 0 && (iptr = (const char *)h->h_addr_list[i]) != NULL; i++)
		    {
		      UINT4 id = 
#ifdef LINUX
			((const struct in_addr*)iptr)->s_addr;
#else
			inet_addr(iptr);
#endif
		      if (id != 0x7f000001U && id != 0x100007fU)
			hostid = (UINT4)id;
		    }
		  if (hostid == 0)
		    {
		      fprintf(stderr, "Can't determine a valid hostid for this platform!\n");
		      hostid = 0x010203;
		    }
		}
	      else
		{
		  fprintf(stderr, "No hostid or network address on this platform !?");
		}
	    }
	}
    }
  return hostid; /* Return what we've got */
}

main()
{

  printf("0x%lX\n", _IB_Hostid());

}
