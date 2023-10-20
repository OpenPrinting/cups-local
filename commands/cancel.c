//
// "cancel" command for CUPS.
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

static int	usage(FILE *out);


//
// 'main()' - Parse options and cancel jobs.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  http_t	*http = NULL;		// HTTP connection to server
  int		i;			// Looping var
  int		job_id;			// Job ID
  size_t	num_dests = 0;		// Number of destinations
  cups_dest_t	*dests = NULL;		// Destinations
  char		*opt,			// Option pointer
		*dest = NULL,		// Destination printer
		*job,			// Job ID pointer
		*user = NULL;		// Cancel jobs for a user
  bool		purge = false;		// Purge or cancel jobs?
  char		uri[1024];		// Printer or job URI
  ipp_op_t	op = IPP_OP_CANCEL_JOB;	// Operation
  ipp_t		*request;		// IPP request


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Process command-line arguments...
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
    else if (!strncmp(argv[i], "--", 2))
    {
      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%s\"."), argv[0], argv[i]);
      return (usage(stderr));
    }
    else if (argv[i][0] == '-' && argv[i][1])
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);

	      if (http)
		httpSetEncryption(http, HTTP_ENCRYPTION_REQUIRED);
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
		  return (usage(stderr));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'a' : // Cancel all jobs
	      op = purge ? IPP_OP_PURGE_JOBS : IPP_OP_CANCEL_JOBS;
	      break;

	  case 'h' : // Connect to host
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
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-h\" option."), argv[0]);
		  return (usage(stderr));
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'u' : // Username
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
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-u\" option."), argv[0]);
		  return (usage(stderr));
		}

		user = argv[i];
	      }
	      break;

	  case 'x' : // Purge job(s)
	      purge = true;

	      if (op == IPP_OP_CANCEL_JOBS)
		op = IPP_OP_PURGE_JOBS;
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), argv[0], *opt);
	      return (usage(stderr));
	}
      }
    }
    else
    {
      // Open a connection to the server...
      if (http == NULL)
      {
	if ((http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, 30000, /*cancel*/NULL)) == NULL)
	{
	  cupsLangPrintf(stderr, _("%s: Unable to connect to server."), argv[0]);
	  return (1);
	}
      }

      if (num_dests == 0)
        num_dests = cupsGetDests(http, &dests);

      // Cancel a job or printer...
      if (!strcmp(argv[i], "-"))
      {
        // Delete the current job...
        dest   = "";
	job_id = 0;
      }
      else if (cupsGetDest(argv[i], NULL, num_dests, dests) != NULL)
      {
        // Delete the current job on the named destination...
        dest   = argv[i];
	job_id = 0;
      }
      else if ((job = strrchr(argv[i], '-')) != NULL && isdigit(job[1] & 255))
      {
        // Delete the specified job ID.
        dest   = NULL;
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(job + 1);
      }
      else if (isdigit(argv[i][0] & 255))
      {
        // Delete the specified job ID.
        dest   = NULL;
	op     = IPP_OP_CANCEL_JOB;
        job_id = atoi(argv[i]);
      }
      else
      {
        // Bad printer name!
        cupsLangPrintf(stderr, _("%s: Error - unknown destination \"%s\"."), argv[0], argv[i]);
	return (1);
      }

      // Build an IPP request, which requires the following
      // attributes:
      //
      //   attributes-charset
      //   attributes-natural-language
      //   printer-uri + job-id *or* job-uri
      //   [requesting-user-name]
      request = ippNewRequest(op);

      if (dest)
      {
        // TODO: Fix printer-uri
	httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/printers/%s", dest);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
	ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", job_id);
      }
      else
      {
        // TODO: Fix job-uri
        snprintf(uri, sizeof(uri), "ipp://localhost/jobs/%d", job_id);
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);
      }

      if (user)
      {
	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
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
      // TODO: Fix resource path
      if (op == IPP_OP_CANCEL_JOBS && (!user || strcasecmp(user, cupsGetUser())))
        ippDelete(cupsDoRequest(http, request, "/admin/"));
      else
        ippDelete(cupsDoRequest(http, request, "/jobs/"));

      if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
      {
	cupsLangPrintf(stderr, _("%s: %s failed: %s"), argv[0], op == IPP_OP_PURGE_JOBS ? "purge-jobs" : "cancel-job", cupsGetErrorString());
	return (1);
      }
    }
  }

  if (num_dests == 0 && op != IPP_OP_CANCEL_JOB)
  {
    // Open a connection to the server...
    if (http == NULL)
    {
      if ((http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, 30000, /*cancel*/NULL)) == NULL)
      {
	cupsLangPrintf(stderr, _("%s: Unable to connect to server."), argv[0]);
	return (1);
      }
    }

    // Build an IPP request, which requires the following
    // attributes:
    //
    //   attributes-charset
    //   attributes-natural-language
    //   printer-uri + job-id *or* job-uri
    //   [requesting-user-name]
    request = ippNewRequest(op);

    // TODO: Fix printer-uri
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, "ipp://localhost/printers/");

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
    // TODO: Fix request resource...
    ippDelete(cupsDoRequest(http, request, "/admin/"));

    if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, _("%s: %s failed: %s"), argv[0], op == IPP_OP_PURGE_JOBS ? "purge-jobs" : "cancel-job", cupsGetErrorString());
      return (1);
    }
  }

  return (0);
}


//
// 'usage()' - Show program usage and exit.
//

static int				// O - Exit code
usage(FILE *out)
{
  cupsLangPuts(out, _("Usage: cancel [OPTIONS] [JOBID]\n"
		      "       cancel [OPTIONS] [DESTINATION]\n"
		      "       cancel [OPTIONS] [DESTINATION-JOBID]"));
  cupsLangPuts(out, _("Options:"));
  cupsLangPuts(out, _("-a                             Cancel all jobs"));
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  cupsLangPuts(out, _("-h SERVER[:PORT]               Connect to the named server and port"));
  cupsLangPuts(out, _("-u OWNER                       Specify the owner to use for jobs"));
  cupsLangPuts(out, _("-U USERNAME                    Specify the username to use for authentication"));
  cupsLangPuts(out, _("-x                             Purge jobs rather than just canceling"));

  return (out == stdout ? 0 : 1);
}
