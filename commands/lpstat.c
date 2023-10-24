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

static bool	list_dest(bool *long_status, cups_dest_flags_t flags, cups_dest_t *dest);
static int	show_accepting(const char *command, size_t num_dests, cups_dest_t *dests, cups_array_t *printers);
static int	show_classes(const char *command, cups_array_t *printers);
static void	show_default(const char *command, size_t num_dests, cups_dest_t *dests);
static int	show_devices(const char *command, size_t num_dests, cups_dest_t *dests, cups_array_t *printers);
static int	show_jobs(const char *command, size_t num_dests, cups_dest_t *dests, cups_array_t *printers, cups_array_t *users, bool long_status, bool show_ranking, const char *which_jobs);
static int	show_printers(const char *command, size_t num_dests, cups_dest_t *dests, cups_array_t *printers, bool long_status);
static bool	show_scheduler(void);
static char	*strdate(char *buf, size_t bufsize, time_t timeval);
static void	update_dests(const char *command, size_t *num_dests, cups_dest_t **dests);
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
  cups_array_t	*list;			// Printer/user list
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
	  case 'a' : // Show acceptance status
	      op = 'a';

	      if (opt[1] != '\0')
	      {
		list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      update_dests(command, &num_dests, &dests);
	      status |= show_accepting(command, num_dests, dests, list);
	      cupsArrayDelete(list);
	      break;

	  case 'c' : // Show classes and members
	      op = 'c';

	      if (opt[1] != '\0')
	      {
	        list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      status |= show_classes(command, list);
	      cupsArrayDelete(list);
	      break;

	  case 'D' : // -D         Show description
	  case 'l' : // -l         Long status or long job status
	      long_status = true;
	      break;

	  case 'd' : // -d         Show default destination
	      op = 'd';

	      update_dests(command, &num_dests, &dests);
	      show_default(command, num_dests, dests);
	      break;

	  case 'E' : // -E         Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'e' : // -e         List destinations
	      op = 'e';

	      cupsEnumDests(CUPS_DEST_FLAGS_NONE, 10000, /*cancel*/NULL, (cups_ptype_t)0, (cups_ptype_t)0, (cups_dest_cb_t)list_dest, &long_status);
              break;

	  case 'H' : // -H         Show server and port
	      if (cupsGetServer()[0] == '/')
		cupsLangPuts(stdout, cupsGetServer());
	      else
		cupsLangPrintf(stdout, "%s:%d", cupsGetServer(), ippGetPort());

	      op = 'H';
	      break;

	  case 'h' : // -h SERVER  Connect to host
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

	  case 'o' : // -o DESTS   Show jobs by destination
	      op = 'o';

	      if (opt[1])
	      {
		list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      update_dests(command, &num_dests, &dests);
	      status |= show_jobs(command, num_dests, dests, list, /*users*/NULL, long_status, show_ranking, which_jobs);
	      cupsArrayDelete(list);
	      break;

	  case 'P' :
	      if (!strcmp(command, "lpq"))
	      {
	        // -P DESTINATIONS Show printer status and jobs
		if (opt[1])
		{
		  list = cupsArrayNewStrings(opt + 1, ',');
		  opt += strlen(opt) - 1;
		}
		else if ((i + 1) < argc && argv[i + 1][0] != '-')
		{
		  i ++;
		  list = cupsArrayNewStrings(argv[i], ',');
		}
		else
		{
		  cupsLangPrintf(stderr, _("%s: Missing destinations after '-P'."), command);
		  return (1);
		}

		update_dests(command, &num_dests, &dests);
		status |= show_jobs(command, num_dests, dests, list, /*users*/NULL, long_status, show_ranking, which_jobs);
		cupsArrayDelete(list);
	      }
	      else
	      {
	        // -P              Show paper types
		op = 'P';
	      }
	      break;

	  case 'p' : // -p DESTS   Show printers
	      op = 'p';

	      if (opt[1] != '\0')
	      {
		list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      update_dests(command, &num_dests, &dests);
	      status |= show_printers(command, num_dests, dests, list, long_status);
	      cupsArrayDelete(list);
	      break;

	  case 'R' : // Show ranking
	      show_ranking = true;
	      break;

	  case 'r' : // Show scheduler status
	      op = 'r';

	      if (!show_scheduler())
		return (0);
	      break;

	  case 'S' : // Show charsets
	      op = 'S';
	      if (!argv[i][2])
		i ++;
	      break;

	  case 's' : // Show summary
	      op = 's';

	      update_dests(command, &num_dests, &dests);
	      show_default(command, num_dests, dests);
	      status |= show_classes(command, /*printers*/NULL);
	      status |= show_devices(command, num_dests, dests, /*printers*/NULL);
	      break;

	  case 't' : // Show all info
	      op = 't';

	      if (!show_scheduler())
		return (0);

	      update_dests(command, &num_dests, &dests);
	      show_default(command, num_dests, dests);
	      status |= show_classes(command, /*printers*/NULL);
	      status |= show_devices(command, num_dests, dests, /*printers*/NULL);
	      status |= show_accepting(command, num_dests, dests, /*printers*/NULL);
	      status |= show_printers(command, num_dests, dests, /*printers*/NULL, long_status);
	      status |= show_jobs(command, num_dests, dests, /*printers*/NULL, /*users*/NULL, long_status, show_ranking, which_jobs);
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
		  return (usage(stderr, command));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  case 'u' : // Show jobs by user
	      op = 'u';

	      if (opt[1] != '\0')
	      {
	        list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
	        list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      update_dests(command, &num_dests, &dests);
	      status |= show_jobs(command, num_dests, dests, /*printers*/NULL, list, long_status, show_ranking, which_jobs);
	      cupsArrayDelete(list);
	      break;

	  case 'v' : // Show printer devices
	      op = 'v';

	      if (opt[1] != '\0')
	      {
		list = cupsArrayNewStrings(opt + 1, ',');
	        opt += strlen(opt) - 1;
	      }
	      else if ((i + 1) < argc && argv[i + 1][0] != '-')
	      {
		i ++;
		list = cupsArrayNewStrings(argv[i], ',');
	      }
	      else
	      {
	        list = NULL;
	      }

	      update_dests(command, &num_dests, &dests);
	      status |= show_devices(command, num_dests, dests, list);
	      cupsArrayDelete(list);
	      break;

	  case 'W' : // Show which jobs?
	      if (opt[1] != '\0')
	      {
		which_jobs = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - need 'completed', 'not-completed', or 'all' after '-W' option."), command);
		  return (usage(stderr, command));
		}

		which_jobs = argv[i];
	      }

	      if (strcmp(which_jobs, "completed") && strcmp(which_jobs, "not-completed") && strcmp(which_jobs, "all"))
	      {
		cupsLangPrintf(stderr, _("%s: Need 'completed', 'not-completed', or 'all' after '-W' option."), command);
		return (usage(stderr, command));
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), command, argv[i][1]);
	      return (usage(stderr, command));
	}
      }
    }
    else
    {
      // DESTINATION[,...,DESTINATION]
      op   = 'o';
      list = cupsArrayNewStrings(argv[i], ',');

      update_dests(command, &num_dests, &dests);
      status |= show_jobs(command, num_dests, dests, list, /*users*/NULL, long_status, show_ranking, which_jobs);
      cupsArrayDelete(list);
    }
  }

  if (!op)
  {
    list = cupsArrayNewStrings(NULL, ',');
    cupsArrayAdd(list, (void *)cupsGetUser());

    update_dests(command, &num_dests, &dests);
    status |= show_jobs(command, num_dests, dests, /*printers*/NULL, list, long_status, show_ranking, which_jobs);
    cupsArrayDelete(list);
  }

  return (status);
}


