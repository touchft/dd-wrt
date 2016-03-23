
/*
 * Broadcom Home Gateway Reference Design
 * Web Page Configuration Support Routines
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: upgrade.c,v 1.4 2005/11/30 11:53:42 seg Exp $
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <broadcom.h>
#include <cyutils.h>
#include <shutils.h>
#include <byteswap.h>
#include <endian.h>		/* for __BYTE_ORDER */

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define HOST_TO_BE32(x)	bswap_32(x)
#  define HOST_TO_BE16(x)	bswap_16(x)
#else
#  define HOST_TO_BE32(x)	(x)
#  define HOST_TO_BE16(x)	(x)
#endif

#ifndef HAVE_MICRO
#define MIN_BUF_SIZE    65536
#else
#define MIN_BUF_SIZE    4096
#endif
#define CODE_PATTERN_ERROR 9999
static int upgrade_ret;

void set_upgrade_ret(int result)
{
	if (result != 0) {
		upgrade_ret = result;
	} else {
		upgrade_ret = NULL;
	}
}

void
// do_upgrade_cgi(char *url, FILE *stream)
do_upgrade_cgi(struct mime_handler *handler, char *url, webs_t stream, char *query)	// jimmy, https,
							// 8/6/2003
{
#ifndef ANTI_FLASH

	fprintf(stderr, "[UPGRADE] ret: %d\n", upgrade_ret);
	if (upgrade_ret) {
		do_ej(handler, "Fail_u_s.asp", stream, NULL);
		killall("ledtool", SIGTERM);
		led_control(LED_DIAG, LED_OFF);
	} else {
		do_ej(handler, "Success_u_s.asp", stream, NULL);
	}
	websDone(stream, 200);

	/*
	 * Reboot if successful 
	 */
	if (upgrade_ret == 0) {
		// sleep (10);
		sys_reboot();
	}
#else

	do_ej(handler, "Fail_u_s.asp", stream, NULL);
	websDone(stream, 200);

#endif
}

typedef struct {
	char magic[8];		// "FIRMWARE"
	char ver_inter[4];
	char ver_outer[4];
	char model[24];
	unsigned int len;
	unsigned int crc32;
} ralink_firmware_header;

static char *allmagics[] = {
	CODE_PATTERN_WRT54G,
	CODE_PATTERN_WRT54GS,
	CODE_PATTERN_WRH54G,
	CODE_PATTERN_WRT150N,
	CODE_PATTERN_WRT160N,
	CODE_PATTERN_WRT300N,
	CODE_PATTERN_WRT300NV11,
	CODE_PATTERN_WRT310N,
	CODE_PATTERN_WRT350N,
	CODE_PATTERN_WRTSL54GS,
	CODE_PATTERN_WRT54G3G,
	CODE_PATTERN_WRT54G3GV,
	CODE_PATTERN_WRT610N,
	CODE_PATTERN_WRT54GSV4,
	CODE_PATTERN_WRT320N,
	CODE_PATTERN_VALET_M10,
	CODE_PATTERN_VALET_M20,
	CODE_PATTERN_E900,
	CODE_PATTERN_E800,
	CODE_PATTERN_E1000,
	CODE_PATTERN_E1200V1,
	CODE_PATTERN_E1200V2,
	CODE_PATTERN_E1500,
	CODE_PATTERN_E1550,
	CODE_PATTERN_E2000,
	CODE_PATTERN_E2500,
	CODE_PATTERN_E3000,
	CODE_PATTERN_E3200,
	CODE_PATTERN_E4200,
	CODE_PATTERN_NV60K,
	CODE_PATTERN_NV64K,
	NULL
};

static char *nv60k[] = {
	CODE_PATTERN_E1550,
	CODE_PATTERN_E2000,
	CODE_PATTERN_E2500,
	CODE_PATTERN_E3000,
	CODE_PATTERN_E3200,
	CODE_PATTERN_E4200,
	CODE_PATTERN_NV60K,
	NULL
};

static char *nv64k[] = {
	CODE_PATTERN_E900,
	CODE_PATTERN_E800,
	CODE_PATTERN_E1200V1,
	CODE_PATTERN_E1200V2,
	CODE_PATTERN_E1500,
	CODE_PATTERN_NV64K,
	NULL
};

