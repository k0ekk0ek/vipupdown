/* system includes */
#include <stdlib.h>

/* ifupdown includes */
#include "ifupdown.h"

void
free_interfaces (interfaces_file *ifaces_file)
{
  allowup_defn *allowup, *next_allowup;
  int ifno, optno;
  interface_defn *iface, *next_iface;

  if (ifaces_file) {
    if (ifaces_file->allowups) {
      for (allowup = ifaces_file->allowups; allowup; allowup = next_allowup) {
        next_allowup = allowup->next;

        if (allowup->when)
          free (allowup->when);
        if (allowup->interfaces) {
          for (ifno = 0; ifno < allowup->n_interfaces; ifno++) {
            if (allowup->interfaces[ifno])
              free (allowup->interfaces[ifno]);
          }
          free (allowup->interfaces);
        }
        free (allowup);
      }
    }
    if (ifaces_file->ifaces) {
      for (iface = ifaces_file->ifaces; iface; iface = next_iface) {
        next_iface = iface->next;

        if (iface->logical_iface)
          free (iface->logical_iface);
        if (iface->option) {
          for (optno = 0; optno < iface->n_options; optno++) {
            if (iface->option[optno].name)
              free (iface->option[optno].name);
            if (iface->option[optno].value)
              free (iface->option[optno].value);
          }
          free (iface->option);
        }

        free (iface);
      }
    }
    //if (ifaces_file->mappings) {
    //}
    free (ifaces_file);
  }
}
