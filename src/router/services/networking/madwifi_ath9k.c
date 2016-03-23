/*
 * madwifi_ath9k.c 
 * Copyright (C) 2010 Christian Scheele <chris@dd-wrt.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_ATH9K
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <err.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <bcmnvram.h>
#include <wlutils.h>
#include <bcmutils.h>
#include <shutils.h>
#include <utils.h>
#include <unistd.h>
#include <glob.h>

#include <services.h>
void setupHostAP_ath9k(char *maininterface, int isfirst, int vapid, int aoss);
static void setupSupplicant_ath9k(char *prefix, char *ssidoverride);
void setupHostAP_generic_ath9k(char *prefix, FILE * fp, int isrepeater, int aoss);

void delete_ath9k_devices(char *physical_iface)
{
	glob_t globbuf;
	char globstring[1024];
	int globresult;
	if (physical_iface)
		sprintf(globstring, "/sys/class/ieee80211/%s/device/net/*", physical_iface);
	else
		sprintf(globstring, "/sys/class/ieee80211/phy*/device/net/*");
	globresult = glob(globstring, GLOB_NOSORT, NULL, &globbuf);
	int i;
	for (i = 0; i < globbuf.gl_pathc; i++) {
		char *ifname;
		ifname = strrchr(globbuf.gl_pathv[i], '/');
		if (!ifname)
			continue;
		eval("ifconfig", ifname + 1, "down");
		eval("iw", ifname + 1, "del");
	}
}

void deconfigure_single_ath9k(int count)
{
	fprintf(stderr, "ath9k deconfigure_single: phy%d ath%d\n", get_ath9k_phy_idx(count), count);
	char wif[10];
	sprintf(wif, "phy%d", get_ath9k_phy_idx(count));
	delete_ath9k_devices(wif);
}

void configure_single_ath9k(int count)
{
	char *next;
	char var[80];
	char mode[80];
	int cnt = 0;
	char dev[10];
	char wif[10];
	int phy_idx = get_ath9k_phy_idx(count);
	char mtikie[32];
	char wl[16];
	char channel[16];
	char ssid[16];
	char net[16];
	char wifivifs[16];
	char broadcast[16];
	char sens[32];
	char basedev[16];
	char diversity[32];
	char athmac[16];
	char wl_poll[32];
	char rxantenna[32];
	char txantenna[32];
	static int vapcount = 0;
	int isath5k = 0;
	char *apm;
	char isolate[32];
	char primary[32] = { 0 };
	char regdomain[16];
	char *country;
	sprintf(dev, "ath%d", count);
	isath5k = is_ath5k(dev);
	// sprintf(regdomain, "%s_regdomain", dev);
	// country = nvram_default_get(regdomain, "US");
	// sysprintf("iw reg set %s", getIsoName(country));
	// // sleep(3);
	if (count == 0)
		vapcount = 0;
	sprintf(wif, "phy%d", phy_idx);
	sprintf(wifivifs, "ath%d_vifs", count);
	fprintf(stderr, "ath9k configure_single: phy%d ath%d\n", phy_idx, count);
	sprintf(channel, "ath%d_channel", count);
	sprintf(sens, "ath%d_distance", count);
	sprintf(diversity, "ath%d_diversity", count);
	sprintf(athmac, "ath%d_hwaddr", count);
	sprintf(rxantenna, "ath%d_rxantenna", count);
	sprintf(txantenna, "ath%d_txantenna", count);
	// create base device
	cprintf("configure base interface %d\n", count);
	sprintf(net, "%s_net_mode", dev);
	char *netmode = nvram_default_get(net, "mixed");
	if (!strcmp(netmode, "disabled"))
		return;
	char bw[32];

	// set channelbw ht40 is also 20!

	sprintf(bw, "%s_channelbw", dev);
	if (isath5k) {
		if (nvram_match(bw, "5"))
			sysprintf("echo 5 > /sys/kernel/debug/ieee80211/%s/ath5k/bwmode", wif);
		else if (nvram_match(bw, "10"))
			sysprintf("echo 10 > /sys/kernel/debug/ieee80211/%s/ath5k/bwmode", wif);
		else if (nvram_match(bw, "40"))
			sysprintf("echo 40 > /sys/kernel/debug/ieee80211/%s/ath5k/bwmode", wif);
		else
			sysprintf("echo 20 > /sys/kernel/debug/ieee80211/%s/ath5k/bwmode", wif);
	} else {
		if (nvram_match(bw, "5"))
			sysprintf("echo 5 > /sys/kernel/debug/ieee80211/%s/ath9k/chanbw", wif);
		else if (nvram_match(bw, "10"))
			sysprintf("echo 10 > /sys/kernel/debug/ieee80211/%s/ath9k/chanbw", wif);
		else
			sysprintf("echo 20 > /sys/kernel/debug/ieee80211/%s/ath9k/chanbw", wif);
	}
	char wl_intmit[32];

	sprintf(wl_intmit, "%s_intmit", dev);
	sysprintf("echo %s > /sys/kernel/debug/ieee80211/%s/ath9k/ani", nvram_default_get(wl_intmit, "1"), wif);
#ifdef HAVE_REGISTER
	int cpeonly = iscpe();
#else
	int cpeonly = 0;
#endif
	int maxrxchain = mac80211_get_avail_rx_antenna(phy_idx);
	int maxtxchain = mac80211_get_avail_tx_antenna(phy_idx);
	int txchain;
	int rxchain;
	if (strlen(nvram_safe_get(rxantenna)) == 0 || strlen(nvram_safe_get(txantenna)) == 0) {
		txchain = maxtxchain;
		rxchain = maxrxchain;
	} else {
		txchain = atoi(nvram_safe_get(txantenna));
		rxchain = atoi(nvram_safe_get(rxantenna));
	}
	int reset = 0;
	if (txchain > maxtxchain) {
		reset = 1;
		txchain = maxtxchain;
	}
	if (rxchain > maxrxchain) {
		reset = 1;
		rxchain = maxrxchain;
	}

	char rxdefstr[32];
	char txdefstr[32];
	sprintf(txdefstr, "%d", rxchain);
	sprintf(rxdefstr, "%d", txchain);
	if (reset) {
		nvram_set(txantenna, txdefstr);
		nvram_set(rxantenna, rxdefstr);
	} else {
		nvram_default_get(txantenna, txdefstr);
		nvram_default_get(rxantenna, rxdefstr);
	}
	mac80211_set_antennas(phy_idx, txchain, rxchain);

	sprintf(wl, "ath%d_mode", count);
	apm = nvram_default_get(wl, "ap");

	if (!strcmp(apm, "ap") || !strcmp(apm, "wdsap") || !strcmp(apm, "sta")
	    || !strcmp(apm, "wet")) {

		eval("iw", wif, "interface", "add", dev, "type", "managed");

		strcpy(primary, dev);
	} else if (!strcmp(apm, "wdssta")) {
		eval("iw", wif, "interface", "add", dev, "type", "managed", "4addr", "on");

		strcpy(primary, dev);
	} else {
		eval("iw", wif, "interface", "add", dev, "type", "ibss");
		// still TBD ;-)
		// ifconfig ath0 up
		// iw dev ath0 ibss join AdHocNetworkName 2412
	}

	char macaddr[32];
	// interface is created at this point, so that should work
#if defined(HAVE_MVEBU) || defined(HAVE_IPQ806X)
	getWirelessMac(macaddr, count);
	eval("ifconfig", dev, "hw", "ether", macaddr);
#else
	getMacAddr(dev, macaddr);
#endif
	nvram_set(athmac, macaddr);
	int distance = atoi(nvram_default_get(sens, "2000"));	// to meter
	char dist[32];
	if (distance > 0)
		sprintf(dist, "%d", distance);
	else
		sprintf(dist, "auto");
	eval("iw", "phy", wif, "set", "distance", dist);
#ifdef HAVE_ATH10K
	if (is_ath10k(dev) && !is_mvebu(dev)) {	// evil hack for QCA 
		set_ath10kdistance(dev, distance);
	}
#endif
// das scheint noch aerger zu machen
	eval("iw", "dev", dev, "set", "power_save", "off");

	cprintf("done()\n");

	cprintf("setup encryption");
	// setup encryption
	int isfirst = 1;
	if (strcmp(apm, "sta") && strcmp(apm, "wdssta") && strcmp(apm, "wet")) {
		setupHostAP_ath9k(dev, isfirst, 0, 0);
		isfirst = 0;
	} else {
		char *clonename = "def_whwaddr";
		if (!strcmp(apm, "sta"))
			clonename = "def_hwaddr";
		if (nvram_match("mac_clone_enable", "1")
		    && nvram_invmatch(clonename, "00:00:00:00:00:00")
		    && nvram_invmatch(clonename, "")) {
			eval("ifconfig", dev, "hw", "ether", nvram_safe_get(clonename));
		}

		setupSupplicant_ath9k(dev, NULL);
	}
	char *vifs = nvram_safe_get(wifivifs);
	int countvaps = 1;
	foreach(var, vifs, next) {
		countvaps++;
	}
	if (countvaps < 4)
		countvaps = 4;
	if (countvaps > vapcount)
		vapcount = countvaps;
	int counter = 1;
	if (strlen(vifs) > 0)
		foreach(var, vifs, next) {
		fprintf(stderr, "setup vifs %s %d\n", var, counter);
		// create the first main hostapd interface when this is repeater mode
		if (isfirst)
			sysprintf("iw %s interface add %s.%d type managed", wif, dev, counter);
		setupHostAP_ath9k(dev, isfirst, counter, 0);
		isfirst = 0;
		counter++;
		}
}

void setupHostAP_generic_ath9k(char *prefix, FILE * fp, int isrepeater, int aoss)
{
	struct wifi_channels *chan;
	int channel = 0;
	int freq = 0;
	char nfreq[16];
	int i = 0;
	char *caps;
	int isath5k = is_ath5k(prefix);
	fprintf(fp, "driver=nl80211\n");
	fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
	fprintf(fp, "wmm_ac_bk_cwmin=4\n");
	fprintf(fp, "wmm_ac_bk_cwmax=10\n");
	fprintf(fp, "wmm_ac_bk_aifs=7\n");
	fprintf(fp, "wmm_ac_bk_txop_limit=0\n");
	fprintf(fp, "wmm_ac_bk_acm=0\n");
	fprintf(fp, "wmm_ac_be_aifs=3\n");
	fprintf(fp, "wmm_ac_be_cwmin=4\n");
	fprintf(fp, "wmm_ac_be_cwmax=10\n");
	fprintf(fp, "wmm_ac_be_acm=0\n");
	fprintf(fp, "wmm_ac_vi_aifs=2\n");
	fprintf(fp, "wmm_ac_vi_cwmin=3\n");
	fprintf(fp, "wmm_ac_vi_cwmax=4\n");
	fprintf(fp, "wmm_ac_vi_txop_limit=94\n");
	fprintf(fp, "wmm_ac_vi_acm=0\n");
	fprintf(fp, "wmm_ac_vo_aifs=2\n");
	fprintf(fp, "wmm_ac_vo_cwmin=2\n");
	fprintf(fp, "wmm_ac_vo_cwmax=3\n");
	fprintf(fp, "wmm_ac_vo_txop_limit=47\n");
	fprintf(fp, "wmm_ac_vo_acm=0\n");
	fprintf(fp, "tx_queue_data3_aifs=7\n");
	fprintf(fp, "tx_queue_data3_cwmin=15\n");
	fprintf(fp, "tx_queue_data3_cwmax=1023\n");
	fprintf(fp, "tx_queue_data3_burst=0\n");
	fprintf(fp, "tx_queue_data2_aifs=3\n");
	fprintf(fp, "tx_queue_data2_cwmin=15\n");
	fprintf(fp, "tx_queue_data2_cwmax=63\n");
	fprintf(fp, "tx_queue_data1_aifs=1\n");
	fprintf(fp, "tx_queue_data1_cwmin=7\n");
	fprintf(fp, "tx_queue_data1_cwmax=15\n");
	fprintf(fp, "tx_queue_data1_burst=3.0\n");
	fprintf(fp, "tx_queue_data0_aifs=1\n");
	fprintf(fp, "tx_queue_data0_cwmin=3\n");
	fprintf(fp, "tx_queue_data0_cwmax=7\n");
	fprintf(fp, "tx_queue_data0_burst=1.5\n");
	char *country = getIsoName(nvram_default_get("ath0_regdomain", "GERMANY"));
	if (!country)
		country = "DE";
	fprintf(fp, "country_code=%s\n", country);
	char *netmode = nvram_nget("%s_net_mode", prefix);
	if (isath5k || !(nvram_match(netmode, "n2-only") || nvram_match(netmode, "n5-only"))) {
		fprintf(fp, "tx_queue_data2_burst=2.0\n");
		fprintf(fp, "wmm_ac_be_txop_limit=64\n");
	} else {
		fprintf(fp, "tx_queue_data2_burst=0\n");
		fprintf(fp, "wmm_ac_be_txop_limit=0\n");
	}

	char *akm = nvram_nget("%s_akm", prefix);
	char *crypto = nvram_nget("%s_crypto", prefix);
	char ht[6];
	int iht = 0;
	char bw[32];
	sprintf(bw, "%s_channelbw", prefix);
	ht[0] = 0;
	if ((!strcmp(netmode, "ng-only") ||	//
	     !strcmp(netmode, "na-only") ||	//
	     !strcmp(netmode, "n2-only") ||	//
	     !strcmp(netmode, "n5-only") ||	//
	     !strcmp(netmode, "ac-only") ||	//
	     !strcmp(netmode, "acn-mixed") ||	//
	     !strcmp(netmode, "mixed"))
	    && strcmp(akm, "wep")
	    && !aoss) {

		if (strcmp(netmode, "mixed") && strcmp(netmode, "ng-only")
		    && strcmp(netmode, "na-only")) {
			if (!isath5k)
				fprintf(fp, "require_ht=1\n");
		}
		if (!isath5k)
			fprintf(fp, "ieee80211n=1\n");
		if (nvram_match(bw, "2040")) {
			fprintf(fp, "dynamic_ht40=1\n");
		} else {
			fprintf(fp, "dynamic_ht40=0\n");
		}

		if (nvram_default_match(bw, "20", "20")) {
			sprintf(ht, "HT20");
		} else if (nvram_match(bw, "80") || nvram_match(bw, "40") || nvram_match(bw, "2040")) {
			char sb[32];
			sprintf(sb, "%s_nctrlsb", prefix);
			if (nvram_default_match(sb, "upper", "lower") || nvram_match(bw, "80")) {
				sprintf(ht, "HT40+");
				iht = 1;
			} else {
				sprintf(ht, "HT40-");
				iht = -1;
			}
		} else
			sprintf(ht, "HT20");
	} else {
		sprintf(ht, "HT20");
	}
	char regdomain[16];
	sprintf(regdomain, "%s_regdomain", prefix);
	country = nvram_default_get(regdomain, "GERMANY");
	// jumps to world if set here?!?
	// fprintf(fp, "country_code=%s\n", getIsoName(country));
	char *iso = getIsoName(country);
	if (!iso)
		iso = "DE";

	chan = mac80211_get_channels(prefix, iso, 40, 0xff);
	if (isrepeater) {
		// for ht40- take second channel otherwise hostapd is unhappy (and does not start)
		if (has_2ghz(prefix)) {
			if (iht == -1) {
				i = 4;	// in 2.4 mhz channel spacing is different
			}
		}
		if (has_5ghz(prefix)) {
			if (iht == -1) {
				if (nvram_match(bw, "80"))
					i = 2;	// second index for 80 mhz
				else
					i = 1;
			}
		}
		if (chan != NULL && chan[i].freq != -1) {
			channel = chan[i].channel;
			freq = chan[i].freq;
		} else {
			// that should never be called
			if (has_2ghz(prefix)) {
				channel = 6;
				freq = 2437;
			}
			if (has_5ghz(prefix)) {
				if (nvram_match(bw, "80")) {
					channel = 44;
					freq = 5220;
				} else {
					channel = 40;
					freq = 5200;
				}
			}
		}
	} else {
		// also we still should take care on the selected mode
		sprintf(nfreq, "%s_channel", prefix);
		freq = atoi(nvram_default_get(nfreq, "0"));

		if (freq == 0) {
			struct mac80211_ac *acs;
			fprintf(stderr, "call mac80211autochannel for interface: %s\n", prefix);
			eval("ifconfig", prefix, "up");
			acs = mac80211autochannel(prefix, NULL, 2, 1, 0);
			if (acs != NULL) {
				freq = acs->freq;
				channel = ieee80211_mhz2ieee(freq);
				fprintf(stderr, "mac80211autochannel interface: %s frequency: %d\n", prefix, freq);
				int i = 0;
				while (chan[i].freq != -1) {
					if (chan[i].freq == freq)
						break;
					i++;
				}
				if (iht != 0) {
					if (chan[i].ht40minus) {
						sprintf(ht, "HT40-");
					} else if (chan[i].ht40plus) {
						sprintf(ht, "HT40+");
					} else {
						sprintf(ht, "HT20");
					}
				}
				free_mac80211_ac(acs);
			} else {
				if (has_2ghz(prefix)) {
					channel = 6;
					freq = 2437;
				}
				if (has_5ghz(prefix)) {
					if (nvram_match(bw, "80")) {
						channel = 44;
						freq = 5220;
					} else {
						channel = 40;
						freq = 5200;
					}
				}
			}
		} else {
			channel = ieee80211_mhz2ieee(freq);
		}
	}
	if (!isath5k) {
		char shortgi[32];
		sprintf(shortgi, "%s_shortgi", prefix);
		caps = mac80211_get_caps(prefix, nvram_default_match(shortgi, "1", "1") ? 1 : 0);
		if (strlen(ht) > 0) {
			fprintf(fp, "ht_capab=[%s]%s\n", ht, caps);
		} else {
			fprintf(fp, "ht_capab=%s\n", caps);
		}
		free(caps);
	}
#ifdef HAVE_ATH10K
	if (is_ath10k(prefix) || is_mvebu(prefix)) {
		if ((!strcmp(netmode, "mixed") ||	//
		     !strcmp(netmode, "ac-only") || !strcmp(netmode, "acn-mixed"))) {
			char shortgi[32];
			sprintf(shortgi, "%s_shortgi", prefix);
			caps = mac80211_get_vhtcaps(prefix, nvram_default_match(shortgi, "1", "1") ? 1 : 0);
			fprintf(fp, "vht_capab=%s\n", caps);
			free(caps);
			fprintf(fp, "ieee80211ac=1\n");
			if (!strcmp(netmode, "ac-only")) {
				fprintf(fp, "require_vht=1\n");
				fprintf(fp, "ieee80211d=1\n");
				fprintf(fp, "ieee80211h=1\n");
				//might be needed for dfs
				//fprintf(fp, "spectrum_mgmt_required=1\n");
				//fprintf(fp, "local_pwr_constraint=3\n");
			}

			if (!strcmp(netmode, "acn-mixed")) {
				fprintf(fp, "require_ht=1\n");
				fprintf(fp, "ieee80211d=1\n");
				fprintf(fp, "ieee80211h=1\n");
				//might be needed for dfs
				//fprintf(fp, "spectrum_mgmt_required=1\n");
				//fprintf(fp, "local_pwr_constraint=3\n");
			}

			if (nvram_match(bw, "40")) {
				fprintf(fp, "vht_oper_chwidth=0\n");
				int idx = channel + (2 * iht);
/*				switch ((channel / 4) % 2) {
				case 0:
					idx = channel + 2;
					break;
				case 1:
					idx = channel - 2;
					break;
				}*/
				fprintf(fp, "vht_oper_centr_freq_seg0_idx=%d\n", idx);
			} else if (nvram_match(bw, "80")) {
				fprintf(fp, "vht_oper_chwidth=1\n");
				int idx = channel + (6 * iht);
/*                                switch ((channel / 4) % 4) {
                                case 0:
					idx = channel + 6;
					break;
				case 1:
					idx = channel - 6;
					break;
				case 2:
					idx = channel - 2;
					break;
				case 3:
					idx = channel + 2;
					break;
				}*/
				fprintf(fp, "vht_oper_centr_freq_seg0_idx=%d\n", idx);
			} else if (nvram_match(bw, "160")) {
				fprintf(fp, "vht_oper_chwidth=2\n");
			} else {
				if (nvram_match(bw, "80+80")) {
					fprintf(fp, "vht_oper_chwidth=3\n");
				} else {
					fprintf(fp, "vht_oper_chwidth=0\n");
				}
			}

		}

	}
