//
// "lpstat" command for CUPS.
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

static void	check_dest(const char *command, const char *name, size_t *num_dests, cups_dest_t **dests);
static int	match_list(const char *list, const char *name);
static int	show_accepting(const char *printers, size_t num_dests, cups_dest_t *dests);
static int	show_classes(const char *dests);
static void	show_default(cups_dest_t *dest);
static int	show_devices(const char *printers, size_t num_dests, cups_dest_t *dests);
static int	show_jobs(const char *dests, const char *users, bool long_status, bool show_ranking, const char *which);
static int	show_printers(const char *printers, size_t num_dests, cups_dest_t *dests, bool long_status);
static int	show_scheduler(void);
static int	usage(FILE *out);


//
// 'main()' - Parse options and show status information.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i,			// Looping var
		status = 0;		// Exit status
  char		*opt;			// Option pointer
  size_t	num_dests = 0;		// Number of user destinations
  cups_dest_t	*dests = NULL;		// User destinations
  bool		long_status = false;	// Long status report?
  bool		show_ranking = false;	// Show job ranking?
  const char	*which_jobs = "not-completed";
					// Which jobs to show?
  char		op = '\0';		// Last operation on command-line


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Parse command-line options...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
      usage();
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'D' : // Show description
	      long_status = 1;
	      break;

	  case 'E' : // Encrypt
#ifdef HAVE_TLS
	      cupsSetEncryption(HTTP_ENCRYPT_REQUIRED);
#else
	      cupsLangPrintf(stderr,
			      _("%s: Sorry, no encryption support."),
			      argv[0]);
