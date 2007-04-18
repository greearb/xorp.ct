## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/cli/Makefile.am,v 1.10 2004/11/23 22:43:42 bms Exp $
##

# -- Subdirectories (directory ordering is important)
SUBDIRS = libtecla . tools

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS	 = test_cli

# -- No-install Libraries
noinst_LTLIBRARIES = libcli.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LTLIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### CLI libraries
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.a
LDADD		+= @CLI_CURSES_LIB@
### XORP misc libraries
LDADD		+= $(top_builddir)/libproto/libproto.la
LDADD		+= $(top_builddir)/libxipc/libxipc.la
LDADD		+= $(top_builddir)/libcomm/libcomm.la
LDADD		+= $(top_builddir)/libxorp/libxorp.la

# -- Program Sources
test_cli_SOURCES = test_cli.cc
test_cli_LDADD	 = $(top_builddir)/libxipc/libfinder.la
test_cli_LDADD	+= $(LDADD)


# -- Library Sources
libcli_la_SOURCES  = cli_module.h
libcli_la_SOURCES += cli_client.hh
libcli_la_SOURCES += cli_command.hh
libcli_la_SOURCES += cli_command_pipe.hh
libcli_la_SOURCES += cli_node.hh
libcli_la_SOURCES += cli_private.hh
libcli_la_SOURCES += xrl_cli_node.hh

libcli_la_SOURCES += cli_client.cc
libcli_la_SOURCES += cli_command.cc
libcli_la_SOURCES += cli_command_pipe.cc
libcli_la_SOURCES += cli_node.cc
libcli_la_SOURCES += cli_node_internal_commands.cc
libcli_la_SOURCES += cli_node_net.cc
libcli_la_SOURCES += xrl_cli_node.cc

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = libtecla
EXTRA_DIST	+= xrl_cli_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
