//
// "lp" and "lpr" commands for CUPS.
//
// Copyright © 2021-2023 by OpenPrinting.
// Copyright © 2007-2021 by Apple Inc.
// Copyright © 1997-2007 by Easy Software Products.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include <config.h>
#include <cups/cups.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


//
// Local functions.
//

static http_t		*connect_dest(const char *command, const char *printer, const char *instance, cups_dest_t **dest, cups_dinfo_t **dinfo, char *resource, size_t resourcesize);
static int		print_files(const char *command, http_t *http, cups_dest_t *dest, cups_dinfo_t *dinfo, size_t num_files, const char **files, const char *title, size_t num_options, cups_option_t *options);
static int		send_document(const char *command, http_t *http, cups_dest_t *dest, cups_dinfo_t *dinfo, int job_id, const char *docname, const char *format, bool last_document, int fd);
static int		set_job_attrs(const char *command, http_t *http, cups_dest_t *dest, cups_dinfo_t *dinfo, const char *resource, int job_id, int num_options, cups_option_t *options);
static int		usage(FILE *out, const char *command);


//
// 'main()' - Parse options and send files for printing.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  const char	*command;		// Command name
  int		i;			// Looping var
  size_t	idx;			// Index into options
  int		job_id = 0;		// Job ID
  char		*printer = NULL,	// Printer name
		*instance = NULL,	// Instance name
		*opt,			// Option pointer
		*val,			// Option value
		*ptr,			// Pointer into value
		*title = NULL,		// Job title
		email[1024],		// EMail address
		emailhost[256];		// This hostname
  int		priority;		// Job priority (1-100)
  int		num_copies;		// Number of copies per file
  size_t	num_files = 0;		// Number of files to print
  const char	*files[1000];		// Files to print
  http_t	*http = NULL;		// HTTP connection
  cups_dest_t	*dest = NULL;		// Selected destination
  cups_dinfo_t	*dinfo = NULL;		// Destination information
  char		resource[1024];		// Resource path for printer
  size_t	num_options = 0;	// Number of options
  cups_option_t	*options = NULL;	// Options
  bool		end_options = false,	// No more options?
		silent = false,		// Silent or verbose output?
		deletefile = false;	// Delete file after submission?


  // Get command name...
  if ((command = strrchr(argv[0], '/')) != NULL)
    command ++;
  else
    command = argv[0];

  // Setup localization...
  cupsLangSetDirectory(CUPS_LOCAL_DATADIR);
  cupsLangSetLocale(argv);

  // Parse command-line...
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
    else if (!strncmp(argv[i], "--", 2) && argv[i][2])
    {
      cupsLangPrintf(stderr, _("%s: Unknown option '%s'."), command, argv[i]);
      return (usage(stderr, command));
    }
    else if (argv[i][0] == '-' && argv[i][1] && !end_options)
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
	{
	  case '-' : // --         Stop processing options
	      end_options = true;
	      break;

	  case 'c' : // Copy to spool dir (always enabled)
	      break;

	  case 'd' : // -d DESTINATION
	      if (opt[1] != '\0')
	      {
		printer = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected destination after '-d' option."), command);
		  return (usage(stderr, command));
		}

		printer = argv[i];
	      }

	      if ((instance = strrchr(printer, '/')) != NULL)
		*instance++ = '\0';

              if ((http = connect_dest(command, printer, instance, &dest, &dinfo, resource, sizeof(resource))) == NULL)
                return (1);
	      break;

	  case 'E' : // -E         Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'h' : // -h SERVER  Destination host
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
		  cupsLangPrintf(stderr, _("%s: Expected server after '-h' option."), command);
		  return (usage(stderr, command));
		}

		cupsSetServer(argv[i]);
	      }
	      break;

	  case 'i' : // -i JOBID   Change job
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
		  cupsLangPrintf(stderr, _("%s: Expected job ID after '-i' option."), command);
		  return (usage(stderr, command));
		}

		val = argv[i];
	      }

	      if (num_files > 0)
	      {
		cupsLangPrintf(stderr, _("%s: Cannot print files and alter jobs simultaneously."), command);
		return (1);
	      }

	      if ((ptr = strrchr(val, '-')) != NULL)
	      {
	        *ptr++ = '\0';
		job_id = atoi(ptr);

                printer = val;
		if ((instance = strrchr(printer, '/')) != NULL)
		  *instance++ = '\0';

		if ((http = connect_dest(command, printer, instance, &dest, &dinfo, resource, sizeof(resource))) == NULL)
		  return (1);
	      }
	      else
	      {
		job_id = atoi(val);
	      }

	      if (job_id < 0)
	      {
		cupsLangPrintf(stderr, _("%s: Bad job ID."), command);
		break;
	      }
	      break;

	  case 'm' : // -m         Send email when job is done
	      snprintf(email, sizeof(email), "mailto:%s@%s", cupsGetUser(), httpGetHostname(NULL, emailhost, sizeof(emailhost)));
	      num_options = cupsAddOption("notify-recipient-uri", email, num_options, &options);

	      silent = true;
	      break;

	  case '#' : // -# COPIES  Set number of copies
	  case 'n' : // -n COPIES  Set number of copies
	      if (opt[1] != '\0')
	      {
		num_copies = atoi(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected copies after '-n' option."), command);
		  return (usage(stderr, command));
		}

		num_copies = atoi(argv[i]);
	      }

	      if (num_copies < 1)
	      {
		cupsLangPrintf(stderr, _("%s: Copies must be 1 or more."), command);
		return (1);
	      }

	      num_options = cupsAddIntegerOption("copies", num_copies, num_options, &options);
	      break;

	  case 'o' : // Option
	      if (opt[1] != '\0')
	      {
		num_options = cupsParseOptions(opt + 1, num_options, &options);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected option=value after '-o' option."), command);
		  return (usage(stderr, command));
		}

		num_options = cupsParseOptions(argv[i], num_options, &options);
	      }
	      break;

	  case 'P' :
	      if (!strcmp(command, "lp"))
	      {
	        // -P FIRST-LAST
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
		    cupsLangPrintf(stderr, _("%s: Expected page list after '-P' option."), command);
		    return (usage(stderr, command));
		  }

		  val = argv[i];
		}

		num_options = cupsAddOption("page-ranges", val, num_options, &options);
	      }
	      else
	      {
	        // -P DESTINATION
		if (opt[1] != '\0')
		{
		  printer = opt + 1;
		  opt += strlen(opt) - 1;
		}
		else
		{
		  i ++;

		  if (i >= argc)
		  {
		    cupsLangPrintf(stderr, _("%s: Expected destination after '-d' option."), command);
		    return (usage(stderr, command));
		  }

		  printer = argv[i];
		}

		if ((instance = strrchr(printer, '/')) != NULL)
		  *instance++ = '\0';

		if ((http = connect_dest(command, printer, instance, &dest, &dinfo, resource, sizeof(resource))) == NULL)
		  return (1);
	      }
	      break;

	  case 'q' : // Queue priority
	      if (opt[1] != '\0')
	      {
		priority = atoi(opt + 1);
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		if ((i + 1) >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected priority after '-%c' option."), command, *opt);
		  return (usage(stderr, command));
		}

		i ++;

		priority = atoi(argv[i]);
	      }

	      if (priority < 1 || priority > 100)
	      {
		cupsLangPrintf(stderr, _("%s: Priority must be between 1 and 100."), command);
		return (1);
	      }

	      num_options = cupsAddIntegerOption("job-priority", priority, num_options, &options);
	      break;

	  case 'r' : // Remove file after printing
	      deletefile = 1;
	      break;

	  case 's' : // Silent
	      silent = true;
	      break;

	  case 'T' : // -T TITLE   Set job name
	  case 't' : // -t TITLE   Set job name
	      if (opt[1] != '\0')
	      {
		title = opt + 1;
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Expected title after '-t' option."), command);
		  return (usage(stderr, command));
		}

		title = argv[i];
	      }
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
		  cupsLangPrintf(stderr, _("%s: Expected username after '-U' option."), command);
		  return (usage(stderr, command));
		}

		cupsSetUser(argv[i]);
	      }
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Unknown option '-%c'."), command, *opt);
	      return (usage(stderr, command));
	}
      }
    }
    else if (!strcmp(argv[i], "-"))
    {
      if (num_files || job_id)
      {
        cupsLangPrintf(stderr, _("%s: Cannot print from stdin if files or a job ID are provided."), command);
	return (1);
      }

      break;
    }
    else if (num_files < 1000 && job_id == 0)
    {
      // Print a file...
      if (access(argv[i], R_OK) != 0)
      {
        cupsLangPrintf(stderr, _("%s: Unable to access '%s': %s"), command, argv[i], strerror(errno));
        return (1);
      }

      files[num_files] = argv[i];
      num_files ++;

      if (title == NULL)
      {
        if ((title = strrchr(argv[i], '/')) != NULL)
	  title ++;
	else
          title = argv[i];
      }
    }
    else
    {
      cupsLangPrintf(stderr, _("%s: Too many files."), command);
    }
  }

  // Make sure we have a destination...
  if (!http)
  {
    if ((http = connect_dest(command, /*printer*/NULL, /*instance*/NULL, &dest, &dinfo, resource, sizeof(resource))) == NULL)
      return (1);
  }

  // Merge default options...
  for (idx = 0; idx < dest->num_options; idx ++)
  {
    if (!cupsGetOption(dest->options[idx].name, num_options, options))
      num_options = cupsAddOption(dest->options[idx].name, dest->options[idx].value, num_options, &options);
  }

  // Process things...
  if (job_id)
  {
    // Update options for an existing job...
    return (set_job_attrs(command, http, dest, dinfo, resource, job_id, num_options, options));
  }
  else if (num_files > 0)
  {
    job_id = print_files(command, http, dest, dinfo, num_files, files, title, num_options, options);

    if (job_id && deletefile)
    {
      // Delete print files...
      for (idx = 0; idx < num_files; idx ++)
        unlink(files[idx]);
    }
  }
  else if (cupsCreateDestJob(http, dest, dinfo, &job_id, title ? title : "(stdin)", num_options, options) >= IPP_STATUS_ERROR_BAD_REQUEST)
  {
    cupsLangPrintf(stderr, _("%s: Unable to create job: %s"), command, cupsGetErrorString());
  }
  else
  {
    if (send_document(command, http, dest, dinfo, job_id, /*docname*/NULL, cupsGetOption("document-format", num_options, options), /*last_document*/true, 0))
      job_id = 0;
  }

  if (job_id > 0 && !silent)
    cupsLangPrintf(stdout, _("request id is %s-%d (%d file(s))"), dest->name, job_id, (int)num_files);

  return (job_id ? 0 : 1);
}