#endif // HAVE_TLS
	      break;

	  case 'H' : // Show server and port
	      if (cupsGetServer()[0] == '/')
		cupsLangPuts(stdout, cupsGetServer());
	      else
		cupsLangPrintf(stdout, "%s:%d", cupsGetServer(), ippGetPort());
	      op = 'H';
	      break;

	  case 'P' : // Show paper types
	      op = 'P';
	      break;

	  case 'R' : // Show ranking
	      ranking = 1;
	      break;

	  case 'S' : // Show charsets
	      op = 'S';
	      if (!argv[i][2])
		i ++;
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
		  usage();
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'W' : // Show which jobs?
	      if (opt[1] != '\0')
	      {
		which = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - need \"completed\", \"not-completed\", or \"all\" after \"-W\" option."), argv[0]);
		  usage();
		}

		which = argv[i];
	      }

	      if (strcmp(which, "completed") && strcmp(which, "not-completed") && strcmp(which, "all"))
	      {
		cupsLangPrintf(stderr, _("%s: Error - need \"completed\", \"not-completed\", or \"all\" after \"-W\" option."), argv[0]);
		usage();
	      }
	      break;

	  case 'a' : // Show acceptance status
	      op = 'a';

	      if (opt[1] != '\0')
	      {
		check_dest(argv[0], opt + 1, &num_dests, &dests);

		status |= show_accepting(opt + 1, num_dests, dests);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(argv[0], argv[i], &num_dests, &dests);

		status |= show_accepting(argv[i], num_dests, dests);
	      }
	      else
	      {
		if (num_dests <= 1)
		{
		  cupsFreeDests(num_dests, dests);
		  num_dests = cupsGetDests(&dests);

		  if (num_dests == 0 && (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		  {
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		    return (1);
		  }
		}

		status |= show_accepting(NULL, num_dests, dests);
	      }
	      break;

	  case 'c' : // Show classes and members
	      op = 'c';

	      if (opt[1] != '\0')
	      {
		check_dest(argv[0], opt + 1, &num_dests, &dests);

		status |= show_classes(opt + 1);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(argv[0], argv[i], &num_dests, &dests);

		status |= show_classes(argv[i]);
	      }
	      else
		status |= show_classes(NULL);
	      break;

	  case 'd' : // Show default destination
	      op = 'd';

	      if (num_dests != 1 || !dests[0].is_default)
	      {
		cupsFreeDests(num_dests, dests);

		dests     = cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL);
		num_dests = dests ? 1 : 0;

		if (num_dests == 0 &&
		    (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		     cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		{
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		  return (1);
		}
	      }

	      show_default(dests);
	      break;

	  case 'e' : // List destinations
	      {
                cups_dest_t *temp = NULL, *dest;
                int j, num_temp = cupsGetDests(&temp);

                op = 'e';

                for (j = num_temp, dest = temp; j > 0; j --, dest ++)
                {
                  if (dest->instance)
                    printf("%s/%s", dest->name, dest->instance);
                  else
                    fputs(dest->name, stdout);

                  if (long_status)
                  {
                    const char *printer_uri_supported = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
                    const char *printer_is_temporary = cupsGetOption("printer-is-temporary", dest->num_options, dest->options);
                    const char *type = "network";

                    if (printer_is_temporary && !strcmp(printer_is_temporary, "true"))
                      type = "temporary";
                    else if (printer_uri_supported)
                      type = "permanent";

                    printf(" %s %s %s\n", type, printer_uri_supported ? printer_uri_supported : "none", cupsGetOption("device-uri", dest->num_options, dest->options));
                  }
                  else
                    putchar('\n');
                }

                cupsFreeDests(num_temp, temp);
              }
              break;

	  case 'f' : // Show forms
	      op   = 'f';
	      if (opt[1] != '\0')
	      {
	        opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;
		if (i >= argc)
		  return (1);
	      }
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
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-h\" option."), argv[0]);
		  return (1);
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'l' : // Long status or long job status
	      long_status = 2;
	      break;

	  case 'o' : // Show jobs by destination
	      op = 'o';

	      if (opt[1])
	      {
		check_dest(argv[0], opt + 1, &num_dests, &dests);

		status |= show_jobs(opt + 1, NULL, long_status, ranking, which);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(argv[0], argv[i], &num_dests, &dests);

		status |= show_jobs(argv[i], NULL, long_status, ranking, which);
	      }
	      else
		status |= show_jobs(NULL, NULL, long_status, ranking, which);
	      break;

	  case 'p' : // Show printers
	      op = 'p';

	      if (opt[1] != '\0')
	      {
		check_dest(argv[0], opt + 1, &num_dests, &dests);

		status |= show_printers(opt + 1, num_dests, dests,
					long_status);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(argv[0], argv[i], &num_dests, &dests);

		status |= show_printers(argv[i], num_dests, dests, long_status);
	      }
	      else
	      {
		if (num_dests <= 1)
		{
		  cupsFreeDests(num_dests, dests);
		  num_dests = cupsGetDests(&dests);

		  if (num_dests == 0 &&
		      (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		       cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		  {
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		    return (1);
		  }
		}

		status |= show_printers(NULL, num_dests, dests, long_status);
	      }
	      break;

	  case 'r' : // Show scheduler status
	      op = 'r';

	      if (!show_scheduler())
		return (0);
	      break;

	  case 's' : // Show summary
	      op = 's';

	      if (num_dests <= 1)
	      {
		cupsFreeDests(num_dests, dests);
		num_dests = cupsGetDests(&dests);

		if (num_dests == 0 &&
		    (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		     cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		{
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		  return (1);
		}
	      }

	      show_default(cupsGetDest(NULL, NULL, num_dests, dests));
	      status |= show_classes(NULL);
	      status |= show_devices(NULL, num_dests, dests);
	      break;

	  case 't' : // Show all info
	      op = 't';

	      if (num_dests <= 1)
	      {
		cupsFreeDests(num_dests, dests);
		num_dests = cupsGetDests(&dests);

		if (num_dests == 0 &&
		    (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		     cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		{
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		  return (1);
		}
	      }

	      if (!show_scheduler())
		return (0);

	      show_default(cupsGetDest(NULL, NULL, num_dests, dests));
	      status |= show_classes(NULL);
	      status |= show_devices(NULL, num_dests, dests);
	      status |= show_accepting(NULL, num_dests, dests);
	      status |= show_printers(NULL, num_dests, dests, long_status);
	      status |= show_jobs(NULL, NULL, long_status, ranking, which);
	      break;

	  case 'u' : // Show jobs by user
	      op = 'u';

	      if (opt[1] != '\0')
	      {
		status |= show_jobs(NULL, opt + 1, long_status, ranking, which);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		status |= show_jobs(NULL, argv[i], long_status, ranking, which);
	      }
	      else
		status |= show_jobs(NULL, NULL, long_status, ranking, which);
	      break;

	  case 'v' : // Show printer devices
	      op = 'v';

	      if (opt[1] != '\0')
	      {
		check_dest(argv[0], opt + 1, &num_dests, &dests);

		status |= show_devices(opt + 1, num_dests, dests);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(argv[0], argv[i], &num_dests, &dests);

		status |= show_devices(argv[i], num_dests, dests);
	      }
	      else
	      {
		if (num_dests <= 1)
		{
		  cupsFreeDests(num_dests, dests);
		  num_dests = cupsGetDests(&dests);

		  if (num_dests == 0 &&
		      (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
		       cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
		  {
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), argv[0]);
		    return (1);
		  }
		}

		status |= show_devices(NULL, num_dests, dests);
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), argv[0], argv[i][1]);
	      usage();
	}
      }
    }
    else
    {
      status |= show_jobs(argv[i], NULL, long_status, ranking, which);
      op = 'o';
    }
  }

  if (!op)
    status |= show_jobs(NULL, cupsGetUser(), long_status, ranking, which);

  return (status);
}


//
// 'check_dest()' - Verify that the named destination(s) exists.
//

static void
check_dest(const char  *command,	// I  - Command name
           const char  *name,		// I  - List of printer/class names
           int         *num_dests,	// IO - Number of destinations
	   cups_dest_t **dests)		// IO - Destinations
{
  const char	*dptr;			// Pointer into name
  char		*pptr,			// Pointer into printer
		printer[1024];		// Current printer/class name


 /*
  * Load the destination list as necessary...
  */

  if (*num_dests <= 1)
  {
    if (*num_dests)
      cupsFreeDests(*num_dests, *dests);

    if (strchr(name, ','))
      *num_dests = cupsGetDests(dests);
    else
    {
      strlcpy(printer, name, sizeof(printer));
      if ((pptr = strchr(printer, '/')) != NULL)
        *pptr++ = '\0';

      if ((*dests = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, pptr)) == NULL)
      {
	if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
	    cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
	  cupsLangPrintf(stderr,
			  _("%s: Error - add '/version=1.1' to server name."),
			  command);
	else
	  cupsLangPrintf(stderr,
			  _("%s: Invalid destination name in list \"%s\"."),
			  command, name);

        exit(1);
      }
      else
      {
        *num_dests = 1;
        return;
      }
    }
  }

 /*
  * Scan the name string for printer/class name(s)...
  */

  for (dptr = name; *dptr;)
  {
   /*
    * Skip leading whitespace and commas...
    */

    while (isspace(*dptr & 255) || *dptr == ',')
      dptr ++;

    if (!*dptr)
      break;

   /*
    * Extract a single destination name from the name string...
    */

    for (pptr = printer; !isspace(*dptr & 255) && *dptr != ',' && *dptr;)
    {
      if ((size_t)(pptr - printer) < (sizeof(printer) - 1))
        *pptr++ = *dptr++;
      else
      {
        cupsLangPrintf(stderr,
	                _("%s: Invalid destination name in list \"%s\"."),
			command, name);
        exit(1);
      }
    }

    *pptr = '\0';

   /*
    * Check the destination...
    */

    if (!cupsGetDest(printer, NULL, *num_dests, *dests))
    {
      if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
          cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
	cupsLangPrintf(stderr,
	                _("%s: Error - add '/version=1.1' to server name."),
			command);
      else
	cupsLangPrintf(stderr,
			_("%s: Unknown destination \"%s\"."), command, printer);

      exit(1);
    }
  }
}


//
// 'match_list()' - Match a name from a list of comma or space-separated names.
//

