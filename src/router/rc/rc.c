
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <dirent.h>

#include <epivers.h>
#include <bcmnvram.h>
#include <shutils.h>
#include <rc.h>
#include <netconf.h>
#include <nvparse.h>
#include <bcmdevs.h>

#include <wlutils.h>
#include <utils.h>
#include <cyutils.h>
#include <code_pattern.h>
#include <cy_conf.h>
#include <typedefs.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <shutils.h>
#include <wlutils.h>
#include <cy_conf.h>
#include <arpa/inet.h>

#include <revision.h>
#include "servicemanager.c"
#include "services.c"
#include "mtd.c"
#include "mtd_main.c"
#ifdef HAVE_PPTPD
#include "pptpd.c"
#endif

#if defined(HAVE_UQMI) || defined(HAVE_LIBQMI)
static void check_qmi(void)
{
#ifdef HAVE_UQMI
	int clientid = 0;
	FILE *fp = fopen("/tmp/qmi-clientid", "rb");
	if (fp) {
		fscanf(fp, "%d", &clientid);
		fclose(fp);
		fp = fopen("/tmp/qmistatus.sh", "wb");
		fprintf(fp, "#!/bin/sh\n");
		fprintf(fp, "uqmi -d /dev/cdc-wdm0 --set-client-id wds,%d --keep-client-id wds --get-serving-system|grep registered|wc -l>/tmp/qmistatus\n", clientid);
		fclose(fp);
		chmod("/tmp/qmistatus.sh", 0700);
		eval("/tmp/qmistatus.sh");
	} else {
		sysprintf("echo 0 > /tmp/qmistatus");
	}
#else
	sysprintf("qmi-network /dev/cdc-wdm0 status|grep disconnected|wc -l>/tmp/qmistatus");
#endif
}
#endif

#ifdef HAVE_IPV6
int dhcp6c_state_main(int argc, char **argv)
{
	char prefix[INET6_ADDRSTRLEN];
	struct in6_addr addr;
	int i, r;

	nvram_set("ipv6_rtr_addr", getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0));

	// extract prefix from configured IPv6 address
	if (inet_pton(AF_INET6, nvram_safe_get("ipv6_rtr_addr"), &addr) > 0) {

		r = atoi(nvram_safe_get("ipv6_pf_len")) ? : 64;
		for (r = 128 - r, i = 15; r > 0; r -= 8) {
			if (r >= 8)
				addr.s6_addr[i--] = 0;
			else
				addr.s6_addr[i--] &= (0xff << r);
		}
		inet_ntop(AF_INET6, &addr, prefix, sizeof(prefix));

		nvram_set("ipv6_prefix", prefix);
	}

	nvram_set("ipv6_get_dns", getenv("new_domain_name_servers"));
	nvram_set("ipv6_get_domain", getenv("new_domain_name"));
	nvram_set("ipv6_get_sip_name", getenv("new_sip_name"));
	nvram_set("ipv6_get_sip_servers", getenv("new_sip_servers"));

	dns_to_resolv();

	eval("stopservice", "radvd", "-f");
	eval("startservice", "radvd", "-f");
	eval("stopservice", "dhcp6s", "-f");
	eval("startservice", "dhcp6s", "-f");
	return 0;
}
#endif
/* 
 * Call when keepalive mode
 */
int redial_main(int argc, char **argv)
{
	int need_redial = 0;
	int status;
	pid_t pid;
	int count = 1;
	int num;

	while (1) {
#if defined(HAVE_UQMI) || defined(HAVE_LIBQMI)
		if (nvram_match("wan_proto", "3g")
		    && nvram_match("3gdata", "qmi") && count == 1) {
			check_qmi();
		}
#endif
		sleep(atoi(argv[1]));
		num = 0;
		count++;

#if defined(HAVE_UQMI) || defined(HAVE_LIBQMI)
		if (nvram_match("wan_proto", "3g")
		    && nvram_match("3gdata", "qmi")) {
			check_qmi();
		}
#endif

		// fprintf(stderr, "check PPPoE %d\n", num);
		if (!check_wan_link(num)) {
			// fprintf(stderr, "PPPoE %d need to redial\n", num);
			need_redial = 1;
		} else {
			// fprintf(stderr, "PPPoE %d not need to redial\n", num);
			continue;
		}

#if 0
		cprintf("Check pppx if exist: ");
		if ((fp = fopen("/proc/net/dev", "r")) == NULL) {
			return -1;
		}

		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strstr(line, "ppp")) {
				match = 1;
				break;
			}
		}
		fclose(fp);
		cprintf("%s", match == 1 ? "have exist\n" : "ready to dial\n");
