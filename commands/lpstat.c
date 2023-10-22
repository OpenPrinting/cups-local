//
// "lpq" and "lpstat" commands for CUPS.
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
static int	show_accepting(const char *command, cups_array_t *printers, size_t num_dests, cups_dest_t *dests);
static int	show_classes(const char *dests);
static void	show_default(cups_dest_t *dest);
static int	show_devices(const char *command, cups_array_t *printers, size_t num_dests, cups_dest_t *dests);
static int	show_jobs(const char *command, cups_array_t *printers, size_t num_dests, cups_dest_t *dests, cups_array_t *users, bool long_status, bool show_ranking, const char *which_jobs);
static int	show_printers(const char *command, cups_array_t *printers, size_t num_dests, cups_dest_t *dests, bool long_status);
static bool	show_scheduler(void);
static char	*strdate(char *buf, size_t bufsize, time_t timeval);
static int	usage(FILE *out, const char *command);


//
// 'main()' - Parse options and show status information.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  const char	*command;		// Command name
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


  // Get command name...
  if ((command = strrchr(argv[0], '/')) != NULL)
    command ++;
  else
    command = argv[0];

  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Parse command-line options...
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
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
	  case 'D' : // -D         Show description
	      long_status = 1;
	      break;

	  case 'E' : // -E         Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);

	      if (http)
		httpSetEncryption(http, HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'H' : // -H         Show server and port
	      if (cupsGetServer()[0] == '/')
		cupsLangPuts(stdout, cupsGetServer());
	      else
		cupsLangPrintf(stdout, "%s:%d", cupsGetServer(), ippGetPort());
	      op = 'H';
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

	  case 'P' :
	      if (!strcmp(command, "lpq"))
	      {
	        // -P DESTINATIONS Show printer status and jobs
		if (opt[1])
		{
		  check_dest(command, opt + 1, &num_dests, &dests);

		  status |= show_jobs(opt + 1, NULL, long_status, ranking, which);
		  opt += strlen(opt) - 1;
		}
		else if ((i + 1) < argc && argv[i + 1][0] != '-')
		{
		  i ++;

		  check_dest(command, argv[i], &num_dests, &dests);

		  status |= show_jobs(argv[i], NULL, long_status, ranking, which);
		}
		else
		{
		  cupsLangPrintf(stderr, _("%s: Missing destinations after '-P'."), command);
		  return (1);
		}
	      }
	      else
	      {
	        // -P              Show paper types
		op = 'P';
	      }
	      break;

	  case 'R' : // Show ranking
	      show_ranking = true;
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
		  cupsLangPrintf(stderr, _("%s: Error - expected username after '-U' option."), command);
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
		  cupsLangPrintf(stderr, _("%s: Error - need 'completed', 'not-completed', or 'all' after '-W' option."), command);
		  usage();
		}

		which = argv[i];
	      }

	      if (strcmp(which, "completed") && strcmp(which, "not-completed") && strcmp(which, "all"))
	      {
		cupsLangPrintf(stderr, _("%s: Error - need 'completed', 'not-completed', or 'all' after '-W' option."), command);
		usage();
	      }
	      break;

	  case 'a' : // Show acceptance status
	      op = 'a';

	      if (opt[1] != '\0')
	      {
		check_dest(command, opt + 1, &num_dests, &dests);

		status |= show_accepting(opt + 1, num_dests, dests);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(command, argv[i], &num_dests, &dests);

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
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
		check_dest(command, opt + 1, &num_dests, &dests);

		status |= show_classes(opt + 1);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(command, argv[i], &num_dests, &dests);

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
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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

	  case 'l' : // Long status or long job status
	      long_status = 2;
	      break;

	  case 'o' : // Show jobs by destination
	      op = 'o';

	      if (opt[1])
	      {
		check_dest(command, opt + 1, &num_dests, &dests);

		status |= show_jobs(opt + 1, NULL, long_status, show_ranking, which);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(command, argv[i], &num_dests, &dests);

		status |= show_jobs(argv[i], NULL, long_status, show_ranking, which);
	      }
	      else
		status |= show_jobs(NULL, NULL, long_status, show_ranking, which);
	      break;

	  case 'p' : // Show printers
	      op = 'p';

	      if (opt[1] != '\0')
	      {
		check_dest(command, opt + 1, &num_dests, &dests);

		status |= show_printers(opt + 1, num_dests, dests,
					long_status);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(command, argv[i], &num_dests, &dests);

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
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
		  cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
	      status |= show_jobs(NULL, NULL, long_status, show_ranking, which);
	      break;

	  case 'u' : // Show jobs by user
	      op = 'u';

	      if (opt[1] != '\0')
	      {
		status |= show_jobs(NULL, opt + 1, long_status, show_ranking, which);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		status |= show_jobs(NULL, argv[i], long_status, show_ranking, which);
	      }
	      else
		status |= show_jobs(NULL, NULL, long_status, show_ranking, which);
	      break;

	  case 'v' : // Show printer devices
	      op = 'v';

	      if (opt[1] != '\0')
	      {
		check_dest(command, opt + 1, &num_dests, &dests);

		status |= show_devices(opt + 1, num_dests, dests);
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;

		check_dest(command, argv[i], &num_dests, &dests);

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
		    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
		    return (1);
		  }
		}

		status |= show_devices(NULL, num_dests, dests);
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), command, argv[i][1]);
	      usage();
	}
      }
    }
    else
    {
      status |= show_jobs(argv[i], NULL, long_status, show_ranking, which);
      op = 'o';
    }
  }

  if (!op)
    status |= show_jobs(NULL, cupsGetUser(), long_status, show_ranking, which);

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
			  _("%s: Invalid destination name in list '%s'."),
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
	                _("%s: Invalid destination name in list '%s'."),
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
			_("%s: Unknown destination '%s'."), command, printer);

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
  * Build a IPP_OP_CUPS_GET_PRINTERS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_CUPS_GET_PRINTERS);

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
  * Build a IPP_OP_CUPS_GET_CLASSES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_CUPS_GET_CLASSES);

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
	* Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the
	* following attributes:
	*
	*    attributes-charset
	*    attributes-natural-language
	*    printer-uri
	*    requested-attributes
	*/

	request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

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
		*envname;		// Environment variable name


  if (dest)
  {
    if (dest->instance)
      cupsLangPrintf(stdout, _("system default destination: %s/%s"), dest->name, dest->instance);
    else
      cupsLangPrintf(stdout, _("system default destination: %s"), dest->name);
  }
  else
  {
    envname = NULL;

    if ((printer = getenv("LPDEST")) != NULL)
      envname = "LPDEST";
    else if ((printer = getenv("PRINTER")) != NULL && strcmp(printer, "lp"))
      envname = "PRINTER";
    else
      printer = NULL;

    if (printer)
      cupsLangPrintf(stdout, _("%s: %s environment variable names non-existent destination '%s'."), envname, printer);
    else
      cupsLangPuts(stdout, _("no system default destination"));
  }
}


