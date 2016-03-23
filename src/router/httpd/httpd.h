/*
 * milli_httpd - pretty small HTTP server
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: httpd.h,v 1.2 2005/11/24 19:38:45 seg Exp $
 */

#include <cy_conf.h>
#ifndef _httpd_h_
#define _httpd_h_

#if defined(DEBUG) && defined(DMALLOC)
#include <dmalloc.h>
#endif

#ifdef HAVE_MATRIXSSL
#define DDWRT
# include <matrixSsl.h>
# include <matrixssl_xface.h>
#endif

#ifdef HAVE_OPENSSL
#include <ssl.h>
extern BIO *bio_err;
#endif

#include <bcmnvram.h>

typedef struct {
	FILE *fp;
	int userid;
} webs;

typedef webs *webs_t;

extern char *wfgets(char *buf, int len, webs_t fp);
extern int wfprintf(webs_t fp, char *fmt, ...);
extern size_t wfwrite(char *buf, int size, int n, webs_t fp);
extern size_t wfread(char *buf, int size, int n, webs_t fp);
extern int wfclose(webs_t fp);
extern int wfflush(webs_t fp);
#ifndef VALIDSOURCE
#ifndef VISUALSOURCE

extern int wfputs(char *buf, webs_t fp);
#endif
#endif
#ifdef HAVE_HTTPS
extern int do_ssl;
#endif
/* Basic authorization userid and passwd limit */
#define AUTH_MAX 64

extern char auth_realm[AUTH_MAX];
extern void send_authenticate(char *realm);

/* Generic MIME type handler */
struct mime_handler {
	char *pattern;
	char *mime_type;
	char *extra_header;
	void (*input) (char *path, webs_t stream, int len, char *boundary);
	void (*output) (struct mime_handler * handler, char *path, webs_t stream, char *query);
	int (*auth) (webs_t wp, char *userid, char *passwd, char *realm, char *authorisation, int (*auth_check) (char *userid, char *passwd, char *dirname, char *authorisation));
	unsigned char send_headers;
};

typedef struct {
	const char *path;	/* Web page URL path */
	unsigned int size;	/* Size of web page in bytes */
} websRomPageIndexType;

//extern void setLength(long len);

extern struct mime_handler mime_handlers[];

/* CGI helper functions */
extern void init_cgi(char *query);
extern char *get_cgi(char *name);
extern void set_cgi(char *name, char *value);
extern int count_cgi();

/* Regular file handler */
extern void do_file(struct mime_handler *handler, char *path, webs_t stream, char *query);
extern void do_file_attach(struct mime_handler *handler, char *path, webs_t stream, char *query, char *attachment);

/* GoAhead 2.1 compatibility */
//typedef FILE * webs_t;
typedef char char_t;
#define T(s) (s)
#define __TMPVAR(x) tmpvar ## x
#define _TMPVAR(x) __TMPVAR(x)
#define TMPVAR _TMPVAR(__LINE__)

#define websDebugWrite(wp, fmt, args...)
#define websError(wp, code, msg, args...)

#define websHeader(wp) wfputs("<html lang=\"en\">", wp)
#define websFooter(wp) wfputs("</html>", wp)
#define websDone(wp, code) wfflush(wp)

#ifndef VALIDSOURCE
#ifndef VISUALSOURCE
char *websGetVar(webs_t wp, char *var, char *d);
#endif
#endif

struct Webenvironment {
	void (*Pdo_ej_buffer) (char *buffer, webs_t stream);
	int (*Phttpd_filter_name) (char *old_name, char *new_name, size_t size, int type);
	char *(*PwebsGetVar) (webs_t wp, char *var, char *d);
	int (*PwebsWrite) (webs_t wp, char *fmt, ...);
	struct wl_client_mac *Pwl_client_macs;
	void (*Pdo_ej) (struct mime_handler * handler, char *path, webs_t stream, char *query);	// jimmy, https, 8/4/2003
	int (*PejArgs) (int argc, char_t ** argv, char_t * fmt, ...);
	FILE *(*PgetWebsFile) (char *path);
	int (*Pwfputs) (char *buf, webs_t fp);
	char *(*PGOZILA_GET) (webs_t wp, char *name);
	char *(*Plive_translate) (const char *tran);
	void (*Pvalidate_cgi) (webs_t fp);
	websRomPageIndexType *PwebsRomPageIndex;
	char *Prequest_url;
#ifdef HAVE_HTTPS
	int Pdo_ssl;
#endif
};

#define websSetVar(wp, var, value) set_cgi(var, value)
#define websDefaultHandler(wp, urlPrefix, webDir, arg, url, path, query) ({ do_ej(path, wp,""); fflush(wp); 1; })
#define websWriteData(wp, buf, nChars) ({ int TMPVAR = wfwrite(buf, 1, nChars, wp); wfflush(wp); TMPVAR; })
#define websWriteDataNonBlock websWriteData
#define a_assert(a)

/* GoAhead 2.1 Embedded JavaScript compatibility */
extern int getWebsFileLen(char *path);

extern void do_filtertable(struct mime_handler *handler, char *path, webs_t stream, char *query);
extern void do_wds(struct mime_handler *handler, char *path, webs_t stream, char *query);
extern void do_wireless_adv(struct mime_handler *handler, char *path, webs_t stream, char *query);
#ifndef VISUALSOURCE
#ifndef VALIDSOURCE
extern FILE *getWebsFile(char *path);
extern int ejArgs(int argc, char_t ** argv, char_t * fmt, ...);
extern void do_ej(struct mime_handler *handler, char *path, webs_t stream, char *query);
extern void do_ej_buffer(char *buffer, webs_t stream);
extern int websWrite(webs_t wp, char *fmt, ...);
#endif
#endif
int do_auth(webs_t wp, char *userid, char *passwd, char *realm, char *authorisation, int (*auth_check) (char *userid, char *passwd, char *dirname, char *authorisation));
void Initnvramtab(void);
void *call_ej(char *name, void *handle, webs_t wp, int argc, char_t ** argv);

#endif				/* _httpd_h_ */
