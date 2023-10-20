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


//
// Local functions.
//

static cups_dest_t	*get_dest(const char *command, const char *printer, const char *instance, cups_dinfo_t *dinfo);
static int		print_files(const char *command, cups_dest_t *dest, cups_dinfo_t *dinfo, size_t num_files, const char **files, const char *title, size_t num_options, cups_option_t *options);
static int		restart_job(const char *command, int job_id, const char *job_hold_until);
static int		send_document(const char *command, cups_dest_t *dest, cups_dinfo_t *dinfo, int job_id, const char *format, bool last_document, int fd);
static int		set_job_attrs(const char *command, int job_id, int num_options, cups_option_t *options);
static int		usage(FILE *out, const char *command);


//
// 'main()' - Parse options and send files for printing.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  const char	*command;		// Command name
  int		i, j;			// Looping vars
  int		job_id = 0;		// Job ID
  char		*printer = NULL,	// Printer name
		*instance = NULL,	// Instance name
		*opt,			// Option pointer
		*val,			// Option value
		*title = NULL;		// Job title
  int		priority;		// Job priority (1-100)
  int		num_copies;		// Number of copies per file
  size_t	num_files = 0;		// Number of files to print
  const char	*files[1000];		// Files to print
  cups_dest_t	*dest = NULL;		// Selected destination
  cups_dinfo_t	*dinfo = NULL;		// Destination information
  size_t	num_options = 0;	// Number of options
  cups_option_t	*options = NULL;	// Options
  bool		end_options = false;	// No more options?
  bool		silent = false;		// Silent or verbose output?
  char		buffer[8192];		// Copy buffer


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
      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%s\"."), command, argv[i]);
      return (usage(stderr, command));
    }
    else if (argv[i][0] == '-' && argv[i][1] && !end_options)
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
        switch (*opt)
	{
	  case 'E' : // Encrypt
	      cupsSetEncryption(HTTP_ENCRYPTION_REQUIRED);
	      break;

	  case 'H' : // Destination host
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
		  cupsLangPrintf(stderr, _("%s: Error - expected hostname after \"-H\" option."), command);
		  return (usage(stderr, command));
		}

		cupsSetServer(argv[i]);
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
		    cupsLangPrintf(stderr, _("%s: Error - expected page list after \"-P\" option."), command);
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
		    cupsLangPrintf(stderr, _("%s: Error - expected destination after \"-d\" option."), command);
		    return (usage(stderr, command));
		  }

		  printer = argv[i];
		}

		if ((instance = strrchr(printer, '/')) != NULL)
		  *instance++ = '\0';

		if ((dest = get_dest(printer, instance, &dinfo)) == NULL)
		  return (1);
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
		  cupsLangPrintf(stderr, _("%s: Error - expected username after \"-U\" option."), command);
		  return (usage(stderr, command));
		}

		cupsSetUser(argv[i]);
	      }
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
		  cupsLangPrintf(stderr, _("%s: Error - expected destination after \"-d\" option."), command);
		  return (usage(stderr, command));
		}

		printer = argv[i];
	      }

	      if ((instance = strrchr(printer, '/')) != NULL)
		*instance++ = '\0';

              if ((dest = get_dest(printer, instance, &dinfo)) == NULL)
                return (1);
	      break;

	  case 'i' : // Change job
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
		  cupsLangPrintf(stderr, _("%s: Expected job ID after \"-i\" option."), command);
		  return (usage(stderr, command));
		}

		val = argv[i];
	      }

	      if (num_files > 0)
	      {
		cupsLangPrintf(stderr, _("%s: Error - cannot print files and alter jobs simultaneously."), command);
		return (1);
	      }

	      if (strrchr(val, '-') != NULL)
		job_id = atoi(strrchr(val, '-') + 1);
	      else
		job_id = atoi(val);

	      if (job_id < 0)
	      {
		cupsLangPrintf(stderr, _("%s: Error - bad job ID."), command);
		break;
	      }
	      break;

	  case 'm' : // Send email when job is done
	  case 'w' : // Write to console or email
	      {
		char	email[1024];	// EMail address


		snprintf(email, sizeof(email), "mailto:%s@%s", cupsGetUser(), httpGetHostname(NULL, buffer, sizeof(buffer)));
		num_options = cupsAddOption("notify-recipient-uri", email, num_options, &options);
	      }

	      silent = true;
	      break;

	  case 'n' : // Number of copies
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
		  cupsLangPrintf(stderr, _("%s: Error - expected copies after \"-n\" option."), command);
		  return (usage(stderr, command));
		}

		num_copies = atoi(argv[i]);
	      }

	      if (num_copies < 1)
	      {
		cupsLangPrintf(stderr, _("%s: Error - copies must be 1 or more."), command);
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
		  cupsLangPrintf(stderr, _("%s: Error - expected option=value after \"-o\" option."), command);
		  return (usage(stderr, command));
		}

		num_options = cupsParseOptions(argv[i], num_options, &options);
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
		  cupsLangPrintf(stderr, _("%s: Error - expected priority after \"-%c\" option."), command, *opt);
		  return (usage(stderr, command));
		}

		i ++;

		priority = atoi(argv[i]);
	      }

	      if (priority < 1 || priority > 100)
	      {
		cupsLangPrintf(stderr, _("%s: Error - priority must be between 1 and 100."), command);
		return (1);
	      }

	      num_options = cupsAddIntegerOption("job-priority", priority, num_options, &options);
	      break;

	  case 's' : // Silent
	      silent = true;
	      break;

	  case 't' : // Title
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
		  cupsLangPrintf(stderr, _("%s: Error - expected title after \"-t\" option."), command);
		  return (usage(stderr, command));
		}

		title = argv[i];
	      }
	      break;

	  case 'y' : // mode-list
	      if (opt[1] != '\0')
	      {
		opt += strlen(opt) - 1;
	      }
	      else
	      {
		i ++;

		if (i >= argc)
		{
		  cupsLangPrintf(stderr, _("%s: Error - expected mode list after \"-y\" option."), command);
		  return (usage(stderr, command));
		}
	      }

	      cupsLangPrintf(stderr, _("%s: Warning - mode option ignored."), command);
	      break;


	  case '-' : // Stop processing options
	      end_options = true;
	      break;

	  default :
	      cupsLangPrintf(stderr, _("%s: Error - unknown option \"%c\"."), command, *opt);
	      return (usage(stderr, command));
	}
      }
    }
    else if (!strcmp(argv[i], "-"))
    {
      if (num_files || job_id)
      {
        cupsLangPrintf(stderr, _("%s: Error - cannot print from stdin if files or a job ID are provided."), command);
	return (1);
      }

      break;
    }
    else if (num_files < 1000 && job_id == 0)
    {
      // Print a file...
      if (access(argv[i], R_OK) != 0)
      {
        cupsLangPrintf(stderr, _("%s: Error - unable to access \"%s\" - %s"), command, argv[i], strerror(errno));
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
      cupsLangPrintf(stderr, _("%s: Error - too many files - \"%s\"."), command, argv[i]);
    }
  }

  // See if we are altering an existing job...
  if (job_id)
    return (set_job_attrs(command, job_id, num_options, options));

  // See if we have any files to print; if not, print from stdin...
  if (printer == NULL)
  {
    if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL)) != NULL)
    {
      printer = dest->name;

      for (j = 0; j < dest->num_options; j ++)
      {
	if (cupsGetOption(dest->options[j].name, num_options, options) == NULL)
	  num_options = cupsAddOption(dest->options[j].name, dest->options[j].value, num_options, &options);
      }
    }
    else if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
    {
      cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server  name."), command);
      return (1);
    }
  }

  if (printer == NULL)
  {
    if (!cupsGetNamedDest(CUPS_HTTP_DEFAULT, NULL, NULL) && cupsGetError() == IPP_STATUS_ERROR_NOT_FOUND)
      cupsLangPrintf(stderr, _("%s: Error - %s"), command, cupsGetErrorString());
    else
      cupsLangPrintf(stderr, _("%s: Error - scheduler not responding."), command);

    return (1);
  }

  if (num_files > 0)
  {
    job_id = cupsPrintFiles(printer, num_files, files, title, num_options, options);
  }
  else if ((job_id = cupsCreateJob(CUPS_HTTP_DEFAULT, printer, title ? title : "(stdin)", num_options, options)) > 0)
  {
    http_status_t	status;		// Write status
    const char		*format;	// Document format
    ssize_t		bytes;		// Bytes read

    if (cupsGetOption("raw", num_options, options))
      format = CUPS_FORMAT_RAW;
    else if ((format = cupsGetOption("document-format", num_options, options)) == NULL)
      format = CUPS_FORMAT_AUTO;

    status = cupsStartDocument(CUPS_HTTP_DEFAULT, printer, job_id, NULL, format, 1);

    while (status == HTTP_STATUS_CONTINUE && (bytes = read(0, buffer, sizeof(buffer))) > 0)
      status = cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer, (size_t)bytes);

    if (status != HTTP_STATUS_CONTINUE)
    {
      cupsLangPrintf(stderr, _("%s: Error - unable to queue from stdin - %s."), command, httpStatus(status));
      cupsFinishDocument(CUPS_HTTP_DEFAULT, printer);
      cupsCancelJob2(CUPS_HTTP_DEFAULT, printer, job_id, 0);
      return (1);
    }

    if (cupsFinishDocument(CUPS_HTTP_DEFAULT, printer) != IPP_STATUS_OK)
    {
      cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
      cupsCancelJob2(CUPS_HTTP_DEFAULT, printer, job_id, 0);
      return (1);
    }
  }

  if (job_id < 1)
  {
    cupsLangPrintf(stderr, "%s: %s", command, cupsGetErrorString());
    return (1);
  }
  else if (!silent)
  {
    cupsLangPrintf(stdout, _("request id is %s-%d (%d file(s))"), printer, job_id, num_files);
  }

  return (0);
}