//
// 'show_devices()' - Show printer devices.
//

static int				// O - 0 on success, 1 on fail
show_devices(const char   *command,	// I - Command name
	     cups_array_t *printers,	// I - Destinations to show
	     int          num_dests,	// I - Number of user-defined dests
	     cups_dest_t  *dests)	// I - User-defined destinations
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
  * Build a IPP_OP_CUPS_GET_PRINTERS request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    requested-attributes
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_CUPS_GET_PRINTERS);

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
show_jobs(const char   *command,	// I - Command name
	  cups_array_t *printers,	// I - Destinations to show
	  int          num_dests,	// I - Number of destinationss
	  cups_dest_t  *dests,		// I - Destinations
          cups_array_t *users,		// I - Users
          bool         long_status,	// I - Show long status?
          bool         show_ranking,	// I - Show job ranking?
	  const char   *which_jobs)	// I - Show which jobs?
{
  size_t	i;			// Looping var
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
  static const char * const jattrs[] =	// Attributes we need for jobs...
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

  // Get jobs list...
  request = ippNewRequest(IPP_OP_GET_JOBS);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, "ipp://localhost/");
  ippAddStrings(request, IPP_TAG_OPERATION, IPP_CONST_TAG(IPP_TAG_KEYWORD), "requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]), NULL, jattrs);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "which-jobs", NULL, which_jobs);

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

static int				// O - Exit status
show_printers(const char   *command,	// I - Command name
              cups_array_t *printers,	// I - Destinations
              int          num_dests,	// I - Number of user-defined dests
	      cups_dest_t  *dests,	// I - User-defined destinations
              bool         long_status)	// I - Show long status?
{
  int		ret = 0;		// Return value
  size_t	i, j;			// Looping vars
  cups_dest_t	*dest;			// Current destination
  http_t	*http;			// Connection to printer
  char		resource[1024];		// Resource path
  ipp_t		*request,		// IPP Request
		*response,		// IPP Response
		*jobs;			// IPP Get-Jobs response
  ipp_attribute_t *attr,		// Current attribute
		*jobattr,		// Current job attribute
		*job_state_reasons;	// "job-state-reasons" attribute
  static const char * const pattrs[] =	// Attributes we need for printers...
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
  static const char * const jattrs[] =	// Attributes we need for jobs...
  {
    "job-id",
    "job-state"
  };


  // Loop through the available destinations and report on those that match...
  for (i = num_dests, dest = dests; i > 0; i --, dest ++)
  {
    int		job_id = 0;			// Current "job-id" value
    const char	*printer_uri,			// "printer-uri" value
		*printer_info = NULL,		// "printer-info" value
		*printer_location = NULL,	// "printer-location" value
		*printer_make_and_model = NULL;	// "printer-make-and-model" value
    ipp_pstate_t printer_state = IPP_PSTATE_STOPPED;
						// "printer-state" value
    char	printer_state_change_date[255];	// "printer-state-change-date-time" string
    time_t	printer_state_change_time;	// "printer-state-change-date-time" value
    const char	*printer_state_message = NULL,	// "printer-state-message" value
    ipp_attribute_t *printer_state_reasons = NULL,
						// "printer-state-reasons" value
    cups_ptype_t printer_type = CUPS_PRINTER_LOCAL;
						// "printer-type" value

    // Filter out printers we don't care about...
    if (dest->instance || (cupsArrayGetCount(printers) > 0 && !cupsArrayFind(printers, dest->name)))
      continue;

    printer_uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);

    // Connect to this printer...
    if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, resourcesize, /*cb*/NULL, /*user_data*/NULL)) == NULL)
    {
      cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, dest->name, cupsGetErrorString());
      ret = 1;
      continue;
    }

    // Get printer attributes...
    request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
    ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes", sizeof(pattrs) / sizeof(pattrs[0]), NULL, pattrs);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

    response = cupsDoRequest(http, request, resource);

    if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
    {
      cupsLangPrintf(stderr, _("%s: Add '/version=1.1' to server name."), command);
      ippDelete(response);
      ret = 1;
      continue;
    }
    else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
    {
      cupsLangPrintf(stderr, _("%s: Unable to get '%s' information: %s"), command, dest->name, cupsGetErrorString());
      ippDelete(response);
      ret = 1;
      continue;
    }

    // Display its status...
    for (attr = ippGetFirstAttribute(response); attr; attr = ippGetNextAttribute(response))
    {
      // Pull the needed attributes for this printer...
      const char	*name;		// Attribute name
      ipp_tag_t		value_tag;	// Attribute syntax

      name      = ippGetName(attr);
      value_tag = ippGetValueTag(attr);

      if (!strcmp(name, "printer-state") && value_tag == IPP_TAG_ENUM)
	printer_state = (ipp_pstate_t)ippGetInteger(attr, 0);
      else if (!strcmp(name, "printer-type") && value_tag == IPP_TAG_ENUM)
	printer_type = (cups_ptype_t)ippGetInteger(attr, 0);
      else if (!strcmp(name, "printer-state-message") && value_tag == IPP_TAG_TEXT)
	printer_state_message = ippGetString(attr, 0, NULL);
      else if (!strcmp(name, "printer-state-change-date-time") && value_tag == IPP_TAG_DATE)
	printer_state_change_time = ippDateToTime(ippGetDate(attr, 0));
      else if (!strcmp(name, "printer-info") && value_tag == IPP_TAG_TEXT)
	printer_info = ippGetString(attr, 0, NULL);
      else if (!strcmp(name, "printer-location") && value_tag == IPP_TAG_TEXT)
	printer_location = ippGetString(attr, 0, NULL);
      else if (!strcmp(name, "printer-make-and-model") && value_tag == IPP_TAG_TEXT)
	printer_make_and_model = ippGetString(attr, 0, NULL);
      else if (!strcmp(name, "printer-state-reasons") && value_tag == IPP_TAG_KEYWORD)
	printer_state_reasons = attr;
    }

    // If the printer state is `IPP_PSTATE_PROCESSING`, then grab the current job for the printer.
    if (printer_state == IPP_PSTATE_PROCESSING)
    {
      request = ippNewRequest(IPP_OP_GET_JOBS);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
      ippAddStrings(request, IPP_TAG_OPERATION, IPP_CONST_TAG(IPP_TAG_KEYWORD), "requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]), NULL, jattrs);
      ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "limit", 1);

      jobs = cupsDoRequest(http, request, resource);

      if (ippGetInteger(ippFindAttribute(jobs, "job-state", IPP_TAG_ENUM), 0) == IPP_JSTATE_PROCESSING)
        job_id = ippGetInteger(ippFindAttribute(jobs, "job-id", IPP_TAG_INTEGER), 0);

      ippDelete(jobs);
    }

    // Display it...
    strdate(printer_state_change_date, sizeof(printer_state_change_date), printer_state_change_time);

    switch (printer_state)
    {
      case IPP_PSTATE_IDLE :
	  if (ippContainsString(printer_state_reasons, "hold-new-jobs"))
	    cupsLangPrintf(stdout, _("printer %s is holding new jobs.  enabled since %s"), dest->name, printer_state_change_date);
	  else
	    cupsLangPrintf(stdout, _("printer %s is idle.  enabled since %s"), dest->name, printer_state_change_date);
	  break;
      case IPP_PSTATE_PROCESSING :
	  cupsLangPrintf(stdout, _("printer %s now printing %s-%d.  enabled since %s"), dest->name, dest->name, job_id, printer_state_change_date);
	  break;
      case IPP_PSTATE_STOPPED :
	  cupsLangPrintf(stdout, _("printer %s disabled since %s -"), dest->name, printer_state_change_date);
	  break;
    }

    if ((printer_state_message && *printer_state_message) || printer_state == IPP_PSTATE_STOPPED)
    {
      if (printer_state_message && *printer_state_message)
	cupsLangPrintf(stdout, "\t%s", printer_state_message);
      else
	cupsLangPuts(stdout, _("\treason unknown"));
    }

    if (long_status)
    {
      cupsLangPrintf(stdout, _("\tDescription: %s"), printer_info ? printer_info : "");
      cupsLangPrintf(stdout, _("\tMake and Model: %s"), printer_make_and_model ? printer_make_and_model : "");

      if (printer_state_reasons)
      {
        size_t	count;			// Number of values
	char	alerts[1024],		// Alerts string
		*aptr;			// Pointer into alerts string

	for (j = 0, count = ippGetCount(printer_state_reasons), aptr = alerts; j < count; j ++)
	{
	  if (j)
	    snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", ippGetString(printer_state_reasons, j, NULL));
	  else
	    cupsCopyString(alerts, ippGetString(printer_state_reasons, j, NULL), sizeof(alerts));

	  aptr += strlen(aptr);
	}

	cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
      }

      cupsLangPrintf(stdout, _("\tLocation: %s"), printer_location ? printer_location : "");
    }

    ippDelete(response);
    httpClose(http);
  }

  return (ret);
}