static int checkmagic(char *magic, char *check[])
{
	int cnt = 0;
	while (check[cnt]) {
		if (!memcmp(magic, check[cnt], 4))
			return 0;
		cnt++;
	}
	return -1;
}

int
// sys_upgrade(char *url, FILE *stream, int *total)
sys_upgrade(char *url, webs_t stream, int *total, int type)	// jimmy,
								// https,
								// 8/6/2003
{

	int brand = getRouterBrand();

#ifndef ANTI_FLASH
	char upload_fifo[] = "/tmp/uploadXXXXXX";
	FILE *fifo = NULL;
	char *write_argv[4];
	pid_t pid;
	char *buf = NULL;
	int count, ret = 0;
	long flags = -1;
	int size = BUFSIZ;
	int i = 0;
#if (defined(HAVE_FONERA) || defined(HAVE_WHRAG108) || defined(HAVE_MERAKI) || defined(HAVE_TW6600) || defined(HAVE_PB42) || defined(HAVE_LS5) || defined(HAVE_USR5453)) && \
(!defined(HAVE_DIR400) && !defined(HAVE_WRT54G2) && !defined(HAVE_GWMF54G2) && !defined(HAVE_FONERA2200) && !defined(HAVE_MR3202A) && !defined(HAVE_CA8PRO) &&  \
!defined(HAVE_CA8) && !defined(HAVE_RT2880) && !defined(HAVE_LS2) && !defined(HAVE_WRK54G) && !defined(HAVE_ADM5120) && !defined(HAVE_DIR300) && !defined(HAVE_DLM101) && \
!defined(HAVE_MERAKI) && !defined(HAVE_SOLO51) && !defined(HAVE_RTG32) && !defined(HAVE_EOC5610) && !defined(HAVE_NP25G))
#define WRITEPART "rootfs"
#else
#define WRITEPART "linux"
#endif

	write_argv[0] = "write";
	write_argv[1] = upload_fifo;
	write_argv[2] = WRITEPART;
	write_argv[3] = NULL;
	eval("fischecksum");
	if (url)
		return eval("write", url, WRITEPART);
	// diag_led(DIAG, START_LED); // blink the diag led
	C_led(1);
#ifdef HAVE_HTTPS
	if (do_ssl)
		ACTION("ACT_WEBS_UPGRADE");
	else
#endif
		ACTION("ACT_WEB_UPGRADE");
	int uploadcount = 0;

	/*
	 * Set nonblock on the socket so we can timeout 
	 */
#ifdef HAVE_HTTPS
	if (!do_ssl) {
#endif
		if ((flags = fcntl(fileno(stream->fp), F_GETFL)) < 0 || fcntl(fileno(stream->fp), F_SETFL, flags | O_NONBLOCK) < 0) {
			ret = errno;
			goto err;
		}
#ifdef HAVE_HTTPS
	}
#endif

	/*
	 ** The buffer must be at least as big as what the stream file is
	 ** using so that it can read all the data that has been buffered
	 ** in the stream file. Otherwise it would be out of sync with fn
	 ** select specially at the end of the data stream in which case
	 ** the select tells there is no more data available but there in
	 ** fact is data buffered in the stream file's buffer. Since no
	 ** one has changed the default stream file's buffer size, let's
	 ** use the constant BUFSIZ until someone changes it.
	 **/

	if (size < MIN_BUF_SIZE)
		size = MIN_BUF_SIZE;
	fprintf(stderr, "use buffer size %d\n", size);
	if ((buf = safe_malloc(size)) == NULL) {
		ret = ENOMEM;
		goto err;
	}
	eval("ledtool", "500");
	/*
	 * Pipe the rest to the FIFO 
	 */
	cprintf("Upgrading\n");
	int lastblock = 0;
	while (total && *total) {
#ifdef HAVE_HTTPS
		if (do_ssl) {
			if (size > *total)
				size = *total;
			count = wfread(buf, 1, size, stream);
		} else
#endif
		{
			if (waitfor(fileno(stream->fp), 5) <= 0) {
				lastblock = 1;
			}
			count = safe_fread(buf, 1, size, stream->fp);
			if (!count && (ferror(stream->fp) || feof(stream->fp))) {
				break;
			}
		}

		if (i == 0) {	// check code pattern, the first data must
#ifdef HAVE_VENTANA
			if (!strncmp(buf, "UBI#", 4)) {	// check for "UBI#"
				eval("mount", "-o", "remount,ro", "/");
				char *write_argv_buf[8];
				fprintf(stderr, "erase nandflash\n");
				eval("mtd", "erase", "rootfs");
				sleep(10);
				write_argv_buf[0] = "mtd";
				write_argv_buf[1] = "-f";
				write_argv_buf[2] = "write";
				write_argv_buf[3] = upload_fifo;
				write_argv_buf[4] = "rootfs";
				write_argv_buf[5] = NULL;
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;
			}
#endif
#if defined(HAVE_DIR860) || defined(HAVE_DIR859)
#define SEAMA_MAGIC		0x5EA3A417

			typedef struct seama_hdr seamahdr_t;
			struct seama_hdr {
				unsigned int magic;	/* should always be SEAMA_MAGIC. */
				unsigned short reserved;	/* reserved for  */
				unsigned short metasize;	/* size of the META data */
				unsigned int size;	/* size of the image */
			} __attribute__((packed));
			seamahdr_t *seama = (seamahdr_t *) buf;

			if (seama->magic == HOST_TO_BE32(SEAMA_MAGIC)) {
				unsigned int skip = HOST_TO_BE16(seama->metasize) + sizeof(seamahdr_t);
				fprintf(stderr, "found seama header, skip seal header of %d bytes\n", skip);
				if (skip > count)
					goto err;
				memcpy(buf, buf + skip, count - skip);
				count -= skip;
				char *write_argv_buf[8];
				write_argv_buf[0] = "mtd";
				write_argv_buf[1] = "-f";
				write_argv_buf[2] = "write";
				write_argv_buf[3] = upload_fifo;
				write_argv_buf[4] = "linux";
				write_argv_buf[5] = NULL;
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;

			}
#endif
#if defined(HAVE_DIR862)
			unsigned int *uboot_magic = (unsigned int *)buf;
			if (*uboot_magic == HOST_TO_BE32(0x27051956)) {
				char *write_argv_buf[8];
				write_argv_buf[0] = "mtd";
				write_argv_buf[1] = "-f";
				write_argv_buf[2] = "write";
				write_argv_buf[3] = upload_fifo;
				write_argv_buf[4] = "linux";
				write_argv_buf[5] = NULL;
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;
			}
#endif
#if defined(HAVE_DAP2230) || defined(HAVE_DAP2330) || defined(HAVE_DAP2660) || defined(HAVE_DAP3662) || defined(HAVE_DAP3320)
#ifdef HAVE_DAP2660
#define MAGIC "wapac09_dkbs_dap2660"
#elif HAVE_DAP2330
#define MAGIC "wapn24_dkbs_dap2330"
#elif HAVE_DAP3662
#define MAGIC "wapac11_dkbs_dap3662"
#elif HAVE_DAP3320
#define MAGIC "wapn29_dkbs_dap3320"
#elif HAVE_DAP2230
#define MAGIC "wapn31_dkbs_dap2230"
#endif
			if (!strncmp(buf, MAGIC, strlen(MAGIC))) {
				char *write_argv_buf[8];
				write_argv_buf[0] = "mtd";
				write_argv_buf[1] = "-f";
				write_argv_buf[2] = "write";
				write_argv_buf[3] = upload_fifo;
				write_argv_buf[4] = "linux";
				write_argv_buf[5] = NULL;
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;
			}
#endif
#ifdef HAVE_BUFFALO
			ralink_firmware_header fh;
			memcpy(&fh, buf, sizeof(fh));
			char *str;
			str = (char *)&fh;
			unsigned char ch, temp;
			int idx, index;
			ch = 0xff;
			index = sizeof(fh) - 1;
			for (idx = 0; idx < sizeof(fh) / 2; idx++) {
				temp = str[idx];
				str[idx] = str[index - idx];
				str[index - idx] = temp;
			}
			for (idx = 0; idx < sizeof(fh); idx++)
				str[idx] ^= ch;
			if (!strncmp(buf, "bgn", 3) || !strncmp(buf, "WZR", 3)
			    || !strncmp(buf, "WHR", 3)
			    || !strncmp(buf, "WLA", 3)) {
				char *write_argv_buf[4];
				write_argv_buf[0] = "buffalo_flash";
				write_argv_buf[1] = upload_fifo;
				write_argv_buf[2] = NULL;
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;
			} else if (!strncmp(fh.magic, "FIRMWARE", 8)) {	// check for "FIRMWARE"
				char *write_argv_buf[4];
				write_argv_buf[0] = "buffalo_flash";
				write_argv_buf[1] = upload_fifo;
				write_argv_buf[2] = "ralink";
				write_argv_buf[3] = NULL;
				memcpy(buf, &fh, sizeof(fh));	//write back decrypted content
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv_buf, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}
				goto write_data;
			} else {
				if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv, NULL, 0, &pid))
				    || !(fifo = fopen(upload_fifo, "w"))) {
					if (!ret)
						ret = errno;
					goto err;
				}

			}

