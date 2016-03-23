/*
 * Shell-like utility functions
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: shutils.h,v 1.4 2005/11/21 15:03:16 seg Exp $
 */

#ifndef _shutils_h_
#define _shutils_h_
#include <string.h>
#include <stdio.h>

#define MAX_NVPARSE 256

/*
 * Reads file and returns contents
 * @param       fd      file descriptor
 * @return      contents of file or NULL if an error occurred
 */
extern char *fd2str(int fd);

/*
 * Reads file and returns contents
 * @param       path    path to file
 * @return      contents of file or NULL if an error occurred
 */
extern char *file2str(const char *path);

/*
 * Waits for a file descriptor to become available for reading or unblocked signal
 * @param       fd      file descriptor
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @return      1 if descriptor changed status or 0 if timed out or -1 on error
 */
extern int waitfor(int fd, int timeout);

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @param       path    NULL, ">output", or ">>output"
 * @param       timeout seconds to wait before timing out or 0 for no timeout
 * @param       ppid    NULL to wait for child termination or pointer to pid
 * @return      return value of executed command or errno
 */

int _evalpid(char *const argv[], char *path, int timeout, int *ppid);

//extern int _eval(char *const argv[]);
extern int eval_va(const char *cmd, ...);
extern int eval_va_silence(const char *cmd, ...);

#define eval(cmd, args...) eval_va(cmd, ## args, NULL)
#define eval_silence(cmd, args...) eval_va_silence(cmd, ## args, NULL)

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param       argv    argument list
 * @return      stdout of executed command or NULL if an error occurred
 */
extern char *_backtick(char *const argv[]);

/*
 * Kills process whose PID is stored in plaintext in pidfile
 * @param       pidfile PID file
 * @return      0 on success and errno on failure
 */
extern int kill_pidfile(char *pidfile);

/*
 * fread() with automatic retry on syscall interrupt
 * @param       ptr     location to store to
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully read
 */
extern int safe_fread(void *ptr, size_t size, size_t nmemb, FILE * stream);

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param       ptr     location to read from
 * @param       size    size of each element of data
 * @param       nmemb   number of elements
 * @param       stream  file stream
 * @return      number of items successfully written
 */
extern int safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE * stream);

/*
 * Convert Ethernet address string representation to binary data
 * @param       a       string in xx:xx:xx:xx:xx:xx notation
 * @param       e       binary data
 * @return      TRUE if conversion was successful and FALSE otherwise
 */
extern int ether_atoe(const char *a, char *e);

int indexof(char *str, char c);

/*
 * Convert Ethernet address binary data to string representation
 * @param       e       binary data
 * @param       a       string in xx:xx:xx:xx:xx:xx notation
 * @return      a
 */
extern char *ether_etoa(const char *e, char *a);

extern int nvifname_to_osifname(const char *nvifname, char *osifname_buf, int osifname_buf_len);
extern int osifname_to_nvifname(const char *osifname, char *nvifname_buf, int nvifname_buf_len);

extern int system2(char *command);
extern int sysprintf(const char *fmt, ...);
extern int f_exists(const char *path);	// note: anything but a directory

extern int get_ifname_unit(const char *ifname, int *unit, int *subunit);

/*
 * Concatenate two strings together into a caller supplied buffer
 * @param       s1      first string
 * @param       s2      second string
 * @param       buf     buffer large enough to hold both strings
 * @return      buf
 */
char *strcat_r(const char *s1, const char *s2, char *buf);

#ifdef MEMDEBUG

void *mymalloc(int size, char *func);
void myfree(void *mem);
void showmemdebugstat();

#define safe_malloc(size) mymalloc(size,__func__)
#define safe_free myfree(mem)
#define free myfree

#define memdebug_enter()  \
	struct sysinfo memdebuginfo; \
	sysinfo(&memdebuginfo); \
	long before = memdebuginfo.freeram;

