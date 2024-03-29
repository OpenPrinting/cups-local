//
// Common header for cupslocald.
//
// Copyright © 2023-2024 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef CUPSLOCALD_H
#  define CUPSLOCALD_H 1
#  include <config.h>
#  include <pappl/pappl.h>


//
// Globals...
//

#  ifdef CUPSLOCALD_MAIN_C
#    define VAR
#    define VALUE(x)	=x
#  else
#    define VAR		extern
#    define VALUE(x)
#  endif // CUPSLOCALD_MAIN_C

VAR pappl_pr_driver_t	LocalDrivers[7]
#  ifdef CUPSLOCALD_MAIN_C
= {
  { "everywhere",      "IPP Everywhere™",                     NULL, NULL },
  { "pcl",             "Generic PCL",                         NULL, NULL },
  { "pcl_duplex",      "Generic PCL w/Duplexer",              NULL, NULL },
  { "ps",              "Generic PostScript",                  NULL, NULL },
  { "ps_color",        "Generic Color PostScript",            NULL, NULL },
  { "ps_duplex",       "Generic PostScript w/Duplexer",       NULL, NULL },
  { "ps_color_duplex", "Generic Color PostScript w/Duplexer", NULL, NULL }
}
#  endif // CUPSLOCALD_MAIN_C
;
VAR char		LocalSocket[256] VALUE("");
					// Domain socket path
VAR char		LocalSpoolDir[256] VALUE("");
					// Spool directory
VAR char		LocalStateFile[256] VALUE("");
					// State file


//
// Functions...
//

#  ifdef HAVE_DBUS
extern void		*LocalDBusService(void *data);
#  endif // HAVE_DBUS

extern const char	*LocalDriverAutoAdd(const char *device_info, const char *device_uri, const char *device_id, void *data);
extern bool		LocalDriverCallback(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *driver_data, ipp_t **driver_attrs, void *data);
extern bool		LocalTransformFilter(pappl_job_t *job, int doc_number, pappl_pr_options_t *options, pappl_device_t *device, void *data);


#endif // !CUPSLOCALD_H