#else
			/*
			 * Feed write from a temporary FIFO 
			 */
			if (!mktemp(upload_fifo) || mkfifo(upload_fifo, S_IRWXU) < 0 || (ret = _evalpid(write_argv, NULL, 0, &pid)) || !(fifo = fopen(upload_fifo, "w"))) {
				if (!ret)
					ret = errno;
				goto err;
			}
#endif
			// have code pattern
			char ver[40];
			long ver1, ver2, ver3;
			snprintf(ver, sizeof(ver), "v%d.%d.%d", buf[11], buf[12], buf[13]);
			ver1 = convert_ver(ver);
			ver2 = convert_ver(INTEL_FLASH_SUPPORT_VERSION_FROM);
			ver3 = convert_ver(BCM4712_CHIP_SUPPORT_VERSION_FROM);
			fprintf(stderr, "upgrade_ver[%s] upgrade_ver[%ld] intel_ver[%ld] 4712_ver[%ld]\n", ver, ver1, ver2, ver3);
#if defined(HAVE_WIKINGS) || defined(HAVE_ESPOD)
#ifdef HAVE_WIKINGS
#ifdef HAVE_SUB3
#define V "XMED"
#elif HAVE_SUB6
#define V "XMIN"
#else
#define V "XMAX"
#endif
#endif
#ifdef HAVE_ESPOD
#ifdef HAVE_SUB3
#define V "EPMN"
#elif HAVE_SUB6
#define V "EPMD"
#else
#define V "EPMX"
#endif
#endif
			if (memcmp(&buf[0], V, 4)) {
				fprintf(stderr, "code pattern error!\n");
				goto write_data;	// must be there, otherwise fail here
				//goto err;     // must be there, otherwise fail here
			}
