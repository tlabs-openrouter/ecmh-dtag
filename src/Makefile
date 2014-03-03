# /**************************************
#  ecmh - Easy Cast du Multi Hub
#  by Jeroen Massar <jeroen@unfix.org>
# ***************************************
# $Author: fuzzel $
# $Id: Makefile,v 1.8 2005/02/09 17:58:06 fuzzel Exp $
# $Date: 2005/02/09 17:58:06 $
# **************************************/
#
# Source Makefile for ecmh - Jeroen Massar <jeroen@unfix.org>
#
# ECMH_VERSION and ECMH_OPTIONS need to be defined, gets done by toplevel Makefile
#
# One should make this using the main Makefile (thus one dir up)
#
# FreeBSD people should uncomment these if they don't have GNU make and
# then run 'make' from this directory
# On FreeBSD4 install the "libgnugetopt" port to get getopt working
#ECMH_OPTIONS=-DECMH_BPF -DECMH_SUPPORT_MLD2 -DECMH_GETIFADDR -I/usr/local/include/
#ECMH_VERSION=2004.10.09
#ECMH_LDFREEBSD=-L/usr/local/lib/ -lgnugetopt

# Below here nothing should have to be changed
BINS	= ecmh
SRCS	= ecmh.c linklist.c common.c interfaces.c groups.c grpint.c subscr.c mrec.c msrc.c mcast_client.c mld2_logic.c kernel_routing.c
INCS	= ecmh.h linklist.h common.h interfaces.h groups.h grpint.h subscr.h mld.h mrec.h msrc.h mcast_client.h mld2.h mld2_logic.h kernel_routing.h
DEPS	= ../Makefile Makefile
OBJS	= ecmh.o linklist.o common.o interfaces.o groups.o grpint.o subscr.o mrec.o msrc.o mcast_client.o mld2_logic.o kernel_routing.o
WARNS	= -W -Wall -pedantic -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wbad-function-cast -fshort-enums -fstrict-aliasing -fno-common -Wpacked -Wpadded
EXTRA   = -g
#EXTRA	= -O3
CFLAGS	+= $(WARNS) $(EXTRA) -D_GNU_SOURCE -D'ECMH_VERSION="$(ECMH_VERSION)"' $(ECMH_OPTIONS)
LDFLAGS	+= $(ECMH_LDFREEBSD)
#CC	:= @echo "[Compiling] $$@"; $(CC)
STRIP	?= strip

all:	$(BINS)

ecmh:	$(OBJS) ${INCS} ${DEPS}
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
#	$(STRIP) $@

clean:
	$(RM) -f $(OBJS) $(BINS)

# Mark targets as phony
.PHONY : all clean

