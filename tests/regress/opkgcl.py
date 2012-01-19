#!/usr/bin/python3

import os, subprocess
import cfg

def opkgcl(opkg_args):
	cmd = "{} -o {} {}".format(cfg.opkgcl, cfg.offline_root, opkg_args)
	#print(cmd)
	return subprocess.getstatusoutput(cmd)

def install(pkg_name, flags=""):
	return opkgcl("{} install {}".format(flags, pkg_name))[0]

def remove(pkg_name, flags=""):
	return opkgcl("{} remove {}".format(flags, pkg_name))[0]

def update():
	return opkgcl("update")[0]

def upgrade(params=None):
	if params:
		opkgcl("upgrade {}".format(params))[0]
	else:
		return opkgcl("upgrade")[0]

def files(pkg_name):
	output = opkgcl("files {}".format(pkg_name))[1]
	return output.split("\n")[1:]


def flag_unpacked(pkg_name):
	out = opkgcl("flag unpacked {}".format(pkg_name))
	return out == "Setting flags for package {} to unpacked.".format(pkg_name)

def is_installed(pkg_name, version=None):
	out = opkgcl("list_installed {}".format(pkg_name))[1]
	if len(out) == 0 or out.split()[0] != pkg_name:
		return False
	if version and out.split()[2] != version:
		return False
	if not os.path.exists("{}/usr/lib/opkg/info/{}.control"\
				.format(cfg.offline_root, pkg_name)):
		return False
	return True

def is_autoinstalled(pkg_name):
    status_path = "{}/usr/lib/opkg/status".format(cfg.offline_root)
    if not os.path.exists(status_path):
        return False
    status_file = open(status_path, "r")
    status = status_file.read()
    status_file.close()
    index_start = status.find("Package: {}".format(pkg_name))
    if index_start < 0:
        return False
    index_end = status.find("\n\n", index_start)
    return status.find("Auto-Installed: yes", index_start, index_end) >= 0


if __name__ == '__main__':
	import sys
	(status, output) = opkgcl(" ".join(sys.argv[1:]))
	print(output)
	exit(status)
