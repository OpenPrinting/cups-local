//
// "lpadmin" command for CUPS.
//
// Copyright © 2021-2025 by OpenPrinting.
// Copyright © 2007-2021 by Apple Inc.
// Copyright © 1997-2006 by Easy Software Products.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include <config.h>
#include <cups/cups.h>

// TODO: Bring back class support once cups-sharing implements it...

//
// Local functions...
//

#if 0
static int		add_printer_to_class(http_t *http, char *printer, char *pclass);
static int		default_printer(http_t *http, char *printer);
static int		delete_printer(http_t *http, char *printer);
static int		delete_printer_from_class(http_t *http, char *printer,
			                          char *pclass);
static int		delete_printer_option(http_t *http, char *printer,
			                      char *option);
static int		enable_printer(http_t *http, char *printer);
static cups_ptype_t	get_printer_type(http_t *http, char *printer, char *uri,
			                 size_t urisize);
static int		set_printer_options(http_t *http, char *printer,
			                    int num_options, cups_option_t *options,
					    char *file, int enable);
static void		usage(void) _CUPS_NORETURN;
static int		validate_name(const char *name);
#endif // 0

static http_t		*connect_dest(const char *printer, cups_dest_t **dest, char *resource, size_t resourcesize);
static cups_dest_t	*create_dest(const char *printer, const char *device_uri, const char *driver, size_t num_options, cups_option_t *options);
static int		delete_printer(http_t *http, cups_dest_t *dest);
static int		enable_printer(http_t *http, cups_dest_t *dest, const char *resource);
static int		set_default_printer(http_t *http, cups_dest_t *dest);
static int		set_printer_options(http_t *http, cups_dest_t *dest, const char *resource, size_t num_options, cups_option_t *options);
static int		usage(FILE *out);


//
// 'main()' - Parse options and configure the scheduler.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  const char	*opt;			// Option pointer
  http_t	*http = NULL;		// Connection to
  cups_dest_t	*dest = NULL;		// Destination
  char		resource[1024],		// Resource path
		op = '\0';		// Operation
  const char	*printer = NULL,	// Destination name
		*device_uri = NULL,	// "smi55357-device-uri" value
		*driver = NULL,		// "smi55357-driver" value
		*name;			// Option name
  bool		do_enable = false;	// Enable/resume printer?
  size_t	num_options = 0;	// Number of options
  cups_option_t	*options = NULL;	// Options
//		*pclass,		// Printer class name
//		*val;			// Pointer to allow/deny value


  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Process command-line arguments...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--enable"))
    {
      do_enable = true;
    }
    else if (!strcmp(argv[i], "--help"))
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
      cupsLangPrintf(stderr, _("%s: Unknown option '%s'."), "lpadmin", argv[i]);
      return (usage(stderr));
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (*opt)
	{
#if 0
	  case 'c' : // Add printer to class
	      if (!http)
	      {
		http = httpConnect2(cupsGetServer(), ippGetPort(), NULL, AF_UNSPEC, cupsEncryption(), 1, 30000, NULL);

		if (http == NULL)
		{
		  cupsLangPrintf(stderr, _("lpadmin: Unable to connect to server: %s"), strerror(errno));
		  return (1);
		}
	      }

	      if (printer == NULL)
	      {
		cupsLangPuts(stderr,
			      _("lpadmin: Unable to add a printer to the class:\n"
				"         You must specify a printer name first."));
		return (1);
	      }

	      if (opt[1] != '\0')
	      {
		pclass = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPuts(stderr, _("lpadmin: Expected class name after \"-c\" option."));
		  return (usage(stderr));
		}

		pclass = argv[i];
	      }

	      if (!validate_name(pclass))
	      {
		cupsLangPuts(stderr,
			      _("lpadmin: Class name can only contain printable "
				"characters."));
		return (1);
	      }

	      if (add_printer_to_class(http, printer, pclass))
		return (1);
	      break;
#endif // 0

	  case 'D' : // -D INFO    Set the printer information/description string
	      if (opt[1] != '\0')
	      {
		num_options = cupsAddOption("printer-info", opt + 1, num_options, &options);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected description after '-D' option."), "lpadmin");
		  return (usage(stderr));
		}

		num_options = cupsAddOption("printer-info", argv[i], num_options, &options);
	      }
	      break;

	  case 'd' : // -d DEST    Set as default destination
	      if (op)
	      {
	        cupsLangPrintf(stderr, _("%s: The '-%c' and '-%c' options are incompatible."), "lpadmin", op, *opt);
	        return (usage(stderr));
	      }

	      op = 'd';

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
		  cupsLangPrintf(stderr, _("%s: Expected printer name after '-%c' option."), "lpadmin", op);
		  return (usage(stderr));
		}

		printer = argv[i];
	      }

#if 0
	      if (!validate_name(printer))
	      {
		cupsLangPuts(stderr, _("lpadmin: Printer name can only contain printable characters."));
		return (1);
	      }

	      if (default_printer(http, printer))
		return (1);