static int				// O - 1 on match, 0 on no match
match_list(const char *list,		// I - List of names
           const char *name)		// I - Name to find
{
  const char	*nameptr;		// Pointer into name


 /*
  * An empty list always matches...
  */

  if (!list || !*list)
    return (1);

  if (!name)
    return (0);

  do
  {
   /*
    * Skip leading whitespace and commas...
    */

    while (isspace(*list & 255) || *list == ',')
      list ++;

    if (!*list)
      break;

   /*
    * Compare names...
    */

    for (nameptr = name;
	 *nameptr && *list && tolower(*nameptr & 255) == tolower(*list & 255);
	 nameptr ++, list ++);

    if (!*nameptr && (!*list || *list == ',' || isspace(*list & 255)))
      return (1);

    while (*list && !isspace(*list & 255) && *list != ',')
      list ++;
  }
  while (*list);

  return (0);
}


//
// 'show_accepting()' - Show acceptance status.
//

static int				// O - 0 on success, 1 on fail
show_accepting(const char  *printers,	// I - Destinations
               int         num_dests,	// I - Number of user-defined dests
	       cups_dest_t *dests)	// I - User-defined destinations
{
  int		i;			// Looping var
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr;		// Current attribute
  const char	*printer,		// Printer name
		*message;		// Printer device URI
  int		accepting;		// Accepting requests?
  time_t	ptime;			// Printer state time
  char		printer_state_time[255];// Printer state time
  static const char *pattrs[] =		// Attributes we need for printers...
		{
		  "printer-name",
		  "printer-state-change-time",
		  "printer-state-message",
		  "printer-is-accepting-jobs"
		};


  if (printers != NULL && !strcmp(printers, "all"))
    printers = NULL;

 /*
  * Build a CUPS_GET_PRINTERS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(CUPS_GET_PRINTERS);

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                "requested-attributes", sizeof(pattrs) / sizeof(pattrs[0]),
		NULL, pattrs);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/");

  if (cupsGetError() == IPP_STATUS_ERROR_SERVICE_UNAVAILABLE)
  {
    cupsLangPrintf(stderr, _("%s: Scheduler is not running."), "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr,
		    _("%s: Error - add '/version=1.1' to server name."),
		    "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
    ippDelete(response);
    return (1);
  }

  if (response)
  {
   /*
    * Loop through the printers returned in the list and display
    * their devices...
    */

    for (attr = response->attrs; attr != NULL; attr = attr->next)
    {
     /*
      * Skip leading attributes until we hit a printer...
      */

      while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
        attr = attr->next;

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this printer...
      */

      printer   = NULL;
      message   = NULL;
      accepting = 1;
      ptime     = 0;

      while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER)
      {
        if (!strcmp(attr->name, "printer-name") &&
	    attr->value_tag == IPP_TAG_NAME)
	  printer = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-state-change-time") &&
	         attr->value_tag == IPP_TAG_INTEGER)
	  ptime = (time_t)attr->values[0].integer;
        else if (!strcmp(attr->name, "printer-state-message") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  message = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-is-accepting-jobs") &&
	         attr->value_tag == IPP_TAG_BOOLEAN)
	  accepting = attr->values[0].boolean;

        attr = attr->next;
      }

     /*
      * See if we have everything needed...
      */

      if (printer == NULL)
      {
        if (attr == NULL)
	  break;
	else
          continue;
      }

     /*
      * Display the printer entry if needed...
      */

      if (match_list(printers, printer))
      {
        _cupsStrDate(printer_state_time, sizeof(printer_state_time), ptime);

        if (accepting)
	  cupsLangPrintf(stdout, _("%s accepting requests since %s"),
			  printer, printer_state_time);
	else
	{
	  cupsLangPrintf(stdout, _("%s not accepting requests since %s -"),
			  printer, printer_state_time);
	  cupsLangPrintf(stdout, _("\t%s"),
			  (message && *message) ?
			      message : "reason unknown");
        }

        for (i = 0; i < num_dests; i ++)
	  if (!_cups_strcasecmp(dests[i].name, printer) && dests[i].instance)
	  {
            if (accepting)
	      cupsLangPrintf(stdout, _("%s/%s accepting requests since %s"),
			      printer, dests[i].instance, printer_state_time);
	    else
	    {
	      cupsLangPrintf(stdout,
	                      _("%s/%s not accepting requests since %s -"),
			      printer, dests[i].instance, printer_state_time);
	      cupsLangPrintf(stdout, _("\t%s"),
	        	      (message && *message) ?
			          message : "reason unknown");
            }
	  }
      }

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'show_classes()' - Show printer classes.
//

