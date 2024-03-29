//
// "lpq" command for CUPS.
//
// Copyright © 2021-2024 by OpenPrinting.
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

static http_t	*connect_server(const char *, http_t *);
static int	show_jobs(const char *, http_t *, const char *,
		          const char *, const int, const int);
static void	show_printer(const char *, http_t *, const char *);
static void	usage(void) _CUPS_NORETURN;


//
// 'main()' - Parse options and commands.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  http_t	*http;			// Connection to server
  const char	*opt,			// Option pointer
		*dest,			// Desired printer
		*user,			// Desired user
		*val;			// Environment variable name
  char		*instance;		// Printer instance
  int		id,			// Desired job ID
		all,			// All printers
		interval,		// Reporting interval
		longstatus;		// Show file details
  cups_dest_t	*named_dest;		// Named destination


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

 /*
  * Check for command-line options...
  */

  http       = NULL;
  dest       = NULL;
  user       = NULL;
  id         = 0;
  interval   = 0;
  longstatus = 0;
  all        = 0;

  for (i = 1; i < argc; i ++)
  {
    if (argv[i][0] == '+')
    {
      interval = atoi(argv[i] + 1);
    }
    else if (!strcmp(argv[i], "--help"))
      usage();
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'E' : // Encrypt
#ifdef HAVE_TLS
	      cupsSetEncryption(HTTP_ENCRYPT_REQUIRED);

	      if (http)
		httpEncryption(http, HTTP_ENCRYPT_REQUIRED);
#else
	      cupsLangPrintf(stderr, _("%s: Sorry, no encryption support."), argv[0]);
#endif // HAVE_TLS
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
		  return (1);
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'P' : // Printer
	      if (opt[1] != '\0')
	      {
		dest = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  httpClose(http);

		  usage();
		}

		dest = argv[i];
	      }

	      if ((instance = strchr(dest, '/')) != NULL)
		*instance++ = '\0';

	      http = connect_server(argv[0], http);

	      if ((named_dest = cupsGetNamedDest(http, dest, instance)) == NULL)
	      {
		if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		    cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		else if (instance)
		  cupsLangPrintf(stderr, _("%s: Error - unknown destination \"%s/%s\"."), argv[0], dest, instance);
		else
		  cupsLangPrintf(stderr, _("%s: Unknown destination \"%s\"."), argv[0], dest);

		return (1);
	      }

	      cupsFreeDests(1, named_dest);
	      break;

	  case 'a' : // All printers
	      all = 1;
	      break;

	  case 'h' : // Connect to host
	      if (http)
	      {
		httpClose(http);
		http = NULL;
	      }

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
		  return (1);
		}
		else
		  cupsSetServer(argv[i]);
	      }
	      break;

	  case 'l' : // Long status
	      longstatus = 1;
	      break;

	  default :
	      httpClose(http);

	      usage();
	}
      }
    }
    else if (isdigit(argv[i][0] & 255))
    {
      id = atoi(argv[i]);
    }
    else
    {
      user = argv[i];
    }
  }

  http = connect_server(argv[0], http);

  if (dest == NULL && !all)
  {
    if ((named_dest = cupsGetNamedDest(http, NULL, NULL)) == NULL)
    {
      if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
          cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
      {
	cupsLangPrintf(stderr,
	                _("%s: Error - add '/version=1.1' to server name."),
			argv[0]);
        return (1);
      }

      val = NULL;

      if ((dest = getenv("LPDEST")) == NULL)
      {
	if ((dest = getenv("PRINTER")) != NULL)
	{
          if (!strcmp(dest, "lp"))
            dest = NULL;
	  else
	    val = "PRINTER";
	}
      }
      else
	val = "LPDEST";

      if (dest && val)
	cupsLangPrintf(stderr,
	                _("%s: Error - %s environment variable names "
			  "non-existent destination \"%s\"."), argv[0], val,
			dest);
      else
	cupsLangPrintf(stderr,
	                _("%s: Error - no default destination available."),
			argv[0]);
      httpClose(http);
      return (1);
    }

    dest = named_dest->name;
  }

 /*
  * Show the status in a loop...
  */

  for (;;)
  {
    if (dest)
      show_printer(argv[0], http, dest);

    i = show_jobs(argv[0], http, dest, user, id, longstatus);

    if (i && interval)
    {
      fflush(stdout);
      sleep((unsigned)interval);
    }
    else
      break;
  }

 /*
  * Close the connection to the server and return...
  */

  httpClose(http);

  return (0);
}