//
// 'list_dest()' - List a destination found via enumeration...
//

static bool				// O - `true` to continue
list_dest(
    bool              *long_status,	// I - User data (unused)
    cups_dest_flags_t flags,		// I - Flags (unused)
    cups_dest_t       *dest)		// I - Destination
{
  (void)flags;

  if (*long_status)
  {
    const char *printer_uri_supported;	// "printer-uri-supported" value
    const char *printer_is_temporary;	// "printer-is-temporary" value
    const char *device_uri;		// "[smi-]device-uri" value
    const char *type = "network";	// Type of queue

    if ((printer_uri_supported = cupsGetOption("printer-uri-supported", dest->num_options, dest->options)) == NULL)
      printer_uri_supported = "none";

    if ((printer_is_temporary = cupsGetOption("printer-is-temporary", dest->num_options, dest->options)) != NULL && !strcmp(printer_is_temporary, "true"))
      type = "temporary";
    else if (printer_uri_supported)
      type = "permanent";

    if ((device_uri = cupsGetOption("smi55357-device-uri", dest->num_options, dest->options)) == NULL)
    {
      if ((device_uri = cupsGetOption("device-uri", dest->num_options, dest->options)) == NULL)
        device_uri = "file:///dev/null";
    }

    if (dest->instance)
      cupsLangPrintf(stdout, "%s/%s %s %s %s\n", dest->name, dest->instance, type, printer_uri_supported, device_uri);
    else
      cupsLangPrintf(stdout, "%s %s %s %s\n", dest->name, type, printer_uri_supported, device_uri);
  }
  else if (dest->instance)
  {
    cupsLangPrintf(stdout, "%s/%s", dest->name, dest->instance);
  }
  else
  {
    cupsLangPuts(stdout, dest->name);
  }

  return (true);
}


