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
		cups-locald
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
	echo Installing cups-locald to $(BUILDROOT)$(sbindir)
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(sbindir)
	$(INSTALL_BIN) cups-locald $(BUILDROOT)$(sbindir)


#
# Uninstall all targets...
#

uninstall:
	$(RM) $(BUILDROOT)$(sbindir)/cups-locald
	-$(RMDIR) $(BUILDROOT)$(sbindir)


#
# cups-locald
#

cups-locald:	$(OBJS)
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