//
// 'connect_server()' - Connect to the server as necessary...
//

static http_t *				// O - New HTTP connection
connect_server(const char *command,	// I - Command name
               http_t     *http)	// I - Current HTTP connection
{
  if (!http)
  {
    http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, /*msec*/30000, /*cancel*/NULL);

    if (http == NULL)
    {
      cupsLangPrintf(stderr, _("%s: Unable to connect to server."), command);
      exit(1);
    }
  }

  return (http);
}


//
// 'show_jobs()' - Show jobs.
//

static int				// O - Number of jobs in queue
show_jobs(const char *command,		// I - Command name
          http_t     *http,		// I - HTTP connection to server
          const char *dest,		// I - Destination
	  const char *user,		// I - User
	  const int  id,		// I - Job ID
	  const int  longstatus)	// I - 1 if long report desired
{
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr;		// Current attribute
  const char	*jobdest,		// Pointer into job-printer-uri
		*jobuser,		// Pointer to job-originating-user-name
		*jobname;		// Pointer to job-name
  ipp_jstate_t	jobstate;		// job-state
  int		jobid,			// job-id
		jobsize,		// job-k-octets
		jobcount,		// Number of jobs
		jobcopies,		// Number of copies
		rank;			// Rank of job
  char		resource[1024];		// Resource string
  char		rankstr[255];		// Rank string
  char		namestr[1024];		// Job name string
  static const char * const jobattrs[] =// Job attributes we want to see
		{
		  "copies",
		  "job-id",
		  "job-k-octets",
		  "job-name",
		  "job-originating-user-name",
		  "job-printer-uri",
		  "job-priority",
		  "job-state"
		};
  static const char * const ranks[10] =	// Ranking strings
		{
		  "th",
		  "st",
		  "nd",
		  "rd",
		  "th",
		  "th",
		  "th",
		  "th",
		  "th",
		  "th"
		};


  if (http == NULL)
    return (0);

 /*
  * Build an IPP_OP_GET_JOBS or IPP_OP_GET_JOB_ATTRIBUTES request, which
  * requires the following attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    job-uri or printer-uri
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(id ? IPP_OP_GET_JOB_ATTRIBUTES : IPP_OP_GET_JOBS);

  if (dest)
  {
    httpAssembleURIf(HTTP_URI_CODING_ALL, resource, sizeof(resource), "ipp", NULL, "localhost", 0, "/ipp/print/%s", dest);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, resource);
  }
  else
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, "ipp://localhost/ipp/print");

  if (id)
    ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", id);

  if (user)
  {
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user);
    ippAddBoolean(request, IPP_TAG_OPERATION, "my-jobs", true);
  }
  else
  {
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
  }

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes", sizeof(jobattrs) / sizeof(jobattrs[0]), NULL, jobattrs);

 /*
  * Do the request and get back a response...
  */

  jobcount = 0;

  if ((response = cupsDoRequest(http, request, "/")) != NULL)
  {
    if (ippGetStatusCode(response) > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
      ippDelete(response);
      return (0);
    }

    rank = 1;

   /*
    * Loop through the job list and display them...
    */

    for (attr = ippGetFirstAttribute(response); attr != NULL; attr = ippGetNextAttribute(response))
    {
     /*
      * Skip leading attributes until we hit a job...
      */

      while (attr != NULL && ippGetGroupTag(attr) != IPP_TAG_JOB)
        attr = ippGetNextAttribute(response);

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this job...
      */

      jobid       = 0;
      jobsize     = 0;
      jobstate    = IPP_JSTATE_PENDING;
      jobname     = "unknown";
      jobuser     = "unknown";
      jobdest     = NULL;
      jobcopies   = 1;

      while (attr != NULL && ippGetGroupTag(attr) == IPP_TAG_JOB)
      {
	const char *name = ippGetName(attr);
	ipp_tag_t value_tag = ippGetValueTag(attr);

        if (!strcmp(name, "job-id") && value_tag == IPP_TAG_INTEGER)
	  jobid = ippGetInteger(attr, 0);

        if (!strcmp(name, "job-k-octets") && value_tag == IPP_TAG_INTEGER)
	  jobsize = ippGetInteger(attr, 0);

        if (!strcmp(name, "job-state") && value_tag == IPP_TAG_ENUM)
	  jobstate = (ipp_jstate_t)ippGetInteger(attr, 0);

        if (!strcmp(name, "job-printer-uri") && value_tag == IPP_TAG_URI)
	  if ((jobdest = strrchr(ippGetString(attr, 0, NULL), '/')) != NULL)
	    jobdest ++;

        if (!strcmp(name, "job-originating-user-name") && value_tag == IPP_TAG_NAME)
	  jobuser = ippGetString(attr, 0, NULL);

        if (!strcmp(name, "job-name") && value_tag == IPP_TAG_NAME)
	  jobname = ippGetString(attr, 0, NULL);

        if (!strcmp(name, "copies") && value_tag == IPP_TAG_INTEGER)
	  jobcopies = ippGetInteger(attr, 0);

        attr = ippGetNextAttribute(response);
      }

     /*
      * See if we have everything needed...
      */

      if (jobdest == NULL || jobid == 0)
      {
        if (attr == NULL)
	  break;
	else
          continue;
      }

      if (!longstatus && jobcount == 0)
	cupsLangPuts(stdout,
	              _("Rank    Owner   Job     File(s)"
		        "                         Total Size"));

      jobcount ++;

     /*
      * Display the job...
      */

      if (jobstate == IPP_JSTATE_PROCESSING)
	cupsCopyString(rankstr, "active", sizeof(rankstr));
      else
      {
       /*
        * Make the rank show the "correct" suffix for each number
	* (11-13 are the only special cases, for English anyways...)
	*/

	if ((rank % 100) >= 11 && (rank % 100) <= 13)
	  snprintf(rankstr, sizeof(rankstr), "%dth", rank);
	else
	  snprintf(rankstr, sizeof(rankstr), "%d%s", rank, ranks[rank % 10]);

	rank ++;
      }

      if (longstatus)
      {
        cupsLangPuts(stdout, "\n");

        if (jobcopies > 1)
	  snprintf(namestr, sizeof(namestr), "%d copies of %s", jobcopies,
	           jobname);
	else
	  cupsCopyString(namestr, jobname, sizeof(namestr));

        cupsLangPrintf(stdout, _("%s: %-33.33s [job %d localhost]"),
	                jobuser, rankstr, jobid);
        cupsLangPrintf(stdout, _("        %-39.39s %.0f bytes"),
	                namestr, 1024.0 * jobsize);
      }
      else
        cupsLangPrintf(stdout,
	                _("%-7s %-7.7s %-7d %-31.31s %.0f bytes"),
			rankstr, jobuser, jobid, jobname, 1024.0 * jobsize);

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }
  else
  {
    cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
    return (0);
  }

  if (jobcount == 0)
    cupsLangPuts(stdout, _("no entries"));

  return (jobcount);
}


//
// 'show_printer()' - Show printer status.
//

static void
show_printer(const char *command,	// I - Command name
             http_t     *http,		// I - HTTP connection to server
             const char *dest)		// I - Destination
{
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr;		// Current attribute
  ipp_pstate_t	state;			// Printer state
  char		uri[HTTP_MAX_URI];	// Printer URI


  if (http == NULL)
    return;

 /*
  * Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  */

  request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, "/ipp/print/%s", dest);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);

 /*
  * Do the request and get back a response...
  */

  if ((response = cupsDoRequest(http, request, "/")) != NULL)
  {
    if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
      ippDelete(response);
      return;
    }

    if ((attr = ippFindAttribute(response, "printer-state", IPP_TAG_ENUM)) != NULL)
      state = (ipp_pstate_t)ippGetInteger(attr, 0);
    else
      state = IPP_PSTATE_STOPPED;

    switch (state)
    {
      case IPP_PSTATE_IDLE :
          cupsLangPrintf(stdout, _("%s is ready"), dest);
	  break;
      case IPP_PSTATE_PROCESSING :
          cupsLangPrintf(stdout, _("%s is ready and printing"), dest);
	  break;
      case IPP_PSTATE_STOPPED :
          cupsLangPrintf(stdout, _("%s is not ready"), dest);
	  break;
    }

    ippDelete(response);
  }
  else
    cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
}


//
// 'usage()' - Show program usage.
//

static void
usage(void)
{
  cupsLangPuts(stderr, _("Usage: lpq [options] [+interval]"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-a                      Show jobs on all destinations"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-l                      Show verbose (long) output"));
  cupsLangPuts(stdout, _("-P destination          Show status for the specified destination"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));

  exit(1);
}