//
// 'show_accepting()' - Show acceptance status.
//

static int				// O - 0 on success, 1 on fail
show_accepting(const char   *command,	// I - Command name
	       size_t       num_dests,	// I - Number of user-defined dests
	       cups_dest_t  *dests,	// I - User-defined destinations
	       cups_array_t *printers)	// I - Destinations to show
{
  size_t	i;			// Looping vars
  cups_dest_t	*dest;			// Current destination


  // Loop through the available destinations and report on those that match...
  for (i = num_dests, dest = dests; i > 0; i --, dest ++)
  {
    const char	*value;			// Temporary value
    bool	is_accepting_jobs;	// "printer-is-accepting-jobs" value
    char	state_change_date[255];	// "printer-state-change-date-time" string
    time_t	state_change_time;	// "printer-state-change-date-time" value
    const char	*state_message;		// "printer-state-message" value

    // Filter out printers we don't care about...
    if (dest->instance || (cupsArrayGetCount(printers) > 0 && !cupsArrayFind(printers, dest->name)))
      continue;

    // Grab values and report them...
    value             = cupsGetOption("printer-is-accepting-jobs", dest->num_options, dest->options);
    is_accepting_jobs = value && !strcmp(value, "true");
    if ((state_change_time = (time_t)cupsGetIntegerOption("printer-state-change-date-time", dest->num_options, dest->options)) == (time_t)LONG_MIN)
      state_change_time = (time_t)cupsGetIntegerOption("printer-state-change-time", dest->num_options, dest->options);
    state_message = cupsGetOption("printer-state-message", dest->num_options, dest->options);

    strdate(state_change_date, sizeof(state_change_date), state_change_time);

    if (is_accepting_jobs)
    {
      cupsLangPrintf(stdout, _("%s accepting requests since %s"), dest->name, state_change_date);
    }
    else
    {
      cupsLangPrintf(stdout, _("%s not accepting requests since %s -"), dest->name, state_change_date);

      if (state_message && *state_message)
      	cupsLangPrintf(stdout, "\t%s", state_message);
      else
        cupsLangPuts(stdout, _("\treason unknown"));
    }
  }

  return (0);
}


//
// 'show_classes()' - Show printer classes.
//

static int				// O - 0 on success, 1 on fail
show_classes(const char   *command,	// I - Command name
	     cups_array_t *printers)	// I - Destinations to show
{
  // TODO: Implement class support once cups-sharing has it...
  (void)command;
  (void)printers;
  return (0);

#if 0
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
#endif // 0
}


//
// 'show_default()' - Show default destination.
//

static void
show_default(const char  *command,	// I - Command name
	     size_t      num_dests,	// I - Number of user-defined dests
	     cups_dest_t *dests)	// I - User-defined destinations
{
  cups_dest_t	*dest;			// Default printer
  const char	*printer,		// Printer name
		*envname;		// Environment variable name


  if ((dest = cupsGetDest(/*printer*/NULL, /*instance*/NULL, num_dests, dests)) != NULL)
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
      cupsLangPrintf(stdout, _("%s: %s environment variable names non-existent destination '%s'."), command, envname, printer);
    else
      cupsLangPuts(stdout, _("no system default destination"));
  }
}


//
// 'show_devices()' - Show printer devices.
//

static int				// O - 0 on success, 1 on fail
show_devices(const char   *command,	// I - Command name
	     size_t       num_dests,	// I - Number of user-defined dests
	     cups_dest_t  *dests,	// I - User-defined destinations
	     cups_array_t *printers)	// I - Destinations to show
{
  size_t	i;			// Looping var
  cups_dest_t	*dest;			// Current destination


  // Loop through the available destinations and report on those that match...
  for (i = num_dests, dest = dests; i > 0; i --, dest ++)
  {
    const char	*device_uri;			// "device-uri" value

    // Filter out printers we don't care about...
    if (dest->instance || (cupsArrayGetCount(printers) > 0 && !cupsArrayFind(printers, dest->name)))
      continue;

    // Grab values and report them...
    if ((device_uri = cupsGetOption("smi55357-device-uri", dest->num_options, dest->options)) == NULL)
      device_uri = cupsGetOption("device-uri", dest->num_options, dest->options);

    if (device_uri)
      cupsLangPrintf(stdout, _("device for %s: %s"), dest->name, device_uri);
    else
      cupsLangPrintf(stdout, _("device for %s: unknown"), dest->name);
  }

  return (0);
}