#endif

		if (need_redial) {
			pid = fork();
			switch (pid) {
			case -1:
				perror("fork failed");
				exit(1);
			case 0:
#ifdef HAVE_PPPOE
				if (nvram_match("wan_proto", "pppoe")) {
					sleep(1);
					start_service_force("wan_redial");
				}
#if defined(HAVE_PPTP) || defined(HAVE_L2TP) || defined(HAVE_HEARTBEAT) || defined(HAVE_PPPOATM) || defined(HAVE_PPPOEDUAL)
				else
#endif
#endif

#ifdef HAVE_PPPOEDUAL
				if (nvram_match("wan_proto", "pppoe_dual")) {
					sleep(1);
					start_service_force("wan_redial");
				}
#if defined(HAVE_PPTP) || defined(HAVE_L2TP) || defined(HAVE_HEARTBEAT) || defined(HAVE_PPPOATM)
				else
#endif
#endif

#ifdef HAVE_PPPOATM
				if (nvram_match("wan_proto", "pppoa")) {
					sleep(1);
					start_service_force("wan_redial");
				}
#if defined(HAVE_PPTP) || defined(HAVE_L2TP) || defined(HAVE_HEARTBEAT)
				else
#endif
#endif

#ifdef HAVE_PPTP
				if (nvram_match("wan_proto", "pptp")) {
					stop_service_force("pptp");
					unlink("/tmp/services/pptp.stop");
					sleep(1);
					start_service_force("wan_redial");
				}
#if defined(HAVE_L2TP) || defined(HAVE_HEARTBEAT)
				else
#endif
#endif
#ifdef HAVE_L2TP
				if (nvram_match("wan_proto", "l2tp")) {
					stop_service_force("l2tp");
					unlink("/tmp/services/l2tp.stop");
					sleep(1);
					start_service_force("wan_redial");
				}
#ifdef HAVE_HEARTBEAT
				else
#endif
#endif
					// Moded by Boris Bakchiev
					// We dont need this at all.
					// But if this code is executed by any of pppX programs
					// we might have to do this.

#ifdef HAVE_HEARTBEAT
				if (nvram_match("wan_proto", "heartbeat")) {
					if (is_running("bpalogin") == 0) {
						stop_service_force("heartbeat_redial");
						sleep(1);
						start_service_force("heartbeat_redial");
					}

				}
#endif
#ifdef HAVE_3G
				else if (nvram_match("wan_proto", "3g")) {
					sleep(1);
					start_service_force("wan_redial");
				}
#endif
#ifdef HAVE_IPETH
				else if (nvram_match("wan_proto", "iphone")) {
					sleep(1);
					start_service_force("wan_redial");
				}
#endif
				exit(0);
				break;
			default:
				waitpid(pid, &status, 0);
				// dprintf("parent\n");
				break;
			}	// end switch
		}		// end if
	}			// end while
}				// end main

int get_wanface(int argc, char **argv)
{
	fprintf(stdout, "%s", get_wan_face());
	return 0;
}

int get_nfmark(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "usage: get_nfmark <service> <mark>\n\n" "	services: FORWARD\n" "		  HOTSPOT\n" "		  QOS\n\n" "	eg: get_nfmark QOS 10\n");
		return 1;
	}

	fprintf(stdout, "%s\n", get_NFServiceMark(argv[1], atol(argv[2])));
}

int gratarp(int argc, char **argv)
{

	signal(SIGCHLD, SIG_IGN);

	pid_t pid;

	if (argc < 2) {
		fprintf(stderr, "usage: gratarp <interface>\n");
		return 1;
	}

	pid = fork();
	switch (pid) {
	case -1:
		perror("fork failed");
		exit(1);
		break;
	case 0:
		gratarp_main(argv[1]);
		return 0;
		break;
	default:
		//waitpid(pid, &status, 0);
		// dprintf("parent\n");
		break;
	}

	return 0;
}

struct MAIN {
	char *callname;
	char *execname;
	int (*exec) (int argc, char **argv);
};
extern char *getSoftwareRevision(void);
int softwarerevision_main(int argc, char **argv)
{
	fprintf(stdout, "%s\n", getSoftwareRevision());
	return 0;
}

