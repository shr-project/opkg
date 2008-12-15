#include <opkg.h>
#include <stdlib.h>
#include <stdio.h>

void
progress_callback (opkg_t *opkg, int percent, void *data)
{
  printf ("\r%s %3d%%", (char*) data, percent);
  fflush (stdout);
}

void
package_list_callback (opkg_t *opkg, opkg_package_t *pkg, void *data)
{
  static install_count = 0;
  static total_count = 0;

  if (pkg->installed)
    install_count++;

  total_count++;

  printf ("\rPackage count: %d Installed, %d Total Available", install_count, total_count);
  fflush (stdout);

  opkg_package_free (pkg);
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

  opkg_list_packages (opkg, package_list_callback, NULL);
  printf ("\n");

  opkg_free (opkg);
}
