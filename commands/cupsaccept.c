//
// "cupsaccept", "cupsdisable", "cupsenable", and "cupsreject" commands for
// CUPS.
//
// Copyright © 2021-2023 by OpenPrinting.
// Copyright © 2007-2018 by Apple Inc.
// Copyright © 1997-2006 by Easy Software Products.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include <config.h>
#include <cups/cups.h>


//
// Local functions...
//

static int	usage(FILE *out, const char *command);


//
// 'main()' - Parse options and accept/reject jobs or disable/enable printers.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  char		*command,		// Command to do
		*opt,			// Option pointer
		uri[1024],		// Printer URI
		*reason = NULL;		// Reason for reject/disable
  ipp_t		*request;		// IPP request
  ipp_op_t	op;			// Operation
  bool		cancel = false;		// Cancel jobs?


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // See what operation we're supposed to do...
  if ((command = strrchr(argv[0], '/')) != NULL)
    command ++;
  else
    command = argv[0];

  if (!strcmp(command, "cupsaccept"))
  {
    op = IPP_OP_CUPS_ACCEPT_JOBS;
  }
  else if (!strcmp(command, "cupsreject"))
  {
    op = IPP_OP_CUPS_REJECT_JOBS;
  }
  else if (!strcmp(command, "cupsdisable"))
  {
    op = IPP_OP_PAUSE_PRINTER;
  }
  else if (!strcmp(command, "cupsenable"))
  {
    op = IPP_OP_RESUME_PRINTER;
  }
  else
  {
    cupsLangPrintf(stderr, _("%s: Don't know what to do."), command);
    return (1);
  }

  // Process command-line arguments...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      return (usage(stdout, command));
    }
    else if (!strcmp(argv[i], "--hold"))
    {
      op = IPP_OP_HOLD_NEW_JOBS;
    }
    else if (!strcmp(argv[i], "--release"))
    {
      op = IPP_OP_RELEASE_HELD_NEW_JOBS;
    }
    else if (!strcmp(argv[i], "--version"))
    {
      puts(CUPS_LOCAL_VERSION);
      return (0);
    }
    else if (!strncmp(argv[i], "--", 2))
    {
      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%s\"."), argv[0], argv[i]);
      return (usage(stderr, command));
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'U' : // Username
	      if (opt[1] != '\0')
	      {
		cupsSetUser(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-U\" option."), argv[0]);
		  return (usage(stderr, command));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'c' : // Cancel jobs
	      cancel = true;
	      break;

	  case 'h' : // Connect to host
	      if (opt[1] != '\0')
	      {
		cupsSetServer(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-h\" option."), command);
		  return (usage(stderr, command));
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'r' : // Reason for cancelation
	      if (opt[1] != '\0')
	      {
		reason = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected reason text after \"-r\" option."), command);
		  return (usage(stderr, command));
		}

		reason = argv[i];
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), command, *opt);
	      return (usage(stderr, command));
	}
      }
    }
    else
    {
      // Accept/disable/enable/reject a destination...
      request = ippNewRequest(op);

      // TODO: Fix printer-uri
      httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", argv[i]);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

      if (reason != NULL)
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_TEXT, "printer-state-message", NULL, reason);

      // Do the request and get back a response...
      // TODO: Fix resource path
      ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/admin/"));

      if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
      {
	cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
	return (1);
      }

      // Cancel all jobs if requested...
      if (cancel)
      {
        // Build an IPP_PURGE_JOBS request, which requires the following
	// attributes:
	//
	//   attributes-charset
	//   attributes-natural-language
	//   printer-uri
	request = ippNewRequest(IPP_OP_PURGE_JOBS);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
	// TODO: Fix resource path
	ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/admin/"));

        if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
	{
	  cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
	  return (1);
	}
      }
    }
  }

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static int				// O - Exit code
usage(FILE       *out,			// I - Output file
      const char *command)		// I - Command name
{
  cupsLangPrintf(out, _("Usage: %s [options] destination(s)"), command);
  cupsLangPuts(out, _("Options:"));
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  cupsLangPuts(out, _("-h SERVER[:PORT]               Connect to the named server and port"));
  cupsLangPuts(out, _("-r reason                      Specify a reason message that others can see"));
  cupsLangPuts(out, _("-U username                    Specify the username to use for authentication"));
  if (!strcmp(command, "cupsdisable"))
    cupsLangPuts(out, _("--hold                         Hold new jobs"));
  if (!strcmp(command, "cupsenable"))
    cupsLangPuts(out, _("--release                      Release previously held jobs"));

  return (out == stdout ? 0 : 1);
}
