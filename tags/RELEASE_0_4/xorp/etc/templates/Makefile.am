## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/pim/Makefile.am,v 1.8 2003/07/30 23:43:30 pavlin Exp $
##

# Redefine the data installation directory
datadir = $(prefix)/$(subdir)

# -- Data
data_DATA	  = bgp.cmds bgp.tp
data_DATA	 += fea.cmds fea.tp
data_DATA	 += ospf.tp
data_DATA	 += rib.tp
data_DATA	 += snmp.tp

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = $(data_DATA)


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)

