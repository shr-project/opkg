#!/usr/bin/python

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Architecture="all", Depends="b", )
o.add(Package="b", Version="1.0", Architecture="all")
o.add(Package="c", Version="1.0", Architecture="all", Depends="b")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.install("a")
opkgcl.install("c")

opkgcl.flag_unpacked("a")

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Architecture="all", Depends="b", )
o.add(Package="b", Version="1.0", Architecture="all")
o.add(Package="c", Version="2.0", Architecture="all")
o.write_opk()
o.write_list()

opkgcl.update()
opkgcl.upgrade("--autoremove")

if not opkgcl.is_installed("b", "1.0"):
	print("b has been removed even though a still depends on it")
	exit(False)
