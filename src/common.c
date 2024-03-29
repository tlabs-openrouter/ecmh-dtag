/*****************************************************
 ecmh - Easy Cast du Multi Hub - Common Functions
******************************************************
 $Author: fuzzel $
 $Id: common.c,v 1.3 2005/02/09 17:58:06 fuzzel Exp $
 $Date: 2005/02/09 17:58:06 $
*****************************************************/

#include "ecmh.h"

void dolog(int level, const char *fmt, ...)
{
	va_list ap;
	if (g_conf && !g_conf->verbose && level == LOG_DEBUG) return;
	va_start(ap, fmt);
	if (g_conf && g_conf->daemonize) vsyslog(LOG_LOCAL7|level, fmt, ap);
	else
	{
		if (g_conf->verbose)
		{
			printf("[%6s] ",
				level == LOG_DEBUG ?	"debug" :
				(level == LOG_ERR ?	"error" :
				(level == LOG_WARNING ?	"warn" :
				(level == LOG_INFO ?	"info" : ""))));
		}
		vprintf(fmt, ap);
	}
	va_end(ap);
}

void log_grp(int log_level, const char *text, const struct in6_addr *src, const struct in6_addr *grp) {
    char addr1[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, src, addr1, sizeof(addr1));
    char addr2[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, grp, addr2, sizeof(addr2));
    dolog(log_level, "%s %i: %s: src=%s grp=%s\n", __FILE__, __LINE__, text, addr1, addr2);
}

void log_ip6addr(int log_level, const struct in6_addr *addr) {
    char addr1[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, addr, addr1, sizeof(addr1));
    dolog(log_level, "%s\n", addr1);
}

int huprunning()
{
	int pid;

	FILE *f = fopen(PIDFILE, "r");
	if (!f) return 0;
	fscanf(f, "%d", &pid);
	fclose(f);
	/* If we can HUP it, it still runs */
	return (kill(pid, SIGHUP) == 0 ? 1 : 0);
}

void savepid()
{
	FILE *f = fopen(PIDFILE, "w");
	if (!f) return;
	fprintf(f, "%d", getpid());
	fclose(f);

	dolog(LOG_INFO, "Running as PID %d\n", getpid());
}

void cleanpid(int i)
{
	dolog(LOG_INFO, "Trying to exit, got signal %d...\n", i);
	unlink(PIDFILE);
	g_conf->quit = true;
}

