#
# Local server makefile for cups-local.
#
# Copyright © 2023 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs


TARGETS	=	\
		cupslocald
OBJS	=	\
		main.o \
		dbus.o \
		drivers.o \
		transform.o


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


#
# Update dependencies (without system header dependencies...)
#

depend:
	$(CC) -MM $(CPPFLAGS) $(OBJS:.o=.c) | sed -e '1,$$s/ \/usr\/include\/[^ ]*//g' -e '1,$$s/ \/usr\/local\/include\/[^ ]*//g' >Dependencies


#
# Install all targets...
#

install:
	echo Installing cupslocald to $(BUILDROOT)$(sbindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) cupslocald $(BUILDROOT)$(sbindir)
	if test "x$(DBUSDIR)" != x; then \
		echo Installing D-Bus files to $(BUILDROOT)$(DBUSDIR); \
		$(INSTALL_DIR) -m 755 $(BUILDROOT)$(DBUSDIR)/interfaces; \
		$(INSTALL_DATA) org.openprinting.cupslocald.xml $(BUILDROOT)$(DBUSDIR)/interfaces; \
		$(INSTALL_DIR) -m 755 $(BUILDROOT)$(DBUSDIR)/services; \
		$(INSTALL_DATA) cupslocald-dbus.service $(BUILDROOT)$(DBUSDIR)/services/cupslocald.service; \
	fi
	if test "x$$(uname)" = xDarwin; then \
		echo Installing launchd files to $(BUILDROOT)/Library/LaunchAgents; \
		$(INSTALL_DIR) -m 755 $(BUILDROOT)/Library/LaunchAgents; \
		$(INSTALL_DATA) org.openprinting.cupslocald.plist $(BUILDROOT)/Library/LaunchAgents; \
	elif test "x$(SYSTEMDDIR)" != x; then \
		echo Installing systemd files to $(BUILDROOT)$(SYSTEMDDIR); \
		$(INSTALL_DIR) -m 755 $(BUILDROOT)$(SYSTEMDDIR)/user; \
		$(INSTALL_DATA) cupslocald-systemd.service $(BUILDROOT)$(SYSTEMDDIR)/user/cupslocald.service; \
	fi


#
# Uninstall all targets...
#

uninstall:
	$(RM) $(BUILDROOT)$(sbindir)/cupslocald
	-$(RMDIR) $(BUILDROOT)$(sbindir)
	if test "x$(DBUSDIR)" != x; then \
		$(RM) $(BUILDROOT)$(DBUSDIR)/interfaces/org.openprinting.cupslocald.xml; \
		$(RMDIR) $(BUILDROOT)$(DBUSDIR)/interfaces; \
		$(RM) $(BUILDROOT)$(DBUSDIR)/services/cupslocald.service; \
		$(RMDIR) $(BUILDROOT)$(DBUSDIR)/services; \
	fi
	if test "x$$(uname)" = xDarwin; then \
		$(RM) $(BUILDROOT)/Library/LaunchAgents/org.openprinting.cupslocald.plist; \
		$(RMDIR) $(BUILDROOT)/Library/LaunchAgents; \
	elif test "x$(SYSTEMDDIR)" != x; then \
		$(RM) $(BUILDROOT)$(SYSTEMDDIR)/user/cupslocald.service; \
		$(RMDIR) $(BUILDROOT)$(SYSTEMDDIR)/user; \
	fi


#
# cupslocald
#

cupslocald:	$(OBJS)
	echo Linking $@...
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	$(CODE_SIGN) -s "$(CODESIGN_IDENTITY)" $@


#
# iconsh - all of the PNG icons as a header file...
#

.PHONY:	iconsh
iconsh:
	echo Generating icons.h...
	pappl-makeresheader *.png >icons.h


#
# Dependencies...
#

include Dependencies