//
// 'show_scheduler()' - Show scheduler status.
//

static bool				// O - `true` is running, `false` otherwise
show_scheduler(void)
{
  http_t	*http;			// Connection to server


  if ((http = httpConnect(cupsGetServer(), ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, 30000, /*cancel*/NULL)) != NULL)
  {
    cupsLangPuts(stdout, _("scheduler is running"));
    httpClose(http);
    return (true);
  }
  else
  {
    cupsLangPuts(stdout, _("scheduler is not running"));
    return (false);
  }
}


//
// 'strdate()' - Return a localized date for a given time value.
//
// This function works around the locale encoding issues of strftime...
//

static char *				// O - Formatted date
strdate(char   *buf,			// I - Buffer
	size_t bufsize,			// I - Size of buffer
	time_t timeval)			// I - Time value
{
  struct tm	date;			// Local date/time
  char		temp[1024];		// Temporary buffer


  // Generate the local date and time...
  localtime_r(&timeval, &date);

  if (cupsLangGetEncoding() != CUPS_ENCODING_UTF8)
  {
    // Format to the temporary buffer and convert to UTF-8...
    strftime(temp, sizeof(temp), "%c", &date);
    cupsCharsetToUTF8(buf, temp, bufsize, cupsLangGetEncoding());
  }
  else
  {
    // Format directly to the buffer (UTF-8)
    strftime(buf, bufsize, "%c", &date);
  }

  return (buf);
}


