#include <opkg.h>
#include <stdlib.h>
#include <stdio.h>

opkg_package_t *find_pkg = NULL;

char *errors[10] = {
  "No Error",
  "Unknown Eror",
  "Download failed",
  "Dependancies failed",
  "Package already installed",
  "Package not available",
  "Package not found",
  "Package not installed",
  "Signature check failed",
  "MD5 sum failed"
};


#define TEST_PACKAGE "aspell"

void
progress_callback (opkg_t *opkg, const opkg_progress_data_t *progress, void *data)
{
  printf ("\r%s %3d%%", (char*) data, progress->percentage);
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

  if (!find_pkg)
  {
    /* store the first package to print out later */
    find_pkg = pkg;
  }
  else
    opkg_package_free (pkg);
}

void
package_list_upgradable_callback (opkg_t *opkg, opkg_package_t *pkg, void *data)
{
  printf ("%s - %s\n", pkg->name, pkg->version);
  opkg_package_free (pkg);
}

void
print_package (opkg_package_t *pkg)
{
  printf (
      "Name:         %s\n"
      "Version:      %s\n"
      "Repository:   %s\n"
      "Architecture: %s\n"
      "Description:  %s\n"
      "Tags:         %s\n"
      "URL:          %s\n"
      "Size:         %d\n"
      "Installed:    %s\n",
      pkg->name,
      pkg->version,
      pkg->repository,
      pkg->architecture,
      pkg->description,
      pkg->tags,
      pkg->url,
      pkg->size,
      (pkg->installed ? "True" : "False")
      );
}

int
main (int argc, char **argv)
{
  opkg_t *opkg;
  opkg_package_t *pkg;
  int err;
  
  opkg = opkg_new ();

  opkg_set_option (opkg, "offline_root", "/tmp/");

  opkg_re_read_config_files (opkg);

  err = opkg_update_package_lists (opkg, progress_callback, "Updating...");
  printf ("\nopkg_update_package_lists returned %d (%s)\n", err, errors[err]);

  opkg_list_packages (opkg, package_list_callback, NULL);
  printf ("\n");

  if (find_pkg)
  {
    printf ("Finding package \"%s\"\n", find_pkg->name);
    pkg = opkg_find_package (opkg, find_pkg->name, find_pkg->version, find_pkg->architecture, find_pkg->repository);
    if (pkg)
    {
      print_package (pkg);
      opkg_package_free (pkg);
    }
    else
      printf ("Package \"%s\" not found!\n", find_pkg->name);
    opkg_package_free (find_pkg);
  }
  else
    printf ("No package available to test find_package.\n");

  err = opkg_install_package (opkg, TEST_PACKAGE, progress_callback, "Installing...");
  printf ("\nopkg_install_package returned %d (%s)\n", err, errors[err]);

  err = opkg_upgrade_package (opkg, TEST_PACKAGE, progress_callback, "Upgrading...");
  printf ("\nopkg_upgrade_package returned %d (%s)\n", err, errors[err]);

  err = opkg_remove_package (opkg, TEST_PACKAGE, progress_callback, "Removing...");
  printf ("\nopkg_remove_package returned %d (%s)\n", err, errors[err]);

  printf ("Listing upgradable packages...\n");
  opkg_list_upgradable_packages (opkg, package_list_upgradable_callback, NULL);

  err = opkg_upgrade_all (opkg, progress_callback, "Upgrading all...");
  printf ("\nopkg_upgrade_all returned %d (%s)\n", err, errors[err]);

  opkg_free (opkg);

  return 0;
}