//
// 'get_dest()' - Get the named destination and its information.
//

static cups_dest_t *			// O - Destination or `NULL` on error
get_dest(const char   *command,		// I - Command name
         const char   *printer,		// I - Printer name
         const char   *instance,	// I - Instance name
         cups_dinfo_t *dinfo)		// O - Destination information or `NULL`
{
  cups_dest_t	*dest;			// Destination


  // Try to find the destination...
  if ((dest = cupsGetNamedDest(CUPS_HTTP_DEFAULT, printer, instance)) == NULL)
  {
    // Destination doesn't exist...
    *dinfo = NULL;

    if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
      cupsLangPrintf(stderr, _("%s: Error - try adding '/version=1.1' to server name."), command);
    else if (cupsGetError() == IPP_STATUS_ERROR_NOT_FOUND)
      cupsLangPrintf(stderr, _("%s: Error - The printer or class does not exist."), command);
    else
      cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());

    return (NULL);
  }

  // Get the destination info...
  if ((dinfo = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, dest)) == NULL)
  {
    cupsLangPrintf(stderr, _("%s: %s"), command, cupsGetErrorString());
    cupsFreeDests(1, dest);
    dest = NULL;
  }

  return (dest);
}


//
// 'print_files()' - Print one or more files to the specified destination...
//