static struct MAIN maincalls[] = {
	// {"init", NULL, &main_loop},
	{"ip-up", "ipup", NULL},
	{"ip-down", "ipdown", NULL},
	{"ipdown", "disconnected_pppoe", NULL},
	{"udhcpc_tv", "udhcpc_tv", NULL},
	{"udhcpc", "udhcpc", NULL},
	{"mtd", NULL, mtd_main},
#ifdef HAVE_PPTPD
	{"poptop", NULL, &pptpd_main},
#endif
	{"redial", NULL, &redial_main},
#ifndef HAVE_RB500
	// {"resetbutton", NULL, &resetbutton_main},
#endif
	// {"wland", NULL, &wland_main},
	{"hb_connect", "hb_connect", NULL},
	{"hb_disconnect", "hb_disconnect", NULL},
	{"gpio", "gpio", NULL},
	{"beep", "beep", NULL},
	{"ledtracking", "ledtracking", NULL},
	// {"listen", NULL, &listen_main},
	// {"check_ps", NULL, &check_ps_main},
	{"ddns_success", "ddns_success", NULL},
	// {"process_monitor", NULL, &process_monitor_main},
	// {"radio_timer", NULL, &radio_timer_main},
	// {"ttraf", NULL, &ttraff_main},
#ifdef HAVE_WIVIZ
	{"run_wiviz", NULL, &run_wiviz_main},
	{"autokill_wiviz", NULL, &autokill_wiviz_main},
#endif
	{"site_survey", "site_survey", NULL},
#ifdef HAVE_WOL
	{"wol", NULL, &wol_main},
#endif
	{"event", NULL, &event_main},
//    {"switch", "switch", NULL},
#ifdef HAVE_MICRO
	{"brctl", "brctl", NULL},
#endif
	{"getbridgeprio", "getbridgeprio", NULL},
#ifdef HAVE_NORTHSTAR
	{"rtkswitch", "rtkswitch", NULL},
#endif
	{"setuserpasswd", "setuserpasswd", NULL},
	{"getbridge", "getbridge", NULL},
	{"getmask", "getmask", NULL},
	{"stopservices", NULL, stop_services_main},
#ifdef HAVE_PPPOESERVER
	{"addpppoeconnected", "addpppoeconnected", NULL},
	{"delpppoeconnected", "delpppoeconnected", NULL},
	{"addpppoetime", "addpppoetime", NULL},
#endif
	{"startservices", NULL, start_services_main},
	{"start_single_service", NULL, start_single_service_main},
	{"startstop_f", NULL, startstop_main_f},
	{"startstop", NULL, startstop_main},
	{"stop_running", NULL, stop_running_main},
	{"softwarerevision", NULL, softwarerevision_main},
#if !defined(HAVE_MICRO) || defined(HAVE_ADM5120) || defined(HAVE_WRK54G)
	{"watchdog", NULL, &watchdog_main},
#endif
	// {"nvram", NULL, &nvram_main},
#ifdef HAVE_ROAMING
//      {"roaming_daemon", NULL, &roaming_daemon_main},
	{"supplicant", "supplicant", NULL},
#endif
	{"get_wanface", NULL, &get_wanface},
#ifndef HAVE_XSCALE
	// {"ledtool", NULL, &ledtool_main},
#endif
#ifdef HAVE_REGISTER
	{"regshell", NULL, &reg_main},
#endif
	{"gratarp", NULL, &gratarp},
	{"get_nfmark", NULL, &get_nfmark},
#ifdef HAVE_IPV6
	{"dhcp6c-state", NULL, &dhcp6c_state_main},
#endif
};

