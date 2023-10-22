//
// "cancel" and "lprm" commands for CUPS.
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
#include <ctype.h>


//
// Local functions...
//

static int	usage(FILE *out, const char *command);


//
// 'main()' - Parse options and cancel jobs.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  const char	*command;		// Command name
  int		i;			// Looping var
  http_t	*http = NULL;		// HTTP connection to server
  cups_dest_t	*dest = NULL;		// Destination
  char		resource[1024];		// Resource path
  int		job_id;			// Job ID
  char		*opt,			// Option pointer
		*printer = NULL,	// Destination printer
		*job,			// Job ID pointer
		*user = NULL;		// Cancel jobs for a user
  bool		purge = false;		// Purge or cancel jobs?
  ipp_op_t	op = IPP_OP_CANCEL_JOB;	// Operation
  ipp_t		*request;		// IPP request


  // Get command name...
  if ((command = strrchr(argv[0], '/')) != NULL)
    command ++;
  else
    command = argv[0];

  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Process command-line arguments...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      return (usage(stdout, command));
    }
    else if (!strcmp(argv[i], "--version"))
    {
      puts(CUPS_LOCAL_VERSION);
      return (0);
    }
    else if (!strncmp(argv[i], "--", 2))
    {
      cupsLangPrintf(stderr, _("%s: Unknown option '%s'."), command, argv[i]);
      return (usage(stderr, command));
    }
    else if (argv[i][0] == '-' && argv[i][1])
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'a' : // -a         Cancel all jobs
	      op = purge ? IPP_OP_PURGE_JOBS : IPP_OP_CANCEL_JOBS;
	      break;

	  case 'E' : // -E         Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);

	      if (http)
		httpSetEncryption(http, HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'h' : // -h SERVER  Connect to host
	      httpClose(http);
	      http = NULL;

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
		  cupsLangPrintf(stderr, _("%s: Expected hostname after '-H' option."), command);
		  return (usage(stderr, command));
		}

		cupsSetServer(argv[i]);
	      }
	      break;

          case 'P' : // -P DESTINATION
	      if (opt[1] != '\0')
	      {
		printer = opt + 1;
		opt     += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected destination after '-P' option."), command);
		  return (usage(stderr, command));
		}

		printer = argv[i];
	      }

	      if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, /*instance*/NULL)) == NULL)
	      {
		cupsLangPrintf(stderr, _("%s: Unknown destination '%s'."), command, printer);
		return (1);
	      }
	      break;

	  case 'U' : // -U USER    Set username
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
		  cupsLangPrintf(stderr, _("%s: Expected username after '-U' option."), command);
		  return (usage(stderr, command));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'u' : // -u OWNER   Set owner
	      op = IPP_OP_CANCEL_MY_JOBS;

	      if (opt[1] != '\0')
	      {
		user = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected owner after '-u' option."), command);
		  return (usage(stderr, command));
		}

		user = argv[i];
	      }
	      break;

	  case 'x' : // -x         Purge job(s)
	      purge = true;

	      if (op == IPP_OP_CANCEL_JOBS)
		op = IPP_OP_PURGE_JOBS;
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), command, *opt);
	      return (usage(stderr, command));
	}
      }
    }
    else
    {
      // Cancel a job or printer...
      if (!strcmp(argv[i], "-"))
      {
        // Delete the current job...
	job_id = 0;
      }
      else if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, argv[i], /*instance*/NULL)) != NULL)
      {
        // Delete the current job on the named destination...
	job_id  = 0;
      }
      else if ((job = strrchr(argv[i], '-')) != NULL && isdigit(job[1] & 255))
      {
        // Delete the specified job ID.
        *job++ = '\0';
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(job);

        if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, argv[i], /*instance*/NULL)) == NULL)
        {
	  cupsLangPrintf(stderr, _("%s: Unknown destination '%s'."), command, argv[i]);
	  return (1);
        }

        printer = dest->name;
      }
      else if (isdigit(argv[i][0] & 255))
      {
        // Delete the specified job ID.
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(argv[i]);
      }
      else
      {
        // Bad printer name!
        cupsLangPrintf(stderr, _("%s: Unknown destination '%s'."), command, argv[i]);
	return (1);
      }

      // Connect to the printer...
      if (!dest)
      {
        if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, /*printer*/NULL, /*instance*/NULL)) == NULL)
        {
	  cupsLangPrintf(stderr, _("%s: No default destination."), command);
	  return (1);
        }
      }

      if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, sizeof(resource), /*cb*/NULL, /*user_data*/NULL)) == NULL)
      {
	cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, dest->name, cupsGetErrorString());
        return (1);
      }

      // Build an IPP request, which requires the following
      // attributes:
      //
      //   attributes-charset
      //   attributes-natural-language
      //   printer-uri + job-id
      //   [requesting-user-name]
      request = ippNewRequest(op);

      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, cupsGetOption("printer-uri-supported", dest->num_options, dest->options));
      if (job_id > 0)
        ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", job_id);
      if (user)
      {
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
        if (job_id == 0)
	  ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", true);

        if (op == IPP_OP_CANCEL_JOBS)
          op = IPP_OP_CANCEL_MY_JOBS;
      }
      else
      {
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
      }

      if (purge)
	ippAddBoolean(request, IPP_TAG_OPERATION, "purge-jobs", purge);

      // Do the request and get back a response...
      ippDelete(cupsDoRequest(http, request, resource));

      httpClose(http);

      if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
      {
	cupsLangPrintf(stderr, _("%s: Unable to cancel job(s): %s"), command, cupsGetErrorString());
	return (1);
      }
    }
  }

  if (!dest && op != IPP_OP_CANCEL_JOB)
  {
    // Connect to the default printer...
    if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, /*printer*/NULL, /*instance*/NULL)) == NULL)
    {
      cupsLangPrintf(stderr, _("%s: No default destination."), command);
      return (1);
    }

    if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, sizeof(resource), /*cb*/NULL, /*user_data*/NULL)) == NULL)
    {
      cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, dest->name, cupsGetErrorString());
      return (1);
    }

    // Build an IPP request, which requires the following
    // attributes:
    //
    //   attributes-charset
    //   attributes-natural-language
    //   printer-uri
    //   [requesting-user-name]
    request = ippNewRequest(op);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, cupsGetOption("printer-uri-supported", dest->num_options, dest->options));

    if (user)
    {
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
      ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", true);
    }
    else
    {
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
    }

    ippAddBoolean(request, IPP_TAG_OPERATION, "purge-jobs", purge);

    // Do the request and get back a response...
    ippDelete(cupsDoRequest(http, request, resource));

    if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, _("%s: Unable to cancel job(s): %s"), command, cupsGetErrorString());
      return (1);
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
  cupsLangPrintf(out, _("Usage: %s [OPTIONS] [JOBID]\n"
			"       %s [OPTIONS] [DESTINATION]\n"
			"       %s [OPTIONS] [DESTINATION-JOBID]"), command, command, command);
  cupsLangPuts(out, _("Options:"));
  cupsLangPuts(out, _("-a                             Cancel all jobs"));
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  cupsLangPuts(out, _("-h SERVER[:PORT]               Connect to the named server and port"));
  cupsLangPuts(out, _("-u OWNER                       Specify the owner to use for jobs"));
  cupsLangPuts(out, _("-U USERNAME                    Specify the username to use for authentication"));
  cupsLangPuts(out, _("-x                             Purge jobs rather than just canceling"));

  return (out == stdout ? 0 : 1);
}
