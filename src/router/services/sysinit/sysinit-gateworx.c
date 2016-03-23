/*
 * sysinit-gateworx.c
 *
 * Copyright (C) 2006 Sebastian Gottschall <gottschall@dd-wrt.com>
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
 * 
 * System Initialisation for Avila Gateworks and compatible Routers
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <utils.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/mii.h>
#include "devices/ethernet.c"
#include "devices/wireless.c"

#ifndef HAVE_TONZE
#ifndef HAVE_NOP8670

/*void checkupdate( void )
{
    int res, res2 = 0;
    FILE *in =
	popen( "/bin/cat /dev/mtdblock/0|/bin/grep NewMedia|wc -l", "rb" );
    fscanf( in, "%d", &res );
    pclose( in );
    if( res == 0 )
    {
	in = popen( "/bin/cat /dev/mtdblock/0|/bin/grep \"2\\.02\"|wc -l",
		    "rb" );
	fscanf( in, "%d", &res2 );
	pclose( in );
	if( res2 == 0 )
	{
	    in = popen( "/bin/cat /dev/mtdblock/0|/bin/grep \"2\\.04\"|wc -l",
			"rb" );
	    fscanf( in, "%d", &res2 );
	    pclose( in );
	    if( res2 == 1 || res2 == 7 )	// 7 is the result for debug
		// info enabled reboot builds
	    {
		fprintf( stderr, "updating avila type 2 redboot\n" );
		eval( "tar", "-xaf", "/usr/lib/firmware/redboot.tg7", "-C",
		      "/tmp" );
		eval( "mtd", "-r", "-f", "write", "/tmp/avila-rb.bin",
		      "RedBoot" );
		return;
	    }
	}
    }
    if( res == 1 )
    {
	in = popen( "/bin/cat /dev/mtdblock/0|/bin/grep \"2\\.03\"|wc -l",
		    "rb" );
	fscanf( in, "%d", &res2 );
	pclose( in );
    }
    if( res2 == 1 )		// redboot update is needed
    {
	in = popen( "/bin/dmesg|/bin/grep \"Memory: 64MB\"|wc -l", "rb" );
	fscanf( in, "%d", &res );
	pclose( in );
	if( res == 1 )
	    res2 = 64;
	in = popen( "/bin/dmesg|/bin/grep \"Memory: 32MB\"|wc -l", "rb" );
	fscanf( in, "%d", &res );
	pclose( in );
	if( res == 1 )
	    res2 = 32;
	in = popen( "/bin/dmesg|/bin/grep \"Memory: 128MB\"|wc -l", "rb" );
	fscanf( in, "%d", &res );
	pclose( in );
	if( res == 1 )
	    res2 = 128;
	in = popen( "/bin/dmesg|/bin/grep \"Memory: 256MB\"|wc -l", "rb" );
	fscanf( in, "%d", &res );
	pclose( in );
	if( res == 1 )
	    res2 = 256;
	fprintf( stderr, "updating redboot %d MB\n", res2 );
	char fname[64];

	sprintf( fname, "/tmp/rb-%d.bin", res2 );
	eval( "tar", "-xaf", "/usr/lib/firmware/redboot.tg7", "-C", "/tmp" );
	eval( "mtd", "-r", "-f", "write", fname, "RedBoot" );
    }
}
*/
#endif
#endif
void start_sysinit(void)
{
	struct stat tmp_stat;
	time_t tm = 0;

	mknod("/dev/gpio", S_IFCHR | 0644, makedev(127, 0));
	mknod("/dev/rtc", S_IFCHR | 0644, makedev(254, 0));
	mknod("/dev/crypto", S_IFCHR | 0644, makedev(10, 70));
	eval("mount", "-o", "remount,rw", "/");

	/*
	 * Setup console 
	 */

	fprintf(stderr, "set console loglevel\n");
	cprintf("sysinit() klogctl\n");
	klogctl(8, NULL, atoi(nvram_safe_get("console_loglevel")));
	cprintf("sysinit() get router\n");

	/*
	 * Modules 
	 */

#ifndef HAVE_TONZE
#ifndef HAVE_NOP8670
//    checkupdate(  );
#endif
#endif
	fprintf(stderr, "try modules for ethernet adapters\n");
	nvram_set("intel_eth", "0");
	if (detect_ethernet_devices())
		nvram_set("intel_eth", "1");

	detect_wireless_devices();

#if 1
	fprintf(stderr, "load IXP helper\n");
	insmod("ixp400th");
	fprintf(stderr, "load IXP Core Driver\n");
	insmod("ixp400");
//      system2("cat /usr/lib/firmware/IxNpeMicrocode.dat > /dev/IxNpe");
	fprintf(stderr, "load IXP Ethernet Driver\n");
	insmod("ixp400_eth");
	fprintf(stderr, "initialize Ethernet\n");
	eval("ifconfig", "ixp0", "0.0.0.0", "up");
	eval("ifconfig", "ixp1", "0.0.0.0", "up");
#ifndef HAVE_WAVESAT
	fprintf(stderr, "Load OCF Drivers\n");
	insmod("ocf");
	insmod("cryptodev");
#endif
	// insmod("ixp4xx", "init_crypto=0");
#else
	// eval ("mknod", "/dev/IxNpe","c","10","184");
	system2("cat /usr/lib/firmware/NPE-B > /dev/misc/ixp4xx_ucode");
	system2("cat /usr/lib/firmware/NPE-C > /dev/misc/ixp4xx_ucode");
#endif

	// insmod("ath_pci", "rfkill=0", "autocreate=none");

	/*
	 * if (ifexists ("wifi0")) eval ("ifconfig", "wifi0", "up"); if (ifexists 
	 * ("wifi1")) eval ("ifconfig", "wifi1", "up"); if (ifexists ("wifi2"))
	 * eval ("ifconfig", "wifi2", "up"); if (ifexists ("wifi3")) eval
	 * ("ifconfig", "wifi3", "up"); if (ifexists ("wifi4")) eval ("ifconfig", 
	 * "wifi4", "up"); if (ifexists ("wifi5")) eval ("ifconfig", "wifi5",
	 * "up"); 
	 */

	fprintf(stderr, "Load Sensor Driver\n");
	insmod("ad7418");	// temp / voltage sensor
	/*
	 * Configure mac addresses by reading data from eeprom 
	 */
	// char *filename =
	// "/sys/devices/platform/IXP4XX-I2C.0/i2c-0/0-0051/eeprom"; /*
	// bank2=0x100 */
#ifdef HAVE_NOP8670

	char filename[64];

	sprintf(filename, "/dev/mtdblock/%d", getMTD("RedBoot config"));
	FILE *file = fopen(filename, "r");

	if (file) {
		eval("ifconfig", "ixp0", "0.0.0.0", "down");
		eval("ifconfig", "ixp1", "0.0.0.0", "down");
		unsigned char buf[20];
		int i;

		fseek(file, 0x422, SEEK_SET);
		fread(&buf[0], 6, 1, file);
		char mac[20];
		unsigned int copy[20];

		for (i = 0; i < 6; i++)
			copy[i] = buf[i] & 0xff;
		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[0], copy[1], copy[2], copy[3], copy[4], copy[5]);
		fprintf(stderr, "configure IXP0 to %s\n", mac);
		nvram_set("et0macaddr_safe", mac);
		nvram_set("et0macaddr", mac);
		eval("ifconfig", "ixp0", "hw", "ether", mac);
		fseek(file, 0x43b, SEEK_SET);
		fread(&buf[6], 6, 1, file);
		for (i = 0; i < 12; i++)
			copy[i] = buf[i] & 0xff;
		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[6], copy[7], copy[8], copy[9], copy[10], copy[11]);
		fprintf(stderr, "configure IXP1 to %s\n", mac);
		eval("ifconfig", "ixp1", "hw", "ether", mac);
		fclose(file);
		eval("ifconfig", "ixp0", "0.0.0.0", "up");
		eval("ifconfig", "ixp1", "0.0.0.0", "up");
	}
