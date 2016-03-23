/*
 * wireless_madwifi.c
 *
 * Copyright (C) 2005 - 2013 Sebastian Gottschall <gottschall@dd-wrt.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id:
 */
#ifdef HAVE_MADWIFI
#define VISUALSOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/statfs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <broadcom.h>
#include <cymac.h>
#include <wlutils.h>
#include <bcmparams.h>
#include <dirent.h>
#include <netdb.h>
#include <utils.h>
#include <wlutils.h>
#include <bcmnvram.h>

#include "wireless_generic.c"

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <err.h>

#ifndef __UCLIBC__
/* Convenience types.  */
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

/* Fixed-size types, underlying types depend on word size and compiler.  */
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
#endif
#include "wireless_copy.h"
#include "net80211/ieee80211.h"
#include "net80211/ieee80211_crypto.h"
#include "net80211/ieee80211_ioctl.h"

unsigned char madbuf[24 * 1024];

static const char *ieee80211_ntoa(const uint8_t mac[IEEE80211_ADDR_LEN])
{
	static char a[18];
	int i;

	i = snprintf(a, sizeof(a), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	return (i < 17 ? NULL : a);
}

int ej_active_wireless_if(webs_t wp, int argc, char_t ** argv, char *ifname, int cnt, int turbo, int macmask)
{
	// unsigned char buf[24 * 1024];

	unsigned char *cp;
	int s, len;
	struct iwreq iwr;
	char nb[32];
	sprintf(nb, "%s_bias", ifname);
	int bias = atoi(nvram_default_get(nb, "0"));
	if (!ifexists(ifname)) {
		printf("IOCTL_STA_INFO ifresolv %s failed!\n", ifname);
		return cnt;
	}
	int state = get_radiostate(ifname);

	if (state == 0 || state == -1) {
		printf("IOCTL_STA_INFO radio %s not enabled!\n", ifname);
		return cnt;
	}
	s = getsocket();
	if (s < 0) {
		fprintf(stderr, "socket(SOCK_DRAGM)\n");
		return cnt;
	}
	(void)memset(&iwr, 0, sizeof(struct iwreq));
	(void)strncpy(iwr.ifr_name, ifname, sizeof(iwr.ifr_name));

	iwr.u.data.pointer = (void *)&madbuf[0];
	iwr.u.data.length = 24 * 1024;
	if (ioctl(s, IEEE80211_IOCTL_STA_INFO, &iwr) < 0) {
		fprintf(stderr, "IOCTL_STA_INFO for %s failed!\n", ifname);
		closesocket();
		return cnt;
	}
	len = iwr.u.data.length;
	if (len < sizeof(struct ieee80211req_sta_info)) {
		// fprintf(stderr,"IOCTL_STA_INFO len<struct %s failed!\n",ifname);
		closesocket();
		return cnt;
	}
	cp = madbuf;
	int bufcount = 0;
	do {
		struct ieee80211req_sta_info *si;
		uint8_t *vp;

		si = (struct ieee80211req_sta_info *)cp;
		vp = (u_int8_t *)(si + 1);

		if (cnt)
			websWrite(wp, ",");
		cnt++;
		char mac[32];

		strncpy(mac, ieee80211_ntoa(si->isi_macaddr), 31);
		if (nvram_match("maskmac", "1") && macmask) {
			mac[0] = 'x';
			mac[1] = 'x';
			mac[3] = 'x';
			mac[4] = 'x';
			mac[6] = 'x';
			mac[7] = 'x';
			mac[9] = 'x';
			mac[10] = 'x';
		}
		if (si->isi_noise == 0) {
			si->isi_noise = -95;
		}
		int qual = (si->isi_noise + si->isi_rssi) * 124 + 11600;
		qual /= 10;
		char *type = "";
		if (si->isi_athflags & IEEE80211_ATHC_WDS)
			type = "WDS:";

		if (si->isi_rates && ((si->isi_rates[si->isi_txrate] & IEEE80211_RATE_VAL) != 0)
		    && ((si->isi_rates[si->isi_rxrate] & IEEE80211_RATE_VAL) != 0)) {
			websWrite(wp,
				  "'%s','%s%s','%s','%3dM','%3dM','N/A','%d','%d','%d','%d'",
				  mac, type, ifname, UPTIME(si->isi_uptime),
				  ((si->isi_rates[si->isi_txrate] &
				    IEEE80211_RATE_VAL) / 2) * turbo, ((si->isi_rates[si->isi_rxrate] & IEEE80211_RATE_VAL) / 2) * turbo, si->isi_noise + si->isi_rssi + bias, si->isi_noise + bias, si->isi_rssi, qual);
		} else {
			websWrite(wp, "'%s','%s%s','%s','N/A','N/A','N/A','%d','%d','%d','%d'", mac, type, ifname, UPTIME(si->isi_uptime), si->isi_noise + si->isi_rssi + bias, si->isi_noise + bias, si->isi_rssi, qual);
		}
		bufcount += si->isi_len;
		cp += si->isi_len;
		len -= si->isi_len;
	}
	while (len >= sizeof(struct ieee80211req_sta_info)
	       && bufcount < (sizeof(madbuf) - sizeof(struct ieee80211req_sta_info)));
	closesocket();

	return cnt;
}

#if defined(HAVE_MADWIFI_MIMO)
extern int ej_active_wireless_if_11n(webs_t wp, int argc, char_t ** argv, char *ifname, int cnt, int turbo, int macmask);
#endif
#if defined(HAVE_ATH9K)
extern int ej_active_wireless_if_ath9k(webs_t wp, int argc, char_t ** argv, char *ifname, int cnt, int turbo, int macmask);
#endif

void ej_active_wireless(webs_t wp, int argc, char_t ** argv)
{
	int c = getdevicecount();
	char devs[32];
	int i;
	int cnt = 0;
	char turbo[32];
	int t;
	int macmask;
	int gotassocs = 0;
	ejArgs(argc, argv, "%d", &macmask);
	for (i = 0; i < c; i++) {
		sprintf(devs, "ath%d", i);
		sprintf(turbo, "%s_channelbw", devs);
		if (nvram_match(turbo, "40"))
			t = 2;
		else
			t = 1;
#if defined(HAVE_MADWIFI_MIMO)
		if (is_ar5008(devs)) {
			cnt = ej_active_wireless_if_11n(wp, argc, argv, devs, cnt, t, macmask);
			gotassocs = 1;
		}
#endif
#ifdef HAVE_ATH9K
		if (is_ath9k(devs)) {
			cnt = ej_active_wireless_if_ath9k(wp, argc, argv, devs, cnt, t, macmask);
			gotassocs = 1;
		}
#endif
		if (!gotassocs) {
			cnt = ej_active_wireless_if(wp, argc, argv, devs, cnt, t, macmask);
		}
#ifdef HAVE_ATH9K
		if (!is_ath9k(devs))
#endif
		{
			char vif[32];

			sprintf(vif, "%s_vifs", devs);
			char var[80], *next;
			char *vifs = nvram_get(vif);
			if (vifs != NULL)
				foreach(var, vifs, next) {
#if defined(HAVE_MADWIFI_MIMO)
				if (is_ar5008(devs))
					cnt = ej_active_wireless_if_11n(wp, argc, argv, var, cnt, t, macmask);
				else
#endif
					cnt = ej_active_wireless_if(wp, argc, argv, var, cnt, t, macmask);
				}
		}
	}

	// show wds links
	for (i = 0; i < c; i++) {
#ifdef HAVE_ATH9K
		sprintf(devs, "ath%d", i);
		if (!is_ath9k(devs))
#endif

		{

			int s;

			for (s = 1; s <= 10; s++) {
				char wdsvarname[32] = { 0 };
				char wdsdevname[32] = { 0 };
				char wdsmacname[32] = { 0 };
				char *dev;
				char *hwaddr;
				char var[80];

				sprintf(wdsvarname, "ath%d_wds%d_enable", i, s);
				sprintf(wdsdevname, "ath%d_wds%d_if", i, s);
				sprintf(wdsmacname, "ath%d_wds%d_hwaddr", i, s);
				sprintf(turbo, "ath%d_channelbw", i);
				if (nvram_match(turbo, "40"))
					t = 2;
				else
					t = 1;

				dev = nvram_safe_get(wdsdevname);
				if (dev == NULL || strlen(dev) == 0)
					continue;
				if (nvram_match(wdsvarname, "0"))
					continue;
#if defined(HAVE_MADWIFI_MIMO)
				if (is_ar5008(devs))
					cnt = ej_active_wireless_if_11n(wp, argc, argv, dev, cnt, t, macmask);
				else
#endif
					cnt = ej_active_wireless_if(wp, argc, argv, dev, cnt, t, macmask);
			}
		}
	}
}

int get_distance(char *ifname)
{
	char path[64];
	int ifcount, distance = 0;

	strcpy(path, ifname);
	sscanf(path, "ath%d", &ifcount);
	sprintf(path, "/proc/sys/dev/wifi%d/distance", ifcount);
	FILE *in = fopen(path, "rb");

	if (in != NULL) {
		fscanf(in, "%d", &distance);
		fclose(in);
	}

/*	sprintf(path, "/proc/sys/dev/wifi%d/timingoffset", ifcount);
	in = fopen(path, "rb");

	if (in != NULL) {
		fscanf(in, "%d", &tim);
		fclose(in);
	}
	ack -= tim * 2;
*/
	return distance;
}

int get_acktiming(char *ifname)
{
	char path[64];
	int ifcount, ack = 0;

	strcpy(path, ifname);
	sscanf(path, "ath%d", &ifcount);
	sprintf(path, "/proc/sys/dev/wifi%d/acktimeout", ifcount);
	FILE *in = fopen(path, "rb");

	if (in != NULL) {
		fscanf(in, "%d", &ack);
		fclose(in);
	}

/*	sprintf(path, "/proc/sys/dev/wifi%d/timingoffset", ifcount);
	in = fopen(path, "rb");

	if (in != NULL) {
		fscanf(in, "%d", &tim);
		fclose(in);
	}
	ack -= tim * 2;
*/
	return ack;
}

void ej_update_acktiming(webs_t wp, int argc, char_t ** argv)
{
	unsigned int ack, distance;
	char *ifname = nvram_safe_get("wifi_display");
	if (nvram_nmatch("disabled", "%s_net_mode", ifname)) {
		websWrite(wp, "N/A");
		return;
	}
#ifdef HAVE_ATH10K
	if (is_ath10k(ifname) && !is_mvebu(ifname)) {
		ack = get_ath10kack(ifname);
		distance = get_ath10kdistance(ifname);
	} else
#endif
#ifdef HAVE_ATH9K
	if (is_ath9k(ifname) || is_mvebu(ifname)) {
		int coverage = mac80211_get_coverageclass(ifname);
		ack = coverage * 3;
		/* See handle_distance() for an explanation where the '450' comes from */
		distance = coverage * 450;
	} else {
#endif
		ack = get_acktiming(ifname);
		distance = get_distance(ifname);
#ifdef HAVE_ATH9K
	}
#endif
	if (ack <= 0 || distance <= 0)
		websWrite(wp, "N/A");
	else
		websWrite(wp, "%d&#181;s (%dm)", ack, distance);
}

void ej_show_acktiming(webs_t wp, int argc, char_t ** argv)
{
	unsigned int ack, distance;
	websWrite(wp, "<div class=\"setting\">\n");
	websWrite(wp, "<div class=\"label\">%s</div>\n", live_translate("share.acktiming"));
	websWrite(wp, "<span id=\"wl_ack\">\n");
	ej_update_acktiming(wp, argc, argv);
	websWrite(wp, "</span> &nbsp;\n", ack, distance);
	websWrite(wp, "</div>\n");
}

extern int wifi_getrate(char *ifname);

#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

void ej_get_currate(webs_t wp, int argc, char_t ** argv)
{
	char mode[32];
	char *ifname = nvram_safe_get("wifi_display");
	int state = get_radiostate(ifname);

	if (state == 0 || state == -1) {
		websWrite(wp, "%s", live_translate("share.disabled"));
		return;
	}
	int rate = wifi_getrate(ifname);
	char scale;
	int divisor;

	if (rate >= MEGA) {
		scale = 'M';
		divisor = MEGA;
	} else {
		scale = 'k';
		divisor = KILO;
	}
	sprintf(mode, "%s_channelbw", ifname);
#ifdef HAVE_ATH9K
	if (!is_ath9k(ifname))
#endif
	{

		if (nvram_match(mode, "40"))
			rate *= 2;
	}
	if (rate > 0.0) {
		websWrite(wp, "%d %cb/s", rate / divisor, scale);
	} else
		websWrite(wp, "%s", live_translate("share.auto"));
}

void ej_get_curchannel(webs_t wp, int argc, char_t ** argv)
{
	char *prefix = nvram_safe_get("wifi_display");
	int channel = wifi_getchannel(prefix);

	if (channel > 0 && channel < 1000) {
		struct wifi_interface *interface = wifi_getfreq(prefix);
		if (!interface) {
			websWrite(wp, "%s", live_translate("share.unknown"));
			return;
		}

		int freq = get_wififreq(prefix, interface->freq);	// translation for special frequency devices
#ifdef HAVE_ATH9K
		if (is_ath9k(prefix)) {
			websWrite(wp, "%d", ieee80211_mhz2ieee(interface->freq));
			if (interface->center1 != -1 && interface->center1 != interface->freq)
				websWrite(wp, " + %d", ieee80211_mhz2ieee(interface->center1));
			if (interface->center2 != -1 && interface->center1 != interface->freq)
				websWrite(wp, " + %d", ieee80211_mhz2ieee(interface->center2));
		} else
#endif
		{
			websWrite(wp, "%d", channel);
		}
		free(interface);
		websWrite(wp, " (%d MHz)", freq);

	} else
		websWrite(wp, "%s", live_translate("share.unknown"));
	return;
}

void ej_active_wds(webs_t wp, int argc, char_t ** argv)
{
}

#endif
