## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

bindir = $(prefix)/$(subdir)

bin_PROGRAMS			 = show_routes
noinst_PROGRAMS			 = show_routes

# -- Program Sources
show_routes_SOURCES		 = show_routes.cc
show_routes_LDADD	 	 = $(top_builddir)/xrl/interfaces/libfindereventnotifierxif.a
show_routes_LDADD		+= $(top_builddir)/xrl/interfaces/libribxif.a
show_routes_LDADD		+= $(top_builddir)/xrl/targets/libshowroutesbase.a
show_routes_LDADD		+= $(LDADD)

LDADD				 = $(top_builddir)/libxipc/libxipc.a
LDADD				+= $(top_builddir)/libcomm/libcomm.a
LDADD				+= $(top_builddir)/libxorp/libxorp.a

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

gen_wrappers 	= $(top_srcdir)/utils/xrl_wrappers_generator.sh
rip_xrls 	= $(top_srcdir)/xrl/targets/rip.xrls
ripng_xrls 	= $(top_srcdir)/xrl/targets/ripng.xrls

$(tool_scripts): 							\
		$(top_builddir)/rip/tools/xrl_rip_shell_funcs.sh	\
		$(top_builddir)/rip/tools/xrl_ripng_shell_funcs.sh

$(top_builddir)/rip/tools/xrl_rip_shell_funcs.sh:			\
		$(rip_xrls) $(gen_wrappers)
	$(gen_wrappers) $(rip_xrls) > $@

$(top_builddir)/rip/tools/xrl_ripng_shell_funcs.sh:			\
		$(ripng_xrls) $(gen_wrappers)
	$(gen_wrappers) $(ripng_xrls) > $@

################################################################

INCLUDES =	-I$(top_srcdir)
