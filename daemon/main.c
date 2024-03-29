//
// Main entry for cupslocald.
//
// Copyright © 2023-2024 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#define CUPSLOCALD_MAIN_C
#include "cupslocald.h"
#include <cups/thread.h>
#ifdef __APPLE__
#  include <launch.h>
#endif // __APPLE__


//
// Local functions...
//

static int	usage(FILE *out);


//
// 'main()' - Main entry.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  const char	*opt,			// Current option
		*home = getenv("HOME"),	// Home directory
		*log_file = "syslog",	// Log file
		*snap_common = getenv("SNAP_COMMON"),
					// Common data directory for snaps
		*tmpdir = getenv("TMPDIR");
					// Temporary directory
  pappl_loglevel_t log_level = PAPPL_LOGLEVEL_INFO;
					// Log level
  pappl_system_t *system;		// System object
#ifdef HAVE_DBUS
  cups_thread_t	dbus;			// D-Bus thread
#endif // HAVE_DBUS


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Parse command-line options...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      return (usage(stdout));
    }
    else if (!strcmp(argv[i], "--version"))
    {
      puts(CUPS_LOCAL_VERSION);
      return (0);
    }
    else if (argv[i][0] == '-' && argv[i][1] && argv[i][1] != '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
	{
	  case 'd' : // -d SPOOLDIR
	      i ++;
	      if (i >= argc)
	      {
	        cupsLangPrintf(stderr, _("%s: Missing spool directory after '-d'."), "cups-locald");
	        return (usage(stderr));
	      }

	      cupsCopyString(LocalSpoolDir, argv[i], sizeof(LocalSpoolDir));
	      break;

	  case 'L' : // -L LOGLEVEL
	      i ++;
	      if (i >= argc)
	      {
	        cupsLangPrintf(stderr, _("%s: Missing log level after '-L'."), "cups-locald");
	        return (usage(stderr));
	      }

	      if (!strcmp(argv[i], "debug"))
	      {
	        log_level = PAPPL_LOGLEVEL_DEBUG;
	      }
	      else if (!strcmp(argv[i], "error"))
	      {
	        log_level = PAPPL_LOGLEVEL_ERROR;
	      }
	      else if (!strcmp(argv[i], "info"))
	      {
	        log_level = PAPPL_LOGLEVEL_INFO;
	      }
	      else if (!strcmp(argv[i], "warn"))
	      {
	        log_level = PAPPL_LOGLEVEL_WARN;
	      }
	      else
	      {
	        cupsLangPrintf(stderr, _("%s: Invalid log level '%s'."), "cups-locald", argv[i]);
	        return (usage(stderr));
	      }
	      break;

	  case 'l' : // -l LOGFILE
	      i ++;
	      if (i >= argc)
	      {
	        cupsLangPrintf(stderr, _("%s: Missing log file after '-l'."), "cups-locald");
	        return (usage(stderr));
	      }

	      log_file = argv[i];
	      break;

	  case 'S' : // -S SOCKETFILE
	      i ++;
	      if (i >= argc)
	      {
	        cupsLangPrintf(stderr, _("%s: Missing socket file after '-S'."), "cups-locald");
	        return (usage(stderr));
	      }

	      cupsCopyString(LocalSocket, argv[i], sizeof(LocalSocket));
	      break;

	  case 's' : // -s STATEFILE
	      i ++;
	      if (i >= argc)
	      {
	        cupsLangPrintf(stderr, _("%s: Missing state file after '-s'."), "cups-locald");
	        return (usage(stderr));
	      }

	      cupsCopyString(LocalStateFile, argv[i], sizeof(LocalStateFile));
	      break;

	  default : //
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), "cups-locald", *opt);
	      return (usage(stderr));
	}
      }
    }
    else
    {
      cupsLangPrintf(stderr, _("%s: Unknown option '%s'."), "cups-locald", argv[i]);
      return (usage(stderr));
    }
  }

  // Set defaults...
#ifdef __APPLE__
  if (!tmpdir)
    tmpdir = "/private/tmp";
#else
  if (!tmpdir)
    tmpdir = "/tmp";