#endif

	if (chan)
		free(chan);
	if (freq < 4000) {
		if (!strcmp(netmode, "b-only")) {
			fprintf(fp, "hw_mode=b\n");
			fprintf(fp, "supported_rates=10 20 55 110\n");
		} else {
			fprintf(fp, "hw_mode=g\n");
		}
	} else {
		fprintf(fp, "hw_mode=a\n");
		if (!strcmp(netmode, "a-only")) {
			fprintf(fp, "supported_rates=60 90 120 180 240 360 480 540\n");
		}

	}
	fprintf(fp, "channel=%d\n", channel);
	fprintf(fp, "frequency=%d\n", freq);
	char bcn[32];
	sprintf(bcn, "%s_bcn", prefix);
	fprintf(fp, "beacon_int=%s\n", nvram_default_get(bcn, "100"));
	fprintf(fp, "\n");

	char dtim[32];
	sprintf(dtim, "%s_dtim", prefix);
	fprintf(fp, "dtim_period=%s\n", nvram_default_get(dtim, "2"));
	fprintf(fp, "\n");
}

static void setMacFilter(FILE * fp, char *iface)
{
	char *next;
	char var[32];
	char nvvar[32];
	sprintf(nvvar, "%s_macmode", iface);
	if (nvram_match(nvvar, "deny")) {
		fprintf(fp, "deny_mac_file=/tmp/%s_deny\n", iface);
		fprintf(fp, "macaddr_acl=0\n");
		char nvlist[32];
		sprintf(nvlist, "%s_maclist", iface);
		char name[32];
		sprintf(name, "/tmp/%s_deny", iface);
		FILE *out = fopen(name, "wb");
		foreach(var, nvram_safe_get(nvlist), next) {
			fprintf(out, "%s\n", var);
		}
		fclose(out);
	} else if (nvram_match(nvvar, "allow")) {

		fprintf(fp, "accept_mac_file=/tmp/%s_accept\n", iface);
		fprintf(fp, "macaddr_acl=1\n");
		char nvlist[32];
		sprintf(nvlist, "%s_maclist", iface);
		char name[32];
		sprintf(name, "/tmp/%s_accept", iface);
		FILE *out = fopen(name, "wb");
		foreach(var, nvram_safe_get(nvlist), next) {
			fprintf(out, "%s\n", var);
		}
		fclose(out);
	}

}

