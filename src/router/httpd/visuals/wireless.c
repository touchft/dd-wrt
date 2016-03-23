/*
 * wireless.c
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
#define VISUALSOURCE 1

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
 * $Id: wireless.c,v 1.6 2005/11/30 11:53:42 seg Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef __UCLIBC__
#include <error.h>
#endif
#include <signal.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <broadcom.h>
#include <wlioctl.h>
#include <wlutils.h>

#define ASSOCLIST_TMP	"/tmp/.wl_assoclist"
#define ASSOCLIST_CMD	"assoclist"

#define LEASES_NAME_IP	"/tmp/.leases_name_ip"

#undef ABURN_WSEC_CHECK

/*
 * WEP Format: wl_wep_buf=111:371A82447F:FBEA2AB7D4:1C9D814E6C:B4695172B4:2
 * (only for UI read)
 * wl_wep_gen=111:371A82447F:FBEA2AB7D4:1C9D814E6C:B4695172B4:2 (only for UI
 * read) wl_key=2 wl_key1=371A82447F wl_key2=FBEA2AB7D4 wl_key3=1C9D814E6C
 * wl_key4=B4695172B4 wl_passphrase=111 (only for UI) wl_wep_bit=64 (only for 
 * UI)
 * 
 */

struct lease_table {
	unsigned char hostname[32];
	char ipaddr[20];
	char hwaddr[20];
} *dhcp_lease_table;

static char *wl_filter_mac_get(char *ifname2, char *type, int which, char *word)
{
	char *wordlist, *next;
	int temp;
	char ifname[32];
	strcpy(ifname, ifname2);
	rep(ifname, 'X', '.');
#ifdef HAVE_SPOTPASS
	int wildcard, count;
	char mac[18];
#endif

	char var[32];
	if (!strcmp(nvram_safe_get("wl_active_add_mac"), "1")) {
		sprintf(var, "%s_active_mac", ifname2);
	} else {
		sprintf(var, "%s_maclist", ifname);
	}
	wordlist = nvram_safe_get(var);

	temp = which;

	foreach(word, wordlist, next) {
		if (which-- == 0) {
#ifdef HAVE_SPOTPASS
			//rep( word, '/', ' ');
			if (sscanf(word, "%17s/%i", &mac, &wildcard) != 0) {
				wildcard = wildcard / 4;
				for (count = strlen(mac) - 1; wildcard > 0; count--) {
					if (mac[count] != ':') {
						mac[count] = '*';
						wildcard--;
					}
				}
				return mac;

			} else
#endif
				return word;
		}
	}
	return "";

}

