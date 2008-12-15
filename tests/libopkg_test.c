#include <opkg.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{
  opkg_t *opkg;
  int err;
  
  opkg = opkg_new ();

  opkg_set_option (opkg, "offline_root", "/tmp/");

  opkg_read_config_files (opkg);

  err = opkg_update_package_lists (opkg);

  printf ("opkg_update_package_lists returned %d\n", err);

  opkg_free (opkg);
}