static int				// O - 0 on success, 1 on fail
show_classes(const char *dests)		// I - Destinations
{
  int		i;			// Looping var
  ipp_t		*request,		// IPP Request
		*response,		// IPP Response
		*response2;		// IPP response from remote server
  http_t	*http2;			// Remote server
  ipp_attribute_t *attr;		// Current attribute
  const char	*printer,		// Printer class name
		*printer_uri;		// Printer class URI
  ipp_attribute_t *members;		// Printer members
  char		method[HTTP_MAX_URI],	// Request method
		username[HTTP_MAX_URI],	// Username:password
		server[HTTP_MAX_URI],	// Server name
		resource[HTTP_MAX_URI];	// Resource name
  int		port;			// Port number
  static const char *cattrs[] =		// Attributes we need for classes...
		{
		  "printer-name",
		  "printer-uri-supported",
		  "member-names"
		};


  if (dests != NULL && !strcmp(dests, "all"))
    dests = NULL;

 /*
  * Build a CUPS_GET_CLASSES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(CUPS_GET_CLASSES);

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                "requested-attributes", sizeof(cattrs) / sizeof(cattrs[0]),
		NULL, cattrs);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/");

  if (cupsGetError() == IPP_STATUS_ERROR_SERVICE_UNAVAILABLE)
  {
    cupsLangPrintf(stderr, _("%s: Scheduler is not running."), "lpstat");
    ippDelete(response);
    return (1);
  }
  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
      cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr,
		    _("%s: Error - add '/version=1.1' to server name."),
		    "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
    ippDelete(response);
    return (1);
  }

  if (response)
  {
    if (response->request.status.status_code > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
      ippDelete(response);
      return (1);
    }

   /*
    * Loop through the printers returned in the list and display
    * their devices...
    */

    for (attr = response->attrs; attr != NULL; attr = attr->next)
    {
     /*
      * Skip leading attributes until we hit a job...
      */

      while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
        attr = attr->next;

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this job...
      */

      printer     = NULL;
      printer_uri = NULL;
      members     = NULL;

      do
      {
        if (!strcmp(attr->name, "printer-name") &&
	    attr->value_tag == IPP_TAG_NAME)
	  printer = attr->values[0].string.text;

        if (!strcmp(attr->name, "printer-uri-supported") &&
	    attr->value_tag == IPP_TAG_URI)
	  printer_uri = attr->values[0].string.text;

        if (!strcmp(attr->name, "member-names") &&
	    attr->value_tag == IPP_TAG_NAME)
	  members = attr;

        attr = attr->next;
      }
	  while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER);

     /*
      * If this is a remote class, grab the class info from the
      * remote server...
      */

      response2 = NULL;
      if (members == NULL && printer_uri != NULL)
      {
        httpSeparateURI(HTTP_URI_CODING_ALL, printer_uri, method, sizeof(method),
	                username, sizeof(username), server, sizeof(server),
			&port, resource, sizeof(resource));

        if (!_cups_strcasecmp(server, cupsGetServer()))
	  http2 = CUPS_HTTP_DEFAULT;
	else
	  http2 = httpConnectEncrypt(server, port, cupsEncryption());

       /*
	* Build an IPP_GET_PRINTER_ATTRIBUTES request, which requires the
	* following attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    requested-attributes
	*/

	request = ippNewRequest(IPP_GET_PRINTER_ATTRIBUTES);

	ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
		     "printer-uri", NULL, printer_uri);

	ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
		      "requested-attributes",
		      sizeof(cattrs) / sizeof(cattrs[0]),
		      NULL, cattrs);

	if ((response2 = cupsDoRequest(http2, request, "/")) != NULL)
	  members = ippFindAttribute(response2, "member-names", IPP_TAG_NAME);

	if (http2)
	  httpClose(http2);
      }

     /*
      * See if we have everything needed...
      */

      if (printer == NULL)
      {
        if (response2)
	  ippDelete(response2);

        if (attr == NULL)
	  break;
	else
          continue;
      }

     /*
      * Display the printer entry if needed...
      */

      if (match_list(dests, printer))
      {
        cupsLangPrintf(stdout, _("members of class %s:"), printer);

	if (members)
	{
	  for (i = 0; i < members->num_values; i ++)
	    cupsLangPrintf(stdout, "\t%s", members->values[i].string.text);
        }
	else
	  cupsLangPuts(stdout, "\tunknown");
      }

      if (response2)
	ippDelete(response2);

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'show_default()' - Show default destination.
//

static void
show_default(cups_dest_t *dest)		// I - Default destination
{
  const char	*printer,		// Printer name
		*val;			// Environment variable name


  if (dest)
  {
    if (dest->instance)
      cupsLangPrintf(stdout, _("system default destination: %s/%s"),
                      dest->name, dest->instance);
    else
      cupsLangPrintf(stdout, _("system default destination: %s"),
                      dest->name);
  }
  else
  {
    val = NULL;

    if ((printer = getenv("LPDEST")) == NULL)
    {
      if ((printer = getenv("PRINTER")) != NULL)
      {
        if (!strcmp(printer, "lp"))
          printer = NULL;
	else
	  val = "PRINTER";
      }
    }
    else
      val = "LPDEST";

    if (printer)
      cupsLangPrintf(stdout,
                      _("lpstat: error - %s environment variable names "
		        "non-existent destination \"%s\"."),
        	      val, printer);
    else
      cupsLangPuts(stdout, _("no system default destination"));
  }
}


//
// 'show_devices()' - Show printer devices.
//