void ej_wireless_filter_table(webs_t wp, int argc, char_t ** argv)
{
	int i;
	char *type;
	char *ifname;
	char ifname2[32];
	int item;

#if LANGUAGE == JAPANESE
#define BOX_LEN 20
#else
#define BOX_LEN 17
#endif

	char *mac_mess = "MAC";

	ejArgs(argc, argv, "%s %s", &type, &ifname);

	char var[32];
	char *wordlist;

	if (!strcmp(nvram_safe_get("wl_active_add_mac"), "1")) {
		strcpy(ifname2, ifname);
		rep(ifname2, 'X', '.');
		sprintf(var, "%s_active_mac", ifname);
		wordlist = nvram_safe_get(var);
		sprintf(var, "%s_maclist", ifname2);
		nvram_set(var, wordlist);
	}

	if (!strcmp(type, "input")) {
		char word[50];
		websWrite(wp, "<div class=\"col2l\">\n");
		websWrite(wp, "<fieldset><legend>Table 1</legend>\n");
		for (i = 0; i < WL_FILTER_MAC_NUM / 2; i++) {

			item = 0 * WL_FILTER_MAC_NUM + i + 1;

			websWrite(wp,
#ifdef HAVE_SPOTPASS
				  "<div class=\"setting\"><div class=\"label\" style=\"width: 16%%\">%s %03d : </div><input maxlength=\"17\" style=\"float: left; width: 30%%;\" onblur=\"\" size=%d name=\"%s_mac%d\" value=\"%s\"/>",
#else
				  "<div class=\"setting\"><div class=\"label\" style=\"width: 16%%\">%s %03d : </div><input maxlength=\"17\" style=\"float: left; width: 30%%;\" onblur=\"valid_macs_all(this)\" size=%d name=\"%s_mac%d\" value=\"%s\"/>",
#endif
				  mac_mess, item, BOX_LEN, ifname, item - 1, wl_filter_mac_get(ifname, "mac", item - 1, word));

			websWrite(wp,
#ifdef HAVE_SPOTPASS
				  "<div class=\"label\" style=\"width: 16%%; margin-left: 7px;\">%s %03d : </div><input style=\"width: 30%%;\" maxlength=\"17\" onblur=\"\" size=%d name=\"%s_mac%d\" value=\"%s\"/></div>\n",
#else
				  "<div class=\"label\" style=\"width: 16%%; margin-left: 7px;\">%s %03d : </div><input style=\"width: 30%%;\" maxlength=\"17\" onblur=\"valid_macs_all(this)\" size=%d name=\"%s_mac%d\" value=\"%s\"/></div>\n",
#endif
				  mac_mess, item + (WL_FILTER_MAC_NUM / 2), BOX_LEN, ifname, item + (WL_FILTER_MAC_NUM / 2) - 1, wl_filter_mac_get(ifname, "mac", item + (WL_FILTER_MAC_NUM / 2) - 1, word));

		}

		websWrite(wp, "</fieldset></div>\n");
		websWrite(wp, "<div class=\"col2r\">\n");
		websWrite(wp, "<fieldset><legend>Table 2</legend>\n");

		for (i = 0; i < WL_FILTER_MAC_NUM / 2; i++) {

			item = 1 * WL_FILTER_MAC_NUM + i + 1;

			websWrite(wp,
				  "<div class=\"setting\"><div class=\"label\" style=\"width: 16%%\">%s %03d : </div><input maxlength=\"17\" style=\"float: left; width: 30%%;\" onblur=\"valid_macs_all(this)\" size=%d name=\"%s_mac%d\" value=\"%s\"/>",
				  mac_mess, item, BOX_LEN, ifname, item - 1, wl_filter_mac_get(ifname, "mac", item - 1, word));

			websWrite(wp,
				  "<div class=\"label\" style=\"width: 16%%; margin-left: 7px;\">%s %03d : </div><input style=\"width: 30%%;\" maxlength=\"17\" onblur=\"valid_macs_all(this)\" size=%d name=\"%s_mac%d\" value=\"%s\"/></div>\n",
				  mac_mess, item + (WL_FILTER_MAC_NUM / 2), BOX_LEN, ifname, item + (WL_FILTER_MAC_NUM / 2) - 1, wl_filter_mac_get(ifname, "mac", item + (WL_FILTER_MAC_NUM / 2) - 1, word));

		}

		websWrite(wp, "</fieldset>\n");
		websWrite(wp, "</div><br clear=\"all\" /><br />\n");

	}
	// cprintf("%s():set wl_active_add_mac = 0\n",__FUNCTION__);
	nvram_set("wl_active_add_mac", "0");
	return;
}

int dhcp_lease_table_init(void)
{
	FILE *fp, *fp_w;
	int count = 0;

	if (nvram_match("dhcp_dnsmasq", "1")) {
		unsigned long expires;
		char mac[32];
		char ip[32];
		char hostname[256];
		char buf[512];
		char *p;

		killall("dnsmasq", SIGUSR2);
		sleep(1);

		if ((fp_w = fopen(LEASES_NAME_IP, "w")) != NULL) {
			// Parse leases file
			if ((fp = fopen("/tmp/dnsmasq.leases", "r")) != NULL) {
				while (fgets(buf, sizeof(buf), fp)) {
					if (sscanf(buf, "%lu %17s %15s %255s", &expires, mac, ip, hostname) != 4)
						continue;
					p = mac;
					while ((*p = toupper(*p)) != 0)
						++p;
					fprintf(fp_w, "%s %s %s\n", mac, ip, hostname);
					++count;
				}
				fclose(fp);
			}
			fclose(fp_w);
		}
	} else {
		struct lease_t lease;
		struct in_addr addr;
		char mac[20] = "";

		killall("udhcpd", SIGUSR1);
		fp_w = fopen(LEASES_NAME_IP, "w");

		// Parse leases file 
		if ((fp = fopen("/tmp/udhcpd.leases", "r"))) {
			while (fread(&lease, sizeof(lease), 1, fp)) {
				snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X", lease.chaddr[0], lease.chaddr[1], lease.chaddr[2], lease.chaddr[3], lease.chaddr[4], lease.chaddr[5]);
				if (!strcmp("00:00:00:00:00:00", mac))
					continue;
				addr.s_addr = lease.yiaddr;
				char client[32];
				char *peer = (char *)inet_ntop(AF_INET, &addr, client,
							       16);
				fprintf(fp_w, "%s %s %s\n", mac, peer, lease.hostname);
				count++;
			}
			fclose(fp);
		}
		fclose(fp_w);
	}

	return count;
}