#undef V
#endif

#if defined(HAVE_WIKINGS) || defined(HAVE_ESPOD)
#else
#ifdef HAVE_WRT160NL
			if (memcmp(&buf[0], &CODE_PATTERN_WRT160NL, 4)
			    && memcmp(&buf[0], &CODE_PATTERN_E2100L, 4)) {
				cprintf("code pattern error!\n");
				goto err;	// must be there, otherwise fail here
			}
#else

#ifndef HAVE_80211AC
			if (brand == ROUTER_LINKSYS_E1550 || (brand == ROUTER_WRT320N && nvram_match("boardrev", "0x1307"))	//E2000
			    || brand == ROUTER_LINKSYS_E2500 || (brand == ROUTER_WRT610NV2 && nvram_match("boot_hw_model", "E300"))	//E3000
			    || brand == ROUTER_LINKSYS_E3200 || brand == ROUTER_LINKSYS_E4200) {
				if (checkmagic(&buf[0], nv60k)) {
					cprintf("image not compatible with nv60k router!\n");
					goto err;	// must be there, otherwise fail here
				}
			} else if (brand == ROUTER_NETGEAR_WNDR4000 || brand == ROUTER_NETGEAR_WNDR3400 || brand == ROUTER_LINKSYS_E900 || brand == ROUTER_LINKSYS_E800 || brand == ROUTER_LINKSYS_E1500) {
				if (checkmagic(&buf[0], nv64k)) {
					cprintf("image not compatible with nv64k router!\n");
					goto err;	// must be there, otherwise fail here
				}
			} else {
				if (memcmp(&buf[0], &CODE_PATTERN_NV60K, 4) == 0 || memcmp(&buf[0], &CODE_PATTERN_NV64K, 4) == 0) {
					cprintf("image not compatible with your router!\n");
					goto err;	// fail here                            
				}
			}
#endif
			if (checkmagic(&buf[0], allmagics)) {
				cprintf("code pattern error!\n");
				goto write_data;
			}