#endif // 0

	      i = argc;
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
		  cupsLangPrintf(stderr, _("%s: Expected server after '-h' option."), "lpadmin");
		  return (usage(stderr));
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'L' : // -L PLACE   Set the printer-location attribute
	      if (opt[1] != '\0')
	      {
		num_options = cupsAddOption("printer-location", opt + 1, num_options, &options);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected location after '-L' option."), "lpadmin");
		  return (usage(stderr));
		}

		num_options = cupsAddOption("printer-location", argv[i], num_options, &options);
	      }
	      break;

	  case 'm' : // -m DRIVER  Use the specified standard script/PPD file
	      if (opt[1] != '\0')
	      {
		driver = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected driver name after '-m' option."), "lpadmin");
		  return (usage(stderr));
		}

		driver = argv[i];
	      }
	      break;

	  case 'o' : // -o OPTION=VALUE
	      if (opt[1] != '\0')
	      {
		num_options = cupsParseOptions(opt + 1, /*end*/NULL, num_options, &options);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected option=value after '-o' option."), "lpadmin");
		  return (usage(stderr));
		}

		num_options = cupsParseOptions(argv[i], /*end*/NULL, num_options, &options);
	      }
	      break;

	  case 'p' : // -p DEST    Add/modify a printer
	      if (op)
	      {
	        cupsLangPrintf(stderr, _("%s: The '-%c' and '-%c' options are incompatible."), "lpadmin", op, *opt);
	        return (usage(stderr));
	      }

	      op = 'p';

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
		  cupsLangPrintf(stderr, _("%s: Expected printer name after '-%c' option."), "lpadmin", op);
		  return (usage(stderr));
		}

		printer = argv[i];
	      }

#if 0
	      if (!validate_name(printer))
	      {
		cupsLangPuts(stderr, _("lpadmin: Printer name can only contain printable characters."));
		return (1);
	      }
#endif // 0
	      break;

	  case 'R' : // Remove option
	      if (opt[1] != '\0')
	      {
		name = opt + 1;
		opt  += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected name after '-R' option."), "lpadmin");
		  return (usage(stderr));
		}

		name = argv[i];
	      }

              num_options = cupsAddOption(name, "__delete__", num_options, &options);
	      break;

#if 0
	  case 'r' : // Remove printer from class
	      if (!http)
	      {
		http = httpConnect2(cupsGetServer(), ippGetPort(), NULL, AF_UNSPEC, cupsEncryption(), 1, 30000, NULL);

		if (http == NULL)
		{
		  cupsLangPrintf(stderr,
				  _("lpadmin: Unable to connect to server: %s"),
				  strerror(errno));
		  return (1);
		}
	      }

	      if (printer == NULL)
	      {
		cupsLangPuts(stderr,
			      _("lpadmin: Unable to remove a printer from the class:\n"
				"         You must specify a printer name first."));
		return (1);
	      }

	      if (opt[1] != '\0')
	      {
		pclass = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPuts(stderr, _("lpadmin: Expected class after \"-r\" option."));
		  return (usage(stderr));
		}

		pclass = argv[i];
	      }

	      if (!validate_name(pclass))
	      {
		cupsLangPuts(stderr, _("lpadmin: Class name can only contain printable characters."));
		return (1);
	      }

	      if (delete_printer_from_class(http, printer, pclass))
		return (1);
	      break;
#endif // 0

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
		  cupsLangPrintf(stderr, _("%s: Expected username after '-U' option."), "lpdmin");
		  return (usage(stderr));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

#if 0
	  case 'u' : // Allow/deny users
	      if (opt[1] != '\0')
	      {
		val = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPuts(stderr, _("lpadmin: Expected allow/deny:userlist after \"-u\" option."));
		  return (usage(stderr));
		}

		val = argv[i];
	      }

	      if (!_cups_strncasecmp(val, "allow:", 6))
		num_options = cupsAddOption("requesting-user-name-allowed", val + 6, num_options, &options);
	      else if (!_cups_strncasecmp(val, "deny:", 5))
		num_options = cupsAddOption("requesting-user-name-denied", val + 5, num_options, &options);
	      else
	      {
		cupsLangPrintf(stderr, _("lpadmin: Unknown allow/deny option \"%s\"."), val);
		return (1);
	      }
	      break;
#endif // 0

	  case 'v' : // Set the device-uri attribute
	      if (opt[1] != '\0')
	      {
		device_uri = opt + 1;
		opt        += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected device URI after '-v' option."), "lpadmin");
		  return (usage(stderr));
		}

		device_uri = argv[i];
	      }
	      break;

	  case 'x' : // Delete a printer
	      if (op)
	      {
	        cupsLangPrintf(stderr, _("%s: The '-%c' and '-%c' options are incompatible."), "lpadmin", op, *opt);
	        return (usage(stderr));
	      }

	      op = 'p';

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
		  cupsLangPrintf(stderr, _("%s: Expected printer name after '-%c' option."), "lpadmin", op);
		  return (usage(stderr));
		}

		printer = argv[i];
	      }

#if 0
	      if (!validate_name(printer))
	      {
		cupsLangPuts(stderr, _("lpadmin: Printer name can only contain printable characters."));
		return (1);
	      }

	      if (delete_printer(http, printer))
		return (1);

	      i = argc;
