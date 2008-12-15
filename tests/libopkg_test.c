#include <opkg.h>
#include <stdlib.h>
#include <stdio.h>

void
progress_callback (opkg_t *opkg, int percent, void *data)
{
  printf ("\r%s %3d%%", (char*) data, percent);
  fflush (stdout);
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
  printf ("\nopkg_update_package_lists returned %d\n", err);

  err = opkg_install_package (opkg, "aspell", progress_callback, "Installing...");
  printf ("\nopkg_install_package returned %d\n", err);

  err = opkg_upgrade_package (opkg, "aspell", progress_callback, "Upgrading...");
  printf ("\nopkg_upgrade_package returned %d\n", err);

  err = opkg_upgrade_all (opkg, progress_callback, "Upgrading all...");
  printf ("\nopkg_upgrade_package returned %d\n", err);

  err = opkg_remove_package (opkg, "aspell", progress_callback, "Removing...");
  printf ("\nopkg_remove_package returned %d\n", err);

  opkg_free (opkg);
}
