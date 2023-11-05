//
// Transform support for cups-local.
//
// Copyright © 2023 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cups-locald.h"
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;


//
// Local functions...
//

static void	process_attr_message(pappl_job_t *job, char *message);
static void	process_state_message(pappl_job_t *job, char *message);


//
// 'LocalTransformFilter()' - Convert an input document to PDF or raster.
//

bool					// O - `true` on success, `false` on failure
LocalTransformFilter(
    pappl_job_t    *job,		// I - Job
    pappl_device_t *device,		// I - Output device
    void           *cbdata)		// I - Callback data (not used)
{
  size_t		i;		// Looping var
  pappl_printer_t	*printer;	// Printer for job
  pappl_pr_driver_data_t pdata;		// Printer driver data
  ipp_t			*pattrs;	// Printer driver attributes
  ipp_attribute_t	*attr;		// Current attribute
  const char 		*xargv[3];	// Command-line arguments for ipptransform
  size_t		xenvc;		// Number of environment variables
  char			*xenvp[1000];	// Environment variables for ipptransform
  pid_t			xpid;		// Process ID for ipptransform program
  int			xstdout[2] = {-1,-1},
					// Standard output pipe for ipptransform
			xstderr[2] = {-1,-1},
					// Standard error pipe for ipptransform
			xstatus;	// Exit status of ipptransform
  posix_spawn_file_actions_t xactions;	// File actions
  struct pollfd		polldata[2];	// poll() file descriptors
  ssize_t		bytes;		// Number of bytes read
  char			val[1280],	// IPP_NAME=value
			*valptr,	// Pointer into string
			data[32768],	// Data from stdout
			line[2048],	// Line from stderr
			*ptr,		// Pointer into line
			*endptr;	// End of line
  static const char	*jattrs[] =	// Job attributes
  {
    "copies",
    "finishings",
    "media",
    "media-col",
    "orientation-requested",
    "output-bin",
    "page-ranges",
    "print-color-mode",
    "print-content-optimize",
    "print-quality",
    "print-scaling",
    "printer-resolution",
    "sides"
  };


  (void)cbdata;

  // Get job and printer information...
  printer = papplJobGetPrinter(job);
  pattrs  = papplPrinterGetDriverAttributes(printer);
  papplPrinterGetDriverData(printer, &pdata);

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Running ipptransform command.");

  // Setup the command-line arguments...
  xargv[0] = "ipptransform";
  xargv[1] = papplJobGetFilename(job);
  xargv[2] = NULL;

  // Copy the current environment, then add environment variables for every
  // Job attribute and select Printer attributes...
  for (xenvc = 0; environ[xenvc] && xenvc < (sizeof(xenvp) / sizeof(xenvp[0]) - 1); xenvc ++)
    xenvp[xenvc] = strdup(environ[xenvc]);

  if (xenvc > (sizeof(xenvp) / sizeof(xenvp[0]) - 32))
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Too many environment variables to transform job.");
    goto transform_failure;
  }

  if (asprintf(xenvp + xenvc, "CONTENT_TYPE=%s", papplJobGetFormat(job)) > 0)
    xenvc ++;

  if (pdata.format && asprintf(xenvp + xenvc, "OUTPUT_TYPE=%s", pdata.format) > 0)
    xenvc ++;

  for (attr = ippGetFirstAttribute(pattrs); attr && xenvc < (sizeof(xenvp) / sizeof(xenvp[0]) - 1); attr = ippGetNextAttribute(pattrs))
  {
    // Convert "attribute-name-default" to "IPP_ATTRIBUTE_NAME_DEFAULT=" and
    // "pwg-xxx" to "IPP_PWG_XXX=", then add the value(s) from the attribute.
    const char	*name = ippGetName(attr);
					// Attribute name

    if (strncmp(name, "pwg-", 4) && !strstr(name, "-default"))
      continue;

    valptr = val;
    *valptr++ = 'I';
    *valptr++ = 'P';
    *valptr++ = 'P';
    *valptr++ = '_';
    while (*name && valptr < (val + sizeof(val) - 2))
    {
      if (*name == '-')
	*valptr++ = '_';
      else
	*valptr++ = (char)toupper(*name & 255);

      name ++;
    }
    *valptr++ = '=';
    ippAttributeString(attr, valptr, sizeof(val) - (size_t)(valptr - val));

    xenvp[xenvc++] = strdup(val);
  }

  xenvp[xenvc ++] = strdup("SERVER_LOGLEVEL=debug");

  for (i = 0; i < (sizeof(jattrs) / sizeof(jattrs[0])) && xenvc < (sizeof(xenvp) / sizeof(xenvp[0]) - 1); i ++)
  {
    // Convert "attribute-name" to "IPP_ATTRIBUTE_NAME=" and then add the
    // value(s) from the attribute.
    const char *name;			// Attribute name

    if ((attr = papplJobGetAttribute(job, jattrs[i])) == NULL)
      continue;

    name   = ippGetName(attr);
    valptr = val;
    *valptr++ = 'I';
    *valptr++ = 'P';
    *valptr++ = 'P';
    *valptr++ = '_';
    while (*name && valptr < (val + sizeof(val) - 2))
    {
      if (*name == '-')
        *valptr++ = '_';
      else
        *valptr++ = (char)toupper(*name & 255);

      name ++;
    }
    *valptr++ = '=';
    ippAttributeString(attr, valptr, sizeof(val) - (size_t)(valptr - val));

    xenvp[xenvc++] = strdup(val);
  }

  xenvp[xenvc] = NULL;

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Transform environment:");
  for (i = 0; i < xenvc; i ++)
    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "    %s", xenvp[i]);

  // Now run the program...
  if (pipe(xstdout))
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to create pipe for stdout: %s", strerror(errno));
    goto transform_failure;
  }

  if (pipe(xstderr))
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to create pipe for stderr: %s", strerror(errno));
    goto transform_failure;
  }

  posix_spawn_file_actions_init(&xactions);
  posix_spawn_file_actions_addopen(&xactions, 0, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_adddup2(&xactions, xstdout[1], 1);
  posix_spawn_file_actions_adddup2(&xactions, xstderr[1], 2);

  if (posix_spawn(&xpid, "ipptransform", &xactions, NULL, (char * const *)xargv, xenvp))
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to start 'ipptransform' command: %s", strerror(errno));
    posix_spawn_file_actions_destroy(&xactions);

    goto transform_failure;
  }

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Started 'ipptransform' command, pid=%d", (int)xpid);

  // Free memory used for command...
  posix_spawn_file_actions_destroy(&xactions);

  while (xenvc > 0)
    free(xenvp[-- xenvc]);

  // Read from the stdout and stderr pipes until EOF...
  close(xstdout[1]);
  close(xstderr[1]);

  endptr = line;

  polldata[0].fd     = xstdout[0];
  polldata[0].events = POLLIN;
  polldata[1].fd     = xstderr[0];
  polldata[1].events = POLLIN;

  while (poll(polldata, (nfds_t)2, -1) > 0)
  {
    if (polldata[0].revents & POLLIN)
    {
      if ((bytes = read(xstdout[0], data, sizeof(data))) > 0)
	papplDeviceWrite(device, data, (size_t)bytes);
    }

    if (polldata[1].revents & POLLIN)
    {
      // Message on stderr - log message or update progress...
      if ((bytes = read(xstderr[0], endptr, sizeof(line) - (size_t)(endptr - line) - 1)) > 0)
      {
	endptr += bytes;
	*endptr = '\0';

	while ((ptr = strchr(line, '\n')) != NULL)
	{
	  *ptr++ = '\0';

          if ((valptr = strchr(line, ':')) != NULL)
          {
            // Find text after ':'...
            valptr ++;
          }
          else
          {
            // No prefix, just point at start of line...
            valptr = line;
          }

          // Skip whitespace...
	  while (*valptr && isspace(*valptr & 255))
	    valptr ++;

          // Parse line...
	  if (!strncmp(line, "STATE:", 6))
	  {
	    // Process printer-state-reasons keywords.
	    process_state_message(job, valptr);
	  }
	  else if (!strncmp(line, "ATTR:", 5))
	  {
	    // Process job/printer attribute update.
	    process_attr_message(job, valptr);
	  }
	  else if (!strncmp(line, "ERROR:", 6))
	  {
	    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "%s", valptr);
	  }
	  else if (!strncmp(line, "WARN:", 5))
	  {
	    papplLogJob(job, PAPPL_LOGLEVEL_WARN, "%s", valptr);
	  }
	  else if (!strncmp(line, "INFO:", 5))
	  {
	    papplLogJob(job, PAPPL_LOGLEVEL_INFO, "%s", valptr);
	  }
	  else if (!strncmp(line, "DEBUG:", 6))
	  {
	    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "%s", valptr);
	  }
	  else
	  {
	    // No recognizable prefix, just log it...
	    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "%s", line);
	  }

	  bytes = ptr - line;
	  if (ptr < endptr)
	    memmove(line, ptr, (size_t)(endptr - ptr));
	  endptr -= bytes;
	  *endptr = '\0';
	}
      }
    }

    if ((polldata[0].revents & POLLHUP) || (polldata[1].revents & POLLHUP))
      break;
  }

  close(xstdout[0]);
  close(xstderr[0]);

  // Wait for child to complete...
  while (waitpid(xpid, &xstatus, 0) < 0);

  if (xstatus)
  {
    if (WIFEXITED(xstatus))
      papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "ipptransform command exited with status %d.", WEXITSTATUS(xstatus));
    else if (WIFSIGNALED(xstatus) && WTERMSIG(xstatus) != SIGTERM)
      papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "ipptransform command crashed on signal %d.", WTERMSIG(xstatus));
  }

  return (xstatus ? true : false);

  // This is where we go for hard failures...
  transform_failure:

  if (xstdout[0] >= 0)
    close(xstdout[0]);
  if (xstdout[1] >= 0)
    close(xstdout[1]);

  if (xstderr[0] >= 0)
    close(xstderr[0]);
  if (xstderr[1] >= 0)
    close(xstderr[1]);

  while (xenvc > 0)
    free(xenvp[-- xenvc]);

  return (false);

}