#else
	char *filename = "/sys/devices/platform/IXP4XX-I2C.0/i2c-adapter:i2c-0/0-0051/eeprom";	/* bank2=0x100 
												 */
	fprintf(stderr, "Read MAC Addresses from EEPROM\n");
	FILE *file = fopen(filename, "r");
	if (!file) {
		filename = "/sys/devices/platform/IXP4XX-I2C.0/i2c-0/0-0051/eeprom";	//for 2.6.34.6
		file = fopen(filename, "r");
	}

	if (file) {
		unsigned char buf[20];

		fread(&buf[0], 16, 1, file);
		char mac[20];

		unsigned int copy[20];
		int i;

		for (i = 0; i < 12; i++)
			copy[i] = buf[i] & 0xff;
		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[0], copy[1], copy[2], copy[3], copy[4], copy[5]);
		nvram_set("et0macaddr_safe", mac);
		nvram_set("et0macaddr", mac);
		eval("ifconfig", "ixp0", "hw", "ether", mac);
		sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[6], copy[7], copy[8], copy[9], copy[10], copy[11]);
		eval("ifconfig", "ixp1", "hw", "ether", mac);

		fclose(file);
	}
#endif
	eval("ifconfig", "ixp0", "0.0.0.0", "up");
	eval("ifconfig", "ixp1", "0.0.0.0", "up");
	if (getRouterBrand() == ROUTER_BOARD_GATEWORX_GW2345)	// lets load
		// the spi
		// drivers
		// for this
		// switch
	{
		fprintf(stderr, "Load SPI Kendin Switch Driver\n");
		insmod("spi-algo-bit");
		if (nvram_match("DD_BOARD", "Gateworks Avila GW2355"))
			insmod("spi-ixp4xx-gw2355");
		else
			insmod("spi-ixp4xx");
		insmod("ks8995m");
		sleep(1);
		writeproc("/proc/driver/KS8995M", "R01=01");
	}

	char filename2[64];

	fprintf(stderr, "Detect additional Device capabilities\n");
	sprintf(filename2, "/dev/mtdblock/%d", getMTD("RedBoot"));
	file = fopen(filename2, "r");
	if (file) {
		fseek(file, 0x1f800, SEEK_SET);
		unsigned int signature;

		fread(&signature, 4, 1, file);
		if (signature == 0x20021103) {
			fprintf(stderr, "Compex WP188 detected\n");
			eval("ifconfig", "ixp0", "0.0.0.0", "down");
			eval("ifconfig", "ixp1", "0.0.0.0", "down");
			unsigned char buf[20];

			fseek(file, 0x1f810, SEEK_SET);
			fread(&buf[0], 6, 1, file);
			char mac[20];
			int i;

			unsigned int copy[20];

			for (i = 0; i < 12; i++)
				copy[i] = buf[i] & 0xff;

			sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[0], copy[1], copy[2], copy[3], copy[4], copy[5]);
			fprintf(stderr, "configure IXP0 to %s\n", mac);
			nvram_set("et0macaddr_safe", mac);
			nvram_set("et0macaddr", mac);
			eval("ifconfig", "ixp0", "hw", "ether", mac);
			fseek(file, 0x1f818, SEEK_SET);
			fread(&buf[6], 6, 1, file);
			for (i = 0; i < 12; i++)
				copy[i] = buf[i] & 0xff;
			sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", copy[6], copy[7], copy[8], copy[9], copy[10], copy[11]);
			fprintf(stderr, "configure IXP1 to %s\n", mac);
			eval("ifconfig", "ixp1", "hw", "ether", mac);
			eval("ifconfig", "ixp0", "0.0.0.0", "up");
			eval("ifconfig", "ixp1", "0.0.0.0", "up");
		}
		fclose(file);
	}
	if (nvram_match("DD_BOARD2", "ADI Engineering Pronghorn Metro")
	    || nvram_match("DD_BOARD", "ADI Engineering Pronghorn Metro")) {
		fprintf(stderr, "Pronghorn Metro detected\n");
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "0", "-r", "npe_eth0_esa");
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "1", "-r", "npe_eth1_esa");
		struct ifreq ifr;
		int s;

		if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
			char eabuf[32];

			strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
			ioctl(s, SIOCGIFHWADDR, &ifr);
			nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			nvram_set("et0macaddr", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			close(s);
		}
	}