extern struct wl_client_mac *wl_client_macs;

void get_hostname_ip(char *type, char *filename)
{
	FILE *fp;
	char line[80];
	char leases[3][50];
	int i;

	if ((fp = fopen(filename, "r"))) {	// find out hostname and ip
		while (fgets(line, sizeof(line), fp) != NULL) {
			strcpy(leases[0], "");
			strcpy(leases[1], "");
			strcpy(leases[2], "");
			// 00:11:22:33:44:55 192.168.1.100 honor
			if (sscanf(line, "%s %s %s", leases[0], leases[1], leases[2]) != 3)
				continue;
			for (i = 0; i < MAX_LEASES; i++) {
				if (!strcmp(leases[0], wl_client_macs[i].hwaddr)) {
					snprintf(wl_client_macs[i].ipaddr, sizeof(wl_client_macs[i].ipaddr), "%s", leases[1]);
					snprintf(wl_client_macs[i].hostname, sizeof(wl_client_macs[i].hostname), "%s", leases[2]);
					break;
				}
			}
		}
	}
	fclose(fp);
}

int nv_count = 0;
void save_hostname_ip(void)
{
	FILE *fp, *fp_w;
	char line[80];
	char leases[3][50];
	int i = 0, j = 0, count;
	int match = 0;
	struct wl_client {
		char hostname[32];
		char ipaddr[20];
		char hwaddr[20];
	} wl_clients[MAX_LEASES];

	for (i = 0; i < MAX_LEASES; i++) {	// init value
		strcpy(wl_clients[i].hostname, "");
		strcpy(wl_clients[i].ipaddr, "");
		strcpy(wl_clients[i].hwaddr, "");
	}
	i = 0;
	if ((fp = fopen(OLD_NAME_IP, "r"))) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			// 00:11:22:33:44:55 192.168.1.100 honor
			strcpy(leases[0], "");
			strcpy(leases[1], "");
			strcpy(leases[2], "");
			if (sscanf(line, "%s %s %s", leases[0], leases[1], leases[2]) != 3)
				continue;
			snprintf(wl_clients[i].hwaddr, sizeof(wl_clients[i].hwaddr), "%s", leases[0]);
			snprintf(wl_clients[i].ipaddr, sizeof(wl_clients[i].ipaddr), "%s", leases[1]);
			snprintf(wl_clients[i].hostname, sizeof(wl_clients[i].hostname), "%s", leases[2]);
			i++;
		}
		fclose(fp);

	}
	count = i;
	for (i = 0; i < nv_count; i++) {	// init value
		if (wl_client_macs[i].status == 1 && strcmp(wl_client_macs[i].ipaddr, "")) {	// online && have ip address
			for (j = 0; j < MAX_LEASES; j++) {
				match = 0;
				if (!strcmp(wl_clients[j].hwaddr, wl_client_macs[i].hwaddr)) {
					snprintf(wl_clients[j].ipaddr, sizeof(wl_clients[j].ipaddr), "%s", wl_client_macs[i].ipaddr);
					snprintf(wl_clients[j].hostname, sizeof(wl_clients[j].hostname), "%s", wl_client_macs[i].hostname);
					match = 1;
					break;
				}

			}
			if (match == 0) {
				snprintf(wl_clients[count].hwaddr, sizeof(wl_clients[i].hwaddr), "%s", wl_client_macs[i].hwaddr);
				snprintf(wl_clients[count].ipaddr, sizeof(wl_clients[i].ipaddr), "%s", wl_client_macs[i].ipaddr);
				snprintf(wl_clients[count].hostname, sizeof(wl_clients[i].hostname), "%s", wl_client_macs[i].hostname);
				count++;
			}
		}
	}

	if ((fp_w = fopen(OLD_NAME_IP, "w"))) {
		for (i = 0; i < MAX_LEASES; i++) {
			if (strcmp(wl_clients[i].hwaddr, ""))
				fprintf(fp_w, "%s %s %s\n", wl_clients[i].hwaddr, wl_clients[i].ipaddr, wl_clients[i].hostname);
		}
		fclose(fp_w);
	}

}

