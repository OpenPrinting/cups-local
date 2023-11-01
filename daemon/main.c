//
// Main entry for cups-local.
//
// Copyright © 2023 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#define CUPS_LOCAL_MAIN_C
#include "cups-locald.h"


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
		*log_file = "syslog",	// Log file
		*spool_directory = NULL,// Spool directory
		*state_file = NULL;	// State file
  pappl_loglevel_t log_level = PAPPL_LOGLEVEL_INFO;
					// Log level
  pappl_system_t *system;		// System object


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

	      spool_directory = argv[i];
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

	      state_file = argv[i];
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

  // Create the system object...
  system = papplSystemCreate(PAPPL_SOPTIONS_MULTI_QUEUE, "cups-locald", /*port*/0, /*subtypes*/NULL, spool_directory, log_file, log_level, /*auth_service*/NULL, /*tls_only*/false);

  // Setup domain socket listener
  if (!LocalSocket[0])
  {
    const char *tmpdir = getenv("TMPDIR");
					// Temporary directory

    snprintf(LocalSocket, sizeof(LocalSocket), "%s/cups-locald%d.sock", tmpdir, (int)getuid());
  }

  papplSystemAddListeners(system, LocalSocket);

  // Setup the generic drivers...
  papplSystemSetPrinterDrivers(system, sizeof(LocalDrivers) / sizeof(LocalDrivers[0]), LocalDrivers, LocalDriverAutoAdd, /* create_cb */NULL, LocalDriverCallback, NULL);

  // Run until we are no longer needed...
  papplSystemRun(system);

  return (1);
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
