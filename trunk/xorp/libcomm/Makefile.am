## Process this file with automake to produce Makefile.in.
##
## $XORP: xorp/libcomm/Makefile.am,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $
##

# -- Real Programs
bin_PROGRAMS  = 

# -- Test Programs
bin_PROGRAMS += test_comm

noinst_LIBRARIES = libcomm.a

# -- Libraries and library flags to link the binary programs
LDADD		 = $(noinst_LIBRARIES)
LDADD		+= $(top_builddir)/libxorp/libxorp.a

# -- Program Sources
test_comm_SOURCES = test_comm.c

# -- Library Sources
libcomm_a_SOURCES  = comm_module.h
libcomm_a_SOURCES += comm_api.h
libcomm_a_SOURCES += comm_private.h

libcomm_a_SOURCES += comm_sock.c
libcomm_a_SOURCES += comm_user.c


################################################################
##
## Everything past here is useful to the maintainer, but probably not
## to anybody else
##
