/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "common.hxx"
#include <stdio.h>


const long def_timeout =955958400; // Expires Mon Apr 17 10:00:00 2000

static const char mail_cmd[] = "/usr/lib/sendmail";
static const char IB_REG_ADDR[] = "edz@nonmonotonic.com";

main(int argc, char **argv)
{
  if (argc > 4)
     {
       cerr << "Usage is: " << argv[0] << " [timeout] [hostid]" << endl;
       exit(-1);
     }
  const long timeout = argc >1 ? strtol(argv[1], NULL, 8) : def_timeout;
  const unsigned long hostid = argc < 3 ? _IB_Hostid() : strtol(argv[2], NULL, 0) ;

  char tmp[512];
  sprintf(tmp, "Timeout=%lo, Hostid=0x%lx", timeout, hostid);
  cout << tmp << endl;

  const long want = (timeout/5) - (hostid)/3;

#if PRIVATE
  cout << want << endl;
#else

  sprintf(tmp, "%s %s", mail_cmd, IB_REG_ADDR);
  FILE *fp = popen(tmp, "w");
 if (fp != NULL)
   {
     fprintf(fp, "\
To: %s <%s>\n\
Subject: ****IB License Key Generation**** v%s %lX (%lo)\n\
Priority: Urgent\n\
X-Action: KeyGen\n\n\n\
Key Generated for HOSTID %lX (%lo)\n",
   "IB Registration Authority", IB_REG_ADDR,
   __IB_Version, hostid, timeout, hostid, timeout);
    pclose(fp);

//   x-y-z  as HEX numbers
//   y = 0 means unlimited
//  so
//  t = timeout
//  if (((x/11 + y/13 + t/17) == z)  && ((x+y) == want))

    cout << want << endl;
  }
#endif
}
