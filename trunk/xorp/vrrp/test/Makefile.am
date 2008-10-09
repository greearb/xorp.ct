## Process this file with automake to produce Makefile.in.
##
## $XORP$
##

# -- Test Programs
TESTS            = vrrp_test

# -- No-install Programs
noinst_PROGRAMS  = vrrp_test

vrrp_test_SOURCES          = vrrp_test.cc

vrrp_test_LDADD            = $(top_builddir)/vrrp/libvrrp.la
vrrp_test_LDADD           += $(top_builddir)/xrl/targets/libvrrpbase.la
vrrp_test_LDADD           += $(top_builddir)/libfeaclient/libfeaclient.la
vrrp_test_LDADD           += $(top_builddir)/xrl/interfaces/libfeaifmgrxif.la
vrrp_test_LDADD           += $(top_builddir)/xrl/interfaces/libfeaifmgrmirrorxif.la
vrrp_test_LDADD           += $(top_builddir)/xrl/interfaces/libfeaifmgrreplicatorxif.la
vrrp_test_LDADD           += $(top_builddir)/xrl/interfaces/libfearawlinkxif.la
vrrp_test_LDADD           += $(top_builddir)/xrl/interfaces/libfearawpkt4xif.la
vrrp_test_LDADD           += $(top_builddir)/xrl/targets/libfeaifmgrmirrorbase.la
vrrp_test_LDADD           += $(top_builddir)/libxipc/libxipc.la
vrrp_test_LDADD           += $(top_builddir)/libproto/libproto.la
vrrp_test_LDADD           += $(top_builddir)/libcomm/libcomm.la
vrrp_test_LDADD           += $(top_builddir)/libxorp/libxorp.la

################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##

INCLUDES =      -I$(top_srcdir)