#endif // 0
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), "lpadmin", *opt);
	      return (usage(stderr));
	}
      }
    }
    else
    {
      cupsLangPrintf(stderr, _("%s: Unknown option '%s'."), "lpadmin", argv[i]);
      return (usage(stderr));
    }
  }

  // Do what was asked...
  switch (op)
  {
    default :
        return (usage(stderr));

    case 'd' : // Set default printer
        if ((http = connect_dest(printer, &dest, resource, sizeof(resource))) == NULL)
          return (1);

        return (set_default_printer(http, dest));

    case 'p' : // Add/modify printer
        if ((http = connect_dest(printer, &dest, resource, sizeof(resource))) == NULL)
        {
          if ((dest = create_dest(printer, device_uri, driver, num_options, options)) == NULL)
            return (1);

          if ((http = connect_dest(printer, &dest, resource, sizeof(resource))) == NULL)
            return (1);
        }
        else if (set_printer_options(http, dest, resource, num_options, options))
	  return (1);

        if (do_enable)
          return (enable_printer(http, dest, resource));
        else
          return (0);

    case 'x' : // Delete printer
        if ((http = connect_dest(printer, &dest, resource, sizeof(resource))) == NULL)
          return (1);

        return (delete_printer(http, dest));

  }
}


//
// 'connect_dest()' - Connect to a destination.
//

static http_t *				// O - HTTP connection
connect_dest(const char  *printer,	// O - Printer name
             cups_dest_t **dest,	// O - Destination
             char        *resource,	// I - Resource path buffer
             size_t      resourcesize)	// I - Size of resource path buffer
{
  http_t	*http = NULL;		// HTTP connection
  bool		alloc_dest = false;	// Did we allocate the destination?


  if (!*dest)
  {
    // Try to find the destination...
    if ((*dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, /*instance*/NULL)) == NULL)
    {
      // Destination doesn't exist...
      if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
	cupsLangPrintf(stderr, _("%s: Try adding '/version=1.1' to server name."), "lpadmin");
      else if (cupsGetError() == IPP_STATUS_ERROR_NOT_FOUND)
	cupsLangPrintf(stderr, _("%s: The printer or class does not exist."), "lpadmin");
      else
	cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());
    }
    else
    {
      // Destination exists and we allocated it...
      alloc_dest = true;
    }
  }

  if (*dest)
  {
    if ((http = cupsConnectDest(*dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, resourcesize, /*cb*/NULL, /*user_data*/NULL)) == NULL)
    {
      cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), "lpadmin", (*dest)->name, cupsGetErrorString());

      if (alloc_dest)
      {
        // Free the destination since we allocated it...
        cupsFreeDests(1, *dest);
        *dest = NULL;
      }
    }
  }

  return (http);
}


//
// 'create_dest()' - Create a printer.
//

static cups_dest_t *			// O - New destination
create_dest(const char    *printer,	// I - Printer name
            const char    *device_uri,	// I - Device URI
            const char    *driver,	// I - Driver name ("everywhere", "pcl", "ps")
            size_t        num_options,	// I - Number of options
            cups_option_t *options)	// I - Options
{
  cups_dest_t	*dest = NULL;		// New destination
  http_t	*http;			// HTTP connection
  const char	*system_host;		// System hostname
  char		system_uri[1024];	// System URI
  ipp_t		*request,		// IPP request
		*response;		// IPP response
  ipp_attribute_t *xri_uri;		// "printer-xri-supported/xri-uri" attribute


  // Connect to the server...
  system_host = cupsGetServer();

  if ((http = httpConnect(system_host, ippGetPort(), /*addrlist*/NULL, AF_UNSPEC, cupsGetEncryption(), /*blocking*/true, 30000, /*cancel*/NULL)) == NULL)
  {
    cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), "lpadmin", system_host, cupsGetErrorString());
    return (NULL);
  }

  // Create the print queue...
  request = ippNewRequest(IPP_OP_CREATE_PRINTER);

  if (system_host[0] == '/')
    cupsCopyString(system_uri, "ipp://localhost/ipp/system", sizeof(system_uri));
  else
    httpAssembleURI(HTTP_URI_CODING_ALL, system_uri, sizeof(system_uri), cupsGetEncryption() == HTTP_ENCRYPTION_ALWAYS ? "ipps" : "ipp", /*userpass*/NULL, system_host, ippGetPort(), "/ipp/system");
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "system-uri", NULL, system_uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
  ippAddString(request, IPP_TAG_OPERATION, IPP_CONST_TAG(IPP_TAG_KEYWORD), "printer-service-type", NULL, "print");
  ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_NAME, "printer-name", NULL, printer);
  ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "smi55357-device-type", NULL, driver);
  ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "smi55357-device-uri", NULL, device_uri);
  cupsEncodeOptions(request, num_options, options, IPP_TAG_PRINTER);

  response = cupsDoRequest(http, request, "/ipp/system");

  if (cupsGetError() == IPP_STATUS_OK && (xri_uri = ippFindAttribute(response, "printer-xri-supported/xri-uri", IPP_TAG_URI)) != NULL)
    dest = cupsGetDestWithURI(printer, ippGetString(xri_uri, 0, NULL));
  else
    cupsLangPrintf(stderr, _("lpstat: Unable to create printer '%s': %s"), printer, cupsGetErrorString());

  ippDelete(response);

  return (dest);
}


//
// 'delete_printer()' - Delete a printer.
//