static int				// O - Exit status
print_files(cups_dest_t   *dest,	// I - Destination
            size_t        num_files,	// I - Number of files
            const char    **files,	// I - Files
            const char    *title,	// I - Title
            size_t        num_options,	// I - Number of options
            cups_option_t *options)	// I - Options
{
  cups_dinfo_t	*dinfo;			// Destination information
}


//
// 'restart_job()' - Restart a job.
//

static int				// O - Exit status
restart_job(const char *command,	// I - Command name
            int        job_id,		// I - Job ID
            const char *job_hold_until)	// I - "job-hold-until" value, if any
{
  ipp_t		*request;		// IPP request
  char		uri[HTTP_MAX_URI];	// URI for job


  // TODO: Fix job-uri
  request = ippNewRequest(IPP_RESTART_JOB);

  snprintf(uri, sizeof(uri), "ipp://localhost/jobs/%d", job_id);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());

  if (job_hold_until)
    ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "job-hold-until", NULL, job_hold_until);

  // TODO: Fix resource path
  ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/jobs"));

  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
// 'send_document()' - Send a single document for printing.
//

static int				// O - Exit status
send_document(
    const char   *command,		// I - Command name
    cups_dest_t  *dest,			// I - Destination
    cups_dinfo_t *dinfo,		// I - Destination info
    int          job_id,		// I - Job ID
    const char   *format,		// I - File format
    bool         last_document,		// I - Is this the last document?
    int          fd)			// I - File to send
{
}


//
// 'set_job_attrs()' - Set job attributes.
//

static int				// O - Exit status
set_job_attrs(
    const char    *command,		// I - Command name
    int           job_id,		// I - Job ID
    int           num_options,		// I - Number of options
    cups_option_t *options)		// I - Options
{
  ipp_t		*request;		// IPP request
  char		uri[HTTP_MAX_URI];	// URI for job


  if (num_options == 0)
    return (0);

  request = ippNewRequest(IPP_SET_JOB_ATTRIBUTES);

  // TODO: Fix job-uri
  snprintf(uri, sizeof(uri), "ipp://localhost/jobs/%d", job_id);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsGetUser());
  cupsEncodeOptions(request, num_options, options);

  // TODO: Fix resource path
  ippDelete(cupsDoRequest(CUPS_HTTP_DEFAULT, request, "/jobs"));

  if (cupsGetError() == IPP_STATUS_ERROR_BAD_REQUEST || cupsGetError() == IPP_STATUS_ERROR_VERSION_NOT_SUPPORTED)
  {
    cupsLangPrintf(stderr, _("%s: Error - add '/version=1.1' to server name."), command);
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
