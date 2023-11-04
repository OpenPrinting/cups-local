dnl
dnl Configuration script for the CUPS commands.
dnl
dnl Copyright © 2022-2023 by OpenPrinting.
dnl
dnl Licensed under Apache License v2.0.  See the file "LICENSE" for more
dnl information.
dnl
dnl ***********************************************************************
dnl
dnl Note: Using autoheader or automake on this project will break the build
dnl system.  Use "autoconf -f" to regenerate the configure script if you
dnl make changes to this file.
dnl
dnl ***********************************************************************

dnl We need at least autoconf 2.70 for --runstatedir...
AC_PREREQ([2.70])


dnl Package name and version...
AC_INIT([cups_local], [3.0b1], [https://github.com/OpenPrinting/cups-local/issues], [cups-local], [https://openprinting.github.io/cups])
AC_CONFIG_HEADERS([config.h])

CUPS_LOCAL_VERSION="AC_PACKAGE_VERSION"

AC_SUBST([CUPS_LOCAL_VERSION])

AC_DEFINE_UNQUOTED([CUPS_LOCAL_VERSION], ["$CUPS_LOCAL_VERSION"], [Version number])


dnl This line is provided to ensure that you don't run the autoheader program
dnl against this project.  Doing so is completely unsupported and WILL cause
dnl problems!
AH_TOP([#error "Somebody ran autoheader on this project which is unsupported and WILL cause problems."])


dnl Get the build and host platforms and split the host_os value
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

[host_os_name="$(echo $host_os | sed -e '1,$s/[0-9.]*$//g')"]
[host_os_version="$(echo $host_os | sed -e '1,$s/^[^0-9.]*//g' | awk -F. '{print $1 $2}')"]
# Linux often does not yield an OS version we can use...
AS_IF([test "x$host_os_version" = x], [
    host_os_version="0"
])


dnl Compiler options...
CFLAGS="${CFLAGS:=}"
CPPFLAGS="${CPPFLAGS:=}"
LDFLAGS="${LDFLAGS:=}"
LIBS="${LIBS:=}"
OPTIM="${OPTIM:=}"

AC_SUBST([OPTIM])


dnl Programs...
AC_PROG_CC
AC_PATH_PROGS([CODE_SIGN], [codesign true])
AC_PATH_PROG([MKDIR], [mkdir])
AC_PATH_PROG([RM], [rm])
AC_PATH_PROG([RMDIR], [rmdir])
AC_PATH_PROG([LN], [ln])


dnl install-sh
AC_MSG_CHECKING([for install-sh script])
INSTALL="$(pwd)/install-sh"
AC_SUBST([INSTALL])
AC_MSG_RESULT([using $INSTALL])


dnl Check for pkg-config, which is used for some other tests later on...
AC_PATH_TOOL([PKGCONFIG], [pkg-config])


dnl CUPS library v3
AC_MSG_CHECKING([for CUPS library v3])
AS_IF([$PKGCONFIG --exists cups3], [
    AC_MSG_RESULT([yes])
    CPPFLAGS="$CPPFLAGS $($PKGCONFIG --cflags cups3)"
    LIBS="$($PKGCONFIG --libs cups3) $LIBS"
], [
    AC_MSG_RESULT([no])
    $PKGCONFIG --debug --exists cups3
    AC_MSG_ERROR([CUPS library v3.0 or later is required.])
])


dnl PAPPL library...
AC_MSG_CHECKING([for PAPPL library v2.0 or later])
AS_IF([$PKGCONFIG --exists pappl --atleast-version=2.0], [
    AC_MSG_RESULT([yes])
    CPPFLAGS="$CFLAGS $($PKGCONFIG --cflags pappl)"
    LIBS="$($PKGCONFIG --libs pappl) $LIBS"
], [
    AC_MSG_RESULT([no])
    $PKGCONFIG --debug --exists pappl
    AC_MSG_ERROR([PAPPL v2.0 or later is required.])
])


dnl Check for DBUS support
DBUSDIR=""
SYSTEMDDIR=""

AC_ARG_ENABLE([dbus], AS_HELP_STRING([--disable-dbus], [build without D-Bus support]))
AC_ARG_WITH([dbusdir], AS_HELP_STRING([--with-dbusdir], [set D-Bus configuration directory]), [
    DBUSDIR="$withval"
])
AC_ARG_WITH([systemddir], AS_HELP_STRING([--with-systemddir], [set systemd configuration directory]), [
    SYSTEMDDIR="$withval"
])

AS_IF([test "x$enable_dbus" != xno], [
    AC_MSG_CHECKING([for D-Bus])
    AS_IF([$PKGCONFIG --exists dbus-1], [
	AC_MSG_RESULT([yes])
    	have_dbus=1
    ], [
	AC_MSG_RESULT([no])
        have_dbus=0
    ])
    AC_MSG_CHECKING([for systemd])
    AS_IF([$PKGCONFIG --exists systemd], [
	AC_MSG_RESULT([yes])
        have_systemd=1
    ], [
	AC_MSG_RESULT([no])
        have_systemd=0
    ])
    AS_IF([test $have_dbus = 1 -a $have_systemd = 1], [
	AC_DEFINE([HAVE_DBUS], [1], [Have dbus library?])
	CPPFLAGS="$CPPFLAGS $($PKGCONFIG --cflags dbus-1) -DDBUS_API_SUBJECT_TO_CHANGE"
	LIBS="$($PKGCONFIG --libs dbus-1) $LIBS"
	AC_CHECK_FUNC([dbus_message_iter_init_append], [
	    AC_DEFINE([HAVE_DBUS_MESSAGE_ITER_INIT_APPEND], [1], [Have dbus_message_iter_init_append function?])
	])
	AC_CHECK_FUNC([dbus_threads_init_default], [
	    AC_DEFINE([HAVE_DBUS_THREADS_INIT], [1], [Have dbus_threads_init_default function?])
	])
	AS_IF([test "x$DBUSDIR" = x], [
	    DBUSDIR="$($PKGCONFIG --variable=datadir dbus-1)/dbus-1"
	])
	AS_IF([test "x$DBUSDIR" = x], [
	    AC_MSG_ERROR([No default D-Bus directory found, use --with-dbusdir option.])
	])
	AS_IF([test "x$SYSTEMDDIR" = x], [
	    SYSTEMDDIR="$($PKGCONFIG --variable=systemd_util_dir systemd)"
	])
	AS_IF([test "x$SYSTEMDDIR" = x], [
	    AC_MSG_ERROR([No default systemd directory found, use --with-systemddir option.])
	])
    ], [test "$xenable_dbus" = xyes], [
	AC_MSG_ERROR([Required D-Bus library and systemd are missing.])
    ])
])

AC_SUBST([DBUSDIR])
AC_SUBST([SYSTEMDDIR])


dnl Extra compiler options...
AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [turn on debugging, default=no]))
AC_ARG_ENABLE([maintainer], AS_HELP_STRING([--enable-maintainer], [turn on maintainer mode, default=no]))
AC_ARG_ENABLE([sanitizer], AS_HELP_STRING([--enable-sanitizer], [build with AddressSanitizer, default=no]))

AS_IF([test x$enable_debug = xyes], [
    OPTIM="$OPTIM -g"
    CSFLAGS=""
], [
    OPTIM="$OPTIM -g -Os"
    CSFLAGS="-o runtime"
])

AC_SUBST([CSFLAGS])

WARNINGS=""
AC_SUBST([WARNINGS])

AS_IF([test -n "$GCC"], [
    AS_IF([test x$enable_sanitizer = xyes], [
	# Use -fsanitize=address with debugging...
	OPTIM="$OPTIM -fsanitize=address"
    ], [
	# Otherwise use the Fortify enhancements to catch any unbounded
	# string operations...
	CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=2"
    ])

    dnl Show all standard warnings + unused variables when compiling...
    WARNINGS="-Wall -Wunused"

    dnl Drop some not-useful/unreliable warnings...
    for warning in char-subscripts format-truncation format-y2k switch unused-result; do
	AC_MSG_CHECKING([whether compiler supports -Wno-$warning])

	OLDCFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Wno-$warning -Werror"

	AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [
	    AC_MSG_RESULT(yes)
	    WARNINGS="$WARNINGS -Wno-$warning"
        ], [
	    AC_MSG_RESULT(no)
	])

	CFLAGS="$OLDCFLAGS"
    done

    dnl Maintainer mode enables -Werror...
    AS_IF([test x$enable_maintainer = xyes], [
	WARNINGS="$WARNINGS -Werror"
    ])

    dnl See if PIE options are supported...
    AC_MSG_CHECKING(whether compiler supports -fPIE)
    OLDCFLAGS="$CFLAGS"
    AS_CASE(["$host_os_name"],
    [darwin*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -Wl,-pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -Wl,-pie $LDFLAGS"
	    AC_MSG_RESULT(yes)
	],[
	    AC_MSG_RESULT(no)
	])
    ], [*], [
	CFLAGS="$CFLAGS -fPIC -fPIE -pie"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[
	    OLDCFLAGS="-fPIC $OLDCFLAGS"
	    LDFLAGS="-fPIE -pie $LDFLAGS"
	    AC_MSG_RESULT(yes)
	],[
	    AC_MSG_RESULT(no)
	])
    ])
    CFLAGS="$OLDCFLAGS"

    dnl OS-specific compiler options...
    AS_CASE(["$host_os_name"], [linux*], [
	CPPFLAGS="$CPPFLAGS -D__USE_MISC -D_GNU_SOURCE"
    ], [darwin*], [
        AS_IF([test "$host_os_version" -ge 200 -a x$enable_debug != xyes], [
            # macOS 11.0 and higher support the Apple Silicon (arm64) CPUs
	    OPTIM="$OPTIM -mmacosx-version-min=11.0 -arch x86_64 -arch arm64"
	], [test x$enable_debug != xyes], [
	    OPTIM="$OPTIM -mmacosx-version-min=11.0 -arch x86_64"
	])
    ])
])