static int				// O - Exit status
delete_printer(http_t      *http,	// I - HTTP connection
               cups_dest_t *dest)	// I - Destination
{
  const char	*printer_uri;		// Printer URI
  ipp_t		*request;		// IPP request


  // Send a Delete-Printer request to the printer...
  printer_uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
  request     = ippNewRequest(IPP_OP_DELETE_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  ippDelete(cupsDoRequest(http, request, "/ipp/system"));

  if (cupsGetError() == IPP_STATUS_ERROR_OPERATION_NOT_SUPPORTED)
  {
    // Try the older CUPS-Delete-Printer operation...
    request = ippNewRequest(IPP_OP_CUPS_DELETE_PRINTER);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

    ippDelete(cupsDoRequest(http, request, "/admin"));
  }

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("lpadmin: Unable to delete printer: %s"), cupsGetErrorString());
    return (1);
  }

  return (0);
}


//
// 'enable_printer()' - Enable and resume a printer to accept and print jobs.
//

static int				// O - Exit status
enable_printer(http_t      *http,	// I - HTTP connection
               cups_dest_t *dest,	// I - Destination
               const char  *resource)	// I - Resource path
{
  const char	*printer_uri;		// Printer URI
  ipp_t		*request;		// IPP request


  // Send an Enable-Printer request to the printer...
  printer_uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
  request     = ippNewRequest(IPP_OP_ENABLE_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  ippDelete(cupsDoRequest(http, request, resource));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("lpadmin: Unable to enable printer: %s"), cupsGetErrorString());
    return (1);
  }

  // Send a Resume-Printer request to the printer...
  request = ippNewRequest(IPP_OP_RESUME_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  ippDelete(cupsDoRequest(http, request, resource));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("lpadmin: Unable to resume printer: %s"), cupsGetErrorString());
    return (1);
  }

  return (0);
}


//
// 'set_default_printer()' - Set the default printer.
//

static int				// O - Exit status
set_default_printer(
    http_t      *http,			// I - HTTP
    cups_dest_t *dest)			// I - Destination
{
  return (1);
}


//
// 'set_printer_options()' - Set/remove options.
//

static int				// O - Exit status
set_printer_options(
    http_t        *http,		// I - HTTP connection
    cups_dest_t   *dest,		// I - Destination
    const char    *resource,		// I - Resource path
    size_t        num_options,		// I - Number of options
    cups_option_t *options)		// I - Options
{
  const char	*printer_uri;		// Printer URI
  ipp_t		*request;		// IPP request
  int		i;			// Looping var
  cups_option_t	*option;		// Current option


  // Send an Set-Printer-Attributes request to the printer...
  printer_uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
  request     = ippNewRequest(IPP_OP_SET_PRINTER_ATTRIBUTES);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, printer_uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  for (i = num_options, option = options; i > 0; i --, option ++)
  {
    if (!strcmp(option->value, "__delete__"))
      ippAddOutOfBand(request, IPP_TAG_PRINTER, IPP_TAG_DELETEATTR, option->name);
    else
      cupsEncodeOption(request, IPP_TAG_PRINTER, option->name, option->value);
  }

  ippDelete(cupsDoRequest(http, request, resource));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("lpadmin: Unable to set printer options: %s"), cupsGetErrorString());
    return (1);
  }

  return (0);
}


#if 0
//
// 'connect_dest()' - Connect to the named destination and get its information.
//

static http_t *				// O - HTTP connection or `NULL` on error
connect_dest(const char   *printer,	// I - Printer or `NULL` for default
             const char   *instance,	// I - Instance or `NULL` for primary
             cups_dest_t  **dest,	// O - Destination or `NULL` on error
             char         *resource,	// I - Resource buffer
             size_t       resourcesize,	// I - Size of resource buffer

{
  http_t	*http = NULL;		// HTTP connection


  // Try to find the destination...
  if ((*dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, instance)) == NULL)
  {
    // Destination doesn't exist...
    if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
      cupsLangPrintf(stderr, _("%s: Try adding '/version=1.1' to server name."), command);
    else if (cupsGetError() == IPP_STATUS_ERROR_NOT_FOUND)
      cupsLangPrintf(stderr, _("%s: The printer or class does not exist."), command);
    else
      cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
  }
  else if ((http = cupsConnectDest(*dest, CUPS_DEST_FLAGS_NONE, 30000, /*cancel*/NULL, resource, resourcesize, /*cb*/NULL, /*user_data*/NULL)) == NULL)
  {
    cupsLangPrintf(stderr, _("%s: Unable to connect to '%s': %s"), command, (*dest)->name, cupsGetErrorString());

    cupsFreeDests(1, *dest);
    *dest = NULL;
  }

  return (http);
}
#endif // 0


#if 0
//
// 'add_printer_to_class()' - Add a printer to a class.
//

