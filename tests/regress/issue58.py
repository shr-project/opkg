#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Recommends="b")
o.add(Package="b")
o.add(Package="c", Recommends="b")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
opkgcl.install("c")

opkgcl.remove("a", "--autoremove")
if not opkgcl.is_installed("b"):
	print(__file__, ": Pacakge 'b' orphaned despite remaining "
			"recommending package 'c'.")
	exit(False)

opkgcl.remove("c", "--autoremove")
if opkgcl.is_installed("b"):
	print(__file__, ": Recommended package 'b' not autoremoved.")
	exit(False)