//
// 'connect_dest()' - Connect to the named destination and get its information.
//

static http_t *				// O - HTTP connection or `NULL` on error
connect_dest(const char   *command,	// I - Command name
             const char   *printer,	// I - Printer or `NULL` for default
             const char   *instance,	// I - Instance or `NULL` for primary
             cups_dest_t  **dest,	// O - Destination or `NULL` on error
             cups_dinfo_t **dinfo,	// O - Destination information or `NULL` on error
             char         *resource,	// I - Resource buffer
             size_t       resourcesize)	// I - Size of resource buffer
{
  http_t	*http = NULL;		// HTTP connection


  // Try to find the destination...
  *dinfo = NULL;

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
  else if ((*dinfo = cupsCopyDestInfo(http, *dest)) == NULL)
  {
    cupsLangPrintf(stderr, _("%s: Unable to get information on '%s': %s"), command, (*dest)->name, cupsGetErrorString());

    cupsFreeDests(1, *dest);
    *dest = NULL;

    httpClose(http);
    http = NULL;
  }

  return (http);
}


//
// 'print_files()' - Print one or more files to the specified destination...
//

static int				// O - Job ID or 0 on error
print_files(const char    *command,	// I - Command name
            http_t        *http,	// I - HTTP connection
            cups_dest_t   *dest,	// I - Destination
            cups_dinfo_t  *dinfo,	// I - Destination info
            size_t        num_files,	// I - Number of files
            const char    **files,	// I - Files
            const char    *title,	// I - Title
            size_t        num_options,	// I - Number of options
            cups_option_t *options)	// I - Options
{
  size_t	i;			// Looping var...
  int		job_id;			// Job ID


  if (cupsCreateDestJob(http, dest, dinfo, &job_id, title, num_options, options) >= IPP_STATUS_ERROR_BAD_REQUEST)
  {
    cupsLangPrintf(stderr, _("%s: Unable to create job: %s"), command, cupsGetErrorString());
    return (0);
  }

  for (i = 0; i < num_files; i ++)
  {
    int		fd;			// Print file
    const char	*docname;		// Document name
    int		status;			// Send status

    if ((fd = open(files[i], O_RDONLY)) < 0)
    {
      cupsLangPrintf(stderr, _("%s: Unable to open '%s': %s"), command, files[i], strerror(errno));
      cupsCancelDestJob(http, dest, job_id);
      return (0);
    }

    if ((docname = strrchr(files[i], '/')) != NULL)
      docname ++;
    else
      docname = files[i];

    status = send_document(command, http, dest, dinfo, job_id, docname, cupsGetOption("document-format", num_options, options), i == (num_files - 1), fd);

    close(fd);

    if (status)
    {
      cupsCancelDestJob(http, dest, job_id);
      return (0);
    }
  }

  return (job_id);
}


