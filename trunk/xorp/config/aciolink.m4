dnl
dnl $XORP: xorp/config/aciolink.m4,v 1.3 2007/06/27 06:22:58 pavlin Exp $
dnl

dnl
dnl Raw link-level I/O access checks.
dnl
dnl The supported mechanisms are:
dnl   - pcap(3)
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([pcap.h])

dnl -----------------------------------------------
dnl Check for libraries
dnl -----------------------------------------------
AC_SEARCH_LIBS(pcap_open_live, pcap)

dnl -----------------------------------------------
dnl Check for functions
dnl -----------------------------------------------
AC_CHECK_FUNCS([pcap_sendpacket])

dnl -----------------------------------------------------------
dnl Check whether the system has pcap(3) that is reasonably new
dnl -----------------------------------------------------------
AC_MSG_CHECKING(whether the system has pcap(3) that is reasonable)
AC_TRY_LINK([
#ifdef HAVE_PCAP_H
#include <pcap.h>
#endif
],
[
    int dummy = 0;
    pcap_t *pcap;
    char errbuf[PCAP_ERRBUF_SIZE];
    u_char databuf[65535];
    int datalink_type;
    struct bpf_program bpf_program;
    int fd;
    const u_char* packet;
    struct pcap_pkthdr pcap_pkthdr;

    /* Test whether various functions are available */
    pcap = pcap_open_live("foo", 0xffff, 0, 1, errbuf);
    datalink_type = pcap_datalink(pcap);
    pcap_setnonblock(pcap, 1, errbuf);
    pcap_breakloop(pcap);
    fd = pcap_get_selectable_fd(pcap);
    pcap_compile(pcap, &bpf_program, "tcpdump program", 1, 0);
    pcap_setfilter(pcap, &bpf_program);
    pcap_freecode(&bpf_program);
#ifdef PCAP_D_INOUT
    pcap_setdirection(pcap, PCAP_D_INOUT);
#endif
    packet = pcap_next(pcap, &pcap_pkthdr);
    pcap_sendpacket(pcap, databuf, 1);

    /* Dummy integer values that must be defined in some of the header files */
    dummy += DLT_EN10MB;
],
    [AC_DEFINE(HAVE_PCAP, 1,
	       [Define to 1 if you have pcap(3) that is reasonable])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

AC_LANG_POP(C)
AC_CACHE_SAVE
