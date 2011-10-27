#!/usr/bin/python3

import opk, cfg, opkgcl

def cleanup():
	opkgcl.remove("a")
	opkgcl.remove("b")
	opkgcl.remove('c')

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Provides="v")
o.add(Package="b", Provides="v", Depends="b_nonexistant")
o.add(Package="c", Depends="v")

o.write_opk()
o.write_list()

opkgcl.update()

# install ``c'' from repository
opkgcl.install("c")
if not opkgcl.is_installed("c"):
	print(__file__, ": package ``c'' not installed.")
	cleanup()
	exit(False)

cleanup()
