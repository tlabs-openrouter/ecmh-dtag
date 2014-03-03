/*****************************************************
 ecmh - Easy Cast du Multi Hub - Common Functions
******************************************************
 $Author: fuzzel $
 $Id: common.h,v 1.2 2005/02/09 17:58:06 fuzzel Exp $
 $Date: 2005/02/09 17:58:06 $
*****************************************************/

void dolog(int level, const char *fmt, ...);
void log_ip6addr(int log_level, const struct in6_addr *addr);
void log_grp(int log_level, const char *text, const struct in6_addr *src, const struct in6_addr *grp);

int huprunning(void);
void savepid(void);
void cleanpid(int i);
