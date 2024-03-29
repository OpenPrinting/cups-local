#
# Commands makefile for cups-local.
#
# Copyright © 2022-2024 by OpenPrinting.
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
		lpq \
		lpstat
#		lpc
#		lpmove
#		lpoptions
OBJS	=	\
		cancel.o \
		cupsaccept.o \
		lp.o \
		lpadmin.o \
		lpq.o \
		lpstat.o
#		lpc.o
#		lpmove.o
#		lpoptions.o


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
	$(RM) cupsdisable cupsenable cupsreject lpr lprm


#
# Update dependencies (without system header dependencies...)
#

depend:
	$(CC) -MM $(CPPFLAGS) $(OBJS:.o=.c) | sed -e '1,$$s/ \/usr\/include\/[^ ]*//g' -e '1,$$s/ \/usr\/local\/include\/[^ ]*//g' >Dependencies


#
# Install all targets...
#

install:
	echo Installing user commands tp $(BUILDROOT)$(bindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) cancel $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lp $(BUILDROOT)$(bindir)
#	$(INSTALL_BIN) lpoptions $(BUILDROOT)$(bindir)
	$(INSTALL_BIN) lpstat $(BUILDROOT)$(bindir)
	$(RM) $(BUILDROOT)$(bindir)/lpq
	$(LN) lpstat $(BUILDROOT)$(bindir)/lpq
	$(RM) $(BUILDROOT)$(bindir)/lpr
	$(LN) lp $(BUILDROOT)$(bindir)/lpr
	$(RM) $(BUILDROOT)$(bindir)/lprm
	$(LN) cancel $(BUILDROOT)$(bindir)/lprm
	echo Installing admin commands to $(BUILDROOT)$(sbindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) cupsaccept $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) lpadmin $(BUILDROOT)$(sbindir)
#	$(INSTALL_BIN) lpc $(BUILDROOT)$(sbindir)
#	$(INSTALL_BIN) lpmove $(BUILDROOT)$(sbindir)
	$(RM) $(BUILDROOT)$(sbindir)/cupsdisable
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsdisable
	$(RM) $(BUILDROOT)$(sbindir)/cupsenable
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsenable
	$(RM) $(BUILDROOT)$(sbindir)/cupsreject
	$(LN) cupsaccept $(BUILDROOT)$(sbindir)/cupsreject


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
#	$(RM) $(BUILDROOT)$(sbindir)/lpc
#	$(RM) $(BUILDROOT)$(sbindir)/lpmove
	-$(RMDIR) $(BUILDROOT)$(sbindir)


#
# cancel
#

cancel:	cancel.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o cancel cancel.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# cupsaccept
#

cupsaccept:	cupsaccept.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o cupsaccept cupsaccept.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@
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
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpadmin
#

lpadmin:	lpadmin.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpadmin lpadmin.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpc
#

lpc:	lpc.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpc lpc.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpmove
#

lpmove:	lpmove.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpmove lpmove.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpoptions
#

lpoptions:	lpoptions.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpoptions lpoptions.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpq
#

lpq:	lpq.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpq lpq.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# lpstat
#

lpstat:	lpstat.o
	echo Linking $@...
	$(CC) $(LDFLAGS) -o lpstat lpstat.o $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# Dependencies...
#

include Dependencies
