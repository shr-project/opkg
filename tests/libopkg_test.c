#include <opkg.h>


int
main (int argc, char **argv)
{
  opkg_t *opkg;
  
  opkg = opkg_new ();

  opkg_set_option (opkg, "offline_root", "/tmp/");


  opkg_free (opkg);
}
