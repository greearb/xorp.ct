## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/Makefile.am,v 1.5 2003/03/11 01:02:59 mjh Exp $
##

# -- Subdirectories
SUBDIRS = . tools

# -- Real Programs
bin_PROGRAMS	 = rtrmgr xorpsh
#bin_PROGRAMS	 = rtrmgr

# -- Test Programs
#bin_PROGRAMS	+=

TESTS		 = test_templates

noinst_PROGRAMS = $(TESTS)

EXTRA_DIST	 = config.template config.boot

# -- Libraries and library flags to link the binary programs
LDFLAGS		 = -all-static
LDADD		 = $(top_builddir)/libxipc/libfinder.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
rtrmgr_SOURCES	 = rtrmgr_module.h
rtrmgr_SOURCES	+= md5.h
rtrmgr_SOURCES	+= version.h
rtrmgr_SOURCES	+= y.tplt_tab.cc
rtrmgr_SOURCES	+= lex.tplt.cc
rtrmgr_SOURCES	+= y.boot_tab.cc
rtrmgr_SOURCES	+= lex.boot.cc
rtrmgr_SOURCES	+= main_rtrmgr.cc
rtrmgr_SOURCES	+= template_tree.cc
rtrmgr_SOURCES	+= template_tree_node.cc
rtrmgr_SOURCES	+= template_commands.cc
rtrmgr_SOURCES	+= conf_tree_node.cc
rtrmgr_SOURCES	+= conf_tree.cc
rtrmgr_SOURCES	+= master_conf_tree_node.cc
rtrmgr_SOURCES	+= master_conf_tree.cc
rtrmgr_SOURCES	+= module_manager.cc
rtrmgr_SOURCES	+= xorp_client.cc
rtrmgr_SOURCES	+= xrldb.cc
rtrmgr_SOURCES	+= split.cc
rtrmgr_SOURCES	+= userdb.cc
rtrmgr_SOURCES	+= xrl_rtrmgr_interface.cc
rtrmgr_SOURCES	+= randomness.cc
rtrmgr_SOURCES	+= unexpanded_xrl.cc

rtrmgr_LDADD	= $(top_builddir)/xrl/targets/librtrmgrbase.a 
rtrmgr_LDADD	+= $(top_builddir)/xrl/interfaces/librtrmgr_clientxif.a 
rtrmgr_LDADD	+= $(LDADD) 

xorpsh_SOURCES	 = rtrmgr_module.h
xorpsh_SOURCES	+= version.h
xorpsh_SOURCES	+= xorpsh_main.cc
xorpsh_SOURCES	+= y.tplt_tab.cc
xorpsh_SOURCES	+= lex.tplt.cc
xorpsh_SOURCES	+= y.boot_tab.cc
xorpsh_SOURCES	+= lex.boot.cc
xorpsh_SOURCES	+= y.opcmd_tab.cc
xorpsh_SOURCES	+= lex.opcmd.cc
xorpsh_SOURCES	+= template_tree.cc
xorpsh_SOURCES	+= template_tree_node.cc
xorpsh_SOURCES	+= template_commands.cc
xorpsh_SOURCES	+= conf_tree_node.cc
xorpsh_SOURCES	+= slave_conf_tree_node.cc
xorpsh_SOURCES	+= conf_tree.cc
#slave_module_manager really shouldn't be needed
xorpsh_SOURCES	+= slave_module_manager.cc 
xorpsh_SOURCES	+= slave_conf_tree.cc
xorpsh_SOURCES	+= xorp_client.cc
xorpsh_SOURCES	+= xrldb.cc
xorpsh_SOURCES	+= cli.cc
xorpsh_SOURCES	+= command_tree.cc
xorpsh_SOURCES	+= op_commands.cc
xorpsh_SOURCES	+= popen.cc
xorpsh_SOURCES	+= split.cc
xorpsh_SOURCES	+= xrl_xorpsh_interface.cc
#unexpanded_xrl shouldn't really be needed
xorpsh_SOURCES	+= unexpanded_xrl.cc

xorpsh_LDADD	= $(top_builddir)/cli/libcli.a
xorpsh_LDADD	+= $(top_builddir)/libproto/libproto.a
xorpsh_LDADD	+= $(top_builddir)/cli/libtecla/libtecla.a
xorpsh_LDADD	+= -lcurses
xorpsh_LDADD	+= $(top_builddir)/xrl/interfaces/librtrmgrxif.a
xorpsh_LDADD	+= $(top_builddir)/xrl/targets/libxorpshbase.a
xorpsh_LDADD	+= $(LDADD)

test_templates_SOURCES	 = test_templates.cc
test_templates_SOURCES	+= y.tplt_tab.cc
test_templates_SOURCES	+= lex.tplt.cc
test_templates_SOURCES	+= template_tree.cc
test_templates_SOURCES	+= template_tree_node.cc
test_templates_SOURCES	+= template_commands.cc
test_templates_SOURCES	+= xrldb.cc
test_templates_SOURCES	+= unexpanded_xrl.cc
test_templates_SOURCES	+= conf_tree_node.cc
test_templates_SOURCES	+= command_tree.cc
test_templates_SOURCES	+= xorp_client.cc
test_templates_SOURCES	+= module_manager.cc
test_templates_SOURCES	+= split.cc
test_templates_LDADD 	 = $(LDADD)

# -- Library Sources

# -- Extra files and directories to add to the distribution
EXTRA_DIST	  = Notes
EXTRA_DIST	 += config.boot
EXTRA_DIST	 += config.template
EXTRA_DIST	 += fea.boot


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

################################################################ 
## Yes, this isn't portable, but the portable way in the automake
## documentation doesn't work for multiple scanners in one binary.
## Automake sucks.
##
## Note use of mv below.  Yacc also sucks, if the output filename
## does not end .c, it makes the generated header file the output
## filename plus .h.
##
## Due to portability issues with yacc, we don't build the yacc
## generated files automatically.  If you touch the .yy files, you
## need to explicitly "gmake yacc" to run yacc.

yacc:	.yacc_generated_files

.yacc_generated_files: template.yy boot.yy op_commands.yy
	yacc -d -p tplt -o y.tplt_tab.c template.yy
	mv y.tplt_tab.c y.tplt_tab.cc
	yacc -d -p boot -o y.boot_tab.c boot.yy
	mv y.boot_tab.c y.boot_tab.cc
	yacc -d -p opcmd -o y.opcmd_tab.c op_commands.yy
	mv y.opcmd_tab.c y.opcmd_tab.cc
	touch .yacc_generated_files

lex.tplt.cc:     template.ll y.tplt_tab.cc
	flex -Ptplt -olex.tplt.cc template.ll
 
lex.boot.cc:     boot.ll y.boot_tab.cc
	flex -Pboot -olex.boot.cc boot.ll 

lex.opcmd.cc:     op_commands.ll y.opcmd_tab.cc
	flex -Popcmd -olex.opcmd.cc op_commands.ll 
