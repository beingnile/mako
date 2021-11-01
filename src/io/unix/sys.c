/*!
 * sys.c - system functions for libsatoshi
 * Copyright (c) 2020, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/libsatoshi
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <io/core.h>

/*
 * Compat
 */

#undef HAVE_SYSCTL

#if defined(__APPLE__)     \
 || defined(__FreeBSD__)   \
 || defined(__OpenBSD__)   \
 || defined(__NetBSD__)    \
 || defined(__DragonFly__)
#  include <sys/types.h>
#  include <sys/sysctl.h>
#  if defined(CTL_HW) && (defined(HW_AVAILCPU) || defined(HW_NCPU))
#    define HAVE_SYSCTL
#  endif
#elif defined(__hpux)
#  include <sys/mpctl.h>
#endif

/*
 * System
 */

#ifdef HAVE_SYSCTL
static int
try_sysctl(int name) {
  int ret = -1;
  size_t len;
  int mib[4];

  len = sizeof(ret);

  mib[0] = CTL_HW;
  mib[1] = name;

  if (sysctl(mib, 2, &ret, &len, NULL, 0) != 0)
    return -1;

  return ret;
}
#endif

int
btc_sys_cpu_count(void) {
  /* https://stackoverflow.com/questions/150355 */
#if defined(__linux__) || defined(__sun) || defined(_AIX)
  /* Linux, Solaris, AIX */
# if defined(_SC_NPROCESSORS_ONLN)
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
# else
  return -1;
# endif
#elif defined(HAVE_SYSCTL)
  /* Apple, FreeBSD, OpenBSD, NetBSD, DragonFly BSD */
  int ret = -1;
# if defined(HW_AVAILCPU)
  ret = try_sysctl(HW_AVAILCPU);
# endif
# if defined(HW_NCPU)
  if (ret < 1)
    ret = try_sysctl(HW_NCPU);
# endif
  return ret;
#elif defined(__hpux)
  /* HP-UX */
  return mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(__sgi)
  /* IRIX */
# if defined(_SC_NPROC_ONLN)
  return (int)sysconf(_SC_NPROC_ONLN);
# else
  return -1;
# endif
#else
  return -1;
#endif
}

int
btc_sys_homedir(char *out, size_t size) {
  char *home = getenv("HOME");
  struct passwd *result;
  struct passwd pwd;
  char buf[8192];
  size_t len;
  uid_t uid;
  int rc;

  if (home != NULL) {
    len = strlen(home);

    if (len + 1 > size)
      return 0;

    memcpy(out, home, len + 1);

    return 1;
  }

  uid = geteuid();

  do {
    rc = getpwuid_r(uid, &pwd, buf, sizeof(buf), &result);
  } while (rc == EINTR);

  if (rc != 0 || result == NULL)
    return 0;

  len = strlen(pwd.pw_dir);

  if (len + 1 > size)
    return 0;

  memcpy(out, pwd.pw_dir, len + 1);

  return 1;
}
