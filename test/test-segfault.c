/* This is simply a process that segfaults */
#include <config.h>
#include <stdlib.h>
#include <signal.h>

#ifdef DBUS_WIN
#define RLIMIT_CORE	4		/* max core file size */
typedef unsigned long rlim_t;
struct rlimit {
	rlim_t	rlim_cur;
	rlim_t	rlim_max;
};
static int getrlimit (int __resource, struct rlimit *__rlp) {
  return -1;
}
static int setrlimit (int __resource, const struct rlimit *__rlp) {
  return -1;
}
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif


int
main (int argc, char **argv)
{
  char *p;  

  struct rlimit r = { 0, };
  
  getrlimit (RLIMIT_CORE, &r);
  r.rlim_cur = 0;
  setrlimit (RLIMIT_CORE, &r);
  
  raise (SIGSEGV);

  p = NULL;
  *p = 'a';
  
  return 0;
}
