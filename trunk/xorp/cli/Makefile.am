## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/cli/Makefile.am,v 1.12 2009/01/21 16:05:36 jtc Exp $
##

# -- Subdirectories (directory ordering is important)
SUBDIRS = libtecla . tools

# -- Test Programs
TESTS		 =

# -- No-install Programs
check_PROGRAMS	 = test_cli

# -- Libraries
lib_LTLIBRARIES = libcli.la

# -- Libraries and library flags to link the binary programs
LDADD		 = $(lib_LTLIBRARIES)
### XRL libraries
LDADD		+= $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
LDADD		+= $(top_builddir)/xrl/targets/libclibase.la
### CLI libraries
LDADD		+= $(top_builddir)/cli/libtecla/libtecla.la
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

libcli_la_LIBADD  = $(top_builddir)/xrl/interfaces/libcliprocessorxif.la
libcli_la_LIBADD += $(top_builddir)/xrl/targets/libclibase.la
libcli_la_LIBADD += $(top_builddir)/cli/libtecla/libtecla.la
libcli_la_LIBADD += @CLI_CURSES_LIB@
libcli_la_LIBADD += $(top_builddir)/libproto/libproto.la
libcli_la_LIBADD += $(top_builddir)/libxipc/libxipc.la
libcli_la_LIBADD += $(top_builddir)/libcomm/libcomm.la
libcli_la_LIBADD += $(top_builddir)/libxorp/libxorp.la

# -- Extra files and directories to add to the distribution
EXTRA_DIST	 = libtecla
EXTRA_DIST	+= xrl_cli_shell_funcs.sh


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =	-I$(top_srcdir)
