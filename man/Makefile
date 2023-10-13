#
# Man page makefile for cups-local.
#
# Copyright © 2023 by OpenPrinting.
#
# Licensed under Apache License v2.0.  See the file "LICENSE" for more
# information.
#

include ../Makedefs


#
# Man pages...
#

MAN1	=	\
		cancel.1 \
		cups.1 \
		lp.1 \
		lpoptions.1 \
		lpq.1 \
		lpr.1 \
		lprm.1 \
		lpstat.1
MAN5	=	\
		client.conf.5
MAN8	=	\
		cupsaccept.8 \
		cupsenable.8 \
		lpadmin.8 \
		lpc.8 \
		lpmove.8


#
# Make everything...
#

all:		$(LOCALTARGET)


#
# Run unit tests...
#

test:


#
# Clean all config and object files...
#

clean:


#
# Dummy depend target...
#

depend:


#
# Install all targets...
#

install:
	echo "Installing man pages in $(BUILDROOT)$(mandir)/man1..."
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(mandir)/man1
	for file in $(MAN1); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(mandir)/man1; \
	done
	echo "Installing man pages in $(BUILDROOT)$(mandir)/man5..."
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(mandir)/man5
	for file in $(MAN5); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(mandir)/man5; \
	done
	echo "Installing man pages in $(BUILDROOT)$(mandir)/man8..."
	$(INSTALL_DIR) -m 755 $(BUILDROOT)$(mandir)/man8
	for file in $(MAN8); do \
		$(INSTALL_MAN) $$file $(BUILDROOT)$(mandir)/man8; \
	done


#
# Uninstall files...
#

uninstall:
	echo "Uninstalling man pages from $(BUILDROOT)$(mandir)/man1..."
	for file in $(MAN1); do \
		$(RM) $(BUILDROOT)$(mandir)/man1/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(mandir)/man1
	echo "Uninstalling man pages from $(BUILDROOT)$(mandir)/man5..."
	for file in $(MAN5); do \
		$(RM) $(BUILDROOT)$(mandir)/man5/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(mandir)/man5
	echo "Uninstalling man pages from $(BUILDROOT)$(mandir)/man8..."
	for file in $(MAN8); do \
		$(RM) $(BUILDROOT)$(mandir)/man8/$$file; \
	done
	-$(RMDIR) $(BUILDROOT)$(mandir)/man8


#
# Unit test programs (not built when cross-compiling...)
#

unittests:


#
# Make html versions of man pages using <https://www.msweet.org/mantohtml>.
#

html:
	echo Converting man pages to HTML...
	for file in $(MAN1); do \
		echo "    $$file..."; \
		mantohtml --css man.css $$file >../doc/`basename $$file .1`.html; \
	done
	for file in $(MAN5); do \
		echo "    $$file..."; \
		mantohtml --css man.css $$file >../doc/`basename $$file .5`.html; \
	done
	for file in $(MAN8); do \
		echo "    $$file..."; \
		mantohtml --css man.css $$file >../doc/`basename $$file .8`.html; \
	done