//
// 'send_document()' - Send a single document for printing.
//

static int				// O - Exit status
send_document(
    const char   *command,		// I - Command name
    http_t       *http,			// I - HTTP connection
    cups_dest_t  *dest,			// I - Destination
    cups_dinfo_t *dinfo,		// I - Destination info
    int          job_id,		// I - Job ID
    const char   *docname,		// I - Document name
    const char   *format,		// I - File format
    bool         last_document,		// I - Is this the last document?
    int          fd)			// I - File to send
{
  http_status_t	status;			// Write status
  char		buffer[8192];		// Copy buffer
  ssize_t	bytes;			// Bytes read


  // Start sending another document...
  status = cupsStartDestDocument(http, dest, dinfo, job_id, docname, format, /*num_options*/0, /*options*/NULL, last_document);

  // Copy the document to the job...
  while (status == HTTP_STATUS_CONTINUE && (bytes = read(fd, buffer, sizeof(buffer))) > 0)
    status = cupsWriteRequestData(http, buffer, (size_t)bytes);

  // Finish up...
  if (status != HTTP_STATUS_CONTINUE)
  {
    cupsLangPrintf(stderr, _("%s: Unable to spool document file."), command);
    cupsFinishDestDocument(http, dest, dinfo);
    cupsCancelDestJob(http, dest, job_id);
    return (1);
  }

  if (cupsFinishDestDocument(http, dest, dinfo) >= IPP_STATUS_ERROR_BAD_REQUEST)
  {
    cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
    cupsCancelDestJob(http, dest, job_id);
    return (1);
  }

  return (0);
}