static int ieee80211_aton(char *str, unsigned char mac[6])
{
	unsigned int addr[6];
	int i;
	if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x", &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]) != 6)
		return -1;
	/*
	 * sscanf needs an unsigned int, but mac address bytes cannot exceed 0xff
	 */
	for (i = 0; i < 6; i++)
		mac[i] = addr[i] & 0xff;
	return 0;
}

extern char *hostapd_eap_get_types(void);
extern void addWPS(FILE * fp, char *prefix, int configured);
extern void setupHS20(FILE * fp, char *prefix);

void setupHostAP_ath9k(char *maininterface, int isfirst, int vapid, int aoss)
{
#ifdef HAVE_REGISTER
	if (!isregistered())
		return;
#endif
	char psk[32];
	char akm[16];
	char fstr[32];
	FILE *fp = NULL;
	char *ssid;
	char nssid[16];
	char maxassoc[32];
	char ifname[10];
	int isrepeater = 0;
	unsigned char hwbuff[16];
	char macaddr[32];
	char *types;
	char *debug;
	char tmp[256];
	if (isfirst && vapid == 0) {
		sprintf(ifname, "%s", maininterface);
	} else {
		sprintf(ifname, "%s.%d", maininterface, vapid);
		isrepeater = 1;
	}
#ifdef HAVE_WZRHPAG300NH
	if (aoss) {
		if (!strncmp(ifname, "ath0", 4))
			sprintf(ifname, "aossg");
		else
			sprintf(ifname, "aossa");
	}
#else
	if (aoss)
		sprintf(ifname, "aoss");
#endif
	sprintf(akm, "%s_akm", ifname);
	if (nvram_match(akm, "8021X"))
		return;
	sprintf(fstr, "/tmp/%s_hostap.conf", maininterface);
	if (isfirst) {
		fp = fopen(fstr, "wb");
		setupHostAP_generic_ath9k(maininterface, fp, isrepeater, aoss);
		fprintf(fp, "interface=%s\n", ifname);
	} else {
		fp = fopen(fstr, "ab");
		fprintf(stderr, "setup vap %d bss %s\n", vapid, ifname);
		fprintf(fp, "bss=%s\n", ifname);
	}
	fprintf(fp, "disassoc_low_ack=1\n");
	char *mode = nvram_nget("%s_mode", ifname);
	if (!strcmp(mode, "wdsap"))
		fprintf(fp, "wds_sta=1\n");

	char wmm[32];
	sprintf(wmm, "%s_wmm", ifname);
	fprintf(fp, "wmm_enabled=%s\n", nvram_default_get(wmm, "1"));
	if (nvram_match("mac_clone_enable", "1")
	    && nvram_invmatch("def_whwaddr", "00:00:00:00:00:00")
	    && nvram_invmatch("def_whwaddr", "")
	    && !strcmp(maininterface, "ath0")) {
		ieee80211_aton(nvram_safe_get("def_whwaddr"), hwbuff);
	} else {
		int i = wl_hwaddr(maininterface, hwbuff);
	}
	if (vapid > 0) {
		hwbuff[0] ^= ((vapid - 1) << 2) | 0x2;
	}
	sprintf(macaddr, "%02X:%02X:%02X:%02X:%02X:%02X", hwbuff[0], hwbuff[1], hwbuff[2], hwbuff[3], hwbuff[4], hwbuff[5]);
	fprintf(fp, "bssid=%s\n", macaddr);
	char vathmac[16];
	sprintf(vathmac, "%s_hwaddr", ifname);
	nvram_set(vathmac, macaddr);
	fprintf(stderr, "setup %s %s\n", ifname, macaddr);
	setMacFilter(fp, ifname);
	char isolate[32];
	char broadcast[32];
	sprintf(isolate, "%s_ap_isolate", ifname);
	if (nvram_default_match(isolate, "1", "0"))
		fprintf(fp, "ap_isolate=1\n");
	sprintf(broadcast, "%s_closed", ifname);
	if (nvram_default_match(broadcast, "1", "0"))
		fprintf(fp, "ignore_broadcast_ssid=1\n");
	else
		fprintf(fp, "ignore_broadcast_ssid=0\n");
	sprintf(maxassoc, "%s_maxassoc", ifname);
	fprintf(fp, "max_num_sta=%s\n", nvram_default_get(maxassoc, "256"));
	if (aoss) {
		if (!strncmp(ifname, "aossa", 5))
			ssid = "ESSID-AOSS-1";
		else
			ssid = "ESSID-AOSS";
	} else {
		sprintf(nssid, "%s_ssid", ifname);
		ssid = nvram_default_get(nssid, "dd-wrt");
	}

	fprintf(fp, "ssid=%s\n", ssid);
	// wep key support
	if (nvram_match(akm, "wep") || aoss) {
//              if (!isfirst || aoss)
//                      fprintf(fp, "ieee80211n=0\n");

		if (nvram_nmatch("1", "%s_bridged", ifname))
			fprintf(fp, "bridge=%s\n", getBridge(ifname, tmp));
		if (!aoss) {
			if (!strncmp(ifname, "ath0", 4))
				led_control(LED_SEC0, LED_ON);
			if (!strncmp(ifname, "ath1", 4))
				led_control(LED_SEC1, LED_ON);
		}
		fprintf(fp, "logger_syslog=-1\n");
		debug = nvram_nget("%s_wpa_debug", ifname);
		if (debug != NULL) {
			if (!strcmp(debug, "1"))
				fprintf(fp, "logger_syslog_level=1\n");
			else if (!strcmp(debug, "2"))
				fprintf(fp, "logger_syslog_level=0\n");
			else if (!strcmp(debug, "3"))
				fprintf(fp, "logger_syslog_level=0\n");
		} else
			fprintf(fp, "logger_syslog_level=2\n");
		fprintf(fp, "logger_stdout=-1\n");
		fprintf(fp, "logger_stdout_level=2\n");
		char *authmode = nvram_nget("%s_authmode", ifname);
		if (aoss)
			authmode = "auto";
		if (!strcmp(authmode, "shared"))
			fprintf(fp, "auth_algs=2\n");
		else if (!strcmp(authmode, "auto"))
			fprintf(fp, "auth_algs=3\n");
		else
			fprintf(fp, "auth_algs=1\n");
		int i;
		if (aoss) {
			for (i = 1; i < 5; i++) {
				fprintf(fp, "wep_key%d=4D454C434F\n", i - 1);
			}
			fprintf(fp, "wep_default_key=0\n");
		} else {
			for (i = 1; i < 5; i++) {
				char *athkey = nvram_nget("%s_key%d", ifname, i);
				if (athkey != NULL && strlen(athkey) > 0) {
					fprintf(fp, "wep_key%d=%s\n", i - 1, athkey);
				}
			}
			fprintf(fp, "wep_default_key=%d\n", atoi(nvram_nget("%s_key", ifname)) - 1);
			addWPS(fp, ifname, 1);
		}
	} else if (nvram_match(akm, "disabled")) {
		addWPS(fp, ifname, 0);
	} else if (nvram_match(akm, "psk") || nvram_match(akm, "psk2") || nvram_match(akm, "psk psk2") || nvram_match(akm, "wpa") || nvram_match(akm, "wpa2")
		   || nvram_match(akm, "wpa wpa2")) {
		if (!strncmp(ifname, "ath0", 4))
			led_control(LED_SEC0, LED_ON);
		if (!strncmp(ifname, "ath1", 4))
			led_control(LED_SEC1, LED_ON);
		// sprintf(buf, "rsn_preauth_interfaces=%s\n", "br0");
		if (nvram_nmatch("1", "%s_bridged", ifname))
			fprintf(fp, "bridge=%s\n", getBridge(ifname, tmp));
		fprintf(fp, "logger_syslog=-1\n");
		debug = nvram_nget("%s_wpa_debug", ifname);
		if (debug != NULL) {
			if (!strcmp(debug, "1"))
				fprintf(fp, "logger_syslog_level=1\n");
			else if (!strcmp(debug, "2"))
				fprintf(fp, "logger_syslog_level=0\n");
			else if (!strcmp(debug, "3"))
				fprintf(fp, "logger_syslog_level=0\n");
		} else
			fprintf(fp, "logger_syslog_level=2\n");
		fprintf(fp, "logger_stdout=-1\n");
		fprintf(fp, "logger_stdout_level=2\n");
		// fprintf (fp, "eap_server=0\n");
		// fprintf (fp, "own_ip_addr=127.0.0.1\n");
		fprintf(fp, "eapol_version=1\n");
		fprintf(fp, "eapol_key_index_workaround=0\n");
		if (nvram_match(akm, "psk") || nvram_match(akm, "wpa"))
			fprintf(fp, "wpa=1\n");
		if (nvram_match(akm, "psk2")
		    || nvram_match(akm, "wpa2"))
			fprintf(fp, "wpa=2\n");
		if (nvram_match(akm, "psk psk2")
		    || nvram_match(akm, "wpa wpa2"))
			fprintf(fp, "wpa=3\n");
		if (nvram_match(akm, "psk") || nvram_match(akm, "psk2")
		    || nvram_match(akm, "psk psk2")) {
			if (strlen(nvram_nget("%s_wpa_psk", ifname)) == 64)
				fprintf(fp, "wpa_psk=%s\n", nvram_nget("%s_wpa_psk", ifname));
			else
				fprintf(fp, "wpa_passphrase=%s\n", nvram_nget("%s_wpa_psk", ifname));
			fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
			addWPS(fp, ifname, 1);
		} else {
			// if (nvram_invmatch (akm, "radius"))
			fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");
			// else
			// fprintf (fp, "macaddr_acl=2\n");
			fprintf(fp, "ieee8021x=1\n");
			// fprintf (fp, "accept_mac_file=/tmp/hostapd.accept\n");
			// fprintf (fp, "deny_mac_file=/tmp/hostapd.deny\n");

			char local_ip[32];
			sprintf(local_ip, "%s_local_ip", ifname);
			char *lip = nvram_default_get(local_ip, "0.0.0.0");
			if (strcmp(lip, "0.0.0.0")) {
				fprintf(fp, "radius_client_addr=%s\n", lip);
				fprintf(fp, "own_ip_addr=%s\n", lip);
			} else {
				if (nvram_match("wan_proto", "disabled"))
					fprintf(fp, "own_ip_addr=%s\n", nvram_safe_get("lan_ipaddr"));
				else {
					char *wip = get_wan_ipaddr();
					if (strlen(wip))
						fprintf(fp, "own_ip_addr=%s\n", wip);
					else
						fprintf(fp, "own_ip_addr=%s\n", nvram_safe_get("lan_ipaddr"));
				}

			}

			fprintf(fp, "eap_server=0\n");
			fprintf(fp, "auth_algs=1\n");
			char retry[32];
			sprintf(retry, "%s_radius_retry", ifname);
			fprintf(fp, "radius_retry_primary_interval=%s\n", nvram_default_get(retry, "600"));
			types = hostapd_eap_get_types();
			fprintf(fp, "%s", types);
			free(types);
			fprintf(fp, "auth_server_addr=%s\n", nvram_nget("%s_radius_ipaddr", ifname));
			fprintf(fp, "auth_server_port=%s\n", nvram_nget("%s_radius_port", ifname));
			fprintf(fp, "auth_server_shared_secret=%s\n", nvram_nget("%s_radius_key", ifname));
			char check[64];
			sprintf(check, "%s_radius2_ipaddr", ifname);
			nvram_default_get(check, "0.0.0.0");
			if (!nvram_nmatch("", "%s_radius2_ipaddr", ifname)
			    && !nvram_nmatch("0.0.0.0", "%s_radius2_ipaddr", ifname)
			    && !nvram_nmatch("", "%s_radius2_port", ifname)) {
				fprintf(fp, "auth_server_addr=%s\n", nvram_nget("%s_radius2_ipaddr", ifname));
				fprintf(fp, "auth_server_port=%s\n", nvram_nget("%s_radius2_port", ifname));
				fprintf(fp, "auth_server_shared_secret=%s\n", nvram_nget("%s_radius2_key", ifname));
			}
			if (nvram_nmatch("1", "%s_acct", ifname)) {
				fprintf(fp, "acct_server_addr=%s\n", nvram_nget("%s_acct_ipaddr", ifname));
				fprintf(fp, "acct_server_port=%s\n", nvram_nget("%s_acct_port", ifname));
				fprintf(fp, "acct_server_shared_secret=%s\n", nvram_nget("%s_acct_key", ifname));
			}
		}
		if (nvram_invmatch(akm, "radius")) {
			sprintf(psk, "%s_crypto", ifname);
			if (nvram_match(psk, "aes"))
				fprintf(fp, "wpa_pairwise=CCMP\n");
			if (nvram_match(psk, "tkip")) {
				if (!isfirst)
					fprintf(fp, "ieee80211n=0\n");
				fprintf(fp, "wpa_pairwise=TKIP\n");
			}
			if (nvram_match(psk, "tkip+aes"))
				fprintf(fp, "wpa_pairwise=TKIP CCMP\n");
			fprintf(fp, "wpa_group_rekey=%s\n", nvram_nget("%s_wpa_gtk_rekey", ifname));
		}
		// fprintf (fp, "jumpstart_p1=1\n");
	}
#ifdef HAVE_HOTSPOT20
	setupHS20(fp, ifname);
#endif
	char *v = nvram_nget("%s_config", ifname);
	fprintf(fp, "\n");
	if (v && strlen(v) > 0)
		fprintf(fp, "%s", v);
	fprintf(fp, "\n");

	fclose(fp);
}

