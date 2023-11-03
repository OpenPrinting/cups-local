//
// D-Bus API support for cups-local.
//
// Copyright © 2023 by OpenPrinting.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "cups-locald.h"
#ifdef HAVE_DBUS
#  include <dbus/dbus.h>


//
// The D-Bus API is used to start cups-locald and return the proper Unix
// domain socket to use.  The interface is "org.openprinting.cups-locald".
//
// Method: GetSocket
//
// Get the Unix domain socket path for the current session.  There are no
// arguments.  The reply contains a single string containing the socket path.
//


//
// 'LocalDBusService()' - Listen for D-Bus messages.
//

void *					// O - Thread exit status
LocalDBusService(void *data)		// I - Thread data (unused)
{
  DBusConnection	*dbus;		// D-Bus connection
  DBusError		error;		// Last error
  DBusMessage		*msg;		// D-Bus message


  (void)data;

  // Connect to the session bus...
  if ((dbus = dbus_bus_get(DBUS_BUS_SESSION, &error)) == NULL)
    return (NULL);

  // Read message until error/exit...
  for (;;)
  {
    // Get/send new messages...
    dbus_connection_read_write(dbus, -1);

    // Parse any incoming messages...
    if ((msg = dbus_connection_pop_message(dbus)) != NULL)
    {
      if (dbus_message_is_method_call(msg, "org.openprinting.cups-locald", "GetSocket"))
      {
        // Reply with the domain socket path...
	DBusMessage	*reply;		// Reply message

	if ((reply = dbus_message_new_method_return(msg)) != NULL)
	{
	  DBusMessageIter iter;		// Iterator

	  dbus_message_iter_init_append(reply, &iter);
	  dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, LocalSocket);
	  dbus_connection_send(dbus, reply, NULL);
	  dbus_connection_flush(dbus);
	  dbus_message_unref(reply);
	}
      }

      dbus_message_unref(msg);
    }
  }

  return (NULL);
}
#endif // HAVE_DBUS