static int				// O - 0 on success, 1 on fail
show_devices(const char  *printers,	// I - Destinations
             int         num_dests,	// I - Number of user-defined dests
	     cups_dest_t *dests)	// I - User-defined destinations
{
  int		i;			// Looping var
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr;		// Current attribute
  const char	*printer,		// Printer name
		*uri,			// Printer URI
		*device;		// Printer device URI
  static const char *pattrs[] =		// Attributes we need for printers...
		{
		  "printer-name",
		  "printer-uri-supported",
		  "device-uri"
		};


  if (printers != NULL && !strcmp(printers, "all"))
    printers = NULL;

 /*
  * Build a CUPS_GET_PRINTERS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(CUPS_GET_PRINTERS);

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                "requested-attributes", sizeof(pattrs) / sizeof(pattrs[0]),
		NULL, pattrs);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/");

  if (cupsGetError() == IPP_STATUS_ERROR_SERVICE_UNAVAILABLE)
  {
    cupsLangPrintf(stderr, _("%s: Scheduler is not running."), "lpstat");
    ippDelete(response);
    return (1);
  }
  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
      cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr,
		    _("%s: Error - add '/version=1.1' to server name."),
		    "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
    ippDelete(response);
    return (1);
  }

  if (response)
  {
   /*
    * Loop through the printers returned in the list and display
    * their devices...
    */

    for (attr = response->attrs; attr != NULL; attr = attr->next)
    {
     /*
      * Skip leading attributes until we hit a job...
      */

      while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
        attr = attr->next;

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this job...
      */

      printer = NULL;
      device  = NULL;
      uri     = NULL;

      while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER)
      {
        if (!strcmp(attr->name, "printer-name") &&
	    attr->value_tag == IPP_TAG_NAME)
	  printer = attr->values[0].string.text;

        if (!strcmp(attr->name, "printer-uri-supported") &&
	    attr->value_tag == IPP_TAG_URI)
	  uri = attr->values[0].string.text;

        if (!strcmp(attr->name, "device-uri") &&
	    attr->value_tag == IPP_TAG_URI)
	  device = attr->values[0].string.text;

        attr = attr->next;
      }

     /*
      * See if we have everything needed...
      */

      if (printer == NULL)
      {
        if (attr == NULL)
	  break;
	else
          continue;
      }

     /*
      * Display the printer entry if needed...
      */

      if (match_list(printers, printer))
      {
        if (device == NULL)
          cupsLangPrintf(stdout, _("device for %s: %s"),
	                  printer, uri);
        else if (!strncmp(device, "file:", 5))
          cupsLangPrintf(stdout, _("device for %s: %s"),
	                  printer, device + 5);
        else
          cupsLangPrintf(stdout, _("device for %s: %s"),
	                  printer, device);

        for (i = 0; i < num_dests; i ++)
        {
	  if (!_cups_strcasecmp(printer, dests[i].name) && dests[i].instance)
	  {
            if (device == NULL)
              cupsLangPrintf(stdout, _("device for %s/%s: %s"),
	                      printer, dests[i].instance, uri);
            else if (!strncmp(device, "file:", 5))
              cupsLangPrintf(stdout, _("device for %s/%s: %s"),
	                      printer, dests[i].instance, device + 5);
            else
              cupsLangPrintf(stdout, _("device for %s/%s: %s"),
	                      printer, dests[i].instance, device);
	  }
	}
      }

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'show_jobs()' - Show active print jobs.
//

static int				// O - Exit status
show_jobs(const char *dests,		// I - Destinations
          const char *users,		// I - Users
          bool       long_status,	// I - Show long status?
          bool       show_ranking,	// I - Show job ranking?
	  const char *which)		// I - Show which jobs?
{
  int		i;			// Looping var
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr,		// Current attribute
		*reasons;		// Job state reasons attribute
  const char	*dest,			// Pointer into job-printer-uri
		*username,		// Pointer to job-originating-user-name
		*message,		// Pointer to job-printer-state-message
		*time_at;		// time-at-xxx attribute name to use
  int		rank,			// Rank in queue
		jobid,			// job-id
		size;			// job-k-octets
  time_t	jobtime;		// time-at-creation
  char		temp[255],		// Temporary buffer
		date[255];		// Date buffer
  static const char *jattrs[] =		// Attributes we need for jobs...
		{
		  "job-id",
		  "job-k-octets",
		  "job-name",
		  "job-originating-user-name",
		  "job-printer-state-message",
		  "job-printer-uri",
		  "job-state-reasons",
		  "time-at-creation",
		  "time-at-completed"
		};


  if (dests != NULL && !strcmp(dests, "all"))
    dests = NULL;

 /*
  * Build a IPP_GET_JOBS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requested-attributes
  *    requesting-user-name
  *    which-jobs
  */

  request = ippNewRequest(IPP_GET_JOBS);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri",
               NULL, "ipp://localhost/");

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                "requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]),
		NULL, jattrs);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "which-jobs",
               NULL, which);

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/");

  if (cupsGetError() == IPP_STATUS_ERROR_SERVICE_UNAVAILABLE)
  {
    cupsLangPrintf(stderr, _("%s: Scheduler is not running."), "lpstat");
    ippDelete(response);
    return (1);
  }
  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
      cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr,
		    _("%s: Error - add '/version=1.1' to server name."),
		    "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
    ippDelete(response);
    return (1);
  }

  if (response)
  {
   /*
    * Loop through the job list and display them...
    */

    if (!strcmp(which, "aborted") ||
        !strcmp(which, "canceled") ||
        !strcmp(which, "completed"))
      time_at = "time-at-completed";
    else
      time_at = "time-at-creation";

    rank = -1;

    for (attr = response->attrs; attr != NULL; attr = attr->next)
    {
     /*
      * Skip leading attributes until we hit a job...
      */

      while (attr != NULL && attr->group_tag != IPP_TAG_JOB)
        attr = attr->next;

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this job...
      */

      jobid    = 0;
      size     = 0;
      username = NULL;
      dest     = NULL;
      jobtime  = 0;
      message  = NULL;
      reasons  = NULL;

      while (attr != NULL && attr->group_tag == IPP_TAG_JOB)
      {
        if (!strcmp(attr->name, "job-id") &&
	    attr->value_tag == IPP_TAG_INTEGER)
	  jobid = attr->values[0].integer;
        else if (!strcmp(attr->name, "job-k-octets") &&
		 attr->value_tag == IPP_TAG_INTEGER)
	  size = attr->values[0].integer;
        else if (!strcmp(attr->name, time_at) && attr->value_tag == IPP_TAG_INTEGER)
	  jobtime = attr->values[0].integer;
        else if (!strcmp(attr->name, "job-printer-state-message") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  message = attr->values[0].string.text;
        else if (!strcmp(attr->name, "job-printer-uri") &&
	         attr->value_tag == IPP_TAG_URI)
	{
	  if ((dest = strrchr(attr->values[0].string.text, '/')) != NULL)
	    dest ++;
        }
        else if (!strcmp(attr->name, "job-originating-user-name") &&
	         attr->value_tag == IPP_TAG_NAME)
	  username = attr->values[0].string.text;
        else if (!strcmp(attr->name, "job-state-reasons") &&
	         attr->value_tag == IPP_TAG_KEYWORD)
	  reasons = attr;

        attr = attr->next;
      }

     /*
      * See if we have everything needed...
      */

      if (dest == NULL || jobid == 0)
      {
        if (attr == NULL)
	  break;
	else
          continue;
      }

     /*
      * Display the job...
      */

      rank ++;

      if (match_list(dests, dest) && match_list(users, username))
      {
        snprintf(temp, sizeof(temp), "%s-%d", dest, jobid);

	_cupsStrDate(date, sizeof(date), jobtime);

	if (ranking)
	  cupsLangPrintf(stdout, "%3d %-21s %-13s %8.0f %s",
			  rank, temp, username ? username : "unknown",
			  1024.0 * size, date);
	else
	  cupsLangPrintf(stdout, "%-23s %-13s %8.0f   %s",
			  temp, username ? username : "unknown",
			  1024.0 * size, date);
	if (long_status)
	{
	  if (message)
	    cupsLangPrintf(stdout, _("\tStatus: %s"), message);

	  if (reasons)
	  {
	    char	alerts[1024],	// Alerts string
		      *aptr;		// Pointer into alerts string

	    for (i = 0, aptr = alerts; i < reasons->num_values; i ++)
	    {
	      if (i)
		snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", reasons->values[i].string.text);
	      else
		strlcpy(alerts, reasons->values[i].string.text, sizeof(alerts));

	      aptr += strlen(aptr);
	    }

	    cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
	  }

	  cupsLangPrintf(stdout, _("\tqueued for %s"), dest);
	}
      }

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'show_printers()' - Show printers.
//

static int				// O - 0 on success, 1 on fail
show_printers(const char  *printers,	// I - Destinations
              int         num_dests,	// I - Number of user-defined dests
	      cups_dest_t *dests,	// I - User-defined destinations
              int         long_status)	// I - Show long status?
{
  int		i, j;			// Looping vars
  ipp_t		*request,		// IPP Request
		*response,		// IPP Response
		*jobs;			// IPP Get Jobs response
  ipp_attribute_t *attr,		// Current attribute
		*jobattr,		// Job ID attribute
		*reasons;		// Job state reasons attribute
  const char	*printer,		// Printer name
		*message,		// Printer state message
		*description,		// Description of printer
		*location,		// Location of printer
		*make_model,		// Make and model of printer
		*uri;			// URI of printer
  ipp_attribute_t *allowed,		// requesting-user-name-allowed
		*denied;		// requestint-user-name-denied
  ipp_pstate_t	pstate;			// Printer state
  cups_ptype_t	ptype;			// Printer type
  time_t	ptime;			// Printer state time
  int		jobid;			// Job ID of current job
  char		printer_uri[HTTP_MAX_URI],
					// Printer URI
		printer_state_time[255];// Printer state time
  _cups_globals_t *cg = _cupsGlobals();	// Global data
  static const char *pattrs[] =		// Attributes we need for printers...
		{
		  "printer-name",
		  "printer-state",
		  "printer-state-message",
		  "printer-state-reasons",
		  "printer-state-change-time",
		  "printer-type",
		  "printer-info",
                  "printer-location",
		  "printer-make-and-model",
		  "printer-uri-supported",
		  "requesting-user-name-allowed",
		  "requesting-user-name-denied"
		};
  static const char *jattrs[] =		// Attributes we need for jobs...
		{
		  "job-id",
		  "job-state"
		};


  if (printers != NULL && !strcmp(printers, "all"))
    printers = NULL;

 /*
  * Build a CUPS_GET_PRINTERS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(CUPS_GET_PRINTERS);

  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                "requested-attributes", sizeof(pattrs) / sizeof(pattrs[0]),
		NULL, pattrs);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/");

  if (cupsGetError() == IPP_STATUS_ERROR_SERVICE_UNAVAILABLE)
  {
    cupsLangPrintf(stderr, _("%s: Scheduler is not running."), "lpstat");
    ippDelete(response);
    return (1);
  }
  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST ||
      cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr,
		    _("%s: Error - add '/version=1.1' to server name."),
		    "lpstat");
    ippDelete(response);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, "lpstat: %s", cupsGetErrorString());
    ippDelete(response);
    return (1);
  }

  if (response)
  {
   /*
    * Loop through the printers returned in the list and display
    * their status...
    */

    for (attr = response->attrs; attr != NULL; attr = attr->next)
    {
     /*
      * Skip leading attributes until we hit a job...
      */

      while (attr != NULL && attr->group_tag != IPP_TAG_PRINTER)
        attr = attr->next;

      if (attr == NULL)
        break;

     /*
      * Pull the needed attributes from this job...
      */

      printer     = NULL;
      ptime       = 0;
      ptype       = CUPS_PRINTER_LOCAL;
      pstate      = IPP_PRINTER_IDLE;
      message     = NULL;
      description = NULL;
      location    = NULL;
      make_model  = NULL;
      reasons     = NULL;
      uri         = NULL;
      jobid       = 0;
      allowed     = NULL;
      denied      = NULL;

      while (attr != NULL && attr->group_tag == IPP_TAG_PRINTER)
      {
        if (!strcmp(attr->name, "printer-name") &&
	    attr->value_tag == IPP_TAG_NAME)
	  printer = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-state") &&
	         attr->value_tag == IPP_TAG_ENUM)
	  pstate = (ipp_pstate_t)attr->values[0].integer;
        else if (!strcmp(attr->name, "printer-type") &&
	         attr->value_tag == IPP_TAG_ENUM)
	  ptype = (cups_ptype_t)attr->values[0].integer;
        else if (!strcmp(attr->name, "printer-state-message") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  message = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-state-change-time") &&
	         attr->value_tag == IPP_TAG_INTEGER)
	  ptime = (time_t)attr->values[0].integer;
	else if (!strcmp(attr->name, "printer-info") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  description = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-location") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  location = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-make-and-model") &&
	         attr->value_tag == IPP_TAG_TEXT)
	  make_model = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-uri-supported") &&
	         attr->value_tag == IPP_TAG_URI)
	  uri = attr->values[0].string.text;
        else if (!strcmp(attr->name, "printer-state-reasons") &&
	         attr->value_tag == IPP_TAG_KEYWORD)
	  reasons = attr;
        else if (!strcmp(attr->name, "requesting-user-name-allowed") &&
	         attr->value_tag == IPP_TAG_NAME)
	  allowed = attr;
        else if (!strcmp(attr->name, "requesting-user-name-denied") &&
	         attr->value_tag == IPP_TAG_NAME)
	  denied = attr;

        attr = attr->next;
      }

     /*
      * See if we have everything needed...
      */

      if (printer == NULL)
      {
        if (attr == NULL)
	  break;
	else
          continue;
      }

     /*
      * Display the printer entry if needed...
      */

      if (match_list(printers, printer))
      {
       /*
        * If the printer state is "IPP_PRINTER_PROCESSING", then grab the
	* current job for the printer.
	*/

        if (pstate == IPP_PRINTER_PROCESSING)
	{
	 /*
	  * Build an IPP_GET_JOBS request, which requires the following
	  * attributes:
	  *
	  *    attributes-charset
	  *    attributes-natural-language
	  *    printer-uri
	  *    limit
          *    requested-attributes
	  */

	  request = ippNewRequest(IPP_GET_JOBS);

	  request->request.op.operation_id = IPP_GET_JOBS;
	  request->request.op.request_id   = 1;

	  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
                	"requested-attributes",
		        sizeof(jattrs) / sizeof(jattrs[0]), NULL, jattrs);

	  httpAssembleURIf(HTTP_URI_CODING_ALL, printer_uri, sizeof(printer_uri),
	                   "ipp", NULL, "localhost", 0, "/printers/%s", printer);
	  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
	               "printer-uri", NULL, printer_uri);

          if ((jobs = cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/")) != NULL)
	  {
	   /*
	    * Get the current active job on this queue...
	    */

            ipp_jstate_t jobstate = IPP_JOB_PENDING;
	    jobid = 0;

	    for (jobattr = jobs->attrs; jobattr; jobattr = jobattr->next)
	    {
	      if (!jobattr->name)
	      {
	        if (jobstate == IPP_JOB_PROCESSING)
		  break;
	        else
		  continue;
              }

	      if (!strcmp(jobattr->name, "job-id") &&
	          jobattr->value_tag == IPP_TAG_INTEGER)
		jobid = jobattr->values[0].integer;
              else if (!strcmp(jobattr->name, "job-state") &&
	               jobattr->value_tag == IPP_TAG_ENUM)
		jobstate = (ipp_jstate_t)jobattr->values[0].integer;
	    }

            if (jobstate != IPP_JOB_PROCESSING)
	      jobid = 0;

            ippDelete(jobs);
	  }
        }

       /*
        * Display it...
	*/

        _cupsStrDate(printer_state_time, sizeof(printer_state_time), ptime);

        switch (pstate)
	{
	  case IPP_PRINTER_IDLE :
	      if (ippContainsString(reasons, "hold-new-jobs"))
		cupsLangPrintf(stdout, _("printer %s is holding new jobs.  enabled since %s"), printer, printer_state_time);
	      else
		cupsLangPrintf(stdout, _("printer %s is idle.  enabled since %s"), printer, printer_state_time);
	      break;
	  case IPP_PRINTER_PROCESSING :
	      cupsLangPrintf(stdout, _("printer %s now printing %s-%d.  enabled since %s"), printer, printer, jobid, printer_state_time);
	      break;
	  case IPP_PRINTER_STOPPED :
	      cupsLangPrintf(stdout, _("printer %s disabled since %s -"), printer, printer_state_time);
	      break;
	}

        if ((message && *message) || pstate == IPP_PRINTER_STOPPED)
	{
	  if (message && *message)
	  	cupsLangPrintf(stdout, "\t%s", message);
	  else
	    cupsLangPuts(stdout, _("\treason unknown"));
	}

        if (long_status > 1)
	{
	  cupsLangPuts(stdout, _("\tForm mounted:"));
	  cupsLangPuts(stdout, _("\tContent types: any"));
	  cupsLangPuts(stdout, _("\tPrinter types: unknown"));
	}

        if (long_status)
	{
	  cupsLangPrintf(stdout, _("\tDescription: %s"),
	                  description ? description : "");

	  if (reasons)
	  {
	    char	alerts[1024],	// Alerts string
			*aptr;		// Pointer into alerts string

	    for (i = 0, aptr = alerts; i < reasons->num_values; i ++)
	    {
	      if (i)
		snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", reasons->values[i].string.text);
	      else
		strlcpy(alerts, reasons->values[i].string.text, sizeof(alerts));

	      aptr += strlen(aptr);
	    }

	    cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
	  }
	}
        if (long_status > 1)
	{
	  cupsLangPrintf(stdout, _("\tLocation: %s"),
	                  location ? location : "");

	  if (ptype & CUPS_PRINTER_REMOTE)
	  {
	    cupsLangPuts(stdout, _("\tConnection: remote"));

	    if (make_model && !strstr(make_model, "System V Printer") &&
	             !strstr(make_model, "Raw Printer") && uri)
	      cupsLangPrintf(stdout, _("\tInterface: %s.ppd"),
	                      uri);
	  }
	  else
	  {
	    cupsLangPuts(stdout, _("\tConnection: direct"));

	    if (make_model && !strstr(make_model, "Raw Printer"))
	      cupsLangPrintf(stdout,
	                      _("\tInterface: %s/ppd/%s.ppd"),
			      cg->cups_serverroot, printer);
          }
	  cupsLangPuts(stdout, _("\tOn fault: no alert"));
	  cupsLangPuts(stdout, _("\tAfter fault: continue"));
	      // TODO update to use printer-error-policy
          if (allowed)
	  {
	    cupsLangPuts(stdout, _("\tUsers allowed:"));
	    for (j = 0; j < allowed->num_values; j ++)
	      cupsLangPrintf(stdout, "\t\t%s",
	                      allowed->values[j].string.text);
	  }
	  else if (denied)
	  {
	    cupsLangPuts(stdout, _("\tUsers denied:"));
	    for (j = 0; j < denied->num_values; j ++)
	      cupsLangPrintf(stdout, "\t\t%s",
	                      denied->values[j].string.text);
	  }
	  else
	  {
	    cupsLangPuts(stdout, _("\tUsers allowed:"));
	    cupsLangPuts(stdout, _("\t\t(all)"));
	  }
	  cupsLangPuts(stdout, _("\tForms allowed:"));
	  cupsLangPuts(stdout, _("\t\t(none)"));
	  cupsLangPuts(stdout, _("\tBanner required"));
	  cupsLangPuts(stdout, _("\tCharset sets:"));
	  cupsLangPuts(stdout, _("\t\t(none)"));
	  cupsLangPuts(stdout, _("\tDefault pitch:"));
	  cupsLangPuts(stdout, _("\tDefault page size:"));
	  cupsLangPuts(stdout, _("\tDefault port settings:"));
	}

        for (i = 0; i < num_dests; i ++)
	  if (!_cups_strcasecmp(printer, dests[i].name) && dests[i].instance)
	  {
            switch (pstate)
	    {
	      case IPP_PRINTER_IDLE :
		  cupsLangPrintf(stdout,
		                  _("printer %s/%s is idle.  "
				    "enabled since %s"),
				  printer, dests[i].instance,
				  printer_state_time);
		  break;
	      case IPP_PRINTER_PROCESSING :
		  cupsLangPrintf(stdout,
		                  _("printer %s/%s now printing %s-%d.  "
				    "enabled since %s"),
				  printer, dests[i].instance, printer, jobid,
				  printer_state_time);
		  break;
	      case IPP_PRINTER_STOPPED :
		  cupsLangPrintf(stdout,
		                  _("printer %s/%s disabled since %s -"),
				  printer, dests[i].instance,
				  printer_state_time);
		  break;
	    }

            if ((message && *message) || pstate == IPP_PRINTER_STOPPED)
	    {
	      if (message && *message)
		cupsLangPrintf(stdout, "\t%s", message);
	      else
		cupsLangPuts(stdout, _("\treason unknown"));
            }

            if (long_status > 1)
	    {
	      cupsLangPuts(stdout, _("\tForm mounted:"));
	      cupsLangPuts(stdout, _("\tContent types: any"));
	      cupsLangPuts(stdout, _("\tPrinter types: unknown"));
	    }

            if (long_status)
	    {
	      cupsLangPrintf(stdout, _("\tDescription: %s"),
	                      description ? description : "");

	      if (reasons)
	      {
		char	alerts[1024],	// Alerts string
			*aptr;		// Pointer into alerts string

		for (i = 0, aptr = alerts; i < reasons->num_values; i ++)
		{
		  if (i)
		    snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", reasons->values[i].string.text);
		  else
		    strlcpy(alerts, reasons->values[i].string.text, sizeof(alerts));

		  aptr += strlen(aptr);
		}

		cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
	      }
	    }
            if (long_status > 1)
	    {
	      cupsLangPrintf(stdout, _("\tLocation: %s"),
	                      location ? location : "");

	      if (ptype & CUPS_PRINTER_REMOTE)
	      {
		cupsLangPuts(stdout, _("\tConnection: remote"));

		if (make_model && !strstr(make_model, "System V Printer") &&
	        	 !strstr(make_model, "Raw Printer") && uri)
		  cupsLangPrintf(stdout, _("\tInterface: %s.ppd"), uri);
	      }
	      else
	      {
		cupsLangPuts(stdout, _("\tConnection: direct"));

		if (make_model && !strstr(make_model, "Raw Printer"))
		  cupsLangPrintf(stdout,
	                	  _("\tInterface: %s/ppd/%s.ppd"),
				  cg->cups_serverroot, printer);
              }
	      cupsLangPuts(stdout, _("\tOn fault: no alert"));
	      cupsLangPuts(stdout, _("\tAfter fault: continue"));
		  // TODO update to use printer-error-policy
              if (allowed)
	      {
		cupsLangPuts(stdout, _("\tUsers allowed:"));
		for (j = 0; j < allowed->num_values; j ++)
		  cupsLangPrintf(stdout, "\t\t%s",
	                	  allowed->values[j].string.text);
	      }
	      else if (denied)
	      {
		cupsLangPuts(stdout, _("\tUsers denied:"));
		for (j = 0; j < denied->num_values; j ++)
		  cupsLangPrintf(stdout, "\t\t%s",
	                	  denied->values[j].string.text);
	      }
	      else
	      {
		cupsLangPuts(stdout, _("\tUsers allowed:"));
		cupsLangPuts(stdout, _("\t\t(all)"));
	      }
	      cupsLangPuts(stdout, _("\tForms allowed:"));
	      cupsLangPuts(stdout, _("\t\t(none)"));
	      cupsLangPuts(stdout, _("\tBanner required"));
	      cupsLangPuts(stdout, _("\tCharset sets:"));
	      cupsLangPuts(stdout, _("\t\t(none)"));
	      cupsLangPuts(stdout, _("\tDefault pitch:"));
	      cupsLangPuts(stdout, _("\tDefault page size:"));
	      cupsLangPuts(stdout, _("\tDefault port settings:"));
	    }
	  }
      }

      if (attr == NULL)
        break;
    }

    ippDelete(response);
  }

  return (0);
}


