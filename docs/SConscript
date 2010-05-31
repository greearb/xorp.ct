# Copyright (c) 2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
#
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $ID$

Import('env')

subdirs = [
	'bgp',			# XORP BGP Routing Daemon
	'design_arch',		# XORP Design Overview
	'fea',			# FEA
	#'kdoc',		# notyet
	'libxipc',		# Inter-Process Communication Library
	'libxorp',		# LIBXORP Library Overview
	#'man',			# source build not yet implemented
	'mfea',			# Multicast FEA
	'mld6igmp',		# MLD/IGMP
	'multicast',		# Multicast Architecture
	#'olsr',		# don't build by default
	#'papers',		# old content
	'pim',			# PIM-SM
	'pim_testsuite',	# PIM-SM Test Suite
	#'policy',		# source build not yet implemented
	'rib',			# RIB
	#'rip',			# broken, old content
	'rtrmgr',		# Router Manager
	#'slides',		# old content
	#'snmp',		# old content
	'test_harness',		# this is actually BGP's test harness.
	'user_manual',		# XORP User Manual
	#'windows_port',	# don't build by default
	'xorpdev_101',		# Introduction to Writing a XORP Process
	]

SConscript(dirs = subdirs, exports='env')
