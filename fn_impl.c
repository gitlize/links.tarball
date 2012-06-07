#include "links.h"

#ifndef HAVE_SNPRINTF

#define B_SZ	65536

static char snprtintf_buffer[B_SZ];

int my_snprintf(char *str, int n, char *f, ...)
{
	int i;
	va_list l;
	if (!n) return -1;
	va_start(l, f);
	vsprintf(snprtintf_buffer, f, l);
	va_end(l);
	i = strlen(snprtintf_buffer);
	if (i >= B_SZ) {
		fatal_exit("String size too large!");
	}
	if (i >= n) {
		memcpy(str, snprtintf_buffer, n);
		str[n - 1] = 0;
		return -1;
	}
	strcpy(str, snprtintf_buffer);
	return i;
}

#endif

#ifndef HAVE_RAISE
int raise(int s)
{
#ifdef HAVE_GETPID
	pid_t p;
	EINTRLOOP(p, getpid());
	if (p == -1) return -1;
	return kill(p, s);
#else
	return 0;
#endif
};
#endif
#ifndef HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	time_t t;
	EINTRLOOP(t, time(NULL));
	if (tv) tv->tv_sec = t, tv->tv_usec = 0;
	if (tz) tz->tz_minuteswest = tz->tz_dsttime = 0;
	return 0;
}
#endif
#ifndef HAVE_TEMPNAM
char *tempnam(const char *dir, const char *pfx)
{
	static int counter = 0;
	unsigned char *d, *s, *a;
	int l;
	if (!(d = getenv("TMPDIR"))) {
		if (dir) d = (unsigned char *)dir;
		else if (!(d = getenv("TMP")) && !(d = getenv("TEMP"))) {
#ifdef P_tmpdir
			d = P_tmpdir;
#else
			d = "/tmp";
#endif
		}
	}
	l = 0;
	s = init_str();
	add_to_str(&s, &l, d);
	if (s[0] && s[strlen(s) - 1] != '/') add_chr_to_str(&s, &l, '/');
	add_to_str(&s, &l, (unsigned char *)pfx);
	add_num_to_str(&s, &l, counter++);
	a = strdup(s);
	mem_free(s);
	return a;
}
#endif
#ifndef HAVE_SIGFILLSET
int sigfillset(sigset_t *set)
{
	memset(set, -1, sizeof(sigset_t));
	return 0;
}
#endif
#ifndef HAVE_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	if (*nptr == '-') {
		if (endptr) *endptr = nptr;
		return 0;
	}
	return (unsigned long)strtol(nptr,endptr,base);
};
#endif
#ifndef HAVE_STRLEN
size_t strlen(const char *s)
{
	size_t len = 0;
	while (s[len]) len++;
	return len;
}
#endif
#ifndef HAVE_STRCPY
char *strcpy(char *dst, const char *src)
{
	return memcpy(dst, src, strlen(src) + 1);
}
#endif
#ifndef HAVE_STRCHR
char *strchr(const char *s, int c)
{
	do {
		if (*s == (char)c)
			return (char *)s;
	} while (*s++);
	return NULL;
}
#endif
#ifndef HAVE_STRRCHR
char *strrchr(const char *s, int c)
{
	char *ret = NULL;
	do {
		if (*s == (char)c)
			ret = (char *)s;
	} while (*s++);
	return ret;
}
#endif
#ifndef HAVE_STRCMP
int strcmp(const char *s1, const char *s2)
{
	while (1) {
		if (*s1 != *s2) {
			return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
		}
		if (!*s1) break;
		s1++, s2++;
	}
	return 0;
}
#endif
#ifndef HAVE_STRNCMP
int strncmp(const char *s1, const char *s2, size_t n)
{
	while (n--) {
		if (*s1 != *s2) {
			return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
		}
		if (!*s1) break;
		s1++, s2++;
	}
	return 0;
}
#endif
#ifndef HAVE_STRCSPN
size_t strcspn(const char *s, const char *reject)
{
	size_t r;
	for (r = 0; *s; r++, s++) {
		const char *rj;
		for (rj = reject; *rj; rj++) if (*s == *rj) goto brk;
	}
	brk:
	return r;
}
#endif
#ifndef HAVE_STRSTR
char *strstr(const char *haystack, const char *needle)
{
	size_t hs = strlen(haystack);
	size_t ns = strlen(needle);
	while (hs >= ns) {
		if (!memcmp(haystack, needle, ns)) return (char *)haystack;
		haystack++, hs--;
	}
	return NULL;
}
#endif
#ifndef HAVE_MEMMOVE
void *memmove(void *dst0, const void *src0, size_t length)
{
	unsigned char *dst = dst0;
	const unsigned char *src = src0;

	if ((const unsigned char *)dst == src || !length)
		return dst0;

	if ((const unsigned char *)dst <= src) {
		while (length--) *dst++ = *src++;
	} else {
		dst += length - 1;
		src += length - 1;
		while (length--) *dst-- = *src--;
	}
	return dst0;
}
#endif
#ifndef HAVE_STRERROR
extern char *sys_errlist[];
extern int sys_nerr;
char *strerror(int errnum)
{
	if (errnum < 0 || errnum >= sys_nerr) return "Unknown error";
	return sys_errlist[errnum];
};
#endif
