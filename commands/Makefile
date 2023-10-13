#
# Commands makefile for the CUPS commands.
#
# Copyright © 2022 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs


TARGETS	=	\
		cancel \
		cupsaccept \
		lp \
		lpadmin \
		lpc \
		lpmove \
		lpoptions \
		lpq \
		lpr \
		lprm \
		lpstat
OBJS	=	\
		cancel.o \
		cupsaccept.o \
		lp.o \
		lpadmin.o \
		lpc.o \
		lpmove.o \
		lpoptions.o \
		lpq.o \
		lpr.o \
		lprm.o \
		lpstat.o


#
# Make all targets...
#

all:	$(TARGETS)


#
# Make unit tests...
#

test:


#
# Clean all object files...
#

clean:
	$(RM) $(OBJS) $(TARGETS)
	$(RM) cupsdisable cupsenable cupsreject


#
# Update dependencies (without system header dependencies...)
#

depend:
	$(CC) -MM $(CPPFLAGS) $(OBJS:.o=.c) >Dependencies


#
# Install all targets...
#

install:
	echo Installing admin commands to $(BUILDROOT)$(sbindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) cupsaccept $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) lpadmin $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) lpc $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) lpmove $(BUILDROOT)$(sbindir)
	$(RM) $(BUILDROOT)$(sbindir)/cupsdisable
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsdisable
	$(RM) $(BUILDROOT)$(sbindir)/cupsenable
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsenable
	$(RM) $(BUILDROOT)$(sbindir)/cupsreject
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsreject
	echo Installing user commands tp $(BUILDROOT)$(bindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) cancel $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lp $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lpoptions $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lpq $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lpr $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lprm $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lpstat $(BUILDROOT)$(bindir)


#
# Uninstall all targets...
#

uninstall:
	$(RM) $(BUILDROOT)$(bindir)/cancel
	$(RM) $(BUILDROOT)$(bindir)/lp
	$(RM) $(BUILDROOT)$(bindir)/lpoptions
	$(RM) $(BUILDROOT)$(bindir)/lpq
	$(RM) $(BUILDROOT)$(bindir)/lpr
	$(RM) $(BUILDROOT)$(bindir)/lprm
	$(RM) $(BUILDROOT)$(bindir)/lpstat
	-$(RMDIR) $(BUILDROOT)$(bindir)
	$(RM) $(BUILDROOT)$(sbindir)/cupsaccept
	$(RM) $(BUILDROOT)$(sbindir)/cupsdisable
	$(RM) $(BUILDROOT)$(sbindir)/cupsenable
	$(RM) $(BUILDROOT)$(sbindir)/cupsreject
	$(RM) $(BUILDROOT)$(sbindir)/lpadmin
	$(RM) $(BUILDROOT)$(sbindir)/lpc
	$(RM) $(BUILDROOT)$(sbindir)/lpmove
	-$(RMDIR) $(BUILDROOT)$(sbindir)


#
# cancel
#

cancel:	cancel.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o cancel cancel.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# cupsaccept
#

cupsaccept:	cupsaccept.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o cupsaccept cupsaccept.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@
	for file in cupsenable cupsdisable cupsreject; do \
		$(RM) $$file; \
		$(LN) cupsaccept $$file; \
	done


#
# lp
#

lp:	lp.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lp lp.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpadmin
#

lpadmin:	lpadmin.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpadmin lpadmin.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpc
#

lpc:	lpc.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpc lpc.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpmove
#

lpmove:	lpmove.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpmove lpmove.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpoptions
#

lpoptions:	lpoptions.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpoptions lpoptions.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpq
#

lpq:	lpq.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpq lpq.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpr
#

lpr:	lpr.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpr lpr.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lprm
#

lprm:	lprm.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lprm lprm.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# lpstat
#

lpstat:	lpstat.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpstat lpstat.o $(LIBS)
	$(CODE_SIGN) -s "$(CODE_SIGN_IDENTITY)" $@


#
# Dependencies...
#

include Dependencies