//
// 'show_jobs()' - Show active print jobs.
//

static int				// O - Exit status
show_jobs(const char   *command,	// I - Command name
	  size_t       num_dests,	// I - Number of destinationss
	  cups_dest_t  *dests,		// I - Destinations
	  cups_array_t *printers,	// I - Destinations to show
          cups_array_t *users,		// I - Users
          bool         long_status,	// I - Show long status?
          bool         show_ranking,	// I - Show job ranking?
	  const char   *which_jobs)	// I - Show which jobs?
{
  int		ret = 0;		// Return value
  size_t	i;			// Looping var
  cups_dest_t	*dest;			// Current destination
  http_t	*http;			// HTTP connection
  char		resource[1024];		// Resource path
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr;		// Current attribute
  const char	*name;			// Attribute name
  ipp_tag_t	value_tag;		// Value syntax
  const char	*date_time_at_attr,	// "date-time-at-xxx" attribute name to use
		*time_at_attr;		// "time-at-xxx" attribute name to use
  int		rank,			// Rank in queue
		job_id,			// "job-id" value
		k_octets;		// "job-k-octets" value
  ipp_attribute_t *state_reasons;	// "job-state-reasons" value
  time_t	time_at;		// "[date-]time-at-xxx" value
  const char	*username;		// Pointer to job-originating-user-name
  char		dest_job_id[255],	// DEST-JOBID string
		time_at_date[255];	// "[date-]time-at-xxx" string
  static const char * const jattrs[] =	// Attributes we need for jobs...
  {
    "date-time-at-creation",
    "date-time-at-completed",
    "job-id",
    "job-k-octets",
    "job-name",
    "job-originating-user-name",
    "job-state-reasons",
    "time-at-creation",
    "time-at-completed"
  };


  // Loop through the available destinations and report on those that match...
  for (i = num_dests, dest = dests; i > 0; i --, dest ++)
  {
    // Filter out printers we don't care about...
    if (dest->instance || (cupsArrayGetCount(printers) > 0 && !cupsArrayFind(printers, dest->name)))
      continue;

    // Connect to this printer...
    if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, sizeof(resource), /*cb*/NULL, /*user_data*/NULL)) == NULL)
    {
      cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, dest->name, cupsGetErrorString());
      ret = 1;
      continue;
    }

    // Get jobs list...
    request = ippNewRequest(IPP_OP_GET_JOBS);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, cupsGetOption("printer-uri-supported", dest->num_options, dest->options));
    ippAddStrings(request, IPP_TAG_OPERATION, IPP_CONST_TAG(IPP_TAG_KEYWORD), "requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]), NULL, jattrs);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "which-jobs", NULL, which_jobs);

    response = cupsDoRequest(http, request, resource);

    // Loop through the job list and display them...
    if (!strcmp(which_jobs, "completed"))
    {
      date_time_at_attr = "date-time-at-completed";
      time_at_attr      = "time-at-completed";
    }
    else
    {
      date_time_at_attr = "date-time-at-creation";
      time_at_attr      = "time-at-creation";
    }

    rank = -1;

    attr = ippGetFirstAttribute(response);
    while (attr)
    {
      // Skip leading attributes until we hit a job...
      while (attr && ippGetGroupTag(attr) != IPP_TAG_JOB)
        attr = ippGetNextAttribute(response);

      if (!attr)
        break;

      // Pull the needed attributes from this job...
      job_id        = 0;
      k_octets      = 0;
      username      = NULL;
      time_at       = 0;
      state_reasons = NULL;

      while (attr && ippGetGroupTag(attr) == IPP_TAG_JOB)
      {
        name      = ippGetName(attr);
        value_tag = ippGetValueTag(attr);

        if (!strcmp(name, "job-id") && value_tag == IPP_TAG_INTEGER)
	  job_id = ippGetInteger(attr, 0);
        else if (!strcmp(name, "job-k-octets") && value_tag == IPP_TAG_INTEGER)
	  k_octets = ippGetInteger(attr, 0);
        else if (!strcmp(name, date_time_at_attr) && value_tag == IPP_TAG_DATE)
	  time_at = ippDateToTime(ippGetDate(attr, 0));
        else if (!strcmp(name, time_at_attr) && value_tag == IPP_TAG_INTEGER && !time_at)
	  time_at = (time_t)ippGetInteger(attr, 0);
        else if (!strcmp(name, "job-originating-user-name") && value_tag == IPP_TAG_NAME)
	  username = ippGetString(attr, 0, NULL);
        else if (!strcmp(name, "job-state-reasons") && value_tag == IPP_TAG_KEYWORD)
	  state_reasons = attr;

        attr = ippGetNextAttribute(response);
      }

      // See if we have everything needed...
      if (job_id == 0)
	continue;

      // Display the job...
      rank ++;

      if (cupsArrayGetCount(users) > 0 && !cupsArrayFind(users, (void *)username))
        continue;			// This is not the user you are looking for...

      strdate(time_at_date, sizeof(time_at_date), time_at);
      snprintf(dest_job_id, sizeof(dest_job_id), "%s-%d", dest->name, job_id);

      if (show_ranking)
	cupsLangPrintf(stdout, _("%3d %-21s %-13s %8.0f %s"), rank, dest_job_id, username ? username : "anonymous", 1024.0 * k_octets, time_at_date);
      else
	cupsLangPrintf(stdout, _("%-23s %-13s %8.0f   %s"), dest_job_id, username ? username : "anonymous", 1024.0 * k_octets, time_at_date);

      if (long_status)
      {
	size_t	count;			// Number of reasons
	if ((count = ippGetCount(state_reasons)) > 0)
	{
	  char		alerts[1024],	// Alerts string
			*aptr;		// Pointer into alerts string

	  for (i = 0, aptr = alerts; i < count; i ++)
	  {
	    if (i)
	      snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", ippGetString(state_reasons, i, NULL));
	    else
	      cupsCopyString(alerts, ippGetString(state_reasons, i, NULL), sizeof(alerts));

	    aptr += strlen(aptr);
	  }

	  cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
	}

	cupsLangPrintf(stdout, _("\tqueued for %s"), dest->name);
      }
    }

    ippDelete(response);
  }

  return (ret);
}


