#
# Documentation makefile for libcups.
#
# Copyright © 2023 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs

#
# Documentation files...
#

DOCFILES	=	\
			cancel.html \
			client.conf.html \
			cups.html \
			cups.png \
			cupsaccept.html \
			cupsenable.html \
			index.html \
			lp.html \
			lpadmin.html \
			lpc.html \
			lpmove.html \
			lpoptions.html \
			lpq.html \
			lpr.html \
			lprm.html \
			lpstat.html


#
# Make all documents...
#

all:


#
# Make library targets...
#

libs:


#
# Run unit tests...
#

test:


#
# Make unit tests...
#

unittests:


#
# Remove all generated files...
#

clean:


#
# Dummy depend target...
#

depend:


#
# Install all documentation files...
#

install:
	echo "Installing documentation files to $(BUILDROOT)$(datadir)/doc/cups-local..."
	$(INSTALL_DIR) $(BUILDROOT)$(datadir)/doc/cups-local
	for file in $(DOCFILES); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(datadir)/doc/cups-local; \
	done


#
# Uninstall all documentation files...
#

uninstall:
	echo "Uninstalling documentation files from $(BUILDROOT)$(datadir)/doc/cups-local..."
	for file in $(DOCFILES); do \
		$(RM) $(BUILDROOT)$(datadir)/doc/cups-local/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(datadir)/doc/cups-local
