## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/etc/templates/Makefile.am,v 1.2 2004/02/14 00:14:58 pavlin Exp $
##

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA	  = bgp.cmds bgp.tp
data_DATA	 += fea.cmds fea.tp
data_DATA	 += ospf.tp
data_DATA	 += rib.tp
data_DATA	 += rip4.tp
data_DATA	 += rip6.tp
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