#endif
#endif

			if (check_hw_type() == BCM4712_CHIP && ver1 < ver3) {
				fprintf(stderr, "The old firmware version can't support bcm4712 chipset\n");
				fprintf(stderr, "Can't downgrade to this old firmware version (%s), must be above %s(included)\n", ver, BCM4712_CHIP_SUPPORT_VERSION_FROM);
				goto write_data;
			}

			fprintf(stderr, "code pattern correct!\n");
			*total -= count;
#ifdef HAVE_WRT160NL
			safe_fwrite(buf, 1, count, fifo);	// we have to write the whole header to flash too
#else
			safe_fwrite(&buf[sizeof(struct code_header)], 1, count - sizeof(struct code_header), fifo);
#endif
			i++;
			continue;
		}

	      write_data:
		*total -= count;
		safe_fwrite(buf, 1, count, fifo);
		uploadcount += count;
		fprintf(stderr, "uploading [%d]\r", uploadcount);
		if (lastblock)
			break;
		i++;
	}

	fprintf(stderr, "uploading [%d]\n", uploadcount);
	fclose(fifo);
	fifo = NULL;
	/*
	 * Wait for write to terminate 
	 */
	waitpid(pid, &ret, 0);
	cprintf("done\n");
#ifdef HAVE_HTTPS
	if (!do_ssl) {
#endif
		/*
		 * Reset nonblock on the socket 
		 */
		if (fcntl(fileno(stream->fp), F_SETFL, flags) < 0) {
			ret = errno;
			goto err;
		}
#ifdef HAVE_HTTPS
	}
#endif

err:
	if (buf)
		free(buf);
	if (fifo)
		fclose(fifo);
	unlink(upload_fifo);
	// diag_led(DIAG, STOP_LED);
	C_led(0);
	ACTION("ACT_IDLE");
	return ret;
#else
	return 0;
#endif
}

void
// do_upgrade_post(char *url, FILE *stream, int len, char *boundary)
do_upgrade_post(char *url, webs_t stream, int len, char *boundary)	// jimmy, 
									// https, 
									// 8/6/2003
{
	killall("udhcpc", SIGKILL);

#ifndef ANTI_FLASH
	int type = 0;
	char *buf = malloc(1024);
	upgrade_ret = EINVAL;

	/*
	 * Look for our part 
	 */
	while (len > 0) {
		if (!wfgets(buf, MIN(len + 1, 1024), stream)) {
			free(buf);
			return;
		}

		len -= strlen(buf);
		if (!strncasecmp(buf, "Content-Disposition:", 20)) {
			if (strstr(buf, "name=\"erase\"")) {
				while (len > 0 && strcmp(buf, "\n")
				       && strcmp(buf, "\r\n")) {
					if (!wfgets(buf, MIN(len + 1, 1024), stream)) {
						free(buf);
						return;
					}

					len -= strlen(buf);
				}
				if (!wfgets(buf, MIN(len + 1, 1024), stream)) {
					free(buf);
					return;
				}
				len -= strlen(buf);
				buf[1] = '\0';	// we only want the 1st digit
				nvram_set("sv_restore_defaults", buf);
				nvram_commit();
			} else if (strstr(buf, "name=\"file\"")) {	// upgrade image
				type = 0;
				break;
			}
		}
	}

	/*
	 * Skip boundary and headers 
	 */
	while (len > 0) {
		if (!wfgets(buf, MIN(len + 1, 1024), stream)) {
			free(buf);
			return;
		}

		len -= strlen(buf);
		if (!strcmp(buf, "\n") || !strcmp(buf, "\r\n"))
			break;
	}

	upgrade_ret = sys_upgrade(NULL, stream, &len, type);

	/*
	 * Restore factory original settings if told to. This will also cause a
	 * restore defaults on reboot of a Sveasoft firmware. 
	 */
#ifdef HAVE_BUFFALO_SA
	int region_sa = 0;
	if (nvram_default_match("region", "SA", ""))
		region_sa = 1;
#endif
	if (nvram_match("sv_restore_defaults", "1")) {
		eval("erase", "nvram");
#ifdef HAVE_BUFFALO_SA
		nvram_set("sv_restore_defaults", "1");
		if (region_sa)
			nvram_set("region", "SA");
#endif
	}
	sys_commit();

	/*
	 * Slurp anything remaining in the request 
	 */

	wfgets(buf, len, stream);
	free(buf);
#endif
}
