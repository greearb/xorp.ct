bindir = $(prefix)/$(subdir)

bin_PROGRAMS			 = show_stats
bin_PROGRAMS			+= show_peer_stats

noinst_PROGRAMS			 = rip_announcer ripng_announcer

tool_scripts			 = addr_status
tool_scripts			+= add_static
tool_scripts			+= delete_static
noinst_SCRIPTS			 = $(tool_scripts)

# -- Program Sources
show_stats_SOURCES		 = show_stats.cc
show_stats_LDADD		 = $(top_builddir)/xrl/interfaces/libripxif.a
show_stats_LDADD		+= $(top_builddir)/xrl/interfaces/libripngxif.a
show_stats_LDADD		+= $(LDADD)

show_peer_stats_SOURCES	 	 = show_peer_stats.cc
show_peer_stats_LDADD	 	 = $(top_builddir)/xrl/interfaces/libripxif.a
show_peer_stats_LDADD		+= $(top_builddir)/xrl/interfaces/libripngxif.a
show_peer_stats_LDADD		+= $(LDADD)

rip_announcer_SOURCES		 = rip_announcer.cc
rip_announcer_LDADD		 = $(top_builddir)/rip/librip.a
rip_announcer_LDADD		+= $(top_builddir)/libcomm/libcomm.a
rip_announcer_LDADD		+= $(top_builddir)/libxorp/libxorp.a

ripng_announcer_SOURCES		 = ripng_announcer.cc
ripng_announcer_LDADD		 = $(top_builddir)/rip/libripng.a
ripng_announcer_LDADD		+= $(top_builddir)/libcomm/libcomm.a
ripng_announcer_LDADD		+= $(top_builddir)/libxorp/libxorp.a

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