void ej_wireless_active_table(webs_t wp, int argc, char_t ** argv)
{
	int i, flag = 0;
	char word[256], *next;
	FILE *fp;
	char list[2][20];
	char line[80];
	int dhcp_table_count;
	char *type, *ifname2;
	char ifname[32];
	ejArgs(argc, argv, "%s %s", &type, &ifname2);
	strcpy(ifname, ifname2);
	rep(ifname, 'X', '.');
	if (!strcmp(type, "online")) {
		for (i = 0; i < MAX_LEASES; i++) {	// init value
			strcpy(wl_client_macs[i].hostname, "");
			strcpy(wl_client_macs[i].ipaddr, "");
			strcpy(wl_client_macs[i].hwaddr, "");
			wl_client_macs[i].status = -1;
			wl_client_macs[i].check = 0;
		}

		nv_count = 0;	// init mac list

		char var[32];
		if (!strcmp(nvram_safe_get("wl_active_add_mac"), "1")) {
			sprintf(var, "%s_active_mac", ifname);
		} else {
			sprintf(var, "%s_maclist", ifname);
		}
		char *maclist = nvram_safe_get(var);

		foreach(word, maclist, next) {
			snprintf(wl_client_macs[nv_count].hwaddr, sizeof(wl_client_macs[nv_count].hwaddr), "%s", word);
			wl_client_macs[nv_count].status = 0;	// offline (default)
			wl_client_macs[nv_count].check = 1;	// checked
			nv_count++;
		}

		char *iface;
/*#ifdef HAVE_ATH9K
		iface = ifname;
		sysprintf("wl_atheros -i %s %s > %s", iface,
			  ASSOCLIST_CMD, ASSOCLIST_TMP);
#elif HAVE_MADWIFI*/
#if defined(HAVE_MADWIFI) || defined(HAVE_ATH9K)
		iface = ifname;
		sysprintf("wl_atheros -i %s %s > %s", iface, ASSOCLIST_CMD, ASSOCLIST_TMP);
#elif HAVE_RT2880
		sysprintf("wl_rt2880 -i %s %s > %s", ifname, ASSOCLIST_CMD, ASSOCLIST_TMP);
#else
		if (!strcmp(ifname, "wl0"))
			iface = get_wl_instance_name(0);
		else if (!strcmp(ifname, "wl1"))
			iface = get_wl_instance_name(1);
		else if (!strcmp(ifname, "wl2"))
			iface = get_wl_instance_name(2);
		else
			iface = nvram_safe_get("wl0_ifname");
		sysprintf("wl -i %s %s > %s", iface, ASSOCLIST_CMD, ASSOCLIST_TMP);
#endif

		if ((fp = fopen(ASSOCLIST_TMP, "r"))) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				int match = 0;

				strcpy(list[0], "");
				strcpy(list[1], "");
				if (sscanf(line, "%s %s", list[0], list[1]) != 2)	// assoclist 
					// 00:11:22:33:44:55
					continue;
				if (strcmp(list[0], "assoclist"))
					continue;
				for (i = 0; i < nv_count; i++) {
					if (!strcmp(wl_client_macs[i].hwaddr, list[1])) {
						wl_client_macs[i].status = 1;	// online
						wl_client_macs[i].check = 1;	// checked
						match = 1;

						break;
					}
				}
				if (match == 0) {
					snprintf(wl_client_macs[nv_count].hwaddr, sizeof(wl_client_macs[nv_count].hwaddr), "%s", list[1]);
					wl_client_macs[nv_count].status = 1;	// online
					wl_client_macs[nv_count].check = 0;	// no checked
					nv_count++;
				}
			}
			fclose(fp);
		}
		if (!strcmp(type, "online")) {
			dhcp_table_count = dhcp_lease_table_init();	// init dhcp
			// lease
			// table and
			// get count
			get_hostname_ip("online", LEASES_NAME_IP);
		}
		save_hostname_ip();
	}

	if (!strcmp(type, "offline")) {
		get_hostname_ip("offline", OLD_NAME_IP);
	}

	if (!strcmp(type, "online")) {
		for (i = 0; i < nv_count; i++) {
			if (wl_client_macs[i].status != 1)
				continue;
			websWrite(wp, "<tr align=\"middle\"> \n"
				  "<td height=\"20\" width=\"167\">%s</td> \n"
				  "<td height=\"20\" width=\"140\">%s</td> \n"
				  "<td height=\"20\" width=\"156\">%s</td> \n"
				  "<td height=\"20\" width=\"141\"><input type=\"checkbox\" name=\"on%d\" value=\"%d\" %s></td> \n"
				  "</tr>\n", wl_client_macs[i].hostname, wl_client_macs[i].ipaddr, wl_client_macs[i].hwaddr, flag++, i, wl_client_macs[i].check ? "checked=\"checked\"" : "");
		}
	} else if (!strcmp(type, "offline")) {
		for (i = 0; i < nv_count; i++) {
			if (wl_client_macs[i].status != 0)
				continue;
			websWrite(wp, "<tr align=\"middle\"> \n"
				  "<td height=\"20\" width=\"167\">%s</td> \n"
				  "<td height=\"20\" width=\"140\">%s</td> \n"
				  "<td height=\"20\" width=\"156\">%s</td> \n"
				  "<td height=\"20\" width=\"141\"><input type=\"checkbox\" name=\"off%d\" value=\"%d\" %s></td> \n"
				  "</tr>\n", wl_client_macs[i].hostname, wl_client_macs[i].ipaddr, wl_client_macs[i].hwaddr, flag++, i, wl_client_macs[i].check ? "checked=\"checked\"" : "");

		}
	}
	// if(dhcp_lease_table) free(dhcp_lease_table);
	return;
}