#ifdef HAVE_MI424WR
	{
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "0", "-r", "npe_eth0_esa");
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "1", "-r", "npe_eth1_esa");
		struct ifreq ifr;
		int s;

		if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
			char eabuf[32];

			strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
			ioctl(s, SIOCGIFHWADDR, &ifr);
			nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			nvram_set("et0macaddr", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			close(s);
		}
	}
#endif
#ifdef HAVE_USR8200
	{
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "0", "-r", "npe_eth0_esa");
		eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "1", "-r", "npe_eth1_esa");
		struct ifreq ifr;
		int s;

		if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
			char eabuf[32];

			strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
			ioctl(s, SIOCGIFHWADDR, &ifr);
			nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			nvram_set("et0macaddr", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			close(s);
		}
	}
#endif

#ifdef HAVE_WG302V1
	eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "0", "-r", "zcom_npe_esa");
	struct ifreq ifr;
	int s;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
		char eabuf[32];

		strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
		ioctl(s, SIOCGIFHWADDR, &ifr);
		nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
		nvram_set("et0macaddr", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
		close(s);
	}
#else
#ifdef HAVE_WG302
	eval("setmac", "-f", "/dev/mtdblock/7", "-n", "1", "-i", "0", "-r", "npe_eth0_esa");
	struct ifreq ifr;
	int s;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
		char eabuf[32];

		strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
		ioctl(s, SIOCGIFHWADDR, &ifr);
		nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
		nvram_set("et0macaddr", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
		close(s);
	}