//
// 'show_printers()' - Show printers.
//

static int				// O - Exit status
show_printers(const char   *command,	// I - Command name
              size_t       num_dests,	// I - Number of user-defined dests
	      cups_dest_t  *dests,	// I - User-defined destinations
              cups_array_t *printers,	// I - Destinations
              bool         long_status)	// I - Show long status?
{
  int		ret = 0;		// Return value
  size_t	i, j;			// Looping vars
  cups_dest_t	*dest;			// Current destination


  // Loop through the available destinations and report on those that match...
  for (i = num_dests, dest = dests; i > 0; i --, dest ++)
  {
    int		job_id = 0;		// Current "job-id" value
    const char	*info,			// "printer-info" value
		*location,		// "printer-location" value
		*make_and_model;	// "printer-make-and-model" value
    ipp_pstate_t state;			// "printer-state" value
    char	state_change_date[255];	// "printer-state-change-date-time" string
    time_t	state_change_time;	// "printer-state-change-date-time" value
    const char	*state_message;		// "printer-state-message" value
    cups_array_t *state_reasons;	// "printer-state-reasons" value

    // Filter out printers we don't care about...
    if (dest->instance || (cupsArrayGetCount(printers) > 0 && !cupsArrayFind(printers, dest->name)))
      continue;

    // Grab values and report them...
    info              = cupsGetOption("printer-info", dest->num_options, dest->options);
    location          = cupsGetOption("printer-location", dest->num_options, dest->options);
    make_and_model    = cupsGetOption("printer-make-and-model", dest->num_options, dest->options);
    state             = (ipp_pstate_t)cupsGetIntegerOption("printer-state", dest->num_options, dest->options);
    if ((state_change_time = (time_t)cupsGetIntegerOption("printer-state-change-date-time", dest->num_options, dest->options)) == (time_t)LONG_MIN)
      state_change_time = (time_t)cupsGetIntegerOption("printer-state-change-time", dest->num_options, dest->options);
    state_message = cupsGetOption("printer-state-message", dest->num_options, dest->options);
    state_reasons = cupsArrayNewStrings(cupsGetOption("printer-state-reasons", dest->num_options, dest->options), ',');

    strdate(state_change_date, sizeof(state_change_date), state_change_time);

    // If the printer state is `IPP_PSTATE_PROCESSING`, then grab the current job for the printer.
    if (state == IPP_PSTATE_PROCESSING)
    {
      http_t	*http;			// Connection to printer
      char	resource[1024];		// Resource path
      ipp_t	*request,		// IPP Request
		*response;		// IPP Response
      static const char * const jattrs[] =
      {					// Attributes we need for jobs...
	"job-id",
	"job-state"
      };

      // Connect to this printer...
      if ((http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, sizeof(resource), /*cb*/NULL, /*user_data*/NULL)) == NULL)
      {
	cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, dest->name, cupsGetErrorString());
	ret = 1;
	continue;
      }

      request = ippNewRequest(IPP_OP_GET_JOBS);
      ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, cupsGetOption("printer-uri-supported", dest->num_options, dest->options));
      ippAddStrings(request, IPP_TAG_OPERATION, IPP_CONST_TAG(IPP_TAG_KEYWORD), "requested-attributes", sizeof(jattrs) / sizeof(jattrs[0]), NULL, jattrs);
      ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "limit", 1);

      response = cupsDoRequest(http, request, resource);

      if (ippGetInteger(ippFindAttribute(response, "job-state", IPP_TAG_ENUM), 0) == IPP_JSTATE_PROCESSING)
        job_id = ippGetInteger(ippFindAttribute(response, "job-id", IPP_TAG_INTEGER), 0);

      ippDelete(response);
      httpClose(http);
    }

    // Display it...
    switch (state)
    {
      case IPP_PSTATE_IDLE :
	  if (cupsArrayFind(state_reasons, (void *)"hold-new-jobs"))
	    cupsLangPrintf(stdout, _("printer %s is holding new jobs.  enabled since %s"), dest->name, state_change_date);
	  else
	    cupsLangPrintf(stdout, _("printer %s is idle.  enabled since %s"), dest->name, state_change_date);
	  break;
      case IPP_PSTATE_PROCESSING :
	  cupsLangPrintf(stdout, _("printer %s now printing %s-%d.  enabled since %s"), dest->name, dest->name, job_id, state_change_date);
	  break;
      case IPP_PSTATE_STOPPED :
	  cupsLangPrintf(stdout, _("printer %s disabled since %s -"), dest->name, state_change_date);
	  break;
    }

    if ((state_message && *state_message) || state == IPP_PSTATE_STOPPED)
    {
      if (state_message && *state_message)
	cupsLangPrintf(stdout, "\t%s", state_message);
      else
	cupsLangPuts(stdout, _("\treason unknown"));
    }

    if (long_status)
    {
      cupsLangPrintf(stdout, _("\tDescription: %s"), info ? info : "");
      cupsLangPrintf(stdout, _("\tMake and Model: %s"), make_and_model ? make_and_model : "");

      if (cupsArrayGetCount(state_reasons) > 0)
      {
        size_t	count;			// Number of values
	char	alerts[1024],		// Alerts string
		*aptr;			// Pointer into alerts string

	for (j = 0, count = cupsArrayGetCount(state_reasons), aptr = alerts; j < count; j ++)
	{
	  if (j)
	    snprintf(aptr, sizeof(alerts) - (size_t)(aptr - alerts), " %s", (char *)cupsArrayGetElement(state_reasons, j));
	  else
	    cupsCopyString(alerts, (char *)cupsArrayGetElement(state_reasons, j), sizeof(alerts));

	  aptr += strlen(aptr);
	}

	cupsLangPrintf(stdout, _("\tAlerts: %s"), alerts);
      }

      cupsLangPrintf(stdout, _("\tLocation: %s"), location ? location : "");
    }

    cupsArrayDelete(state_reasons);
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

  if (cupsLangGetEncoding() != CUPS_ENCODING_UTF_8)
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
// 'update_dests()' - Update the destinations array as needed.
//

static void
update_dests(const char  *command,	// I  - Command name
             size_t      *num_dests,	// IO - Number of destinations
             cups_dest_t **dests)	// IO - Destinations
{
  if (!*num_dests)
  {
    if ((*num_dests = cupsGetDests(CUPS_HTTP_DEFAULT, dests)) == 0 && (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED))
    {
      cupsLangPrintf(stderr, _("%s: Add '/version=1.1' to server name."), command);
      exit(1);
    }
  }
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
  cupsLangPuts(out, _("--help                         Show this help"));
  cupsLangPuts(out, _("--version                      Show the program version"));
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