/*
 * char * get_wep_value (char *type, char *_bit, char *prefix) { static char
 * word[200]; char *wordlist; char wl_wep[] = "wlX.XX_wep_XXXXXX"; char
 * *wl_passphrase, *wl_key1, *wl_key2, *wl_key3, *wl_key4, *wl_key_tx;
 * 
 * if (generate_key) { snprintf (wl_wep, sizeof (wl_wep), "%s_wep_gen",
 * prefix); } else { snprintf (wl_wep, sizeof (wl_wep), "%s_wep_buf",
 * prefix); }
 * 
 * cprintf ("get %s from %s with bit %s and prefix %s\n", type, wl_wep, _bit,
 * prefix);
 * 
 * wordlist = nvram_safe_get (wl_wep);
 * 
 * if (!strcmp (wordlist, "")) return "";
 * 
 * cprintf ("wordlist = %s\n", wordlist);
 * 
 * strcpy(word, wordlist);
 * 
 * wl_key1 = word; wl_passphrase = strsep (&wl_key1, ":");
 * 
 * wl_key2 = wl_key1; wl_key1 = strsep (&wl_key2, ":");
 * 
 * wl_key3 = wl_key2; wl_key2 = strsep (&wl_key3, ":");
 * 
 * wl_key4 = wl_key3; wl_key3 = strsep (&wl_key4, ":");
 * 
 * wl_key_tx = wl_key4; wl_key4 = strsep (&wl_key_tx, ":");
 * 
 * 
 * cprintf ("key1 = %s\n", wl_key1); cprintf ("key2 = %s\n", wl_key2); cprintf 
 * ("key3 = %s\n", wl_key3); cprintf ("key4 = %s\n", wl_key4); cprintf ("pass
 * = %s\n", wl_passphrase);
 * 
 * 
 * if (!strcmp (type, "passphrase")) { return wl_passphrase; } else if
 * (!strcmp (type, "key1")) { return wl_key1; } else if (!strcmp (type,
 * "key2")) { return wl_key2; } else if (!strcmp (type, "key3")) { return
 * wl_key3; } else if (!strcmp (type, "key4")) { return wl_key4; } else if
 * (!strcmp (type, "tx")) { return wl_key_tx; }
 * 
 * return ""; } 
 */