dnl Extra linker options...
AC_ARG_WITH([ldflags], AS_HELP_STRING([--with-ldflags=...], [Specify additional LDFLAGS]), [
    LDFLAGS="$withval $LDFLAGS"
])

dnl Fix prefix, datadir, and datarootdir...
AS_IF([test "$prefix" = NONE], [
    # Default prefix isn't bound until AC_OUTPUT...
    realprefix="/usr/local"
], [
    realprefix="$prefix"
])

AS_IF([test "$datarootdir" = "\${prefix}/share"], [
    AS_IF([test "$prefix" = "/"], [
	datarootdir="/usr/share"
    ], [
	datarootdir="$realprefix/share"
    ])
])

AS_IF([test "$datadir" = "\${prefix}/share"], [
    AS_IF([test "$prefix" = "/"], [
	datadir="/usr/share"
    ], [
	datadir="$realprefix/share"
    ])
], [test "$datadir" = "\${datarootdir}"], [
    datadir="$datarootdir"
])


dnl Save the installed directory for localizations...
CUPS_LOCAL_DATADIR="$datadir/cups-local"
AC_DEFINE_UNQUOTED([CUPS_LOCAL_DATADIR], ["$datadir/cups-local"], [Location of data files.])
AC_SUBST([CUPS_LOCAL_DATADIR])


dnl Generate a bunch of files...
AC_CONFIG_FILES([
    Makedefs
    daemon/cups-locald-dbus.service
    daemon/cups-locald-systemd.service
    daemon/org.openprinting.cups-locald.plist
])
AC_OUTPUT