static int				// O - 0 on success, 1 on fail
add_printer_to_class(http_t *http,	// I - Server connection
                     char   *printer,	// I - Printer to add
		     char   *pclass)	// I - Class to add to
{
  int		i;			// Looping var
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr,		// Current attribute
		*members;		// Members in class
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL,
                   "localhost", 0, "/classes/%s", pclass);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  response = cupsDoRequest(http, request, "/");

 /*
  * Build a CUPS-Add-Modify-Class request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  *    member-uris
  */

  request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_CLASS);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * See if the printer is already in the class...
  */

  if (response != NULL &&
      (members = ippFindAttribute(response, "member-names",
                                  IPP_TAG_NAME)) != NULL)
    for (i = 0; i < members->num_values; i ++)
      if (_cups_strcasecmp(printer, members->values[i].string.text) == 0)
      {
        cupsLangPrintf(stderr,
	                _("lpadmin: Printer %s is already a member of class "
			  "%s."), printer, pclass);
        ippDelete(request);
	ippDelete(response);
	return (0);
      }

 /*
  * OK, the printer isn't part of the class, so add it...
  */

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL,
                   "localhost", 0, "/printers/%s", printer);

  if (response != NULL &&
      (members = ippFindAttribute(response, "member-uris",
                                  IPP_TAG_URI)) != NULL)
  {
   /*
    * Add the printer to the existing list...
    */

    attr = ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_URI,
                         "member-uris", members->num_values + 1, NULL, NULL);
    for (i = 0; i < members->num_values; i ++)
      attr->values[i].string.text =
          _cupsStrAlloc(members->values[i].string.text);

    attr->values[i].string.text = _cupsStrAlloc(uri);
  }
  else
    ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "member-uris", NULL,
                 uri);

 /*
  * Then send the request...
  */

  ippDelete(response);

  ippDelete(cupsDoRequest(http, request, "/admin/"));
  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);
}


//
// 'default_printer()' - Set the default printing destination.
//

static int				// O - 0 on success, 1 on fail
default_printer(http_t *http,		// I - Server connection
                char   *printer)	// I - Printer name
{
  ipp_t		*request;		// IPP Request
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Build a CUPS-Set-Default request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  */

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL,
                   "localhost", 0, "/printers/%s", printer);

  request = ippNewRequest(IPP_OP_CUPS_SET_DEFAULT);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);
}


//
// 'delete_printer()' - Delete a printer from the system...
//

static int				// O - 0 on success, 1 on fail
delete_printer(http_t *http,		// I - Server connection
               char   *printer)		// I - Printer to delete
{
  ipp_t		*request;		// IPP Request
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Build a CUPS-Delete-Printer request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_CUPS_DELETE_PRINTER);

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL,
                   "localhost", 0, "/printers/%s", printer);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);
}


//
// 'delete_printer_from_class()' - Delete a printer from a class.
//

static int				// O - 0 on success, 1 on fail
delete_printer_from_class(
    http_t *http,			// I - Server connection
    char   *printer,			// I - Printer to remove
    char   *pclass)	  		// I - Class to remove from
{
  int		i, j, k;		// Looping vars
  ipp_t		*request,		// IPP Request
		*response;		// IPP Response
  ipp_attribute_t *attr,		// Current attribute
		*members;		// Members in class
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Build an IPP_OP_GET_PRINTER_ATTRIBUTES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL,
                   "localhost", 0, "/classes/%s", pclass);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name",
               NULL, cupsGetUser());

 /*
  * Do the request and get back a response...
  */

  if ((response = cupsDoRequest(http, request, "/classes/")) == NULL ||
      response->request.status.status_code == IPP_STATUS_ERROR_NOT_FOUND)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    ippDelete(response);

    return (1);
  }

 /*
  * See if the printer is already in the class...
  */

  if ((members = ippFindAttribute(response, "member-names", IPP_TAG_NAME)) == NULL)
  {
    cupsLangPuts(stderr, _("lpadmin: No member names were seen."));

    ippDelete(response);

    return (1);
  }

  for (i = 0; i < members->num_values; i ++)
    if (!_cups_strcasecmp(printer, members->values[i].string.text))
      break;

  if (i >= members->num_values)
  {
    cupsLangPrintf(stderr,
                    _("lpadmin: Printer %s is not a member of class %s."),
	            printer, pclass);

    ippDelete(response);

    return (1);
  }

  if (members->num_values == 1)
  {
   /*
    * Build a CUPS-Delete-Class request, which requires the following
    * attributes:
    *
    *    attributes-charset
    *    attributes-natural-language
    *    printer-uri
    *    requesting-user-name
    */

    request = ippNewRequest(IPP_OP_CUPS_DELETE_CLASS);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
        	 "printer-uri", NULL, uri);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                 "requesting-user-name", NULL, cupsGetUser());
  }
  else
  {
   /*
    * Build a IPP_OP_CUPS_ADD_MODIFY_CLASS request, which requires the following
    * attributes:
    *
    *    attributes-charset
    *    attributes-natural-language
    *    printer-uri
    *    requesting-user-name
    *    member-uris
    */

    request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_CLASS);

    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
        	 "printer-uri", NULL, uri);
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                 "requesting-user-name", NULL, cupsGetUser());

   /*
    * Delete the printer from the class...
    */

    members = ippFindAttribute(response, "member-uris", IPP_TAG_URI);
    attr = ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_URI,
                         "member-uris", members->num_values - 1, NULL, NULL);

    for (j = 0, k = 0; j < members->num_values; j ++)
      if (j != i)
        attr->values[k ++].string.text =
	    _cupsStrAlloc(members->values[j].string.text);
  }

 /*
  * Then send the request...
  */

  ippDelete(response);

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);
}