//
// 'process_attr_message()' - Process an ATTR: message from the ipptransform
//                            command.
//

static void
process_attr_message(
    pappl_job_t *job,			// I - Job
    char        *message)		// I - Message
{
  (void)job;
  (void)message;

#if 0
  size_t	i,			// Looping var
		num_options = 0;	// Number of name=value pairs
  cups_option_t	*options = NULL,	// name=value pairs from message
		*option;		// Current option
  ipp_attribute_t *attr;		// Current attribute


 /*
  * Grab attributes from the message line...
  */

  serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "%s", message);

  num_options = cupsParseOptions(message + 5, num_options, &options);

  serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "num_options=%u", (unsigned)num_options);

 /*
  * Loop through the options and record them in the printer or job objects...
  */

  for (i = num_options, option = options; i > 0; i --, option ++)
  {
    serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "options[%u].name=\"%s\", .value=\"%s\"", (unsigned)(num_options - i), option->name, option->value);

    if (!strcmp(option->name, "job-impressions"))
    {
     /*
      * Update job-impressions attribute...
      */

      serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "Setting Job Status attribute \"%s\" to \"%s\".", option->name, option->value);

      cupsRWLockWrite(&job->rwlock);

      job->impressions = atoi(option->value);

      cupsRWUnlock(&job->rwlock);
    }
    else if (mode == SERVER_TRANSFORM_COMMAND && !strcmp(option->name, "job-impressions-completed"))
    {
     /*
      * Update job-impressions-completed attribute...
      */

      serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "Setting Job Status attribute \"%s\" to \"%s\".", option->name, option->value);

      cupsRWLockWrite(&job->rwlock);

      job->impcompleted = atoi(option->value);

      cupsRWUnlock(&job->rwlock);
    }
    else if (!strcmp(option->name, "job-impressions-col") || !strcmp(option->name, "job-media-sheets") || !strcmp(option->name, "job-media-sheets-col") ||
        (mode == SERVER_TRANSFORM_COMMAND && (!strcmp(option->name, "job-impressions-completed-col") || !strcmp(option->name, "job-media-sheets-completed") || !strcmp(option->name, "job-media-sheets-completed-col"))))
    {
     /*
      * Update Job Status attribute...
      */

      serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "Setting Job Status attribute \"%s\" to \"%s\".", option->name, option->value);

      cupsRWLockWrite(&job->rwlock);

      if ((attr = ippFindAttribute(job->attrs, option->name, IPP_TAG_ZERO)) != NULL)
        ippDeleteAttribute(job->attrs, attr);

      cupsEncodeOption(job->attrs, IPP_TAG_JOB, option->name, option->value);

      cupsRWUnlock(&job->rwlock);
    }
    else if (!strncmp(option->name, "marker-", 7) || !strcmp(option->name, "printer-alert") || !strcmp(option->name, "printer-supply") || !strcmp(option->name, "printer-supply-description"))
    {
     /*
      * Update Printer Status attribute...
      */

      serverLogPrinter(SERVER_LOGLEVEL_DEBUG, job->printer, "Setting Printer Status attribute \"%s\" to \"%s\".", option->name, option->value);

      cupsRWLockWrite(&job->printer->rwlock);

      if ((attr = ippFindAttribute(job->printer->pinfo.attrs, option->name, IPP_TAG_ZERO)) != NULL)
        ippDeleteAttribute(job->printer->pinfo.attrs, attr);

      cupsEncodeOption(job->printer->pinfo.attrs, IPP_TAG_PRINTER, option->name, option->value);

      cupsRWUnlock(&job->printer->rwlock);
    }
    else
    {
     /*
      * Something else that isn't currently supported...
      */

      serverLogJob(SERVER_LOGLEVEL_DEBUG, job, "Ignoring attribute \"%s\" with value \"%s\".", option->name, option->value);
    }
  }

  cupsFreeOptions(num_options, options);
