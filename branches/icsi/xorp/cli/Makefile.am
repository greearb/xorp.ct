## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/cli/Makefile.am,v 1.32 2002/12/11 19:07:42 pavlin Exp $
##

# -- Subdirectories (directory ordering is important)
SUBDIRS = libtecla .

# -- Real Programs
bin_PROGRAMS  = 

# -- Test Programs
bin_PROGRAMS += test_cli

noinst_LIBRARIES = libcli.a

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
### CLI libraries
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= -lcurses
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.a
LDADD		+= $(top_builddir)/xrl/interfaces/libcommonxif.a
LDADD		+= $(top_builddir)/xrl/targets/libclibase.a
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_cli_SOURCES = test_cli.cc

# -- Library Sources
libcli_a_SOURCES  = cli_module.h
libcli_a_SOURCES += cli_client.hh
libcli_a_SOURCES += cli_command.hh
libcli_a_SOURCES += cli_command_pipe.hh
libcli_a_SOURCES += cli_node.hh
libcli_a_SOURCES += cli_private.hh
libcli_a_SOURCES += xrl_cli_node.hh

libcli_a_SOURCES += cli_client.cc
libcli_a_SOURCES += cli_command.cc
libcli_a_SOURCES += cli_command_pipe.cc
libcli_a_SOURCES += cli_node.cc
libcli_a_SOURCES += cli_node_internal_commands.cc
libcli_a_SOURCES += cli_node_net.cc
libcli_a_SOURCES += xrl_cli_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = libtecla


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
