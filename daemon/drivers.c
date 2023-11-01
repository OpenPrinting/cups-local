//
// Generic drivers for cups-local.
//
// Copyright © 2023 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cups-locald.h"


//
// 'LocalDriverAutoAdd()' - Determine a matching driver for a printer.
//

const char *				// O - Driver name
LocalDriverAutoAdd(
    const char *device_info,		// I - Device information
    const char *device_uri,		// I - Device URI
    const char *device_id,		// I - Device ID
    void       *data)			// I - User data (unused)
{
  return (NULL);
}


//
// 'LocalDriverCallback()' - Setup driver data.
//

bool
LocalDriverCallback(
    pappl_system_t *system,
    const char *driver_name,
    const char *device_uri,
    const char *device_id,
    pappl_pr_driver_data_t *driver_data,
    ipp_t **driver_attrs,
    void *data)
{
  return (false);
}
