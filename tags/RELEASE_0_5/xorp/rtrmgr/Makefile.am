## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/rtrmgr/Makefile.am,v 1.18 2003/09/24 16:16:06 hodson Exp $
##

# -- Subdirectories
SUBDIRS = . tools

# -- Real Programs
bin_PROGRAMS	 = xorp_rtrmgr xorpsh

# -- Test Programs
TESTS		 = test_templates
TESTS		+= test_sample_config
TESTS		+= test_module_manager

# -- No-install Programs
noinst_PROGRAMS	 = test_templates
noinst_PROGRAMS	+= test_sample_config
noinst_PROGRAMS	+= test_module_manager

# -- No-install Libraries
noinst_LIBRARIES =

# -- Libraries and library flags to link the binary programs
LDADD		 = $(top_builddir)/libxipc/libfinder.a
LDADD		+= $(top_builddir)/libxipc/libxipc.a
LDADD		+= $(top_builddir)/libcomm/libcomm.a
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
xorp_rtrmgr_SOURCES	 = rtrmgr_module.h
xorp_rtrmgr_SOURCES	+= version.h
xorp_rtrmgr_SOURCES	+= util.hh
xorp_rtrmgr_SOURCES	+= y.tplt_tab.cc
xorp_rtrmgr_SOURCES	+= lex.tplt.cc
xorp_rtrmgr_SOURCES	+= y.boot_tab.cc
xorp_rtrmgr_SOURCES	+= lex.boot.cc
xorp_rtrmgr_SOURCES	+= main_rtrmgr.cc
xorp_rtrmgr_SOURCES	+= template_tree.cc
xorp_rtrmgr_SOURCES	+= template_tree_node.cc
xorp_rtrmgr_SOURCES	+= template_commands.cc
xorp_rtrmgr_SOURCES	+= module_command.cc
xorp_rtrmgr_SOURCES	+= conf_tree_node.cc
xorp_rtrmgr_SOURCES	+= conf_tree.cc
xorp_rtrmgr_SOURCES	+= master_conf_tree_node.cc
xorp_rtrmgr_SOURCES	+= master_conf_tree.cc
xorp_rtrmgr_SOURCES	+= module_manager.cc
xorp_rtrmgr_SOURCES	+= xorp_client.cc
xorp_rtrmgr_SOURCES	+= xrldb.cc
xorp_rtrmgr_SOURCES	+= userdb.cc
xorp_rtrmgr_SOURCES	+= util.cc
xorp_rtrmgr_SOURCES	+= xrl_rtrmgr_interface.cc
xorp_rtrmgr_SOURCES	+= randomness.cc
xorp_rtrmgr_SOURCES	+= unexpanded_xrl.cc
xorp_rtrmgr_SOURCES	+= task.cc

xorp_rtrmgr_LDADD	= $(top_builddir)/xrl/targets/librtrmgrbase.a
xorp_rtrmgr_LDADD	+= $(top_builddir)/xrl/interfaces/librtrmgr_clientxif.a
xorp_rtrmgr_LDADD	+= $(LDADD)

xorpsh_SOURCES	 = rtrmgr_module.h
xorpsh_SOURCES	+= version.h
xorpsh_SOURCES	+= util.hh
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
xorpsh_SOURCES	+= module_command.cc
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
#task.cc shouldn't really be needed
xorpsh_SOURCES  += task.cc
#unexpanded_xrl shouldn't really be needed
xorpsh_SOURCES	+= unexpanded_xrl.cc
xorpsh_SOURCES	+= util.cc

xorpsh_SOURCES	+= xrl_xorpsh_interface.cc

xorpsh_LDADD	= $(top_builddir)/cli/libcli.a
xorpsh_LDADD	+= $(top_builddir)/libproto/libproto.a
xorpsh_LDADD	+= $(top_builddir)/cli/libtecla/libtecla.a
xorpsh_LDADD	+= @CLI_CURSES_LIB@
xorpsh_LDADD	+= $(top_builddir)/xrl/interfaces/librtrmgrxif.a
xorpsh_LDADD	+= $(top_builddir)/xrl/targets/libxorpshbase.a
xorpsh_LDADD	+= $(LDADD)