/* das muessen wir noch machen
void setupradauth_ath9k(char *prefix, char *driver, int iswan) {
	if (nvram_match(akm, "radius")) {
		// wrt-radauth $IFNAME $server $port $share $override $mackey $maxun
		// &
		char *ifname = prefix;
		char *server = nvram_nget("%s_radius_ipaddr", prefix);
		char *port = nvram_nget("%s_radius_port", prefix);
		char *share = nvram_nget("%s_radius_key", prefix);
		char exec[64];
		char type[32];

		sprintf(type, "%s_radmactype", prefix);
		char *pragma = "";

		if (nvram_default_match(type, "0", "0"))
			pragma = "-n1 ";
		if (nvram_match(type, "1"))
			pragma = "-n2 ";
		if (nvram_match(type, "2"))
			pragma = "-n3 ";
		if (nvram_match(type, "3"))
			pragma = "";
		sleep(1);	// some delay is usefull
		sysprintf("wrt-radauth %s %s %s %s %s 1 1 0 &", pragma,
			  prefix, server, port, share);
	}
*/

void setupSupplicant_ath9k(char *prefix, char *ssidoverride)
{
#ifdef HAVE_REGISTER
	if (!isregistered())
		return;
#endif
	char akm[16];
	int i;
	sprintf(akm, "%s_akm", prefix);
	if (nvram_match(akm, "psk") || nvram_match(akm, "psk2")
	    || nvram_match(akm, "psk psk2")) {
		char fstr[32];
		char psk[16];
		if (!strncmp(prefix, "ath0", 4))
			led_control(LED_SEC0, LED_ON);
		if (!strncmp(prefix, "ath1", 4))
			led_control(LED_SEC1, LED_ON);
		sprintf(fstr, "/tmp/%s_wpa_supplicant.conf", prefix);
		FILE *fp = fopen(fstr, "wb");
		fprintf(fp, "ap_scan=1\n");
		fprintf(fp, "fast_reauth=1\n");
		fprintf(fp, "eapol_version=1\n");
		// fprintf (fp, "ctrl_interface_group=0\n");
		// fprintf (fp, "ctrl_interface=/var/run/wpa_supplicant\n");
		fprintf(fp, "network={\n");
		if (!ssidoverride)
			ssidoverride = nvram_nget("%s_ssid", prefix);
		fprintf(fp, "\tssid=\"%s\"\n", ssidoverride);
		char scanlist[32];
		sprintf(scanlist, "%s_scanlist", prefix);
		char *sl = nvram_default_get(scanlist, "default");
		if (strcmp(sl, "default")) {
			fprintf(fp, "\tscan_freq=%s\n", sl);
		}
#ifdef HAVE_UNIWIP
		fprintf(fp, "\tbgscan=\"simple:30:-45:300\"\n");
#endif
		// fprintf (fp, "\tmode=0\n");
		fprintf(fp, "\tscan_ssid=1\n");
		fprintf(fp, "\tkey_mgmt=WPA-PSK\n");
		sprintf(psk, "%s_crypto", prefix);
		if (nvram_match(psk, "aes")) {
#if 1
			fprintf(fp, "\tpairwise=CCMP\n");
			fprintf(fp, "\tgroup=CCMP TKIP\n");
#else
			fprintf(fp, "\tpairwise=CCMP\n");
			fprintf(fp, "\tgroup=CCMP\n");
#endif
		}
		if (nvram_match(psk, "tkip")) {
			fprintf(fp, "\tpairwise=TKIP\n");
			fprintf(fp, "\tgroup=TKIP\n");
		}
		if (nvram_match(psk, "tkip+aes")) {
			fprintf(fp, "\tpairwise=CCMP TKIP\n");
			fprintf(fp, "\tgroup=CCMP TKIP\n");
		}
		if (nvram_match(akm, "psk"))
			fprintf(fp, "\tproto=WPA\n");
		if (nvram_match(akm, "psk2"))
			fprintf(fp, "\tproto=RSN\n");
		if (nvram_match(akm, "psk psk2"))
			fprintf(fp, "\tproto=WPA RSN\n");
		char *wpa_psk = nvram_nget("%s_wpa_psk", prefix);
		if (strlen(wpa_psk) == 64)
			fprintf(fp, "\tpsk=%s\n", wpa_psk);
		else
			fprintf(fp, "\tpsk=\"%s\"\n", wpa_psk);
		fprintf(fp, "}\n");
		char extra[32];
		sprintf(extra, "%s_supplicantext", prefix);
		if (nvram_invmatch(extra, ""))
			fwritenvram(extra, fp);
		fclose(fp);
	} else if (nvram_match(akm, "8021X")) {
		char fstr[32];
		char psk[64];
		char ath[64];
		if (!strncmp(prefix, "ath0", 4))
			led_control(LED_SEC0, LED_ON);
		if (!strncmp(prefix, "ath1", 4))
			led_control(LED_SEC1, LED_ON);
		sprintf(fstr, "/tmp/%s_wpa_supplicant.conf", prefix);
		FILE *fp = fopen(fstr, "wb");
		fprintf(fp, "ap_scan=1\n");
		fprintf(fp, "fast_reauth=1\n");
		fprintf(fp, "eapol_version=1\n");
		// fprintf (fp, "ctrl_interface_group=0\n");
		// fprintf (fp, "ctrl_interface=/var/run/wpa_supplicant\n");
		fprintf(fp, "network={\n");
		if (!ssidoverride)
			ssidoverride = nvram_nget("%s_ssid", prefix);
		fprintf(fp, "\tssid=\"%s\"\n", ssidoverride);
#ifdef HAVE_UNIWIP
		fprintf(fp, "\tbgscan=\"simple:30:-45:300\"\n");
#endif
		fprintf(fp, "\tscan_ssid=1\n");
		if (nvram_prefix_match("8021xtype", prefix, "tls")) {
// -> added habeIchVergessen
			char *keyExchng = nvram_nget("%s_tls8021xkeyxchng", prefix);
			char wpaOpts[40];
			if (strlen(keyExchng) == 0)
				nvram_nset("wep", "%s_tls8021xkeyxchng", prefix);
			sprintf(wpaOpts, "");
			keyExchng = nvram_nget("%s_tls8021xkeyxchng", prefix);
			if (strcmp("wpa2", keyExchng) == 0)
				sprintf(wpaOpts, "\tpairwise=CCMP\n\tgroup=CCMP\n");
			if (strcmp("wpa2mixed", keyExchng) == 0)
				sprintf(wpaOpts, "\tpairwise=CCMP TKIP\n\tgroup=CCMP TKIP\n");
			if (strcmp("wpa", keyExchng) == 0)
				sprintf(wpaOpts, "\tpairwise=TKIP\n\tgroup=TKIP\n");
			fprintf(fp, "\tkey_mgmt=%s\n%s", (strlen(wpaOpts) == 0 ? "IEEE8021X" : "WPA-EAP"), wpaOpts);
// <- added habeIchVergessen
			fprintf(fp, "\teap=TLS\n");
			fprintf(fp, "\tidentity=\"%s\"\n", nvram_prefix_get("tls8021xuser", prefix));
			sprintf(psk, "/tmp/%s", prefix);
			mkdir(psk, 0700);
			sprintf(psk, "/tmp/%s/ca.pem", prefix);
			sprintf(ath, "%s_tls8021xca", prefix);
			write_nvram(psk, ath);
			sprintf(psk, "/tmp/%s/user.pem", prefix);
			sprintf(ath, "%s_tls8021xpem", prefix);
			write_nvram(psk, ath);
			sprintf(psk, "/tmp/%s/user.prv", prefix);
			sprintf(ath, "%s_tls8021xprv", prefix);
			write_nvram(psk, ath);
			fprintf(fp, "\tca_cert=\"/tmp/%s/ca.pem\"\n", prefix);
			fprintf(fp, "\tclient_cert=\"/tmp/%s/user.pem\"\n", prefix);
			fprintf(fp, "\tprivate_key=\"/tmp/%s/user.prv\"\n", prefix);
			fprintf(fp, "\tprivate_key_passwd=\"%s\"\n", nvram_prefix_get("tls8021xpasswd", prefix));
			fprintf(fp, "\teapol_flags=3\n");
			if (strlen(nvram_nget("%s_tls8021xphase2", prefix)) > 0) {
				fprintf(fp, "\tphase2=\"%s\"\n", nvram_nget("%s_tls8021xphase2", prefix));
			}
			if (strlen(nvram_nget("%s_tls8021xanon", prefix)) > 0) {
				fprintf(fp, "\tanonymous_identity=\"%s\"\n", nvram_nget("%s_tls8021xanon", prefix));
			}
			if (strlen(nvram_nget("%s_tls8021xaddopt", prefix)) > 0) {
				sprintf(ath, "%s_tls8021xaddopt", prefix);
				fprintf(fp, "\t");	// tab
				fwritenvram(ath, fp);
				fprintf(fp, "\n");	// extra new line at the end
			}
		}
		if (nvram_prefix_match("8021xtype", prefix, "peap")) {
			fprintf(fp, "\tkey_mgmt=WPA-EAP\n");
			fprintf(fp, "\teap=PEAP\n");
			fprintf(fp, "\tpairwise=CCMP TKIP\n");
			fprintf(fp, "\tgroup=CCMP TKIP\n");
			fprintf(fp, "\tphase1=\"peapver=0\"\n");
			fprintf(fp, "\tidentity=\"%s\"\n", nvram_prefix_get("peap8021xuser", prefix));
			fprintf(fp, "\tpassword=\"%s\"\n", nvram_prefix_get("peap8021xpasswd", prefix));
			sprintf(psk, "/tmp/%s", prefix);
			mkdir(psk, 0700);
			sprintf(psk, "/tmp/%s/ca.pem", prefix);
			sprintf(ath, "%s_peap8021xca", prefix);
			if (!nvram_match(ath, "")) {
				write_nvram(psk, ath);
				fprintf(fp, "\tca_cert=\"/tmp/%s/ca.pem\"\n", prefix);
			}
			if (strlen(nvram_nget("%s_peap8021xphase2", prefix)) > 0) {
				fprintf(fp, "\tphase2=\"%s\"\n", nvram_nget("%s_peap8021xphase2", prefix));
			}
			if (strlen(nvram_nget("%s_peap8021xanon", prefix)) > 0) {
				fprintf(fp, "\tanonymous_identity=\"%s\"\n", nvram_nget("%s_peap8021xanon", prefix));
			}
			if (strlen(nvram_nget("%s_peap8021xaddopt", prefix)) > 0) {
				sprintf(ath, "%s_peap8021xaddopt", prefix);
				fprintf(fp, "\t");	// tab
				fwritenvram(ath, fp);
				fprintf(fp, "\n");	// extra new line at the end
			}
		}
		if (nvram_prefix_match("8021xtype", prefix, "ttls")) {
			fprintf(fp, "\tkey_mgmt=WPA-EAP\n");
			fprintf(fp, "\teap=TTLS\n");
			fprintf(fp, "\tpairwise=CCMP TKIP\n");
			fprintf(fp, "\tgroup=CCMP TKIP\n");
			fprintf(fp, "\tidentity=\"%s\"\n", nvram_prefix_get("ttls8021xuser", prefix));
			fprintf(fp, "\tpassword=\"%s\"\n", nvram_prefix_get("ttls8021xpasswd", prefix));
			if (strlen(nvram_nget("%s_ttls8021xca", prefix)) > 0) {
				sprintf(psk, "/tmp/%s", prefix);
				mkdir(psk, 0700);
				sprintf(psk, "/tmp/%s/ca.pem", prefix);
				sprintf(ath, "%s_ttls8021xca", prefix);
				write_nvram(psk, ath);
				fprintf(fp, "\tca_cert=\"/tmp/%s/ca.pem\"\n", prefix);
			}
			if (strlen(nvram_nget("%s_ttls8021xphase2", prefix)) > 0) {
				fprintf(fp, "\tphase2=\"%s\"\n", nvram_nget("%s_ttls8021xphase2", prefix));
			}
			if (strlen(nvram_nget("%s_ttls8021xanon", prefix)) > 0) {
				fprintf(fp, "\tanonymous_identity=\"%s\"\n", nvram_nget("%s_ttls8021xanon", prefix));
			}
			if (strlen(nvram_nget("%s_ttls8021xaddopt", prefix)) > 0) {
				sprintf(ath, "%s_ttls8021xaddopt", prefix);
				fprintf(fp, "\t");	// tab
				fwritenvram(ath, fp);
				fprintf(fp, "\n");	// extra new line at the end
			}
		}
		if (nvram_prefix_match("8021xtype", prefix, "leap")) {
			fprintf(fp, "\tkey_mgmt=WPA-EAP\n");
			fprintf(fp, "\teap=LEAP\n");
			fprintf(fp, "\tauth_alg=LEAP\n");
			fprintf(fp, "\tproto=WPA RSN\n");
			fprintf(fp, "\tpairwise=CCMP TKIP\n");
			fprintf(fp, "\tgroup=CCMP TKIP\n");
			fprintf(fp, "\tidentity=\"%s\"\n", nvram_prefix_get("leap8021xuser", prefix));
			fprintf(fp, "\tpassword=\"%s\"\n", nvram_prefix_get("leap8021xpasswd", prefix));
			// sprintf (psk, "/tmp/%s", prefix);
			// mkdir (psk);
			// sprintf (psk, "/tmp/%s/ca.pem", prefix);
			// sprintf (ath, "%s_peap8021xca", prefix);
			// write_nvram (psk, ath);
			// fprintf (fp, "\tca_cert=\"/tmp/%s/ca.pem\"\n", prefix);
			if (strlen(nvram_nget("%s_leap8021xphase2", prefix)) > 0) {
				fprintf(fp, "\tphase2=\"%s\"\n", nvram_nget("%s_leap8021xphase2", prefix));
			}
			if (strlen(nvram_nget("%s_leap8021xanon", prefix)) > 0) {
				fprintf(fp, "\tanonymous_identity=\"%s\"\n", nvram_nget("%s_leap8021xanon", prefix));
			}
			if (strlen(nvram_nget("%s_leap8021xaddopt", prefix)) > 0) {
				sprintf(ath, "%s_leap8021xaddopt", prefix);
				fprintf(fp, "\t");	// tab
				fwritenvram(ath, fp);
				fprintf(fp, "\n");	// extra new line at the end
			}
		}
		fprintf(fp, "}\n");
		char extra[32];
		sprintf(extra, "%s_supplicantext", prefix);
		if (nvram_invmatch(extra, ""))
			fwritenvram(extra, fp);
		fclose(fp);
	} else if (nvram_match(akm, "disabled") || nvram_match(akm, "wep")) {
		char fstr[32];
		char psk[16];
		if (nvram_match(akm, "wep")) {
			if (!strncmp(prefix, "ath0", 4))
				led_control(LED_SEC0, LED_ON);
			if (!strncmp(prefix, "ath1", 4))
				led_control(LED_SEC1, LED_ON);
		}
		sprintf(fstr, "/tmp/%s_wpa_supplicant.conf", prefix);
		FILE *fp = fopen(fstr, "wb");
		fprintf(fp, "ap_scan=1\n");
		// fprintf (fp, "ctrl_interface_group=0\n");
		// fprintf (fp, "ctrl_interface=/var/run/wpa_supplicant\n");
		fprintf(fp, "network={\n");
		if (!ssidoverride)
			ssidoverride = nvram_nget("%s_ssid", prefix);
		fprintf(fp, "\tssid=\"%s\"\n", ssidoverride);
#ifdef HAVE_UNIWIP
		fprintf(fp, "\tbgscan=\"simple:30:-45:300\"\n");
#endif
		// fprintf (fp, "\tmode=0\n");
		fprintf(fp, "\tscan_ssid=1\n");
		fprintf(fp, "\tkey_mgmt=NONE\n");
		if (nvram_match(akm, "wep")) {
			int cnt = 0;
			char *authmode = nvram_nget("%s_authmode", prefix);
			if (!strcmp(authmode, "shared"))
				fprintf(fp, "auth_alg=SHARED\n");
			else
				fprintf(fp, "auth_alg=OPEN\n");
			for (i = 1; i < 5; i++) {
				char *athkey = nvram_nget("%s_key%d", prefix, i);
				if (athkey != NULL && strlen(athkey) > 0) {
					fprintf(fp, "wep_key%d=%s\n", i - 1, athkey);	// setup wep
				}
			}

			sprintf(psk, "%s", nvram_nget("%s_key", prefix));
			fprintf(fp, "wep_tx_keyidx=%d\n", atoi(psk) - 1);
		}
		fprintf(fp, "}\n");
		char extra[32];
		sprintf(extra, "%s_supplicantext", prefix);
		if (nvram_invmatch(extra, ""))
			fwritenvram(extra, fp);
		fclose(fp);
	}

}