#endif // __APPLE__

  if (!LocalSocket[0])
    snprintf(LocalSocket, sizeof(LocalSocket), "%s/cups-locald%d.sock", tmpdir, (int)getuid());

  if (!LocalSpoolDir[0])
  {
    if (snap_common)
    {
      // Running inside a snap (https://snapcraft.io), so use the snap's common
      // data directory...
      snprintf(LocalSpoolDir, sizeof(LocalSpoolDir), "%s/cups-locald.d", snap_common);
    }
    else if (home)
    {
#ifdef __APPLE__
      // Put the spool directory in "~/Library/Application Support"
      snprintf(LocalSpoolDir, sizeof(LocalSpoolDir), "%s/Library/Application Support/cups-locald.d", home);
#else
      // Put the spool directory under a ".config" directory in the home directory
      snprintf(LocalSpoolDir, sizeof(LocalSpoolDir), "%s/.config", home);

      // Make ~/.config as needed
      if (mkdir(LocalSpoolDir, 0700) && errno != EEXIST)
      {
        cupsLangPrintf(stderr, _("cups-locald: Unable to create ~/.config directory: %s"), strerror(errno));
        return (1);
      }

      if (LocalSpoolDir[0])
	snprintf(LocalSpoolDir, sizeof(LocalSpoolDir), "%s/.config/cups-locald.d", home);
#endif // __APPLE__
    }
    else
    {
      // As a last resort, put the spool directory in the temporary directory
      // (where it will be lost on the nest reboot/logout...
      snprintf(LocalSpoolDir, sizeof(LocalSpoolDir), "%s/cups-locald%d.d", tmpdir, (int)getuid());
    }
  }

  // Create the system object...
  system = papplSystemCreate(PAPPL_SOPTIONS_MULTI_QUEUE, "cups-locald", /*port*/0, /*subtypes*/NULL, LocalSpoolDir, log_file, log_level, /*auth_service*/NULL, /*tls_only*/false);
  papplSystemSetIdleShutdown(system, 120);

  // Load/save state to the state file...
  if (!papplSystemLoadState(system, LocalStateFile))
  {
    // TODO: Set default values for things...
  }

  papplSystemSetSaveCallback(system, (pappl_save_cb_t)papplSystemSaveState, (void *)LocalStateFile);

  // Setup domain socket and loopback listeners
#ifdef __APPLE__
  if (!strcmp(LocalSocket, "launchd"))
  {
    // Support sockets from launchd...
    int		error;			// Check-in error, if any
    size_t	i,			// Looping var
		ld_count;		// Number of listeners
    int		*ld_sockets;		// Listener sockets

    if ((error = launch_activate_socket("Listeners", &ld_sockets, &ld_count)) != 0)
    {
      cupsLangPrintf(stderr, _("cups-locald: Unable to get listener sockets: %s"), strerror(error));
      return (1);
    }

    for (i = 0; i < ld_count; i ++)
    {
      http_addr_t	addr;		// Socket address
      socklen_t		addrlen;	// Length of socket address

      papplSystemAddListenerFd(system, ld_sockets[i]);

      addrlen = sizeof(addr);
      if (!getsockname(ld_sockets[i], (struct sockaddr *)&addr, &addrlen) && addr.addr.sa_family == AF_LOCAL)
        httpAddrGetString(&addr, LocalSocket, sizeof(LocalSocket));
    }
  }
  else
#endif // __APPLE__
  papplSystemAddListeners(system, LocalSocket);

  papplSystemAddListeners(system, "localhost");

  // Setup the generic drivers...
  papplSystemSetPrinterDrivers(system, sizeof(LocalDrivers) / sizeof(LocalDrivers[0]), LocalDrivers, LocalDriverAutoAdd, /* create_cb */NULL, LocalDriverCallback, NULL);

  papplSystemAddMIMEFilter(system, "application/pdf", "application/pdf", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "application/pdf", "application/postscript", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "application/pdf", "image/pwg-raster", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "application/pdf", "image/urf", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "image/jpeg", "application/pdf", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "image/jpeg", "application/postscript", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "image/png", "application/pdf", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "image/png", "application/postscript", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "text/plain", "application/pdf", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "text/plain", "application/postscript", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "text/plain", "image/pwg-raster", LocalTransformFilter, NULL);
  papplSystemAddMIMEFilter(system, "text/plain", "image/urf", LocalTransformFilter, NULL);

#ifdef HAVE_DBUS
  // Start a background thread for D-Bus...
  dbus = cupsThreadCreate(LocalDBusService, /*arg*/NULL);
#endif // HAVE_DBUS

  // Run until we are no longer needed...
  papplSystemRun(system);

#ifdef HAVE_DBUS
  // Stop background thread for D-Bus...
  cupsThreadCancel(dbus);
  cupsThreadWait(dbus);
#endif // HAVE_DBUS

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static int				// O - Exit code
usage(FILE *out)			// I - Output file
{
  cupsLangPuts(out, _("Usage: cups-locald [OPTIONS]"));

  cupsLangPuts(out, _("Options:"));
  cupsLangPuts(out, _("--help                         Show this help"));
  cupsLangPuts(out, _("--version                      Show the program version"));
  cupsLangPuts(out, _("-d SPOOLDIR                    Set the spool directory"));
  cupsLangPuts(out, _("-L LOGLEVEL                    Set the log level (error,warn,info,debug)"));
  cupsLangPuts(out, _("-l LOGFILE                     Set the log file"));
  cupsLangPuts(out, _("-S SOCKETFILE                  Set the domain socket file"));
  cupsLangPuts(out, _("-s STATEFILE                   Set the state/configuration file"));

  return (out == stdout ? 0 : 1);
}
