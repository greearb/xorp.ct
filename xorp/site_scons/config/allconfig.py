# Copyright (c) 2009-2012 XORP, Inc and Others
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
#
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $Id$

import sys
import os
import string
from SCons.Script.SConscript import SConsEnvironment
import fnmatch;

# TODO SCons support for headerfilename needs to be fixed at source--
# that would let us use confdefs.h for the include file header
# conditionals, instead of having everything in one file like this.
#
# TODO Some of these checks should be fatal.
# TODO Split this up into separate files.
#  Hint: Some tests depend on others.

def DoAllConfig(env, conf, host_os):
    ##########
    # endian
    has_endian_h = conf.CheckHeader('endian.h');
    if not has_endian_h:
        conf.CheckEndianness()

    # Bleh, FC8 era scons doesn't have this check.
    try:
        if not conf.CheckCC:
            print "\nERROR:  Cannot find functional cc compiler."
            print "  On Fedora/RedHat: yum install gcc"
            sys.exit(1);
        print "OK:  c compiler appears functional.";

        if not conf.CheckCXX:
            print "\nERROR:  Cannot find functional c++ compiler."
            print "  On Fedora/RedHat: yum install gcc-g++"
            sys.exit(1);
        print "OK:  C++ compiler appears functional.";
    except:
        print "NOTE:  This version of scons cannot check for"
        print "  existence of gcc and g++ compilers."
        print "  Will assume the exist and function properly...\n"

    # Check for Flex and Bison
    if not (env.has_key('LEX') and env['LEX']):
        print "\nERROR: Cannot find flex."
        print "  On Ubuntu: sudo apt-get install flex"
        print "  On Fedora/RedHat: yum install flex"
        sys.exit(1);
    print "OK:  flex appears functional."

    if not (env.has_key('YACC') and env['YACC']):
        print "\nERROR: Cannot find bison."
        print "  On Ubuntu: sudo apt-get install bison"
        print "  On Fedora/RedHat: yum install bison"
        sys.exit(1);
    print "OK:  bison appears functional."

    # Mingw/windows stuff
    has_iphlpapi_h = conf.CheckHeader(['winsock2.h', 'iphlpapi.h'])
    has_routprot_h = conf.CheckHeader('routprot.h')

    ##########
    # c99
    has_stdint_h = conf.CheckHeader('stdint.h')
    has_inttypes_h = conf.CheckHeader('inttypes.h')
    for type in [ 'int8_t',  'uint8_t',
                  'int16_t', 'uint16_t',
                  'int32_t', 'uint32_t',
                  'int64_t', 'uint64_t'  ]:
        includes = ""
	if has_inttypes_h:
	    includes += '#include <inttypes.h>\n'
	if has_stdint_h:
	    includes += '#include <stdint.h>\n'
	conf.CheckType(type, includes)
    
    has_stdbool_h = conf.CheckHeader('stdbool.h')

    ##########
    # stdc
    has_stddef_h = conf.CheckHeader('stddef.h')
    has_stdarg_h = conf.CheckHeader('stdarg.h')
    has_stdlib_h = conf.CheckHeader('stdlib.h')
    has_strings_h = conf.CheckHeader('strings.h')
    has_string_h = conf.CheckHeader('string.h')
    has_signal_h = conf.CheckHeader('signal.h')
    has_math_h = conf.CheckHeader('math.h') # SUNWlibm
    has_memory_h = conf.CheckHeader('memory.h')
    
    # c90, libc: functions
    has_strftime = conf.CheckFunc('strftime')
    has_strlcpy = conf.CheckFunc('strlcpy')  # not in glibc!
    has_strlcat = conf.CheckFunc('strlcat')  # not in glibc!
    
    has_va_copy = conf.CheckDeclaration('va_copy', '#include <stdarg.h>')
    if has_va_copy:
        conf.Define('HAVE_VA_COPY') # autoconf compat
    
    ##########
    # posix
    has_sys_types_h = conf.CheckHeader('sys/types.h')

    has_fcntl_h = conf.CheckHeader('fcntl.h')
    has_getopt_h = conf.CheckHeader('getopt.h')
    has_glob_h = conf.CheckHeader('glob.h')
    has_grp_h = conf.CheckHeader('grp.h')
    has_pthread_h = conf.CheckHeader('pthread.h')
    has_pwd_h = conf.CheckHeader('pwd.h')
    has_mqueue_h = conf.CheckHeader('mqueue.h')

    prereq_regex_h = []
    if has_sys_types_h:
    	prereq_regex_h.append('sys/types.h')
    has_regex_h = conf.CheckHeader(prereq_regex_h + ['regex.h'])

    has_syslog_h = conf.CheckHeader('syslog.h')
    has_termios_h = conf.CheckHeader('termios.h')
    has_time_h = conf.CheckHeader('time.h')
    has_unistd_h = conf.CheckHeader('unistd.h')
    has_vfork_h = conf.CheckHeader('vfork.h')
    
    # posix: function tests
    has_readv = conf.CheckFunc('readv')
    has_strerror = conf.CheckFunc('strerror')
    has_syslog = conf.CheckFunc('syslog')
    has_uname = conf.CheckFunc('uname')
    has_writev = conf.CheckFunc('writev')

    # may be in -lxnet on opensolaris
    has_libxnet = conf.CheckLib('xnet')
    has_recvmsg = conf.CheckFunc('recvmsg')
    has_sendmsg = conf.CheckFunc('sendmsg')
    
    # may be in -lrt
    has_librt = conf.CheckLib('rt')
    has_clock_gettime = conf.CheckFunc('clock_gettime')
    has_clock_monotonic = conf.CheckDeclaration('CLOCK_MONOTONIC', '#include <time.h>')
    if has_clock_monotonic:
        conf.Define('HAVE_CLOCK_MONOTONIC') # autoconf compat
    # BSD extension
    has_clock_monotonic_fast = conf.CheckDeclaration(
	'CLOCK_MONOTONIC_FAST', '#include <time.h>')
    if has_clock_monotonic_fast:
        conf.Define('HAVE_CLOCK_MONOTONIC_FAST')
    
    has_struct_timespec = conf.CheckType('struct timespec', includes='#include <time.h>')
    
    ##########
    # bsd: headers
    has_paths_h = conf.CheckHeader('paths.h')
    has_sysexits_h = conf.CheckHeader('sysexits.h')
    
    # bsd: functions
    has_realpath = conf.CheckFunc('realpath')
    has_strptime = conf.CheckFunc('strptime')
    has_sysctl = conf.CheckFunc('sysctl')

    # bsd-style resolver: functions
    has_netdb_h = conf.CheckHeader('netdb.h')
    has_libresolv = conf.CheckLib('resolv')  # opensolaris needs it
    has_hstrerror = conf.CheckFunc('hstrerror')
    
    ##########
    # unix: system headers
    has_sys_cdefs_h = conf.CheckHeader('sys/cdefs.h')
    has_sys_param_h = conf.CheckHeader('sys/param.h')
    has_sys_utsname_h = conf.CheckHeader('sys/utsname.h')
    has_sys_errno_h = conf.CheckHeader('sys/errno.h')
    has_sys_wait_h = conf.CheckHeader('sys/wait.h')
    has_sys_signal_h = conf.CheckHeader('sys/signal.h')
    has_sys_time_h = conf.CheckHeader('sys/time.h')
    has_sys_uio_h = conf.CheckHeader('sys/uio.h')
    has_sys_ioctl_h = conf.CheckHeader('sys/ioctl.h')
    has_sys_select_h = conf.CheckHeader('sys/select.h')
    has_sys_socket_h = conf.CheckHeader('sys/socket.h')
    has_sys_sockio_h = conf.CheckHeader('sys/sockio.h')
    has_sys_un_h = conf.CheckHeader('sys/un.h')

    prereq_sys_mount_h = []
    if has_sys_types_h:
        prereq_sys_mount_h.append('sys/types.h')
    if has_sys_param_h:
        prereq_sys_mount_h.append('sys/param.h')
    has_sys_mount_h = conf.CheckHeader(prereq_sys_mount_h + ['sys/mount.h'])

    has_sys_resource_h = conf.CheckHeader('sys/resource.h')
    has_sys_stat_h = conf.CheckHeader('sys/stat.h')
    has_sys_syslog_h = conf.CheckHeader('sys/syslog.h')
    
    # bsd
    has_sys_linker_h = conf.CheckHeader(['sys/param.h', 'sys/linker.h'])
    has_sys_sysctl_h = conf.CheckHeader(['sys/param.h', 'sys/sysctl.h'])
    
    # linux
    has_linux_types_h = conf.CheckHeader('linux/types.h')
    has_linux_sockios_h = conf.CheckHeader('linux/sockios.h')
    
    # XXX needs header conditionals
    has_struct_iovec = conf.CheckType('struct iovec', includes='#include <sys/uio.h>')
    has_struct_msghdr = conf.CheckType('struct msghdr', includes='#include <sys/socket.h>')
    has_struct_cmsghdr = conf.CheckType('struct cmsghdr', includes='#include <sys/socket.h>')

    #  Check for boost noncopyable include file.
    #  This doesn't work..not too sure why. --Ben
    #has_boost_noncopyable_hpp = conf.CheckHeader('boost/noncopyable.hpp');
    #if has_boost_noncopyable_hpp:
    #    conf.Define('HAS_BOOST_NONCOPYABLE_INC')
    
    ##########
    # Socket support checks
    if (env.has_key('mingw') and env['mingw']):
        prereq_af_inet_includes = [ 'winsock2.h' ]
    else:
        prereq_af_inet_includes = [ 'sys/types.h', 'sys/socket.h' ]
    af_inet_includes = []
    for s in prereq_af_inet_includes:
        af_inet_includes.append("#include <%s>\n" % s)
    af_inet_includes = string.join(af_inet_includes, '')
    
    has_af_inet = conf.CheckDeclaration('AF_INET', af_inet_includes)
    has_af_inet6 = conf.CheckDeclaration('AF_INET6', af_inet_includes)
    has_sock_stream = conf.CheckDeclaration('SOCK_STREAM', af_inet_includes)
    has_sock_dgram = conf.CheckDeclaration('SOCK_DGRAM', af_inet_includes)
    has_sock_raw = conf.CheckDeclaration('SOCK_RAW', af_inet_includes)
    
    if has_af_inet and has_sock_stream and has_sock_dgram:
        conf.Define('HAVE_TCPUDP_UNIX_SOCKETS')
    if has_af_inet and has_sock_raw:
        conf.Define('HAVE_IP_RAW_SOCKETS')
        # TODO:  This needs to be properly detected.
        # TODO:  This used to check for openbsd and linux in an error prone
        #        way.  Now, do negative checks, but this could break Solaris and other OS
        #        (or not..no idea if it supports raw or not).
	if not ((env.has_key('mingw') and env['mingw']) or
                fnmatch.fnmatch(host_os, 'freebsd*')):
            conf.Define('IPV4_RAW_OUTPUT_IS_RAW')
            conf.Define('IPV4_RAW_INPUT_IS_RAW')
    
    if has_struct_msghdr:
        has_struct_msghdr_msg_control = conf.CheckTypeMember('struct msghdr', 'msg_control', includes='#include <sys/socket.h>')
        has_struct_msghdr_msg_iov = conf.CheckTypeMember('struct msghdr', 'msg_iov', includes='#include <sys/socket.h>')
        has_struct_msghdr_msg_name = conf.CheckTypeMember('struct msghdr', 'msg_name', includes='#include <sys/socket.h>')
        has_struct_msghdr_msg_namelen = conf.CheckTypeMember('struct msghdr', 'msg_namelen', includes='#include <sys/socket.h>')
    
    has_struct_sockaddr_sa_len = conf.CheckTypeMember('struct sockaddr', 'sa_len', includes='#include <sys/socket.h>')
    has_struct_sockaddr_storage_ss_len = conf.CheckTypeMember('struct sockaddr_storage', 'ss_len', includes='#include <sys/socket.h>')
    
    has_struct_sockaddr_un_sun_len = conf.CheckTypeMember('struct sockaddr_un', 'sun_len', includes='#include <sys/socket.h>\n#include <sys/un.h>')
    
    ##########
    # net stack
    has_net_ethernet_h = conf.CheckHeader(['sys/types.h', 'net/ethernet.h'])
    has_sys_ethernet_h = conf.CheckHeader('sys/ethernet.h')
    has_net_if_h = conf.CheckHeader(['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h'])
    has_net_if_arp_h = conf.CheckHeader(['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h', 'net/if_arp.h'])
    has_net_if_dl_h = conf.CheckHeader(['sys/types.h', 'net/if_dl.h'])
    has_net_if_ether_h = conf.CheckHeader(['sys/types.h', 'net/if.h', 'net/if_ether.h'])
    has_net_if_media_h = conf.CheckHeader(['sys/types.h', 'net/if_media.h'])
    
    has_net_if_var_h = conf.CheckHeader(['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h', 'net/if_var.h'])
    has_net_if_types_h = conf.CheckHeader('net/if_types.h')
    has_net_route_h = conf.CheckHeader(['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h', 'net/route.h'])
    has_ifaddrs_h = conf.CheckHeader(['sys/types.h', 'sys/socket.h', 'ifaddrs.h'])
    has_stropts_h = conf.CheckHeader('stropts.h')

    # Header file <linux/ethtool.h> might need <inttypes.h>, <stdint.h>,
    # and/or <linux/types.h>
    prereq_linux_ethtool_h = []
    if has_inttypes_h:
	prereq_linux_ethtool_h.append('inttypes.h')
    if has_stdint_h:
	prereq_linux_ethtool_h.append('stdint.h')
    if has_linux_types_h:
	prereq_linux_ethtool_h.append('linux/types.h')
    has_linux_ethtool_h = conf.CheckHeader(prereq_linux_ethtool_h + ['linux/ethtool.h'])

    has_linux_if_tun_h = conf.CheckHeader('linux/if_tun.h')

    # Header file <linux/netlink.h> might need <sys/types.h>, <sys/socket.h>,
    # and/or <linux/types.h>
    prereq_linux_netlink_h = []
    if has_sys_types_h:
        prereq_linux_netlink_h.append('sys/types.h')
    if has_sys_socket_h:
        prereq_linux_netlink_h.append('sys/socket.h')
    if has_linux_types_h:
        prereq_linux_netlink_h.append('linux/types.h')
    has_linux_netlink_h = conf.CheckHeader(prereq_linux_netlink_h + ['linux/netlink.h'])

    # Header file <linux/rtnetlink.h> might need <sys/types.h>, <sys/socket.h>,
    # and/or <linux/types.h>
    prereq_linux_rtnetlink_h = []
    if has_sys_types_h:
        prereq_linux_rtnetlink_h.append('sys/types.h')
    if has_sys_socket_h:
        prereq_linux_rtnetlink_h.append('sys/socket.h')
    if has_linux_types_h:
        prereq_linux_rtnetlink_h.append('linux/types.h')
    has_linux_rtnetlink_h = conf.CheckHeader(prereq_linux_rtnetlink_h + ['linux/rtnetlink.h'])
    
    if has_linux_netlink_h:
        conf.Define('HAVE_NETLINK_SOCKETS')
    elif has_net_route_h and host_os != 'linux-gnu':
        conf.Define('HAVE_ROUTING_SOCKETS')

    if has_linux_netlink_h:
        rta_nl_includes = []
        for s in prereq_linux_rtnetlink_h:
            rta_nl_includes.append("#include <%s>\n" % s)
            
        rta_nl_includes.append("#include <linux/rtnetlink.h>\n");
        rta_nl_includes = string.join(rta_nl_includes, '')
        has_netlink_rta_table = conf.CheckDeclaration('RTA_TABLE', rta_nl_includes)
        if has_netlink_rta_table:
            conf.Define('HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE')
    
    # net stack: struct members
    # XXX header conditionals for linux/bsd variants needed.
    has_struct_sockaddr_dl_sdl_len = conf.CheckTypeMember('struct sockaddr_dl', 'sdl_len', includes='#include <sys/types.h>\n#include <net/if_dl.h>')
    has_struct_ifreq_ifr_hwaddr = conf.CheckTypeMember('struct ifreq', 'ifr_hwaddr', includes='#include <sys/types.h>\n#include <net/if.h>')
    has_struct_ifreq_ifr_ifindex = conf.CheckTypeMember('struct ifreq', 'ifr_ifindex', includes='#include <sys/types.h>\n#include <net/if.h>')
    
    # net stack: functions
    # XXX some may be in libc or libnsl
    has_ether_aton = conf.CheckFunc('ether_aton')
    has_ether_aton_r = conf.CheckFunc('ether_aton_r')
    has_ether_ntoa = conf.CheckFunc('ether_ntoa')
    has_ether_ntoa_r = conf.CheckFunc('ether_ntoa_r')
    has_getaddrinfo = conf.CheckFunc('getaddrinfo')
    has_getifaddrs = conf.CheckFunc('getifaddrs')
    has_getnameinfo = conf.CheckFunc('getnameinfo')
    has_if_indextoname = conf.CheckFunc('if_indextoname')
    has_if_nametoindex = conf.CheckFunc('if_nametoindex')
    has_inet_ntop = conf.CheckFunc('inet_ntop')
    has_inet_pton = conf.CheckFunc('inet_pton')
    
    # net stack: types
    # XXX header conditionals for linux/bsd variants needed.
    prereq_ether_includes = [ 'sys/types.h', 'sys/socket.h' ]
    if has_net_ethernet_h:
	prereq_ether_includes.append('net/ethernet.h')
    if has_net_if_h:
	prereq_ether_includes.append('net/if.h')
    if has_net_if_ether_h:
	prereq_ether_includes.append('net/if_ether.h')
    ether_includes = []
    for s in prereq_ether_includes:
        ether_includes.append("#include <%s>\n" % s)
    ether_includes = string.join(ether_includes, '')
    has_struct_ether_addr = conf.CheckType('struct ether_addr', includes=ether_includes)
    
    # net stack: sysctl (bsd)
    conf.CheckSysctl('NET_RT_DUMP', oid='CTL_NET, AF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0', includes='#include <sys/socket.h>')
    conf.CheckSysctl('NET_RT_IFLIST', oid='CTL_NET, AF_ROUTE, 0, AF_INET, NET_RT_IFLIST, 0', includes='#include <sys/socket.h>')
    
    # XXX test for SIOCGIFCONF. Very gnarly.
    siocgifconf_includes = [ 'stdlib.h', 'errno.h' ]
    if has_sys_types_h:
        siocgifconf_includes.append('sys/types.h')
    if has_sys_socket_h:
        siocgifconf_includes.append('sys/socket.h')
    if has_sys_sockio_h:
        siocgifconf_includes.append('sys/sockio.h')
    if has_sys_ioctl_h:
        siocgifconf_includes.append('sys/ioctl.h')
    if has_net_if_h:
        siocgifconf_includes.append('net/if.h')
    si = []
    for s in siocgifconf_includes:
        si.append("#include <%s>\n" % s)
    si = string.join(si, '')
    has_siocgifconf = conf.CheckDeclaration('SIOCGIFCONF', si)
    if has_siocgifconf:
        conf.Define('HAVE_IOCTL_SIOCGIFCONF') # autoconf compat

    ##########
    # v4 stack
    has_netinet_in_h = conf.CheckHeader('netinet/in.h')
    has_netinet_in_systm_h = conf.CheckHeader(['sys/types.h', 'netinet/in_systm.h'])
    prereq_netinet_in_var_h = []
    if has_sys_types_h:
        prereq_netinet_in_var_h.append('sys/types.h')
    if has_sys_socket_h:
        prereq_netinet_in_var_h.append('sys/socket.h')
    if has_net_if_h:
        prereq_netinet_in_var_h.append('net/if.h')
    if has_net_if_var_h:
        prereq_netinet_in_var_h.append('net/if_var.h')
    if has_netinet_in_h:
        prereq_netinet_in_var_h.append('netinet/in.h')
    has_netinet_in_var_h = conf.CheckHeader(prereq_netinet_in_var_h + ['netinet/in_var.h'])

    # Header file <netinet/ip.h> might need <sys/types.h>, <netinet.in.h>,
    # and/or <netinet/in_systm.h>
    prereq_netinet_ip_h = []
    if has_sys_types_h:
	prereq_netinet_ip_h.append('sys/types.h')
    if has_netinet_in_h:
        prereq_netinet_ip_h.append('netinet/in.h')
    if has_netinet_in_systm_h:
	prereq_netinet_ip_h.append('netinet/in_systm.h')
    prereq_mreqn_h = prereq_netinet_ip_h
    has_netinet_ip_h = conf.CheckHeader(prereq_netinet_ip_h + ['netinet/ip.h'])
    if has_netinet_ip_h:
         prereq_mreqn_h.append('netinet/ip.h');

    has_netinet_tcp_h = conf.CheckHeader(['sys/param.h', 'sys/socket.h', 'netinet/in.h', 'netinet/in_systm.h', 'netinet/ip.h', 'netinet/tcp.h'])
    has_netinet_igmp_h = conf.CheckHeader(['sys/types.h', 'netinet/in.h', 'netinet/igmp.h'])
    has_netinet_ether_h = conf.CheckHeader('netinet/ether.h')

    # Check for ip_mreqn struct.
    mreqn_header_includes = []
    for s in prereq_mreqn_h:
        mreqn_header_includes.append("#include <%s>\n" % s)
    mreqn_header_includes = string.join(mreqn_header_includes, '')
    has_struct_ip_mreqn = conf.CheckType('struct ip_mreqn', includes=mreqn_header_includes)
    if not has_struct_ip_mreqn:
        print "\nWARNING: No struct ip_mreqn found.  Each interface must"
        print "  have a unique IP address or IP multicast (at least) will not"
        print "  be transmitted on the correct interface."

    # Header file <netinet/if_ether.h> might need <sys/types.h>, 
    # <sys/socket.h>, <net/if.h>, and/or <netinet/in.h>
    prereq_netinet_if_ether_h = []
    if has_sys_types_h: 
	prereq_netinet_if_ether_h.append('sys/types.h')
    if has_sys_socket_h: 
	prereq_netinet_if_ether_h.append('sys/socket.h')
    if has_net_if_h: 
	prereq_netinet_if_ether_h.append('net/if.h')
    if has_netinet_in_h:
	prereq_netinet_if_ether_h.append('netinet/in.h')
    has_netinet_if_ether_h = conf.CheckHeader(prereq_netinet_if_ether_h + ['netinet/if_ether.h'])

    # opensolaris
    has_inet_nd_h = conf.CheckHeader('inet/nd.h')
    has_inet_ip_h = conf.CheckHeader('inet/ip.h')

    # name lookup, telnet
    has_arpa_inet_h = conf.CheckHeader('arpa/inet.h')
    has_arpa_telnet_h = conf.CheckHeader('arpa/telnet.h')
    
    has_struct_sockaddr_in_sin_len = conf.CheckTypeMember('struct sockaddr_in', 'sin_len', includes='#include <sys/socket.h>\n#include <netinet/in.h>')
    
    # check for v4 multicast capability
    # XXX conditional on these headers please
    prereq_v4mcast = [ 'sys/types.h', 'sys/socket.h', 'netinet/in.h' ]
    v4mcast_symbols = [ 'IP_MULTICAST_IF', 'IP_MULTICAST_TTL', 'IP_MULTICAST_LOOP', 'IP_ADD_MEMBERSHIP', 'IP_DROP_MEMBERSHIP' ]
    # munge header list
    v4mcast_includes = []
    for s in prereq_v4mcast:
        v4mcast_includes.append("#include <%s>\n" % s)
    v4mcast_includes = string.join(v4mcast_includes, '')
    # check for each symbol
    gotv4sym = True
    for s in v4mcast_symbols:
        gotv4sym = gotv4sym and conf.CheckDeclaration(s, v4mcast_includes)
    has_v4_mcast = gotv4sym
    # test result
    if has_v4_mcast:
        conf.Define('HAVE_IPV4_MULTICAST')
        if host_os == 'linux-gnu' or host_os == 'linux-gnueabi':
            print "Enabling MULT_MCAST_TABLES logic since we are compiling for Linux.\n"
            conf.Define('USE_MULT_MCAST_TABLES')
        else:
            print "Disabling MULT_MCAST_TABLES, host_os:", host_os, "\n"

    # v4 stack: sysctl (bsd)
    conf.CheckSysctl('IPCTL_FORWARDING', oid='CTL_NET, AF_INET, IPPROTO_IP, IPCTL_FORWARDING', includes='#include <sys/socket.h>\n#include <netinet/in.h>')
   
    ##########
    # logs
    if not (env.has_key('disable_warninglogs') and env['disable_warninglogs']):
            conf.Define('L_WARNING')
    if not (env.has_key('disable_infologs') and env['disable_infologs']):
            conf.Define('L_INFO')
    if not (env.has_key('disable_errorlogs') and env['disable_errorlogs']):
            conf.Define('L_ERROR')
    if not (env.has_key('disable_tracelogs') and env['disable_tracelogs']):
            conf.Define('L_TRACE')
    if not (env.has_key('disable_assertlogs') and env['disable_assertlogs']):
            conf.Define('L_ASSERT')
    if not (env.has_key('disable_otherlogs') and env['disable_otherlogs']):
            conf.Define('L_OTHER')
    if not (env.has_key('disable_fatallogs') and env['disable_fatallogs']):
            conf.Define('L_FATAL')
    if (env.has_key('disable_assert') and env['disable_assert']):
            conf.Define('NO_ASSERT')

    ##########
    # v6 stack

    if has_af_inet6 and has_sock_stream:
        if not (env.has_key('disable_ipv6') and env['disable_ipv6']):
            conf.Define('HAVE_IPV6')

    prereq_rfc3542 = ['stdlib.h', 'sys/types.h', 'netinet/in.h']
    rfc3542_includes = []
    for s in prereq_rfc3542:
	# XXX: __USE_GNU must be defined for RFC3542 defines under Linux.
	if host_os == 'linux-gnu' and s == 'netinet/in.h':
	    rfc3542_includes.append("#define __USE_GNU\n")
        rfc3542_includes.append("#include <%s>\n" % s)
    rfc3542_includes = string.join(rfc3542_includes, '')

    has___kame__ = conf.CheckDeclaration('__KAME__', rfc3542_includes)
    # CheckFunc() too tight.
    has_inet6_opt_init = conf.CheckDeclaration('inet6_opt_init', rfc3542_includes)
    
    if has___kame__:
        conf.Define('IPV6_STACK_KAME')
    if has_inet6_opt_init:
        conf.Define('HAVE_RFC3542')
    
    has_struct_sockaddr_in6_sin6_len = conf.CheckTypeMember('struct sockaddr_in6', 'sin6_len', includes='#include <sys/socket.h>\n#include <netinet/in.h>')
    has_struct_sockaddr_in6_sin6_scope_id = conf.CheckTypeMember('struct sockaddr_in6', 'sin6_scope_id', includes='#include <sys/socket.h>\n#include <netinet/in.h>')
    
    has_netinet_ip6_h = conf.CheckHeader(['sys/types.h', 'netinet/in.h', 'netinet/ip6.h'])
    
    prereq_netinet_icmp6_h = ['sys/types.h', 'sys/socket.h', 'netinet/in.h', 'netinet/ip6.h']
    netinet_icmp6_h = 'netinet/icmp6.h'
    has_netinet_icmp6_h = conf.CheckHeader(prereq_netinet_icmp6_h + [ netinet_icmp6_h ])
    
    # struct mld_hdr normally defined in <netinet/icmp6.h>
    mld_hdr_includes = []
    for s in prereq_netinet_icmp6_h + [ netinet_icmp6_h ]:
        mld_hdr_includes.append("#include <%s>\n" % s)
    mld_hdr_includes = string.join(mld_hdr_includes, '')
    has_struct_mld_hdr = conf.CheckType('struct mld_hdr', includes=mld_hdr_includes)
    
    # Header file <netinet6/in6_var.h> might need <sys/types.h>, <sys/socket.h>,
    # <net/if.h>, <net/if_var.h>, and/or <netinet/in.h>.
    prereq_netinet6_in6_var_h = []
    if has_sys_types_h:
        prereq_netinet6_in6_var_h.append('sys/types.h')
    if has_sys_socket_h:
        prereq_netinet6_in6_var_h.append('sys/socket.h')
    if has_net_if_h:
        prereq_netinet6_in6_var_h.append('net/if.h')
    if has_net_if_var_h:
        prereq_netinet6_in6_var_h.append('net/if_var.h')
    if has_netinet_in_h:
        prereq_netinet6_in6_var_h.append('netinet/in.h')
    has_netinet6_in6_var_h = conf.CheckHeader(prereq_netinet6_in6_var_h + ['netinet6/in6_var.h'])
   
    # Header file <netinet6/nd6.h> might need <sys/types.h>, <sys/socket.h>,
    # <net/if.h>, <net/if_var.h>, <netinet/in.h>, and/or <netinet6/in6_var.h> 
    prereq_netinet6_nd6_h = []
    if has_sys_types_h:
	prereq_netinet6_nd6_h.append('sys/types.h')
    if has_sys_socket_h:
	prereq_netinet6_nd6_h.append('sys/socket.h')
    if has_net_if_h:
	prereq_netinet6_nd6_h.append('net/if.h')
    if has_net_if_var_h:
	prereq_netinet6_nd6_h.append('net/if_var.h')
    if has_netinet_in_h:
	prereq_netinet6_nd6_h.append('netinet/in.h')
    if has_netinet6_in6_var_h:
	prereq_netinet6_nd6_h.append('netinet6/in6_var.h')
    netinet6_nd6_h = 'netinet6/nd6.h'
    has_netinet6_nd6_h = conf.CheckHeader(prereq_netinet6_nd6_h + [ netinet6_nd6_h ])
    has_cxx_netinet6_nd6_h = conf.CheckHeader(prereq_netinet6_nd6_h + [ netinet6_nd6_h ], language='C++')
    if has_netinet6_nd6_h and not has_cxx_netinet6_nd6_h:
        conf.Define('HAVE_BROKEN_CXX_NETINET6_ND6_H')
    
    # v6 stack: sysctl (bsd)
    conf.CheckSysctl('IPV6CTL_FORWARDING', oid='CTL_NET, AF_INET6, IPPROTO_IPV6, IPV6CTL_FORWARDING', includes='#include <sys/socket.h>\n#include <netinet/in.h>')
    conf.CheckSysctl('IPV6CTL_ACCEPT_RTADV', oid='CTL_NET, AF_INET6, IPPROTO_IPV6, IPV6CTL_ACCEPT_RTADV', includes='#include <sys/socket.h>\n#include <netinet/in.h>')

    # check for v6 multicast capability
    # XXX conditional on these headers please
    prereq_v6mcast = [ 'sys/types.h', 'sys/socket.h', 'netinet/in.h' ]
    v6mcast_symbols = [ 'IPV6_MULTICAST_IF', 'IPV6_MULTICAST_LOOP' ]
    # munge header list
    v6mcast_includes = []
    for s in prereq_v6mcast:
        v6mcast_includes.append("#include <%s>\n" % s)
    v6mcast_includes = string.join(v6mcast_includes, '')
    # check for each symbol
    gotv6sym = True
    for s in v6mcast_symbols:
        gotv6sym = gotv6sym and conf.CheckDeclaration(s, v6mcast_includes)
    has_v6_mcast = gotv6sym
    # test result
    if has_v6_mcast:
        if not (env.has_key('disable_ipv6') and env['disable_ipv6']):
            conf.Define('HAVE_IPV6_MULTICAST')

    # See if we need -std=gnu99 for fpclassify (math.h)
    prereq_fpclassify = [ 'math.h' ]
    fpclassify_includes = []
    for s in prereq_fpclassify:
        fpclassify_includes.append("#include <%s>\n" % s)
    fpclassify_includes = string.join(fpclassify_includes, '')
    has_fpclassify = conf.CheckDeclaration('fpclassify', fpclassify_includes)
    if not has_fpclassify:
        env.AppendUnique(CFLAGS = '-std=gnu99')
        has_fpclassify = conf.CheckDeclaration('fpclassify', fpclassify_includes)
        if not has_fpclassify:
            print "\nERROR:  Cannot find fpclassify, tried -std=gnu99 as well."
            sys.exit(1)
        else:
            print "\nNOTE:  Using -std=gnu99 for fpclassify (math.h)\n"

    ##########
    # v4 mforwarding
    
    # this platform's multicast forwarding header(s)
    prereq_mroute_h = []
    mroute_h = None
    
    if host_os == 'sunos':
        prereq_netinet_ip_mroute_h = ['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'sys/time.h', 'inet/ip.h', 'netinet/in.h']
    else:
        prereq_netinet_ip_mroute_h = ['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'sys/time.h', 'net/if.h', 'net/route.h', 'netinet/in.h']
        if has_net_if_var_h:
            prereq_netinet_ip_mroute_h.append('net/if_var.h')

    netinet_ip_mroute_h = 'netinet/ip_mroute.h'
    has_netinet_ip_mroute_h = conf.CheckHeader(prereq_netinet_ip_mroute_h + [ netinet_ip_mroute_h ])
    if has_netinet_ip_mroute_h:
        prereq_mroute_h = prereq_netinet_ip_mroute_h
        mroute_h = netinet_ip_mroute_h
    
    prereq_net_ip_mroute_ip_mroute_h = ['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'sys/time.h', 'net/if.h', 'net/route.h', 'netinet/in.h']
    if has_net_if_var_h:
        prereq_net_ip_mroute_ip_mroute_h.append('net/if_var.h')

    net_ip_mroute_ip_mroute_h = 'net/ip_mroute/ip_mroute.h'
    has_net_ip_mroute_ip_mroute_h = conf.CheckHeader(prereq_net_ip_mroute_ip_mroute_h + [ net_ip_mroute_ip_mroute_h ])
    if has_net_ip_mroute_ip_mroute_h:
        prereq_mroute_h = prereq_net_ip_mroute_ip_mroute_h
        mroute_h = net_ip_mroute_ip_mroute_h
  
    # Header file <linux/mroute.h> might need <sys/types>, <sys/socket.h>,
    # <netinet/in.h>, and/or <linux/types.h>
    #
    # TODO: The autoconf feature test for this contained a hack to exclude 
    # <linux/in.h> that might be included by <linux/mroute.h>, because
    # <linux/in.h> might conflict with <netinet/in.h> that was included
    # earlier.  This is currently difficult to replicate in SCons, as
    # you can't pass arbitrary code that is prepended to the test.
    prereq_linux_mroute_h = []
    if has_sys_types_h:
	prereq_linux_mroute_h.append('sys/types.h')
    if has_sys_socket_h:
	prereq_linux_mroute_h.append('sys/socket.h')
    if has_netinet_in_h:
	prereq_linux_mroute_h.append('netinet/in.h')
    if has_linux_types_h:
	prereq_linux_mroute_h.append('linux/types.h')
    linux_mroute_h = 'linux/mroute.h'
    has_linux_mroute_h = conf.CheckHeader(prereq_linux_mroute_h + [ linux_mroute_h ])
    if has_linux_mroute_h:
        prereq_mroute_h = prereq_linux_mroute_h
        mroute_h = linux_mroute_h
    else:
        # Try without netinet/in.h, older releases (CentOS 5, for instance) doesn't need it
        # and break with it.
        prereq_linux_mroute_h = []
        if has_sys_types_h:
            prereq_linux_mroute_h.append('sys/types.h')
        if has_sys_socket_h:
            prereq_linux_mroute_h.append('sys/socket.h')
        if has_linux_types_h:
            prereq_linux_mroute_h.append('linux/types.h')
        linux_mroute_h = 'linux/mroute.h'
        has_linux_mroute_h = conf.CheckHeader(prereq_linux_mroute_h + [ linux_mroute_h ])
        if has_linux_mroute_h:
            prereq_mroute_h = prereq_linux_mroute_h
            mroute_h = linux_mroute_h
    
    mfcctl2_includes = []
    for s in prereq_mroute_h + [ mroute_h ]:
        mfcctl2_includes.append("#include <%s>\n" % s)
    mfcctl2_includes = string.join(mfcctl2_includes, '')
    
    # common structs
    has_struct_mfcctl2 = conf.CheckType('struct mfcctl2', includes=mfcctl2_includes)
    has_struct_mfcctl2_mfcc_flags = conf.CheckTypeMember('struct mfcctl2', 'mfcc_flags', includes=mfcctl2_includes)
    has_struct_mfcctl2_mfcc_rp = conf.CheckTypeMember('struct mfcctl2', 'mfcc_rp', includes=mfcctl2_includes)
    
    # pim
    has_netinet_pim_h = conf.CheckHeader('netinet/pim.h')
    has_struct_pim = conf.CheckType('struct pim', includes='#include <netinet/pim.h>')
    has_struct_pim_pim_vt = conf.CheckTypeMember('struct pim', 'pim_vt', includes='#include <netinet/pim.h>')
    
    if has_netinet_ip_mroute_h or has_net_ip_mroute_ip_mroute_h or has_linux_mroute_h:
        conf.Define('HAVE_IPV4_MULTICAST_ROUTING')
    
    
    ##########
    # v6 mforwarding
    
    # this platform's v6 multicast forwarding header(s)
    prereq_mroute6_h = []
    mroute6_h = None
    
    # bsd
    prereq_netinet6_ip6_mroute_h = ['sys/param.h', 'sys/socket.h', 'sys/time.h', 'net/if.h', 'net/route.h', 'netinet/in.h']
    if has_net_if_var_h:
        prereq_netinet6_ip6_mroute_h.append('net/if_var.h')
    netinet6_ip6_mroute_h = 'netinet6/ip6_mroute.h'
    has_netinet6_ip6_mroute_h = conf.CheckHeader(prereq_netinet6_ip6_mroute_h + [ netinet6_ip6_mroute_h ])
    if has_netinet6_ip6_mroute_h:
        prereq_mroute6_h = prereq_netinet6_ip6_mroute_h
        mroute6_h = netinet6_ip6_mroute_h
    
    # linux
    prereq_linux_mroute6_h = ['sys/types.h', 'sys/socket.h', 'netinet/in.h', 'linux/types.h']
    linux_mroute6_h = 'linux/mroute6.h'
    has_linux_mroute6_h = conf.CheckHeader(prereq_linux_mroute6_h + [ linux_mroute6_h ])
    if has_linux_mroute6_h:
        prereq_mroute6_h = prereq_linux_mroute6_h
        mroute6_h = linux_mroute6_h

    i6o_includes = []
    for s in prereq_linux_mroute6_h:
        i6o_includes.append("#include <%s>\n" % s)
    i6o_includes.append("#include <%s>\n" % linux_mroute6_h);
    i6o_includes = string.join(i6o_includes, '')
    has_inet6_option_space = conf.CheckDeclaration('inet6_option_space', i6o_includes);
    
    # common structs
    mf6cctl2_includes = []
    for s in prereq_mroute6_h + [ mroute6_h ]:
        mf6cctl2_includes.append("#include <%s>\n" % s)
    mf6cctl2_includes = string.join(mf6cctl2_includes, '')
    
    has_struct_mf6cctl2 = conf.CheckType('struct mf6cctl2', includes=mf6cctl2_includes)
    has_struct_mfcctl2_mfcc_flags = conf.CheckTypeMember('struct mf6cctl2', 'mf6cc_flags', includes=mf6cctl2_includes)
    has_struct_mfcctl2_mfcc_rp = conf.CheckTypeMember('struct mf6cctl2', 'mf6cc_rp', includes=mf6cctl2_includes)
   
    # XXX: linux marked inet6_option_space() and friends as deprecated;
    # either rework mfea code or do this.
    if has_netinet6_ip6_mroute_h or has_linux_mroute6_h:
        if not (env.has_key('disable_ipv6') and env['disable_ipv6']):
            conf.Define('HAVE_IPV6_MULTICAST_ROUTING')
            if has_inet6_option_space:
                conf.Define('HAVE_IPV6_OPTION_SPACE')
            else:
                if not has_inet6_opt_init:
                    print "\nWARNING:  inet6_option_* and inet6_opt_* are not supported on this system."
                    print "  this might cause some problems with IPv6 multicast routing.\n"

    has_struct_mif6ctl_vifc_threshold = conf.CheckTypeMember('struct mif6ctl', 'vifc_threshold', includes=mf6cctl2_includes) 
    ##########
    # packet filters
    has_netinet_ip_compat_h = conf.CheckHeader(['sys/types.h', 'netinet/ip_compat.h'])
    has_netinet_ip_fil_h = conf.CheckHeader(['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'netinet/in.h', 'netinet/in_systm.h', 'netinet/ip.h', 'netinet/ip_compat.h', 'netinet/ip_fil.h'])

    prereq_ip_fw_h = ['sys/types.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h', 'netinet/in.h']
    if has_net_if_var_h:
        prereq_ip_fw_h.append('net/if_var.h')
    has_netinet_ip_fw_h = conf.CheckHeader(prereq_ip_fw_h + ['netinet/ip_fw.h'])

    prereq_net_pfvar_h = ['sys/param.h', 'sys/file.h', 'sys/ioctl.h', 'sys/socket.h', 'net/if.h', 'netinet/in.h']
    if has_net_if_var_h:
        prereq_net_pfvar_h.append('net/if_var.h')
    has_net_pfvar_h = conf.CheckHeader(prereq_net_pfvar_h + ['net/pfvar.h'])

    # Older linux kernel headers for netfilter wouldn't compile with C++ w/out hacking on the headers themselves.
    has_linux_netfilter_ipv4_ip_tables_h = conf.CheckHeader(['sys/param.h', 'net/if.h', 'netinet/in.h', 'linux/netfilter_ipv4/ip_tables.h'], language = "C++")
    has_linux_netfilter_ipv6_ip6_tables_h = conf.CheckHeader(['sys/param.h', 'net/if.h', 'netinet/in.h', 'linux/netfilter_ipv6/ip6_tables.h'], language = "C++")

    if has_linux_netfilter_ipv4_ip_tables_h or has_linux_netfilter_ipv6_ip6_tables_h:
        if not (env.has_key('disable_fw') and env['disable_fw']):
            conf.Define('HAVE_FIREWALL_NETFILTER')
            
    ##########
    # vlan
    
    # Header files <net/if_vlan_var.h>, <net/if_vlanvar.h>, and 
    # <net/vlan/if_vlan_var.h> might need a list of other files.
    prereq_vlan = []
    if has_sys_types_h:
	prereq_vlan.append('sys/types.h')
    if has_sys_socket_h:
	prereq_vlan.append('sys/socket.h')
    if has_net_if_h:
	prereq_vlan.append('net/if.h')
    if has_net_ethernet_h:
	prereq_vlan.append('net/ethernet.h')
    if has_net_if_ether_h:
	prereq_vlan.append('net/if_ether.h')
    if has_netinet_in_h:
	prereq_vlan.append('netinet/in.h')
    if has_netinet_if_ether_h:
	prereq_vlan.append('netinet/if_ether.h')

    has_net_if_vlanvar_h = conf.CheckHeader(prereq_vlan + ['net/if_vlanvar.h'])

    has_net_if_vlan_var_h = conf.CheckHeader(prereq_vlan +  ['net/if_vlan_var.h'])
    has_net_vlan_if_vlan_var_h = conf.CheckHeader(prereq_vlan + ['net/vlan/if_vlan_var.h'])

    has_linux_if_vlan_h = conf.CheckHeader('linux/if_vlan.h')
    if has_linux_if_vlan_h:
        conf.Define('HAVE_VLAN_LINUX')
        # Check for really old if_vlan.h that doesn't have GET_VLAN_REALDEV_NAME_CMD enum
        # Vlans might still not work if the target kernel doesn't support these options,
        # but at least it will compile and has the potential to work.
        if not conf.CheckDeclaration('GET_VLAN_REALDEV_NAME_CMD', '#include <linux/if_vlan.h>'):
            conf.Define('GET_VLAN_REALDEV_NAME_CMD', '8')
        if not conf.CheckDeclaration('GET_VLAN_VID_CMD', '#include <linux/if_vlan.h>'):
            conf.Define('GET_VLAN_VID_CMD', '9')
    else:
	if has_net_if_vlanvar_h or has_net_if_vlan_var_h:
            conf.Define('HAVE_VLAN_BSD')

    
    ##########
    # pcre posix regexp emulation
    # used by policy for regexps.
    has_pcre_h = conf.CheckHeader('pcre.h')
    has_pcreposix_h = conf.CheckHeader('pcreposix.h')
    has_libpcre = conf.CheckLib('pcre')
    has_libpcreposix = conf.CheckLib('pcreposix')
    
    ##########
    # openssl for md5
    # XXX Check for MD5_Init()
    prereq_md5 = []
    if has_sys_types_h:
	prereq_md5.append('sys/types.h')
    has_openssl_md5_h = conf.CheckHeader(prereq_md5 + ['openssl/md5.h'])
    if not has_openssl_md5_h:
        print "\nERROR:  Cannot find required openssl/md5.h."
        print "  On Fedora/RedHat:  yum install openssl-devel"
        print "  On Ubuntu:  apt-get install libssl-dev"
        print "  After install, rm -fr xorp/obj build directory to"
        print "  clear the configure cache before re-building."
        sys.exit(1)
        
    has_libcrypto = conf.CheckLib('crypto')
    if not has_libcrypto:
        print "\nERROR:  Cannot find required crypto library."
        print "  clear the configure cache before re-building."
        sys.exit(1)
        
    has_md5_init = conf.CheckFunc('MD5_Init')
    
    ##########
    # x/open dynamic linker
    # XXX Check for dlopen() for fea use.
    has_dlfcn_h = conf.CheckHeader('dlfcn.h')
    has_libdl = conf.CheckLib('dl')
    has_dl_open = conf.CheckFunc('dlopen')

    
    ##########
    # pcap for l2 comms
    has_pcap_h = conf.CheckHeader('pcap.h')
    has_libpcap = conf.CheckLib('pcap')
    env['has_libpcap'] = has_libpcap
    has_pcap_sendpacket = conf.CheckFunc('pcap_sendpacket')
    if not has_libpcap:
        print "\nWARNING:  Libpcap was not detected.\n  VRRP and other protocols may have issues."
        print "  On Fedora/RedHat:  yum install libpcap-devel"
        print "  On Ubuntu:  apt-get install libpcap-dev"
        print "  After install, rm -fr xorp/obj build directory to"
        print "  clear the configure cache before re-building.\n"

    # pcap filtering can be used to cut down on un-needed netlink packets.
    #  This is a performance gain only, can function fine without it.
    prereq_pcap_bpf = []
    if has_sys_types_h:
	prereq_pcap_bpf.append('sys/types.h')
    has_pcap_bpf_h = conf.CheckHeader(prereq_pcap_bpf + ['pcap-bpf.h'])
    if not has_pcap_bpf_h:
        print "\nWARNING: PCAP-BPF is not supported on this system,"
        print "  socket filtering will not work."
        print "  This is not a real problem, just a small performance"
        print "  loss when using multiple virtual routers on the same system."
        print "  On Debian:  apt-get install libpcap-dev"
        print "  On Older Ubuntu:  apt-get install pcap-dev\n"
        print "  On Newer Ubuntu:  apt-get install libpcap-dev\n"

    if not (has_linux_netfilter_ipv4_ip_tables_h or has_linux_netfilter_ipv6_ip6_tables_h):
        if not (env.has_key('disable_fw') and env['disable_fw']):
            if has_linux_mroute_h:
                # We are Linux...should warn users about how to make netfiltering work since
                # it appears their headers are busted.
                print "\nWARNING: Netfilter include files are broken or do not exist."
                print "  This means the Linux firewall support will not be compiled in."
                print "  To fix, you may edit: /usr/include/linux/netfilter_ipv4/ip_tables.h"
                print "  line 222 or so, to look like this:"
                print "  /* Helper functions */"
                print "  static __inline__ struct ipt_entry_target *"
                print "  ipt_get_target(struct ipt_entry *e)"
                print "{"
                print "        /* BEN:  Was void* */"
                print "        return (struct ipt_entry_target *)((char*)e + e->target_offset);"
                print "}"
                print "\nYou will also want to edit similar code around line 282 of:"
                print "/usr/include/linux/netfilter_ipv6/ip6_tables.h"
                print "NOTE:  Recent kernels use struct xt_entry_target for the argument"
                print "   for these methods, so use that instead of ipt_entry_target if that"
                print "   is the case for your system."

    ##########
    # curses for cli/libtecla
    has_libcurses = conf.CheckLib('curses')
    has_libpdcurses = conf.CheckLib('pdcurses')
    has_libncurses = conf.CheckLib('ncurses')
    env['has_libcurses'] = has_libcurses
    env['has_libpdcurses'] = has_libpdcurses
    env['has_libncurses'] = has_libncurses
    if not has_libcurses and not has_libncurses and not has_libpdcurses:
        print "\nERROR:  Cannot find required (n)curses or pdcurses library."
        print "  On Fedora/RedHat:  yum install ncurses-devel"
        print "  On Debian/Ubuntu:  apt-get install ncurses-dev"
        print "  After install, rm -fr xorp/obj build directory to"
        print "  clear the configure cache before re-building."
        sys.exit(1)