char *get_wep_value(char *temp, char *type, char *_bit, char *prefix)
{

	int cnt;
	char *wordlist;
	char wl_wep[] = "wlX.XX_wep_XXXXXX";

	if (nvram_match("generate_key", "1")) {
		snprintf(wl_wep, sizeof(wl_wep), "%s_wep_gen", prefix);
	} else {
		snprintf(wl_wep, sizeof(wl_wep), "%s_wep_buf", prefix);
	}

	wordlist = nvram_safe_get(wl_wep);

	if (!strcmp(wordlist, ""))
		return "";

	cnt = count_occurences(wordlist, ':');

	if (!strcmp(type, "passphrase")) {
		if (wordlist[0] == ':')
			return "";
		substring(0, pos_nthoccurence(wordlist, ':', cnt - 4), wordlist, temp);
		return temp;
	} else if (!strcmp(type, "key1")) {
		substring(pos_nthoccurence(wordlist, ':', cnt - 4) + 1, pos_nthoccurence(wordlist, ':', cnt - 3), wordlist, temp);
		return temp;
	} else if (!strcmp(type, "key2")) {
		substring(pos_nthoccurence(wordlist, ':', cnt - 3) + 1, pos_nthoccurence(wordlist, ':', cnt - 2), wordlist, temp);
		return temp;
	} else if (!strcmp(type, "key3")) {
		substring(pos_nthoccurence(wordlist, ':', cnt - 2) + 1, pos_nthoccurence(wordlist, ':', cnt - 1), wordlist, temp);
		return temp;
	} else if (!strcmp(type, "key4")) {
		substring(pos_nthoccurence(wordlist, ':', cnt - 1) + 1, pos_nthoccurence(wordlist, ':', cnt), wordlist, temp);
		return temp;
	} else if (!strcmp(type, "tx")) {
		substring(pos_nthoccurence(wordlist, ':', cnt) + 1, strlen(wordlist), wordlist, temp);
		return temp;
	}

	return "";
}

void ej_get_wep_value(webs_t wp, int argc, char_t ** argv)
{
	char *type, *bit;
	char *value = "", new_value[50] = "";
	char temp[256];
	ejArgs(argc, argv, "%s", &type);
	cprintf("get wep value %s\n", type);
#ifdef HAVE_MADWIFI
	bit = GOZILA_GET(wp, "ath0_wep_bit");
	if (bit == NULL)
		bit = nvram_safe_get("ath0_wep_bit");

	value = get_wep_value(temp, type, bit, "ath0");
#else
	bit = GOZILA_GET(wp, "wl_wep_bit");
	if (bit == NULL)
		bit = nvram_safe_get("ath0_wep_bit");
	cprintf("bit = %s\n", bit);
	value = get_wep_value(temp, type, bit, "wl");
#endif
	cprintf("value = %s\n", value);
	httpd_filter_name(value, new_value, sizeof(new_value), GET);
	cprintf("newvalue = %s\n", new_value);

	websWrite(wp, "%s", new_value);
}

