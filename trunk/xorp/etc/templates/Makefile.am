## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/etc/templates/Makefile.am,v 1.5 2004/05/04 00:31:25 pavlin Exp $
##

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA	  = bgp.cmds bgp.tp
data_DATA	 += fea.cmds fea.tp
data_DATA	 += fib2mrib.tp
data_DATA	 += igmp.cmds igmp.tp
data_DATA	 += mfea.cmds mfea4.tp
data_DATA	 += mfea6.cmds mfea6.tp
data_DATA	 += mld.cmds mld.tp
data_DATA	 += ospf.tp
data_DATA	 += pim.cmds pimsm4.tp
data_DATA	 += pim6.cmds pimsm6.tp
data_DATA	 += rib.tp
data_DATA	 += rip.tp
data_DATA	 += ripng.tp
data_DATA	 += snmp.tp
data_DATA	 += static_routes.tp

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = $(data_DATA)


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

