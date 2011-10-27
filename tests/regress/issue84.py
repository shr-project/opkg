#!/usr/bin/python3

import opk, cfg, opkgcl

def cleanup():
	opkgcl.remove("a1")
	opkgcl.remove("b1")
	opkgcl.remove("a")
	opkgcl.remove("b")
	opkgcl.remove('c')

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Provides="v", Depends="a1")
o.add(Package="b", Provides="v", Depends="b1")
o.add(Package="c", Depends="v")
o.add(Package="a1")
o.add(Package="b1")

o.write_opk()
o.write_list()

opkgcl.update()

# install ``a1`` directly
opkgcl.install("a1_1.0_all.opk")
if not opkgcl.is_installed("a1"):
	print(__file__, ": package ``a1'' not installed.")
	cleanup()
	exit(False)

# install ``c'' from repository
opkgcl.install("c")
if not opkgcl.is_installed("c"):
	print(__file__, ": package ``c'' not installed.")
	cleanup()
	exit(False)

if opkgcl.is_installed("b1"):
	print(__file__, ": package ``b1'' is installed, but should not be.")
	cleanup()
	exit(False)

cleanup()
