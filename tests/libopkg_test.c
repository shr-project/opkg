#include <opkg.h>
#include <stdlib.h>
#include <stdio.h>

void
progress_callback (opkg_t *opkg, int percent, void *data)
{
  printf ("%s %d\n", (char*) data, percent);
}


int
main (int argc, char **argv)
{
  opkg_t *opkg;
  int err;
  
  opkg = opkg_new ();

  opkg_set_option (opkg, "offline_root", "/tmp/");

  opkg_read_config_files (opkg);

  err = opkg_update_package_lists (opkg, progress_callback, "Updating...");

  printf ("opkg_update_package_lists returned %d\n", err);

  err = opkg_install_package (opkg, "aspell", progress_callback, "Installing...");

  printf ("opkg_install_package returned %d\n", err);

  err = opkg_remove_package (opkg, "aspell", progress_callback, "Removing...");

  printf ("opkg_remove_package returned %d\n", err);

  opkg_free (opkg);
}