//
// 'set_job_attrs()' - Set job attributes.
//

static int				// O - Exit status
set_job_attrs(
    const char    *command,		// I - Command name
    http_t        *http,		// I - HTTP connection
    cups_dest_t   *dest,		// I - Destination
    cups_dinfo_t  *dinfo,		// I - Destination info
    const char    *resource,		// I - Resource path
    int           job_id,		// I - Job ID
    int           num_options,		// I - Number of options
    cups_option_t *options)		// I - Options
{
  ipp_t	*request;			// IPP request


  if (num_options == 0)
    return (0);

  request = ippNewRequest(IPP_OP_SET_JOB_ATTRIBUTES);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, cupsGetOption("printer-uri-supported", dest->num_options, dest->options));
  ippAddInteger(request, IPP_TAG_OPERATION, IPP_TAG_INTEGER, "job-id", job_id);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
  cupsEncodeOptions(request, num_options, options, IPP_TAG_OPERATION);
  cupsEncodeOptions(request, num_options, options, IPP_TAG_JOB);

  ippDelete(cupsDoRequest(http, request, resource));

  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr, _("%s: Add '/version=1.1' to server name."), command);
    return (1);
  }
  else if (cupsGetError() > IPP_STATUS_OK_CONFLICTING)
  {
    cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
    return (1);
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
  if (!strcmp(command, "lp"))
    cupsLangPuts(out, _("Usage: lp [OPTIONS] [--] [FILES]\n"
			"       lp [OPTIONS] -i JOBID"));
  else
    cupsLangPuts(out, _("Usage: lpr [OPTIONS] [FILES]"));

  cupsLangPuts(out, _("Options:"));
  cupsLangPuts(out, _("--help                         Show this help"));
  cupsLangPuts(out, _("--version                      Show the program version"));
  if (!strcmp(command, "lp"))
  {
    cupsLangPuts(out, _("-c                             Make a copy of the print file(s)"));
    cupsLangPuts(out, _("-d DESTINATION                 Specify the destination"));
  }
  else
  {
    cupsLangPuts(out, _("-# COPIES                      Specify the number of copies to print"));
  }
  cupsLangPuts(out, _("-E                             Encrypt the connection to the server"));
  cupsLangPuts(out, _("-H SERVER[:PORT]               Connect to the named server and port"));
  if (!strcmp(command, "lp"))
    cupsLangPuts(out, _("-i JOBID                       Specify an existing job ID to modify"));
  cupsLangPuts(out, _("-m                             Send an email notification when the job completes"));
  if (!strcmp(command, "lp"))
    cupsLangPuts(out, _("-n COPIES                      Specify the number of copies to print"));
  cupsLangPuts(out, _("-o OPTION[=VALUE]              Specify a printer-specific option"));
  cupsLangPuts(out, _("-o job-sheets=standard         Print a banner page with the job"));
  cupsLangPuts(out, _("-o media=SIZE                  Specify the media size to use"));
  cupsLangPuts(out, _("-o number-up=N                 Specify that input pages should be printed N-up (1, 2, 4, 6, 9, and 16 are supported)"));
  cupsLangPuts(out, _("-o orientation-requested=N     Specify portrait (3) or landscape (4) orientation"));
  cupsLangPuts(out, _("-o page-ranges=FIRST-LAST      Specify a list of pages to print"));
  cupsLangPuts(out, _("-o print-quality=N             Specify the print quality - draft (3), normal (4), or best (5)"));
  cupsLangPuts(out, _("-o sides=one-sided             Specify 1-sided printing"));
  cupsLangPuts(out, _("-o sides=two-sided-long-edge   Specify 2-sided portrait printing"));
  cupsLangPuts(out, _("-o sides=two-sided-short-edge  Specify 2-sided landscape printing"));
  if (!strcmp(command, "lp"))
  {
    cupsLangPuts(out, _("-P FIRST-LAST                  Specify a list of pages to print"));
    cupsLangPuts(out, _("-q PRIORITY                    Specify the priority from low (1) to high (100)"));
    cupsLangPuts(out, _("-s                             Be silent"));
    cupsLangPuts(out, _("-t TITLE                       Specify the job title"));
  }
  else
  {
    cupsLangPuts(out, _("-P DESTINATION                 Specify the destination"));
    cupsLangPuts(out, _("-q                             Specify the job should be held for printing"));
    cupsLangPuts(out, _("-r                             Remove the file(s) after submission"));
    cupsLangPuts(out, _("-T TITLE                       Specify the job title"));
  }
  cupsLangPuts(out, _("-U USERNAME                    Specify the username to use for authentication"));

  return (out == stdout ? 0 : 1);
}