//
// 'delete_printer_option()' - Delete a printer option.
//

static int				// O - 0 on success, 1 on fail
delete_printer_option(http_t *http,	// I - Server connection
                      char   *printer,	// I - Printer
		      char   *option)	// I - Option to delete
{
  ipp_t		*request;		// IPP request
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Build a IPP_OP_CUPS_ADD_MODIFY_PRINTER or IPP_OP_CUPS_ADD_MODIFY_CLASS request, which
  * requires the following attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  *    option with deleteAttr tag
  */

  if (get_printer_type(http, printer, uri, sizeof(uri)) & CUPS_PRINTER_CLASS)
    request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_CLASS);
  else
    request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
               "requesting-user-name", NULL, cupsGetUser());
  ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_DELETEATTR, option, 0);

 /*
  * Do the request and get back a response...
  */

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);
}


//
// 'enable_printer()' - Enable a printer...
//

static int				// O - 0 on success, 1 on fail
enable_printer(http_t *http,		// I - Server connection
               char   *printer)		// I - Printer to enable
{
  ipp_t		*request;		// IPP Request
  char		uri[HTTP_MAX_URI];	// URI for printer/class


 /*
  * Send IPP_OP_ENABLE_PRINTER and IPP_OP_RESUME_PRINTER requests, which
  * require the following attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  */

  request = ippNewRequest(IPP_OP_ENABLE_PRINTER);

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", ippGetPort(), "/printers/%s", printer);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }

  request = ippNewRequest(IPP_OP_RESUME_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  ippDelete(cupsDoRequest(http, request, "/admin/"));

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }

  return (0);
}


//
// 'get_printer_type()' - Determine the printer type and URI.
//

static cups_ptype_t			// O - printer-type value
get_printer_type(http_t *http,		// I - Server connection
                 char   *printer,	// I - Printer name
		 char   *uri,		// I - URI buffer
                 size_t urisize)	// I - Size of URI buffer
{
  ipp_t			*request,	// IPP request
			*response;	// IPP response
  ipp_attribute_t	*attr;		// printer-type attribute
  cups_ptype_t		type;		// printer-type value


 /*
  * Build a GET_PRINTER_ATTRIBUTES request, which requires the following
  * attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requested-attributes
  *    requesting-user-name
  */

  httpAssembleURIf(HTTP_URI_CODING_ALL, uri, (int)urisize, "ipp", NULL, "localhost", ippGetPort(), "/printers/%s", printer);

  request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD,
               "requested-attributes", NULL, "printer-type");
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
               "requesting-user-name", NULL, cupsGetUser());

 /*
  * Do the request...
  */

  response = cupsDoRequest(http, request, "/");
  if ((attr = ippFindAttribute(response, "printer-type",
                               IPP_TAG_ENUM)) != NULL)
  {
    type = (cups_ptype_t)attr->values[0].integer;

    if (type & CUPS_PRINTER_CLASS)
      httpAssembleURIf(HTTP_URI_CODING_ALL, uri, (int)urisize, "ipp", NULL, "localhost", ippGetPort(), "/classes/%s", printer);
  }
  else
    type = CUPS_PRINTER_LOCAL;

  ippDelete(response);

  return (type);
}


//
// 'set_printer_options()' - Set the printer options.
//

