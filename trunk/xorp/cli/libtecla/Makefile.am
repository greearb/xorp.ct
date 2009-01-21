## Process this file with automake to produce Makefile.in
##
## $XORP: $
##

noinst_LTLIBRARIES = libtecla.la

libtecla_la_SOURCES  = cplfile.c 
libtecla_la_SOURCES += cplfile.h 
libtecla_la_SOURCES += cplmatch.c 
libtecla_la_SOURCES += direader.c 
libtecla_la_SOURCES += direader.h 
libtecla_la_SOURCES += expand.c
libtecla_la_SOURCES += freelist.c
libtecla_la_SOURCES += freelist.h
libtecla_la_SOURCES += getline.c
libtecla_la_SOURCES += getline.h
libtecla_la_SOURCES += hash.c
libtecla_la_SOURCES += hash.h
libtecla_la_SOURCES += history.c
libtecla_la_SOURCES += history.h
libtecla_la_SOURCES += homedir.c
libtecla_la_SOURCES += homedir.h
libtecla_la_SOURCES += keytab.c
libtecla_la_SOURCES += keytab.h
libtecla_la_SOURCES += libtecla.h
libtecla_la_SOURCES += pathutil.c
libtecla_la_SOURCES += pathutil.h
libtecla_la_SOURCES += pcache.c
libtecla_la_SOURCES += stringrp.c
libtecla_la_SOURCES += stringrp.h
libtecla_la_SOURCES += strngmem.c
libtecla_la_SOURCES += strngmem.h
libtecla_la_SOURCES += version.c

# Support for reentrant version of library
#noinst_LTLIBRARIES += libtecla_r.la
#libtecla_r_la_CPPFLAGS = -D_POSIX_C_SOURCE=199506L
#libtecla_r_la_SOURCES  = cplfile.c 
#libtecla_r_la_SOURCES += cplfile.h 
#libtecla_r_la_SOURCES += cplmatch.c 
#libtecla_r_la_SOURCES += direader.c 
#libtecla_r_la_SOURCES += direader.h 
#libtecla_r_la_SOURCES += expand.c
#libtecla_r_la_SOURCES += freelist.c
#libtecla_r_la_SOURCES += freelist.h
#libtecla_r_la_SOURCES += getline.c
#libtecla_r_la_SOURCES += getline.h
#libtecla_r_la_SOURCES += hash.c
#libtecla_r_la_SOURCES += hash.h
#libtecla_r_la_SOURCES += history.c
#libtecla_r_la_SOURCES += history.h
#libtecla_r_la_SOURCES += homedir.c
#libtecla_r_la_SOURCES += homedir.h
#libtecla_r_la_SOURCES += keytab.c
#libtecla_r_la_SOURCES += keytab.h
#libtecla_r_la_SOURCES += libtecla.h
#libtecla_r_la_SOURCES += pathutil.c
#libtecla_r_la_SOURCES += pathutil.h
#libtecla_r_la_SOURCES += pcache.c
#libtecla_r_la_SOURCES += stringrp.c
#libtecla_r_la_SOURCES += stringrp.h
#libtecla_r_la_SOURCES += strngmem.c
#libtecla_r_la_SOURCES += strngmem.h
#libtecla_r_la_SOURCES += version.c

noinst_PROGRAMS = demo
demo_SOURCES = demo.c
demo_LDADD = libtecla.la

noinst_PROGRAMS += demo2
demo2_SOURCES = demo2.c
demo2_LDADD = libtecla.la

noinst_PROGRAMS += enhance
enhance_SOURCES = enhance.c
enhance_LDADD = libtecla.la

#noinst_programs += nettest_libtecla
#nettest_libtecla_SOURCES = nettest_libtecla.c
#nettest_libtecla_CPPFLAGS = -I$(srcdir)/..
#nettest_libtecla_LDADD = libtecla.la

EXTRA_DIST = html man3

ACLOCAL_AMFLAGS = -I ../../config