#endif
#endif
#ifdef HAVE_TONZE
	{
		struct ifreq ifr;
		int s;

		if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW))) {
			char eabuf[32];

			strncpy(ifr.ifr_name, "ixp0", IFNAMSIZ);
			ioctl(s, SIOCGIFHWADDR, &ifr);
			nvram_set("et0macaddr_safe", ether_etoa((char *)ifr.ifr_hwaddr.sa_data, eabuf));
			close(s);
		}
	}
#endif

#ifdef HAVE_CAMBRIA
	if (nvram_match("DD_BOARD", "Gateworks Cambria GW2358-4")
	    || nvram_match("DD_BOARD2", "Gateworks Cambria GW2358-4")) {
		insmod("8250_gw2358");
		writeproc("/proc/sys/dev/wifi0/ledpin", "0");
		writeproc("/proc/sys/dev/wifi0/softled", "1");
		writeproc("/proc/sys/dev/wifi1/ledpin", "1");
		writeproc("/proc/sys/dev/wifi1/softled", "1");
		writeproc("/proc/sys/dev/wifi2/ledpin", "2");
		writeproc("/proc/sys/dev/wifi2/softled", "1");
		writeproc("/proc/sys/dev/wifi3/ledpin", "3");
		writeproc("/proc/sys/dev/wifi3/softled", "1");
	}
	if (nvram_match("DD_BOARD", "Gateworks Cambria GW2350")
	    || nvram_match("DD_BOARD2", "Gateworks Cambria GW2350")) {
		insmod("8250_gw2350");
	}
	set_gpio(26, 0);
	set_gpio(27, 0);
	nvram_set("gpio26", "0");
	nvram_set("gpio27", "0");
#endif

	/* cf capability ? */
	char *modelname = nvram_get("DD_BOARD2");
	if (!modelname || strncmp(modelname, "Gateworks", 9))
		modelname = nvram_safe_get("DD_BOARD");
	if (!strcmp(modelname, "Gateworks Avila GW2348-4")
	    || !strcmp(modelname, "Gateworks Cambria GW2358-4")
	    || !strcmp(modelname, "Gateworks Avila GW2355")
	    || !strcmp(modelname, "Gateworks Avila GW2345")) {
		fprintf(stderr, "Load CF Card Driver\n");
		insmod("scsi_mod");
		insmod("scsi_wait_scan");
		insmod("sd_mod");
		insmod("libata");
		insmod("pata_ixp4xx_cf");
	}

	/* watchdog type */
	fprintf(stderr, "Load Hardware Watchdog\n");
	insmod("ixp4xx_wdt");
	fprintf(stderr, "blink led\n");
	eval("ledtool", "4", "1");	// blink the led 4 times

	fprintf(stderr, "Enable Watchdog\n");
	if (!nvram_match("disable_watchdog", "1"))
		eval("watchdog");	// system watchdog

	/*
	 * Set a sane date 
	 */
	stime(&tm);
	nvram_set("wl0_ifname", "ath0");
#ifndef HAVE_NOP8670
	eval("hwclock", "-s");
#endif
	nvram_set("use_crypto", "0");
	cprintf("done\n");
	eval("/bin/tar", "-xzf", "/dev/mtdblock/4", "-C", "/");
	FILE *in = fopen("/tmp/nvram/nvram.db", "rb");

	if (in != NULL) {
		fclose(in);
		eval("/usr/sbin/convertnvram");
		eval("/sbin/mtd", "erase", "nvram");
		nvram_commit();
	}
	setWirelessLedGeneric(0, 4);
	setWirelessLedGeneric(1, 5);
	setWirelessLedGeneric(2, 6);
	setWirelessLedGeneric(3, 7);
	return;
}

int check_cfe_nv(void)
{
	nvram_set("portprio_support", "0");
	return 0;
}

int check_pmon_nv(void)
{
	return 0;
}

void start_overclocking(void)
{
}

void enable_dtag_vlan(int enable)
{

}