void ej_show_wl_wep_setting(webs_t wp, int argc, char_t ** argv)
{

	/*
	 * char *type;
	 * 
	 * type = gozila_action ? websGetVar(wp, "wl_wep_bit", NULL) :
	 * nvram_safe_get("wl_wep_bit");
	 * 
	 * ret += websWrite(wp," \ <TR> \n\ <TH align=right width=150
	 * bgColor=#6666cc height=25>&nbsp;Passphrase:&nbsp;&nbsp;</TH> \n\ <TD
	 * align=left width=435 height=25>&nbsp;\n\ <INPUT maxLength=16
	 * name=wl_passphrase size=20 value='%s'>&nbsp; \n\ <INPUT type=hidden
	 * value=Null name=generateButton> \n\ <INPUT type=button
	 * value='Generate' onclick=generateKey(this.form)
	 * name=wepGenerate></TD></TR>\n",get_wep_value("passphrase",type));
	 * 
	 * ret += websWrite(wp," \ <TR> \n\ <TH vAlign=center align=right
	 * width=150 bgColor=#6666cc height=25>Key 1:&nbsp;&nbsp;</TH> \n\ <TD
	 * vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;\n\ <INPUT
	 * size=35 name=wl_key1
	 * value='%s'></TD></TR>\n",get_wep_value("key1",type));
	 * 
	 * ret += websWrite(wp," \ <TR> \n\ <TH vAlign=center align=right
	 * width=150 bgColor=#6666cc height=25>Key 2:&nbsp;&nbsp;</TH> \n\ <TD
	 * vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;\n\ <INPUT
	 * size=35 name=wl_key2
	 * value='%s'></TD></TR>\n",get_wep_value("key2",type)); ret +=
	 * websWrite(wp," \ <TR> \n\ <TH vAlign=center align=right width=150
	 * bgColor=#6666cc height=25>Key 3:&nbsp;&nbsp;</TH> \n\ <TD
	 * vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;\n\ <INPUT
	 * size=35 name=wl_key3
	 * value='%s'></TD></TR>\n",get_wep_value("key3",type)); ret +=
	 * websWrite(wp," \ <TR> \n\ <TH vAlign=center align=right width=150
	 * bgColor=#6666cc height=25>Key 4:&nbsp;&nbsp;</TH> \n\ <TD
	 * vAlign=bottom width=435 bgColor=#ffffff height=25>&nbsp;\n\ <INPUT
	 * size=35 name=wl_key4
	 * value='%s'></TD></TR>\n",get_wep_value("key4",type)); 
	 */
	return;
}

void wl_active_onload(webs_t wp, char *arg)
{

	if (!strcmp(nvram_safe_get("wl_active_add_mac"), "1")) {
		websWrite(wp, arg);
	}

}

// only for nonbrand
void ej_get_wl_active_mac(webs_t wp, int argc, char_t ** argv)
{
	char line[80];
	char list[2][20];
	FILE *fp;
	int count = 0;

	sysprintf("wl %s > %s", ASSOCLIST_CMD, ASSOCLIST_TMP);

	if ((fp = fopen(ASSOCLIST_TMP, "r"))) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			strcpy(list[0], "");
			strcpy(list[1], "");
			if (sscanf(line, "%s %s", list[0], list[1]) != 2)	// assoclist 
				// 00:11:22:33:44:55
				continue;
			if (strcmp(list[0], "assoclist"))
				continue;
			websWrite(wp, "%c'%s'", count ? ',' : ' ', list[1]);
			count++;
		}
	}

	return;
}

void ej_get_wl_value(webs_t wp, int argc, char_t ** argv)
{
	char *type;

	ejArgs(argc, argv, "%s", &type);
	if (!strcmp(type, "default_dtim")) {
		websWrite(wp, "1");	// This is a best value for 11b test
	} else if (!strcmp(type, "wl_afterburner_override")) {
		FILE *fp;
		char line[254];

		if ((fp = popen("wl afterburner_override", "r"))) {
			fgets(line, sizeof(line), fp);
			websWrite(wp, "%s", chomp(line));
			pclose(fp);
		}
	}
	return;

}