#define memdebug_leave()  \
	sysinfo(&memdebuginfo); \
	long after = memdebuginfo.freeram; \
	if ((before-after)>0) \
	    { \
	    fprintf(stderr,"function %s->%s:%d leaks %ld bytes memory (before: %ld, after: %ld\n",__FILE__,__func__,__LINE__,(before-after),before,after); \
	    }
#define memdebug_leave_info(a)  \
	sysinfo(&memdebuginfo); \
	long after = memdebuginfo.freeram; \
	if ((before-after)>0) \
	    { \
	    fprintf(stderr,"function %s->%s:%d (%s) leaks %ld bytes memory (before: %ld, after: %ld\n",__FILE__,__func__,__LINE__,a,(before-after),before,after); \
	    }
#else
#define safe_malloc malloc
#define safe_free free
#define memdebug_enter()
#define memdebug_leave()
#define memdebug_leave_info(a)
#define showmemdebugstat()
#endif

/*
 * Check for a blank character; that is, a space or a tab 
 */
#define dd_isblank(c) ((c) == ' ' || (c) == '\t')

/*
 * Strip trailing CR/NL from string <s> 
 */
char *chomp(char *s);

/*
 * Simple version of _backtick() 
 */
#define backtick(cmd, args...) ({ \
	char *argv[] = { cmd, ## args, NULL }; \
	_backtick(argv); \
})
/*
 * Return NUL instead of NULL if undefined 
 */
#define safe_getenv(s) (getenv(s) ? : "")

#define HAVE_SILENCE 1
/*
 * Print directly to the console 
 */
#ifndef HAVE_SILENCE

#define cprintf(fmt, args...) do { \
		fprintf(stderr, fmt, ## args); \
		fflush(stderr); \
} while (0)
#else
#define cprintf(fmt, args...)
#endif

void strcpyto(char *dest, char *src, char c);


char *foreach_first(char *foreachwordlist, char *word);

char *foreach_last(char *next, char *word);


/*
 * Copy each token in wordlist delimited by space into word 
 */
#define foreach(word, foreachwordlist, next) \
	for (next = foreach_first(foreachwordlist, word); \
	     strlen(word); \
	     next = foreach_last(next, word)) 

/*
 * Return NUL instead of NULL if undefined 
 */
#define safe_getenv(s) (getenv(s) ? : "")

/*
 * Debug print 
 */
#ifdef DEBUG
#define dprintf(fmt, args...) cprintf("%s: " fmt, __FUNCTION__, ## args)
#else
#define dprintf(fmt, args...)
#endif

#ifdef HAVE_MICRO
#define FORK(a) a;
#else
#define FORK(func) \
{ \
    switch ( fork(  ) ) \
    { \
	case -1: \
	    break; \
	case 0: \
	    ( void )setsid(  ); \
	    func; \
	    exit(0); \
	    break; \
	default: \
	break; \
    } \
}
#endif

#ifdef vxworks

#include <inetLib.h>
#define inet_aton(a, n) ((inet_aton((a), (n)) == ERROR) ? 0 : 1)
#define inet_ntoa(n) ({ char a[INET_ADDR_LEN]; inet_ntoa_b ((n), a); a; })

#include <typedefs.h>
#include <bcmutils.h>
#define ether_atoe(a, e) bcm_ether_atoe((a), (e))
#define ether_etoa(e, a) bcm_ether_ntoa((e), (a))

/*
 * These declarations are not available where you would expect them 
 */
extern int vsnprintf(char *, size_t, const char *, va_list);
extern int snprintf(char *str, size_t count, const char *fmt, ...);
extern char *strdup(const char *);
extern char *strsep(char **stringp, char *delim);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);

/*
 * Neither are socket() and connect() 
 */
#include <sockLib.h>

#ifdef DEBUG
#undef dprintf
#define dprintf printf
#endif
#endif

#endif				/* _shutils_h_ */