static int				// O - 0 on success, 1 on fail
set_printer_options(
    http_t        *http,		// I - Server connection
    char          *printer,		// I - Printer
    int           num_options,		// I - Number of options
    cups_option_t *options,		// I - Options
    char          *file,		// I - PPD file
    int           enable)		// I - Enable printer?
{
  ipp_t		*request;		// IPP Request
  const char	*ppdfile;		// PPD filename
  int		ppdchanged = 0;		// PPD changed?
  ppd_file_t	*ppd;			// PPD file
  ppd_choice_t	*choice;		// Marked choice
  char		uri[HTTP_MAX_URI],	// URI for printer/class
		line[1024],		// Line from PPD file
		keyword[1024],		// Keyword from Default line
		*keyptr,		// Pointer into keyword...
		tempfile[1024];		// Temporary filename
  cups_file_t	*in,			// PPD file
		*out;			// Temporary file
  const char	*ppdname,		// ppd-name value
		*protocol,		// Old protocol option
		*customval,		// Custom option value
		*boolval;		// Boolean value
  int		wrote_ipp_supplies = 0,	// Wrote cupsIPPSupplies keyword?
		wrote_snmp_supplies = 0,// Wrote cupsSNMPSupplies keyword?
		copied_options = 0;	// Copied options?


 /*
  * Build a CUPS-Add-Modify-Printer or CUPS-Add-Modify-Class request,
  * which requires the following attributes:
  *
  *    attributes-charset
  *    attributes-natural-language
  *    printer-uri
  *    requesting-user-name
  *    other options
  */

  if (get_printer_type(http, printer, uri, sizeof(uri)) & CUPS_PRINTER_CLASS)
    request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_CLASS);
  else
    request = ippNewRequest(IPP_OP_CUPS_ADD_MODIFY_PRINTER);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

 /*
  * Add the options...
  */

  if (file)
    ppdfile = file;
  else if ((ppdname = cupsGetOption("ppd-name", num_options, options)) != NULL && strcmp(ppdname, "everywhere") && strcmp(ppdname, "raw") && num_options > 1)
  {
    if ((ppdfile = cupsGetServerPPD(http, ppdname)) != NULL)
    {
     /*
      * Copy options array and remove ppd-name from it...
      */

      cups_option_t *temp = NULL, *optr;
      int i, num_temp = 0;
      for (i = num_options, optr = options; i > 0; i --, optr ++)
        if (strcmp(optr->name, "ppd-name"))
	  num_temp = cupsAddOption(optr->name, optr->value, num_temp, &temp);

      copied_options = 1;
      ppdchanged     = 1;
      num_options    = num_temp;
      options        = temp;
    }
  }
  else if (request->request.op.operation_id == IPP_OP_CUPS_ADD_MODIFY_PRINTER)
    ppdfile = cupsGetPPD(printer);
  else
    ppdfile = NULL;

  cupsEncodeOptions2(request, num_options, options, IPP_TAG_OPERATION);

  if (enable)
  {
    ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_ENUM, "printer-state", IPP_PSTATE_IDLE);
    ippAddBoolean(request, IPP_TAG_PRINTER, "printer-is-accepting-jobs", 1);
  }

  cupsEncodeOptions2(request, num_options, options, IPP_TAG_PRINTER);

  if ((protocol = cupsGetOption("protocol", num_options, options)) != NULL)
  {
    if (!_cups_strcasecmp(protocol, "bcp"))
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_NAME, "port-monitor",
                   NULL, "bcp");
    else if (!_cups_strcasecmp(protocol, "tbcp"))
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_NAME, "port-monitor",
                   NULL, "tbcp");
  }

  if (ppdfile)
  {
   /*
    * Set default options in the PPD file...
    */

    if ((ppd = ppdOpenFile(ppdfile)) == NULL)
    {
      int		linenum;	// Line number of error
      ppd_status_t	status = ppdLastError(&linenum);
					// Status code

      cupsLangPrintf(stderr, _("lpadmin: Unable to open PPD \"%s\": %s on line %d."), ppdfile, ppdErrorString(status), linenum);
      goto error;
    }

    ppdMarkDefaults(ppd);
    cupsMarkOptions(ppd, num_options, options);

    if ((out = cupsTempFile2(tempfile, sizeof(tempfile))) == NULL)
    {
      cupsLangPrintError(NULL, _("lpadmin: Unable to create temporary file"));
      goto error;
    }

    if ((in = cupsFileOpen(ppdfile, "r")) == NULL)
    {
      cupsLangPrintf(stderr, _("lpadmin: Unable to open PPD \"%s\": %s"), ppdfile, strerror(errno));
      cupsFileClose(out);
      unlink(tempfile);
      goto error;
    }

    while (cupsFileGets(in, line, sizeof(line)))
    {
      if (!strncmp(line, "*cupsIPPSupplies:", 17) &&
	  (boolval = cupsGetOption("cupsIPPSupplies", num_options,
	                           options)) != NULL)
      {
        ppdchanged         = 1;
        wrote_ipp_supplies = 1;
        cupsFilePrintf(out, "*cupsIPPSupplies: %s\n",
	               (!_cups_strcasecmp(boolval, "true") ||
		        !_cups_strcasecmp(boolval, "yes") ||
		        !_cups_strcasecmp(boolval, "on")) ? "True" : "False");
      }
      else if (!strncmp(line, "*cupsSNMPSupplies:", 18) &&
	       (boolval = cupsGetOption("cupsSNMPSupplies", num_options,
	                                options)) != NULL)
      {
        ppdchanged          = 1;
        wrote_snmp_supplies = 1;
        cupsFilePrintf(out, "*cupsSNMPSupplies: %s\n",
	               (!_cups_strcasecmp(boolval, "true") ||
		        !_cups_strcasecmp(boolval, "yes") ||
		        !_cups_strcasecmp(boolval, "on")) ? "True" : "False");
      }
      else if (strncmp(line, "*Default", 8))
        cupsFilePrintf(out, "%s\n", line);
      else
      {
       /*
        * Get default option name...
	*/

        strlcpy(keyword, line + 8, sizeof(keyword));

	for (keyptr = keyword; *keyptr; keyptr ++)
	  if (*keyptr == ':' || isspace(*keyptr & 255))
	    break;

        *keyptr++ = '\0';
        while (isspace(*keyptr & 255))
	  keyptr ++;

        if (!strcmp(keyword, "PageRegion") ||
	    !strcmp(keyword, "PageSize") ||
	    !strcmp(keyword, "PaperDimension") ||
	    !strcmp(keyword, "ImageableArea"))
	{
	  if ((choice = ppdFindMarkedChoice(ppd, "PageSize")) == NULL)
	    choice = ppdFindMarkedChoice(ppd, "PageRegion");
        }
	else
	  choice = ppdFindMarkedChoice(ppd, keyword);

        if (choice && strcmp(choice->choice, keyptr))
	{
	  if (strcmp(choice->choice, "Custom"))
	  {
	    cupsFilePrintf(out, "*Default%s: %s\n", keyword, choice->choice);
	    ppdchanged = 1;
	  }
	  else if ((customval = cupsGetOption(keyword, num_options,
	                                      options)) != NULL)
	  {
	    cupsFilePrintf(out, "*Default%s: %s\n", keyword, customval);
	    ppdchanged = 1;
	  }
	  else
	    cupsFilePrintf(out, "%s\n", line);
	}
	else
	  cupsFilePrintf(out, "%s\n", line);
      }
    }

    if (!wrote_ipp_supplies &&
	(boolval = cupsGetOption("cupsIPPSupplies", num_options,
				 options)) != NULL)
    {
      ppdchanged = 1;

      cupsFilePrintf(out, "*cupsIPPSupplies: %s\n",
		     (!_cups_strcasecmp(boolval, "true") ||
		      !_cups_strcasecmp(boolval, "yes") ||
		      !_cups_strcasecmp(boolval, "on")) ? "True" : "False");
    }

    if (!wrote_snmp_supplies &&
        (boolval = cupsGetOption("cupsSNMPSupplies", num_options,
			         options)) != NULL)
    {
      ppdchanged = 1;

      cupsFilePrintf(out, "*cupsSNMPSupplies: %s\n",
		     (!_cups_strcasecmp(boolval, "true") ||
		      !_cups_strcasecmp(boolval, "yes") ||
		      !_cups_strcasecmp(boolval, "on")) ? "True" : "False");
    }

    cupsFileClose(in);
    cupsFileClose(out);
    ppdClose(ppd);

   /*
    * Do the request...
    */

    ippDelete(cupsDoFileRequest(http, request, "/admin/", ppdchanged ? tempfile : file));

   /*
    * Clean up temp files... (TODO: catch signals in case we CTRL-C during
    * lpadmin)
    */

    if (ppdfile != file)
      unlink(ppdfile);
    unlink(tempfile);
  }
  else
  {
   /*
    * No PPD file - just set the options...
    */

    ippDelete(cupsDoRequest(http, request, "/admin/"));
  }

  if (copied_options)
    cupsFreeOptions(num_options, options);

 /*
  * Check the response...
  */

  if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), "lpadmin", cupsGetErrorString());

    return (1);
  }
  else
    return (0);

 /*
  * Error handling...
  */

  error:

  ippDelete(request);

  if (ppdfile != file)
    unlink(ppdfile);

  if (copied_options)
    cupsFreeOptions(num_options, options);

  return (1);
}
#endif // 0


