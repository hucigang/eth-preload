/* 

Written by Jim Hawkins <jawkins@armedslack.org>
This is so that we can build software that does not know the names of more recent ARM
architectures such as armv5, and also fake host names and MAC addresses for some 
particularly weird stuff ;-)

Compile:
  gcc -o fakeuname.o -c -fPIC -Wall fakeuname.c
  gcc -o libfakeuname.so -shared -W1,export-dynamic fakeuname.o -ldl

Usage:
   export LD_PRELOAD=/tmp/libfakeuname.so
   export FAKEMACADDR=00:09:12:13:1A:c3
   export FAKEUNAME=cheese
   uname -a

*/

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Define the values to override for uname() */
struct uname_override
{
  char *      var;
  char *      val; /* Value to use if environment variable not set */
  size_t      offset;
  size_t      size;
};
#define uname_override(member, var, val) { var, val, offsetof(struct utsname, member), sizeof(((struct utsname *)0)->member) }
static const struct uname_override uname_overrides[] =
{
  uname_override(nodename, "FAKEUNAME",     NULL),
  uname_override(machine,  "FAKEUNAMEARCH", "armv5tel"),
};
#define GETHOSTNAME_OVERRIDE  "FAKEUNAME"
#define IFHWADDR_OVERRIDE     "FAKEMACADDR"

#if defined(RTLD_NEXT)
#define REAL_LIBC RTLD_NEXT
#else
#define REAL_LIBC ((void *) -1L)
#endif

/* Function pointers for real libc versions */
static int (*real_uname)(struct utsname *);
static int (*real_gethostname)(char *name, size_t len);
static int (*real_ioctl)(int fd, unsigned long request, ...);

/* uname() wrapper */
int uname(struct utsname *name)
{
  int rv;

  /* Get real uname() function pointer */
  if (real_uname == NULL)
  {
    real_uname = dlsym(REAL_LIBC, "uname");
    if (real_uname == NULL) exit(1);
  }

  /* Call real uname() */
  rv = real_uname(name);
  if (rv == 0)
  {
    /* Override values */
    int i;
    for (i = 0; i < sizeof(uname_overrides)/sizeof(*uname_overrides); i++)
    {
      const struct uname_override *o = &uname_overrides[i];
      const char *val;
      val = getenv(o->var);
      if (val == NULL) val = o->val;
      if (val != NULL)
      {
        char *buf = ((char *)name) + o->offset;
        strncpy(buf, val, o->size);
        buf[o->size - 1] = '\0';
      }
    }
  }

  return rv;
}

/* gethostname() wrapper */
int gethostname(char *name, size_t len)
{
  const char *val;

  /* Get real gethostname() function pointer */
  if (real_gethostname == NULL)
  {
    real_gethostname = dlsym(REAL_LIBC, "gethostname");
    if (real_gethostname == NULL) exit(1);
  }

  /* Override hostname */
  val = getenv(GETHOSTNAME_OVERRIDE);
  if (val != NULL)
  {
    strncpy(name, val, len);
    return 0;
  }

  /* Call real gethostname() */
  return real_gethostname(name, len);
}

/* ioctl() wrapper */
int ioctl(int fd, unsigned long request, ...)
{
  va_list ap;
  unsigned long arg;

  /* Get real ioctl() function pointer */
  if (real_ioctl == NULL)
  {
    real_ioctl = dlsym(REAL_LIBC, "ioctl");
    if (real_ioctl == NULL) exit(1);
  }

  /* Get argument */
  va_start(ap, request);
  arg = va_arg(ap, unsigned long);
  va_end(ap);

  /* Check if it's a get interface hardware address request for eth0 */
  if (request == SIOCGIFHWADDR && strcmp(((struct ifreq *)arg)->ifr_name, "eth0") == 0)
  {
    /* Get MAC address to use */
    const char *val;
    unsigned char mac[6];
    val = getenv(IFHWADDR_OVERRIDE);
    if (val != NULL)
    {
      int i = 0;
      for (;;)
      {
        unsigned char u, l;
        u = *val++;
        if      (u >= '0' && u <= '9') u = u - '0';
        else if (u >= 'A' && u <= 'F') u = u - 'A' + 10;
        else if (u >= 'a' && u <= 'f') u = u - 'a' + 10;
        else
        {
          val = NULL;
          break;
        }
        l = *val++;
        if      (l >= '0' && l <= '9') l = l - '0';
        else if (l >= 'A' && l <= 'F') l = l - 'A' + 10;
        else if (l >= 'a' && l <= 'f') l = l - 'a' + 10;
        else
        {
          val = NULL;
          break;
        }
        mac[i] = (u << 4) | l;
        if (++i >= 6) break;
        if (*val++ != ':')
        {
          val = NULL;
          break;
        }
      }
      if (val == NULL || *val != '\0')
      {
        fprintf(stderr, "%s malformed - ignoring\n", IFHWADDR_OVERRIDE);
      }
    }

    /* Return fake MAC address */
    if (val != NULL)
    {
      struct sockaddr *sa = &(((struct ifreq *)arg)->ifr_hwaddr);
      sa->sa_family = ARPHRD_ETHER;
      memcpy(sa->sa_data, mac, sizeof(mac));
      return 0;
    }
  }

  /* Call real ioctl() */
  return real_ioctl(fd, request, arg);
}