int main(int argc, char **argv)
{
	char *base = strrchr(argv[0], '/');
	base = base ? base + 1 : argv[0];
	int i;
	for (i = 0; i < sizeof(maincalls) / sizeof(struct MAIN); i++) {
		if (strstr(base, maincalls[i].callname)) {
			if (maincalls[i].execname)
				return start_main(maincalls[i].execname, argc, argv);
			if (maincalls[i].exec)
				return maincalls[i].exec(argc, argv);
		}
	}

	if (strstr(base, "startservice_f")) {
		if (argc < 2) {
			puts("try to be professional\n");
			return 0;
		}
		if (argc == 3 && !strcmp(argv[2], "-f"))
			start_service_force_f(argv[1]);
		else
			start_service_f(argv[1]);
		return 0;
	}
	if (strstr(base, "startservice")) {
		if (argc < 2) {
			puts("try to be professional\n");
			return 0;
		}
		if (argc == 3 && !strcmp(argv[2], "-f"))
			start_service_force(argv[1]);
		else
			start_service(argv[1]);
		return 0;
	}

	if (strstr(base, "stopservice_f")) {
		if (argc < 2) {
			puts("try to be professional\n");
			return 0;
		}
		if (argc == 3 && !strcmp(argv[2], "-f"))
			stop_service_force_f(argv[1]);
		else
			stop_service_f(argv[1]);
		return 0;
	}

	if (strstr(base, "stopservice")) {
		if (argc < 2) {
			puts("try to be professional\n");
			return 0;
		}
		if (argc == 3 && !strcmp(argv[2], "-f"))
			stop_service_force(argv[1]);
		else
			stop_service(argv[1]);
		return 0;
	}
#ifndef HAVE_RB500
#ifndef HAVE_X86
	/* 
	 * erase [device] 
	 */
	if (strstr(base, "erase")) {
		int brand = getRouterBrand();

		if (brand == ROUTER_MOTOROLA || brand == ROUTER_MOTOROLA_V1 || brand == ROUTER_MOTOROLA_WE800G || brand == ROUTER_RT210W || brand == ROUTER_BUFFALO_WZRRSG54)	// these 
			// 
			// 
			// 
			// 
			// 
			// 
			// 
			// routers 
			// have 
			// problem 
			// erasing 
			// nvram,
			// we 
			// only 
			// software 
			// restore 
			// defaults
		{
			if (argv[1] && strcmp(argv[1], "nvram")) {
				fprintf(stderr, "Sorry, erasing nvram will get this router unuseable\n");
				return 0;
			}
		} else {
			if (argv[1])
				return mtd_erase(argv[1]);
			else {
				fprintf(stderr, "usage: erase [device]\n");
				return EINVAL;
			}
		}
		return 0;
	}

	/* 
	 * write [path] [device] 
	 */
	if (strstr(base, "write")) {
		if (argc >= 3)
			return mtd_write(argv[1], argv[2]);
		else {
			fprintf(stderr, "usage: write [path] [device]\n");
			return EINVAL;
		}
	}
#else
	if (strstr(base, "erase")) {
		if (argv[1] && strcmp(argv[1], "nvram")) {
			fprintf(stderr, "Erasing configuration data...\n");
			eval("mount", "/usr/local", "-o", "remount,rw");
			eval("rm", "-f", "/tmp/nvram/*");	// delete nvram database
			unlink("/tmp/nvram/.lock");	// delete nvram
			// database
			eval("rm", "-f", "/etc/nvram/*");	// delete nvram database
			eval("mount", "/usr/local", "-o", "remount,ro");
		}
		return 0;
	}
#endif
#endif

	// ////////////////////////////////////////////////////
	// 
	if (strstr(base, "filtersync")) {
		startstop("filtersync");
		return 0;
	}
	/* 
	 * filter [add|del] number 
	 */
	if (strstr(base, "filter")) {
		if (argv[1] && argv[2]) {
			int num = 0;

			if ((num = atoi(argv[2])) > 0) {
				if (strcmp(argv[1], "add") == 0) {
					start_servicei("filter_add", num);
					return 0;
				} else if (strcmp(argv[1], "del") == 0) {
					start_servicei("filter_del", num);
					return 0;
				}
			}
		} else {
			fprintf(stderr, "usage: filter [add|del] number\n");
			return EINVAL;
		}
		return 0;
	}

	if (strstr(base, "restart_dns")) {
		stop_service("dnsmasq");
		stop_service("udhcpd");
		start_service("udhcpd");
		start_service("dnsmasq");
		return 0;
	}
	if (strstr(base, "setpasswd")) {
		startstop("mkfiles");
		return 0;
	}
	/* 
	 * rc [stop|start|restart ] 
	 */
	else if (strstr(base, "rc")) {
		if (argv[1]) {
			if (strncmp(argv[1], "start", 5) == 0)
				return kill(1, SIGUSR2);
			else if (strncmp(argv[1], "stop", 4) == 0)
				return kill(1, SIGINT);
			else if (strncmp(argv[1], "restart", 7) == 0)
				return kill(1, SIGHUP);
		} else {
			fprintf(stderr, "usage: rc [start|stop|restart]\n");
			return EINVAL;
		}
	}
	return 1;
}