extern void do_hostapd(char *fstr, char *prefix);
void ath9k_start_supplicant(int count)
{
// erst hostapd starten fuer repeater mode
// dann wpa_supplicant mit link zu hostapd
// dann bridgen und konfiguriren
	char *next;
	char var[80];
	char fstr[32];
	char bridged[32];
	char mode[80];
	char dev[10];
	char power[32];
	char *apm, *vifs;
	char wl[16];
	char ctrliface[32] = "";
	char wifivifs[16];
	char tmp[256];
#ifdef HAVE_CONFIG_DEBUG_SYSLOG
	char *background = "-Bs";
#else
	char *background = "-B";
#endif
	char *debug;
	char psk[16];
	char wmode[16];
	int ctrlifneeded = 0;
	char wif[10];
	sprintf(wif, "phy%d", get_ath9k_phy_idx(count));
	sprintf(wl, "ath%d_mode", count);
	sprintf(dev, "ath%d", count);
	apm = nvram_safe_get(wl);
	sprintf(wifivifs, "ath%d_vifs", count);
	sprintf(power, "ath%d_txpwrdbm", count);
	vifs = nvram_safe_get(wifivifs);
	sprintf(psk, "-i%s", dev);
	sprintf(wmode, "%s_mode", dev);
	sprintf(bridged, "%s_bridged", dev);
	debug = nvram_nget("%s_wpa_debug", dev);
#ifdef HAVE_CONFIG_DEBUG_SYSLOG
	if (debug != NULL) {
		if (!strcmp(debug, "1"))
			background = "-Bds";
		else if (!strcmp(debug, "2"))
			background = "-Bdds";
		else if (!strcmp(debug, "3"))
			background = "-Bddds";
	}
#else
	if (debug != NULL) {
		if (!strcmp(debug, "1"))
			background = "-Bd";
		else if (!strcmp(debug, "2"))
			background = "-Bdd";
		else if (!strcmp(debug, "3"))
			background = "-Bddd";
	}
#endif
	if (strcmp(apm, "sta") && strcmp(apm, "wdssta")
	    && strcmp(apm, "wet")) {
		sprintf(fstr, "/tmp/%s_hostap.conf", dev);
		do_hostapd(fstr, dev);
	} else {
		if (strlen(vifs) > 0) {
			sprintf(fstr, "/tmp/%s_hostap.conf", dev);
			do_hostapd(fstr, dev);
			sprintf(ctrliface, "/var/run/hostapd/%s.1", dev);
			sprintf(fstr, "/tmp/%s_wpa_supplicant.conf", dev);
#ifdef HAVE_RELAYD
			if ((nvram_match(wmode, "wdssta"))
			    && nvram_match(bridged, "1"))
				eval("wpa_supplicant", "-b", getBridge(dev, tmp), background, "-Dnl80211", psk, "-H", ctrliface, "-c", fstr);
			else
				eval("wpa_supplicant", background, "-Dnl80211", psk, "-H", ctrliface, "-c", fstr);
#else
			if ((nvram_match(wmode, "wdssta")
			     || nvram_match(wmode, "wet"))
			    && nvram_match(bridged, "1"))
				eval("wpa_supplicant", "-b", getBridge(dev, tmp), background, "-Dnl80211", psk, "-H", ctrliface, "-c", fstr);
			else
				eval("wpa_supplicant", background, "-Dnl80211", psk, "-H", ctrliface, "-c", fstr);
#endif
		} else {
			sprintf(fstr, "/tmp/%s_wpa_supplicant.conf", dev);
#ifdef HAVE_RELAYD
			if ((nvram_match(wmode, "wdssta"))
			    && nvram_match(bridged, "1"))
				eval("wpa_supplicant", "-b", getBridge(dev, tmp), background, "-Dnl80211", psk, "-c", fstr);
			else
				eval("wpa_supplicant", background, "-Dnl80211", psk, "-c", fstr);
#else
			if ((nvram_match(wmode, "wdssta")
			     || nvram_match(wmode, "wet"))
			    && nvram_match(bridged, "1"))
				eval("wpa_supplicant", "-b", getBridge(dev, tmp), background, "-Dnl80211", psk, "-c", fstr);
			else
				eval("wpa_supplicant", background, "-Dnl80211", psk, "-c", fstr);
#endif
		}
	}
#ifdef HAVE_RELAYD
	if (strcmp(apm, "sta") && strcmp(apm, "wet")) {
#else
	if (strcmp(apm, "sta")) {
#endif
		char bridged[32];
		sprintf(bridged, "%s_bridged", dev);
		if (nvram_default_match(bridged, "1", "1")) {
			eval("ifconfig", dev, "0.0.0.0", "up");
			br_add_interface(getBridge(dev, tmp), dev);
			eval("ifconfig", dev, "0.0.0.0", "up");
		} else {
			eval("ifconfig", dev, "mtu", getMTU(dev));
			eval("ifconfig", dev, "txqueuelen", getTXQ(dev));
			eval("ifconfig", dev, nvram_nget("%s_ipaddr", dev), "netmask", nvram_nget("%s_netmask", dev), "up");
		}
	} else {
#ifdef HAVE_RELAYD
		if (!strcmp(apm, "wet")) {
			eval("ifconfig", dev, "0.0.0.0", "up");
//                      sysprintf("relayd -I %s -I %s -D -B", getBridge(dev),
//                                dev);
		}
#endif

		char bridged[32];
		sprintf(bridged, "%s_bridged", dev);
		if (nvram_default_match(bridged, "0", "1")) {
			eval("ifconfig", dev, "mtu", getMTU(dev));
			eval("ifconfig", dev, "txqueuelen", getTXQ(dev));
			eval("ifconfig", dev, nvram_nget("%s_ipaddr", dev), "netmask", nvram_nget("%s_netmask", dev), "up");
		}
	}

	if (strlen(vifs) > 0) {
		foreach(var, vifs, next) {
			sprintf(mode, "%s_mode", var);
			char *m2 = nvram_safe_get(mode);
			if (strcmp(m2, "sta")) {
				char bridged[32];
				sprintf(bridged, "%s_bridged", var);
				if (nvram_default_match(bridged, "1", "1")) {
					eval("ifconfig", dev, "0.0.0.0", "up");
					br_add_interface(getBridge(var, tmp), var);
				} else {
					eval("ifconfig", var, "mtu", getMTU(var));
					eval("ifconfig", var, "txqueuelen", getTXQ(var));
					eval("ifconfig", var, nvram_nget("%s_ipaddr", var), "netmask", nvram_nget("%s_netmask", var), "up");
				}
			}
		}
	}
	if (!strcmp(apm, "ap") || !strcmp(apm, "wdsap")) {
		int s;
		for (s = 1; s <= 10; s++) {
			char wdsvarname[32] = {
				0
			};
			char wdsdevname[32] = {
				0
			};
			char wdsmacname[32] = {
				0
			};
			char *wdsdev;
			char *hwaddr;
			sprintf(wdsvarname, "%s_wds%d_enable", dev, s);
			sprintf(wdsdevname, "%s_wds%d_if", dev, s);
			sprintf(wdsmacname, "%s_wds%d_hwaddr", dev, s);
			wdsdev = nvram_safe_get(wdsdevname);
			if (strlen(wdsdev) == 0)
				continue;
			if (nvram_match(wdsvarname, "0"))
				continue;
			hwaddr = nvram_get(wdsmacname);
			if (hwaddr != NULL) {
				eval("iw", wif, "interface", "add", wdsdev, "type", "wds");
				eval("iw", "dev", wdsdev, "set", "peer", hwaddr);
				eval("ifconfig", wdsdev, "0.0.0.0", "up");
			}
		}
	}
	int newpower = atoi(nvram_default_get(power, "16"));
	sysprintf("iw phy %s set txpower fixed %d", wif, newpower * 100);
}
#endif