//
// 'usage()' - Show program usage and exit.
//

static int				// O - Exit status
usage(FILE *out)			// I - Output file
{
  cupsLangPuts(out, _("Usage: lpadmin [OPTIONS] -d DESTINATION\n"
		      "       lpadmin [OPTIONS] -p DESTINATION\n"
		      "       lpadmin [OPTIONS] -x DESTINATION"));

//		      "       lpadmin [OPTIONS] -p DESTINATION -c CLASS\n"
//		      "       lpadmin [OPTIONS] -p DESTINATION -r CLASS\n"

  cupsLangPuts(out, _("Options:"));
//  cupsLangPuts(out, _("-c CLASS                       Add the named destination to a class"));
  cupsLangPuts(out, _("--enable                       Enable the printer and accept new jobs"));
  cupsLangPuts(out, _("--help                         Show this help"));
  cupsLangPuts(out, _("--version                      Show the program version"));
  cupsLangPuts(out, _("-D DESCRIPTION                 Specify the textual description of the printer"));
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  cupsLangPuts(out, _("-h SERVER[:PORT]               Connect to the named server and port"));
  cupsLangPuts(out, _("-L LOCATION                    Specify the textual location of the printer"));
  cupsLangPuts(out, _("-m DRIVER                      Specify the driver for the printer (everywhere,pcl,ps)"));
  cupsLangPuts(out, _("-o NAME-default=VALUE          Specify the default value for the named option"));
  cupsLangPuts(out, _("-o printer-error-policy=VALUE  Specify the printer error policy (abort-job,retry-current-job,retry-job,stop-printer)"));
  cupsLangPuts(out, _("-o printer-geo-location=VALUE  Specify the printer geographic location as a 'geo:' URI"));
  cupsLangPuts(out, _("-o printer-op-policy=VALUE     Specify the printer operation policy"));
//  cupsLangPuts(out, _("-r CLASS                       Remove the named destination from a class"));
  cupsLangPuts(out, _("-R NAME-default                Remove the default value for the named option"));
  cupsLangPuts(out, _("-U USERNAME                    Specify the username to use for authentication"));
  cupsLangPuts(out, _("-v URI                         Specify the device URI for the printer"));

  return (out == stdout ? 0 : 1);
}


#if 0
//
// 'validate_name()' - Make sure the printer name only contains valid chars.
//

static int				// O - 0 if name is no good, 1 if name is good
validate_name(const char *name)		// I - Name to check
{
  const char	*ptr;			// Pointer into name


 /*
  * Scan the whole name...
  */

  for (ptr = name; *ptr; ptr ++)
    if (*ptr == '@')
      break;
    else if ((*ptr >= 0 && *ptr <= ' ') || *ptr == 127 || *ptr == '/' || *ptr == '\\' || *ptr == '?' || *ptr == '\'' || *ptr == '\"' || *ptr == '#')
      return (0);

 /*
  * All the characters are good; validate the length, too...
  */

  return ((ptr - name) < 128);
}
#endif // 0