//
// 'show_scheduler()' - Show scheduler status.
//

static int				// 1 on success, 0 on failure
show_scheduler(void)
{
  http_t	*http;			// Connection to server


  if ((http = httpConnectEncrypt(cupsGetServer(), ippGetPort(),
                                 cupsEncryption())) != NULL)
  {
    cupsLangPuts(stdout, _("scheduler is running"));
    httpClose(http);
    return (1);
  }
  else
  {
    cupsLangPuts(stdout, _("scheduler is not running"));
    return (0);
  }
}


//
// 'usage()' - Show program usage and exit.
//

static void
usage(void)
{
  cupsLangPuts(stdout, _("Usage: lpstat [options]"));
  cupsLangPuts(stdout, _("Options:"));
  cupsLangPuts(stdout, _("-E                      Encrypt the connection to the server"));
  cupsLangPuts(stdout, _("-h server[:port]        Connect to the named server and port"));
  cupsLangPuts(stdout, _("-l                      Show verbose (long) output"));
  cupsLangPuts(stdout, _("-U username             Specify the username to use for authentication"));

  cupsLangPuts(stdout, _("-H                      Show the default server and port"));
  cupsLangPuts(stdout, _("-W completed            Show completed jobs"));
  cupsLangPuts(stdout, _("-W not-completed        Show pending jobs"));
  cupsLangPuts(stdout, _("-a [destination(s)]     Show the accepting state of destinations"));
  cupsLangPuts(stdout, _("-c [class(es)]          Show classes and their member printers"));
  cupsLangPuts(stdout, _("-d                      Show the default destination"));
  cupsLangPuts(stdout, _("-e                      Show available destinations on the network"));
  cupsLangPuts(stdout, _("-o [destination(s)]     Show jobs"));
  cupsLangPuts(stdout, _("-p [printer(s)]         Show the processing state of destinations"));
  cupsLangPuts(stdout, _("-r                      Show whether the CUPS server is running"));
  cupsLangPuts(stdout, _("-R                      Show the ranking of jobs"));
  cupsLangPuts(stdout, _("-s                      Show a status summary"));
  cupsLangPuts(stdout, _("-t                      Show all status information"));
  cupsLangPuts(stdout, _("-u [user(s)]            Show jobs queued by the current or specified users"));
  cupsLangPuts(stdout, _("-v [printer(s)]         Show the devices for each destination"));

  exit(1);
}