//
// 'usage()' - Show program usage and exit.
//

static int				// O - Exit status
usage(FILE       *out,			// I - Output file
      const char *command)		// I - Command name
{
  cupsLangPrintf(out, _("Usage: %s [OPTIONS]"), command);
  cupsLangPuts(out, _("Options:"));
  if (!strcmp(command, "lpq"))
  {
    cupsLangPuts(out, _("-a                             Show jobs on all destinations"));
  }
  else
  {
    cupsLangPuts(out, _("-a [DESTINATIONS]              Show the accepting state of destinations"));
    cupsLangPuts(out, _("-c [DESTINATIONS]              Show classes and their member printers"));
    cupsLangPuts(out, _("-d                             Show the default destination"));
  }
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  if (!strcmp(command, "lpstat"))
  {
    cupsLangPuts(out, _("-e                             Show available destinations on the network"));
    cupsLangPuts(out, _("-H                             Show the default server and port"));
  }
  cupsLangPuts(out, _("-h SERVER[:PORT]               Connect to the named server and port"));
  cupsLangPuts(out, _("-l                             Show verbose (long) output"));
  if (!strcmp(command, "lpq"))
  {
    cupsLangPuts(out, _("-P [DESTINATIONS]              Show the processing state and jobs of destinations"));
  }
  else
  {
    cupsLangPuts(out, _("-o [DESTINATIONS]              Show jobs of destinations"));
    cupsLangPuts(out, _("-p [DESTINATIONS]              Show the processing state of destinations"));
    cupsLangPuts(out, _("-R                             Show the ranking of jobs"));
    cupsLangPuts(out, _("-r                             Show whether the CUPS server is running"));
    cupsLangPuts(out, _("-s                             Show a status summary"));
    cupsLangPuts(out, _("-t                             Show all status information"));
  }
  cupsLangPuts(out, _("-U USERNAME                    Specify the username to use for authentication"));
  if (!strcmp(command, "lpq"))
  {
    cupsLangPuts(out, _("+INTERVAL                      Repeat every N seconds"));
  }
  else
  {
    cupsLangPuts(out, _("-u [USERS]                     Show jobs queued by the current or specified users"));
    cupsLangPuts(out, _("-v [DESTINATIONS]              Show the devices for each destination"));
    cupsLangPuts(out, _("-W completed                   Show completed jobs"));
    cupsLangPuts(out, _("-W not-completed               Show pending jobs"));
  }

  return (out == stdout ? 0 : 1);
}