void ej_show_wpa_setting(webs_t wp, int argc, char_t ** argv, char *prefix)
{
	char *type, *security_mode;
	char var[80];

	sprintf(var, "%s_security_mode", prefix);
	cprintf("show wpa setting\n");
	ejArgs(argc, argv, "%s", &type);
	rep(var, '.', 'X');
	security_mode = GOZILA_GET(wp, var);
	if (security_mode == NULL)
		security_mode = nvram_safe_get(var);
	rep(var, 'X', '.');
	cprintf("security mode %s = %s\n", security_mode, var);
	if (!strcmp(security_mode, "psk")
	    || !strcmp(security_mode, "psk2")
	    || !strcmp(security_mode, "psk psk2"))
		show_preshared(wp, prefix);
#if UI_STYLE != CISCO
	else if (!strcmp(security_mode, "disabled"))
		show_preshared(wp, prefix);
#endif
	else if (!strcmp(security_mode, "radius"))
		show_radius(wp, prefix, 1, 0);
	else if (!strcmp(security_mode, "wpa")
		 || !strcmp(security_mode, "wpa2")
		 || !strcmp(security_mode, "wpa wpa2"))
		show_wparadius(wp, prefix);
	else if (!strcmp(security_mode, "wep"))
		show_wep(wp, prefix);
#ifdef HAVE_WPA_SUPPLICANT
#ifndef HAVE_MICRO
	else if (!strcmp(security_mode, "8021X"))
		show_80211X(wp, prefix);
#endif
#endif
	return;
}

#if !defined(HAVE_MADWIFI) && !defined(HAVE_RT2880)
void ej_wl_ioctl(webs_t wp, int argc, char_t ** argv)
{
	int unit, val;
	char tmp[100], prefix[] = "wlXXXXXXXXXX_";
	char *op, *type, *var;
	char *name;

	ejArgs(argc, argv, "%s %s %s", &op, &type, &var);

	if ((unit = atoi(nvram_safe_get("wl_unit"))) < 0)
		return;

	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	name = nvram_safe_get(strcat_r(prefix, "ifname", tmp));

	if (strcmp(op, "get") == 0) {
		if (strcmp(type, "int") == 0) {
			websWrite(wp, "%u", wl_iovar_getint(name, var, &val) == 0 ? val : 0);
			return;
		}
	}
	return;
}
#endif
void ej_wme_match_op(webs_t wp, int argc, char_t ** argv)
{
	char word[256], *next;

	char *list = nvram_safe_get(argv[0]);

	foreach(word, list, next) {
		if (!strcmp(word, argv[1])) {
			websWrite(wp, argv[2]);
			return;
		}
	}

	return;
}

void ej_show_wireless_advanced(webs_t wp, int argc, char_t ** argv)
{
#ifdef HAVE_MADWIFI
	int showrate = 1;
#if defined(HAVE_MADWIFI_MIMO) || defined(HAVE_ATH9K)
	showrate = 0;
	char ifname[32];
	int i;
	for (i = 0; i < 16; i++) {
		sprintf(ifname, "ath%d", i);
		if (ifexists(ifname)) {
			if (!is_ath11n(ifname))
				showrate = 1;
		} else
			break;
	}
#endif

	if (showrate) {
		websWrite(wp, "<h2><script type=\"text/javascript\">Capture(wl_basic.advanced_options);</script></h2>\n");
		websWrite(wp, "<fieldset>\n");
		websWrite(wp, "      <legend><script type=\"text/javascript\">Capture(wl_basic.rate_control);</script></legend>\n");
		websWrite(wp, "     	<div class=\"setting\">\n");
		websWrite(wp, "           <div class=\"label\"><script type=\"text/javascript\">Capture(wl_basic.rate_control);</script></div>\n");
		websWrite(wp, "             <select name=\"rate_control\">\n");
		websWrite(wp, "<script type=\"text/javascript\">\n//<![CDATA[\n");
		websWrite(wp, "document.write(\"<option value=\\\"minstrel\\\" %s >Minstrel EWMA</option>\");\n", nvram_match("rate_control", "minstrel") ? "selected=\\\"selected\\\"" : "");
		websWrite(wp, "document.write(\"<option value=\\\"sample\\\" %s >Sample</option>\");\n", nvram_match("rate_control", "sample") ? "selected=\\\"selected\\\"" : "");
		websWrite(wp, "//]]>\n</script>\n</select>\n");
		websWrite(wp, "</div>\n");
		websWrite(wp, "</fieldset>\n");
		websWrite(wp, "<br />\n");
	}
#endif
}