test_templates_SOURCES	 = test_templates.cc
test_templates_SOURCES	+= y.tplt_tab.cc
test_templates_SOURCES	+= lex.tplt.cc
test_templates_SOURCES	+= template_tree.cc
test_templates_SOURCES	+= template_tree_node.cc
test_templates_SOURCES	+= template_commands.cc
test_templates_SOURCES	+= module_command.cc
test_templates_SOURCES	+= xrldb.cc
test_templates_SOURCES	+= unexpanded_xrl.cc
test_templates_SOURCES	+= conf_tree_node.cc
test_templates_SOURCES	+= command_tree.cc
test_templates_SOURCES	+= xorp_client.cc
test_templates_SOURCES	+= module_manager.cc
test_templates_SOURCES	+= util.cc
test_templates_SOURCES	+= task.cc
test_templates_LDADD 	 = $(LDADD)

test_sample_config_SOURCES	 = test_sample_config.cc
test_sample_config_SOURCES	+= y.tplt_tab.cc
test_sample_config_SOURCES	+= lex.tplt.cc
test_sample_config_SOURCES	+= y.boot_tab.cc
test_sample_config_SOURCES	+= lex.boot.cc
test_sample_config_SOURCES	+= template_tree.cc
test_sample_config_SOURCES	+= template_tree_node.cc
test_sample_config_SOURCES	+= template_commands.cc
test_sample_config_SOURCES	+= module_command.cc
test_sample_config_SOURCES	+= xrldb.cc
test_sample_config_SOURCES	+= unexpanded_xrl.cc
test_sample_config_SOURCES	+= conf_tree_node.cc
test_sample_config_SOURCES	+= conf_tree.cc
test_sample_config_SOURCES	+= master_conf_tree_node.cc
test_sample_config_SOURCES	+= master_conf_tree.cc
test_sample_config_SOURCES	+= command_tree.cc
test_sample_config_SOURCES	+= xorp_client.cc
test_sample_config_SOURCES	+= module_manager.cc
test_sample_config_SOURCES	+= util.cc
test_sample_config_SOURCES	+= task.cc
test_sample_config_LDADD 	 = $(LDADD)

test_module_manager_SOURCES	 = test_module_manager.cc
test_module_manager_SOURCES	+= y.tplt_tab.cc
test_module_manager_SOURCES	+= lex.tplt.cc
test_module_manager_SOURCES	+= template_tree.cc
test_module_manager_SOURCES	+= template_tree_node.cc
test_module_manager_SOURCES	+= template_commands.cc
test_module_manager_SOURCES	+= module_command.cc
test_module_manager_SOURCES	+= xrldb.cc
test_module_manager_SOURCES	+= unexpanded_xrl.cc
test_module_manager_SOURCES	+= conf_tree_node.cc
#test_module_manager_SOURCES	+= conf_tree.cc
test_module_manager_SOURCES	+= xorp_client.cc
test_module_manager_SOURCES	+= module_manager.cc
test_module_manager_SOURCES	+= util.cc
test_module_manager_SOURCES	+= task.cc
test_module_manager_LDADD 	 = $(LDADD)

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

INCLUDES =	-I$(top_srcdir)

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

.yacc_generated_files: $(srcdir)/template.yy $(srcdir)/boot.yy $(srcdir)/op_commands.yy
	( cd $(srcdir); \
	yacc -d -p tplt -o y.tplt_tab.c template.yy; \
	mv y.tplt_tab.c y.tplt_tab.cc; \
	yacc -d -p boot -o y.boot_tab.c boot.yy; \
	mv y.boot_tab.c y.boot_tab.cc; \
	yacc -d -p opcmd -o y.opcmd_tab.c op_commands.yy; \
	mv y.opcmd_tab.c y.opcmd_tab.cc; \
	touch .yacc_generated_files )

$(srcdir)/lex.tplt.cc: $(srcdir)/template.ll $(srcdir)/y.tplt_tab.cc
	( cd $(srcdir); \
	flex -Ptplt -olex.tplt.cc template.ll )

$(srcdir)/lex.boot.cc: $(srcdir)/boot.ll $(srcdir)/y.boot_tab.cc
	( cd $(srcdir); \
	flex -Pboot -olex.boot.cc boot.ll )

$(srcdir)/lex.opcmd.cc: $(srcdir)/op_commands.ll $(srcdir)/y.opcmd_tab.cc
	( cd $(srcdir); \
	flex -Popcmd -olex.opcmd.cc op_commands.ll )