#endif // 0
}


//
// 'process_state_message()' - Process a STATE: message from a command.
//

static void
process_state_message(
    pappl_job_t *job,			// I - Job
    char        *message)		// I - Message
{
  (void)job;
  (void)message;

#if 0
  int		i;			// Looping var
  server_preason_t preasons,		// printer-state-reasons values
		pbit;			// Current printer reason bit
  server_jreason_t jreasons,		// job-state-reasons values
		jbit;			// Current job reason bit
  char		*ptr,			// Pointer into message
		*next;			// Next keyword in message
  int		remove;			// Non-zero if we are removing keywords


 /*
  * Skip leading "STATE:" and any whitespace...
  */

  for (message += 6; *message; message ++)
    if (*message != ' ' && *message != '\t')
      break;

 /*
  * Support the following forms of message:
  *
  * "keyword[,keyword,...]" to set the job/printer-state-reasons value(s).
  *
  * "-keyword[,keyword,...]" to remove keywords.
  *
  * "+keyword[,keyword,...]" to add keywords.
  *
  * Keywords may or may not have a suffix (-report, -warning, -error) per
  * RFC 8011.
  */

  if (*message == '-')
  {
    remove   = 1;
    jreasons = job->state_reasons;
    preasons = job->printer->state_reasons;
    message ++;
  }
  else if (*message == '+')
  {
    remove   = 0;
    jreasons = job->state_reasons;
    preasons = job->printer->state_reasons;
    message ++;
  }
  else
  {
    remove   = 0;
    jreasons = job->state_reasons;
    preasons = SERVER_PREASON_NONE;
  }

  while (*message)
  {
    if ((next = strchr(message, ',')) != NULL)
      *next++ = '\0';

    for (i = 0, jbit = 1; i < (int)(sizeof(server_jreasons) / sizeof(server_jreasons[0])); i ++, jbit *= 2)
    {
      if (!strcmp(message, server_jreasons[i]))
      {
        if (remove)
	  jreasons &= ~jbit;
	else
	  jreasons |= jbit;
      }
    }

    if ((ptr = strstr(message, "-error")) != NULL)
      *ptr = '\0';
    else if ((ptr = strstr(message, "-report")) != NULL)
      *ptr = '\0';
    else if ((ptr = strstr(message, "-warning")) != NULL)
      *ptr = '\0';

    for (i = 0, pbit = 1; i < (int)(sizeof(server_preasons) / sizeof(server_preasons[0])); i ++, pbit *= 2)
    {
      if (!strcmp(message, server_preasons[i]))
      {
        if (remove)
	  preasons &= ~pbit;
	else
	  preasons |= pbit;
      }
    }

    if (next)
      message = next;
    else
      break;
  }

  job->state_reasons          = jreasons;
  job->printer->state_reasons = preasons;
#endif // 0
}
