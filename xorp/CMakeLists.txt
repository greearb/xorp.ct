cmake_minimum_required(VERSION 3.15)

project(xorp VERSION 1.8.7)
include(CheckSymbolExists)
include(CheckIncludeFiles)
# include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckTypeSize)
include(CheckCSourceCompiles)

#set(CMAKE_CXX_STANDARD 17)

option(SHARED_LIB           "Build libraries as shared"                     ON)
option(STRIP_ON             "Strip binaries on install"                     OFF)
option(RTLD_ORIGIN          "Use ORIGIN in dynamically linked programs"     ON)
option(IGNORE_ERROR_TEST    "Ignore errors when building tests"             OFF)
option(DEBUG_STL            "Build with checked STL operations"             OFF)
option(DEBUG_MSG            "Build with debug messages"                     OFF)
option(DEBUG_FN             "Build with functions names in debug output"    OFF)
option(DEBUG_CB             "Build with callback debugging"                 OFF)
option(ENABLE_IPV6          "Enable ipv6 support"                           ON)
option(EXTERNAL_TECLA       "Build with external libtecla"                  OFF)
option(FIREWALL             "Firewall feature"                              ON)
option(ENABLE_WARNINGLOG    "Enable warning logs"                           ON)
option(ENABLE_TRACELOG      "Enable trace logs"                             ON)
option(ENABLE_INFOLOG       "Enable info logs"                              ON)
option(ENABLE_ASSERTLOG     "Enable assert logs"                            ON)
option(ENABLE_OTHERLOG      "Enable other logs"                             ON)
option(ENABLE_ERRORLOG      "Enable errors logs"                            ON)
option(ENABLE_FATALLOG      "Enable fatal errors logs"                      ON)
option(DISABLE_PROFILE      "Disable XORP Profiler feature"                 OFF)
option(DISABLE_WERROR       "Disable -Werror compiler flag"                 OFF)
option(DISABLE_WDEPRECATED  "Compile with -Wno-deprecated"                  OFF)
option(DISABLE_ASSERT       "Force disabling assertions"                    OFF)
option(LEX_HACK             "Workaround flex/yacc on FreeBSD"               OFF)
option(BUILD_RUN            "Enable running from build dir"                 OFF)
option(BOOST                "Use boost library"                             OFF)
option(USTL                 "Use uSTL"                                      OFF)
option(BGP                  "Enable BGP"                                    ON)
option(BUILD_INFO           "Build Info, more libxorp/create_buildinfo.sh"  ON)
option(OLSR                 "Build OLSR"                                    ON)
option(WRAPPER              "Build Wrapper"                                 ON)
option(OSPF                 "Build OSPF"                                    ON)
option(RIP                  "Build RIP"                                     ON)
option(VRRP                 "Build VRRP"                                    ON)
option(XORPSH               "Build xorpsh"                                  ON)
option(TESTS_PROGRAMS       "Build Test Programs"                           OFF)
option(CLICK                "Build CLICK support"                           OFF)
option(FEA_DUMMY            "Build fea-dummy target"                        ON)
option(VIFF_USE_IFINDEX     "Use VIFF_USE_IFINDEX feature in Linux kernel"  OFF)
option(ASYNC_SERVER         "Permit asynchronous method implementations"    OFF)
option(XRLDB                "Build with runtime XRL syntax validation"      OFF)
option(LOCAL_TRANSPORT      "Set transport method to unix instead of tcp"   ON)

set(PROFILER "no" CACHE STRING "Build with profiling")
set_property(CACHE PROFILER PROPERTY STRINGS "no" "gprof" "pprof" "override")

set(EXEC_PREFIX         ${CMAKE_INSTALL_PREFIX})
set(SBINDIR             ${EXEC_PREFIX}/sbin)
set(LIBEXECDIR          ${EXEC_PREFIX}/lib)
set(DATAROOTDIR         ${EXEC_PREFIX}/share)
set(DATADIR             ${DATAROOTDIR}/xorp)
set(SYSCONFDIR          ${EXEC_PREFIX}/etc)
set(LOCALSTATEDIR       ${EXEC_PREFIX}/var)
set(LIBDIR              ${EXEC_PREFIX}/lib)
set(MANDIR              ${EXEC_PREFIX}/man)

set(XORP_ROOTDIR        ${EXEC_PREFIX})          # Determine RPATH
set(XORP_CONFDIR        ${SYSCONFDIR})           # Path for xorp.conf
set(XORP_LIBDIR         ${LIBDIR}/xorp/lib)      # Libraries
set(XORP_MODULEDIR      ${LIBDIR}/xorp/sbin)     # Protocol modules
set(XORP_SBINDIR        ${SBINDIR})              # End-user binaries
set(XORP_TEMPLATEDIR    ${DATADIR}/templates)
set(XORP_TOOLDIR        ${LIBDIR}/xorp/bin)      # tools/*
set(XORP_XRLSDIR        ${DATADIR}/xrl/targets)  # *.xrls
set(XORP_SRCDIR         ${CMAKE_SOURCE_DIR})

set(XORP_SBIN_RPATH     ${XORP_LIBDIR})
set(XORP_TOOL_RPATH     ${XORP_LIBDIR})
set(XORP_MODULE_RPATH   ${XORP_LIBDIR})

if (BUILD_INFO)
    add_definitions(-DXORP_BUILDINFO)
endif()
# TODO: Host OSes: FreeBSD, Windows, Linux, NetBSD, Solaris, Darwin
# Use CMAKE_HOST_SYSTEM_NAME
# cmake_host_system_information(RESULT HOST_SYSTEM QUERY OS_NAME)
set(HOST_OS ${CMAKE_HOST_SYSTEM_NAME})
if (${HOST_OS} MATCHES "Linux")
    set(HOST_OS_LINUX ON)
    set(HAVE_PROC_LINUX ON)
    set(ENABLE_ADVANCED_MULTICAST_API ON)
elseif(${HOST_OS} MATCHES "FreeBSD*")
    set(HOST_OS_FREEBSD ON)
    set(ENABLE_ADVANCED_MULTICAST_API ON)
    # add_definitions(-DHOST_OS_FREEBSD -DENABLE_ADVANCED_MULTICAST_API)
elseif(${HOST_OS} EQUAL "Windows")
    # add_definitions(-DHOST_OS_WINDOWS)
    # add_definitions(-DXRL_PF="t")
    set(HOST_OS_WINDOWS ON)
    set(XRL_PF "\'t\'")
elseif(${HOST_OS} MATCHES "SunOS*")
    add_definitions(-DHOST_OS_SOLARIS)
    set(HOST_OS_SOLARIS ON)
elseif(${HOST_OS} MATCHES "NetBSD*")
    set(HOST_OS_NETBSD ON)
    set(ENABLE_ADVANCED_MULTICAST_API ON)
    # add_definitions(-DHOST_OS_NETBSD -DENABLE_ADVANCED_MULTICAST_API)
endif()

set(HOST_OS_NAME "\"${HOST_OS}\"")
if (NOT ${HOST_OS} EQUAL "Windows")
    if(LOCAL_TRANSPORT)
        # UNIX transport (local) has value 'x' == 120 (ASCII)
        set(XRL_PF "\'x\'")
    else()
        set(XRL_PF "\'t\'")
    endif()
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_C_EXTENSIONS OFF)


if(NOT FIREWALL)
    set(XORP_DISABLE_FIREWALL ON)
endif()

set(XORP_DISABLE_PROFILE ${DISABLE_PROFILE})
set(XORP_USE_USTL ${USTL})
set(XORP_USE_CLICK ${CLICK})
set(XORP_FEA_DUMMY ${FEA_DUMMY})
set(XORP_USE_VIFF_USE_IFINDEX ${VIFF_USE_IFINDEX})
set(XORP_ENABLE_ASYNC_SERVER ${ASYNC_SERVER})
set(XORP_USE_XORPSH ${XORPSH})

if(BOOST)
    # TODO: Perform Boost checks (minimum 1.34)
    # system, date_time, iostreams
    # filesystem, program_options, regex, signals, thread
    set(USE_BOOST ON)
    set(HAS_BOOST_NONCOPYABLE_INC ON)
    set(BOOST_DISABLE_THREADS ON)
    # add_definitions(-DUSE_BOOST -DHAS_BOOST_NONCOPYABLE_INC)
    # add_definitions(-DBOOST_DISABLE_THREADS)
endif()




if(NOT PROFILER EQUAL "no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
endif()
if(PROFILER EQUAL "gprof")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
endif()

add_definitions(-DXORP_VERSION="${PROJECT_VERSION}")

if(DISABLE_WDEPRECATED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated")
endif()

set(BASIC_FLAGS "-Wall -Wwrite-strings -Wcast-qual -Wmissing-declarations \
-Wpointer-arith -Wcast-align")
set(BASIC_C_FLAGS "-Wbad-function-cast -Wmissing-prototypes -Wstrict-prototypes -Wnested-externs")
set(CMAKE_CXX_FLAGS "${BASIC_FLAGS} ${CMAKE_CXX_FLAGS}  -ftemplate-depth-25")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${BASIC_FLAGS} ${BASIC_C_FLAGS}")


if(LEX_HACK)
    add_definitions(-DNEED_LEX_H_HACK)
endif()

if(NOT DISABLE_IPV6)
    set(HAVE_IPV6 ON)
endif()

# NOTE: For GNU STL only
if(DEBUG_STL)
    add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
endif()

# NOTE: DEBUG_LOGGING_GLOBAL currently a no-op
if (DEBUG_MSG)
    add_definitions(-DDEBUG_LOGGING)
endif()

if(DEBUG_FN)
    add_definitions(-DDEBUG_PRINT_FUNCTION_NAME)
endif()

if(DEBUG_CB)
    add_definitions(-DDEBUG_CALLBACKS)
endif()


check_include_file("endian.h"       HAVE_ENDIAN_H)
check_include_file("iphlpapi.h"     HAVE_IPHLPAPI_H)
check_include_file("routprot.h"     HAVE_ROUTPROT_H)
# check_include_file("stdint.h"       HAVE_STDINT_H) # Checked with check_type_size
check_include_file("inttypes.h"     HAVE_INTTYPES_H)
# check_symbol_exists(uint64_t "stdint.h" HAVE_UINT64_T)
check_type_size(uint64_t UINT64_T)
check_include_file("stdbool.h"      HAVE_STDBOOL_H)

# check_include_file("stddef.h"       HAVE_STDDEF_H)
check_include_file("stdarg.h"       HAVE_STDARG_H)
check_include_file("stdlib.h"       HAVE_STDLIB_H)
check_include_file("strings.h"      HAVE_STRINGS_H)
check_include_file("string.h"       HAVE_STRING_H)
check_include_file("signal.h"       HAVE_SIGNAL_H)
check_include_file("math.h"         HAVE_MATH_H)
check_include_file("memory.h"       HAVE_MEMORY_H)
check_function_exists(strftime      HAVE_STRFTIME)
check_function_exists(strlcpy       HAVE_STRLCPY)
check_function_exists(strlcap       HAVE_STRLCAT)
check_symbol_exists(va_copy "stdarg.h" HAVE_VA_COPY)
check_include_file("sys/types.h"    HAVE_SYS_TYPES_H)
check_include_file("fcntl.h"        HAVE_FCNTL_H)
check_include_file("getopt.h"       HAVE_GETOPT_H)
check_include_file("glob.h"         HAVE_GLOB_H)
check_include_file("grp.h"          HAVE_GRP_H)
check_include_file("pthread.h"      HAVE_PTHREAD_H)
check_include_file("pwd.h"          HAVE_PWD_H)
check_include_file("mqueue.h"       HAVE_MQUEUE_H)
check_include_file("regex.h"        HAVE_REGEX_H)
check_include_file("syslog.h"       HAVE_SYSLOG_H)
check_include_file("termios.h"      HAVE_TERMIOS_H)
check_include_file("time.h"         HAVE_TIME_H)
check_include_file("unistd.h"       HAVE_UNISTD_H)
check_include_file("vfork.h"        HAVE_VFORK_H)

# check_include_file("xnet.h"         HAVE_LIBXNET)
# check_include_file("rt.h"           HAVE_LIBRT)
check_function_exists(readv         HAVE_READV)
check_function_exists(strerror      HAVE_STRERROR)
check_function_exists(syslog        HAVE_SYSLOG)
check_function_exists(uname         HAVE_UNAME)
check_function_exists(writev        HAVE_WRITEV)
check_function_exists(recvmsg       HAVE_RECVMSG)
check_function_exists(sendmsg       HAVE_SENDMSG)


find_library(RT_LIBRARY rt)
if (RT_LIBRARY)
    message(STATUS "Found rt library")
    set(HAVE_LIBRT 1)
endif()

CHECK_LIBRARY_EXISTS(rt clock_gettime "" HAVE_CLOCK_GETTIME)
check_symbol_exists(CLOCK_MONOTONIC "time.h" HAVE_CLOCK_MONOTONIC)
check_symbol_exists(CLOCK_MONOTONIC_FAST "time.h" HAVE_CLOCK_MONOTONIC_FAST)

if (HAVE_TIME_H)
    check_struct_has_member("struct timespec" tv_sec "time.h" HAVE_STRUCT_TIMESPEC)
endif (HAVE_TIME_H)


check_include_file("paths.h"        HAVE_PATHS_H)
check_include_file("sysexits.h"     HAVE_SYSEXITS_H)
check_function_exists(realpath      HAVE_REALPATH)
check_function_exists(strptime      HAVE_STRPTIME)
check_function_exists(sysctl        HAVE_SYSCTL)
check_include_file("netdb.h"        HAVE_NETDB_H)

find_library(RESOLV_LIB resolv)
if (RESOLV_LIB)
    message(STATUS "Found resolv library")
    set(HAVE_LIBRESOLV 1)
endif()
check_function_exists(hstrerror     HAVE_HSTRERROR)


check_include_file("sys/cdefs.h"    HAVE_SYS_CDEFS_H)
check_include_file("sys/param.h"    HAVE_SYS_PARAM_H)
check_include_file("sys/utsname.h"  HAVE_SYS_UTSNAME_H)
check_include_file("sys/errno.h"    HAVE_SYS_ERRNO_H)
check_include_file("sys/wait.h"     HAVE_SYS_WAIT_H)
check_include_file("sys/signal.h"   HAVE_SYS_SIGNAL_H)
check_include_file("sys/time.h"     HAVE_SYS_TIME_H)
check_include_file("sys/uio.h"      HAVE_SYS_UIO_H)
check_include_file("sys/ioctl.h"    HAVE_SYS_IOCTL_H)
check_include_file("sys/select.h"   HAVE_SYS_SELECT_H)
check_include_file("sys/socket.h"   HAVE_SYS_SOCKET_H)
check_include_file("sys/sockio.h"   HAVE_SYS_SOCKIO_H)
check_include_file("sys/un.h"       HAVE_SYS_UN_H)
check_include_file("sys/mount.h"    HAVE_SYS_MOUNT_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("sys/stat.h"     HAVE_SYS_STAT_H)
check_include_file("sys/syslog.h"   HAVE_SYS_SYSLOG_H)
check_include_file("sys/linker.h"   HAVE_SYS_LINKER_H)
check_include_file("sys/sysctl.h"   HAVE_SYS_SYSCTL_H)
check_include_file("linux/types.h"  HAVE_LINUX_TYPES_H)
check_include_file("linux/sockios.h" HAVE_LINUX_SOCKIOS_H)
check_struct_has_member("struct iovec" iov_base "sys/uio.h" HAVE_STRUCT_IOVEC)
check_struct_has_member("struct msghdr" msg_name "sys/socket.h" HAVE_STRUCT_MSGHDR)
check_struct_has_member("struct cmsghdr" cmsg_level "sys/socket.h" HAVE_STRUCT_CMSGHDR)
check_symbol_exists(AF_INET     "sys/socket.h" HAVE_DECL_AF_INET)
check_symbol_exists(AF_INET6    "sys/socket.h" HAVE_DECL_AF_INET6)
check_symbol_exists(SOCK_STREAM "sys/socket.h" HAVE_DECL_SOCK_STREAM)
check_symbol_exists(SOCK_DGRAM  "sys/socket.h" HAVE_DECL_SOCK_DGRAM)
check_symbol_exists(SOCK_RAW    "sys/socket.h" HAVE_DECL_SOCK_RAW)
# Skip seme parts about sockets

check_struct_has_member("struct msghdr" msg_control "sys/socket.h" HAVE_STRUCT_MSGHDR_MSG_CONTROL)
check_struct_has_member("struct msghdr" msg_iov     "sys/socket.h" HAVE_STRUCT_MSGHDR_MSG_IOV)
check_struct_has_member("struct msghdr" msg_name    "sys/socket.h" HAVE_STRUCT_MSGHDR_MSG_NAME)
check_struct_has_member("struct msghdr" msg_namelen "sys/socket.h" HAVE_STRUCT_MSGHDR_MSG_NAMELEN)
check_struct_has_member("struct sockaddr"   sa_len  "sys/socket.h" HAVE_STRUCT_SOCKADDR_SA_LEN)
check_struct_has_member("struct sockaddr_storage"   ss_len  "sys/socket.h" HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN)
# check_struct_has_member("struct sockaddr_un"    sun_len "sys/socket.h" HAVE_SOCKADDR_UN_SUN_LEN)                  # Unused

check_include_file("net/ethernet.h" HAVE_ETHERNET_H)
check_include_file("sys/ethernet.h" HAVE_SYS_ETHERNET_H)
check_include_file("net/if.h"       HAVE_NET_IF_H)
check_include_file("net/if_arp.h"   HAVE_NET_IF_ARP_H)
check_include_file("net/if_dl.h"    HAVE_NET_IF_DL_H)
check_include_file("net/if_ether.h" HAVE_NET_IF_ETHER_H)
check_include_file("net/if_media.h" HAVE_NET_IF_MEDIA_H)
check_include_file("net/if_var.h"   HAVE_NET_IF_VAR_H)
check_include_file("net/if_types.h" HAVE_NET_IF_TYPES_H)
check_include_file("net/route.h"    HAVE_NET_ROUTE_H)
check_include_file("ifaddrs.h"      HAVE_IFADDRS_H)
check_include_file("stropts.h"      HAVE_STROPTS_H)
check_include_file("linux/ethtool.h" HAVE_LINUX_ETHTOOL_H)
# check_include_file("linux/if_tun.h" HAVE_LINUX_IF_TUN_H)          # Unused
check_include_file("linux/netlink.h"    HAVE_LINUX_NETLINK_H)
check_include_file("linux/rtnetlink.h"  HAVE_LINUX_RTNETLINK_H)
if (${HAVE_LINUX_RTNETLINK_H} AND ${HAVE_LINUX_NETLINK_H})
    set(HAVE_NETLINK_SOCKETS ON)
endif()
check_c_source_compiles("
#include <linux/rtnetlink.h>

int main() { 
    enum rtattr_type_t attr = RTA_TABLE;
    return 0;
}
" HAVE_NETLINK_SOCKET_ATTRIBUTE_RTA_TABLE)
check_struct_has_member("struct sockaddr_dl"    sdl_len "sys/socket.h" HAVE_STRUCTSOCKADDR_DL_SDL_LEN)
check_struct_has_member("struct ifreq"  ifr_hwaddr  "net/if.h" HAVE_STRUCT_IFREQ_IFR_HWADDR)
check_struct_has_member("struct ifreq"  ifr_ifindex "net/if.h" HAVE_STRUCT_IFREQ_IFR_IFINDEX)


check_function_exists(ether_aton    HAVE_ETHER_ATON)
check_function_exists(ether_aton_r  HAVE_ETHER_ATON_R)
check_function_exists(ether_ntoa    HAVE_ETHER_NTOA)
check_function_exists(ether_ntoa_r  HAVE_NTOA_R)
check_function_exists(getaddrinfo   HAVE_GETADDRINFO)
check_function_exists(getifaddrs    HAVE_GETIFADDRS)
check_function_exists(getnameinfo   HAVE_GETNAMEINFO)
check_function_exists(if_indextoname   HAVE_IF_INDEXTONAME)
check_function_exists(if_nametoindex   HAVE_IF_NAMETOINDEX)
check_function_exists(inet_ntop     HAVE_INET_NTOP)
check_function_exists(inet_pton     HAVE_INET_PTON)

check_c_source_compiles("
#include <net/ethernet.h>

int main() {
    struct ether_addr* addr = 0;
    (void*)addr;
    return 0;
}
" HAVE_STRUCT_ETHER_ADDR)
# Drop some variables related to sysctl - strange
check_symbol_exists(SIOCGIFCONF "sys/ioctl.h" HAVE_IOCTL_SIOCGIFCONF)

check_include_file("netinet/in.h"           HAVE_NETINET_IN_H)
check_include_file("netinet/in_systm.h"     HAVE_NETINET_IN_SYSTM_H)
check_include_file("netinet/in_var.h"       HAVE_NETINET_IN_VAR_H)
check_include_file("netinet/ip.h"           HAVE_NETINET_IP_H)
check_include_file("netinet/tcp.h"          HAVE_NETINET_TCP_H)

check_include_file("netinet/igmp.h"         HAVE_NETINET_IGMP_H)
check_include_file("netinet/ether.h"        HAVE_NETINET_ETHER_H)
check_c_source_compiles("
#include <linux/in.h>
#include <netinet/in.h>

int main() {
    struct ip_mreqn* mreq = 0;
    (void*)mreq;
    return 0;
}
" HAVE_STRUCT_IP_MREQN)
check_include_file("netinet/if_ether.h"     HAVE_NETINET_IF_ETHER_H)
check_include_file("inet/nd.h"              HAVE_INET_ND_H)
check_include_file("inet/ip.h"              HAVE_INET_IP_H)
check_include_file("arpa/inet.h"            HAVE_ARPA_INET_H)
check_include_file("arpa/telnet.h"          HAVE_ARPA_TELNET_H)

# check_struct_has_member("struct sockaddr_in" sin_len "netinet/in.h" HAVE_STRUCT_SOCKADDR_IN_SIN_LEN)
check_c_source_compiles("
#include <linux/in.h>
#include <netinet/in.h>

int main() {
    struct sockaddr_in addr;
    auto len = addr.sin_len;
    (void)len;
    return 0;
}
" HAVE_STRUCT_SOCKADDR_IN_SIN_LEN)

# MULTICAST
set(HAVE_IPV4_MULTICAST OFF)
if (${HAVE_SYS_TYPES_H} EQUAL 1 
    AND ${HAVE_SYS_SOCKET_H} EQUAL 1 
    AND ${HAVE_NETINET_IN_H} EQUAL 1)
    # check_c_source_compiles("
    # #include <sys/types.h>
    # #include <sys/socket.h>
    # #include <netinet/in.h>

    # int main() {
    #     int sockfd = socket(AF_INET, SOCK_RAW, 4); // Raw ip socket
    #     int ret = getsockopt(sockfd, 4, IP_MULTICAST_TTL, NULL);
    #     ret = getsockopt(sockfd, 4, IP_MULTICAST_IF, NULL);
    #     ret = getsockopt(sockfd, 4, IP_MULTICAST_LOOP, NULL);
    #     ret = getsockopt(sockfd, 4, IP_ADD_MEMBERSHIP, NULL);
    #     ret = getsockopt(sockfd, 4, IP_DROP_MEMBERSHIP, NULL);
    #     close(sockfd);
    #     return 0;
    # }" HAVE_IPV4_MULTICAST)
    check_symbol_exists(IP_MULTICAST_IF "netinet/in.h" HAVE_DECL_IP_MULTICAST_IF)
    check_symbol_exists(IP_MULTICAST_TTL "netinet/in.h" HAVE_DECL_IP_MULTICAST_TTL)
    check_symbol_exists(IP_MULTICAST_LOOP "netinet/in.h" HAVE_DECL_IP_MULTICAST_LOOP)
    check_symbol_exists(IP_ADD_MEMBERSHIP "netinet/in.h" HAVE_DECL_IP_ADD_MEMBERSHIP)
    check_symbol_exists(IP_DROP_MEMBERSHIP "netinet/in.h" HAVE_DECL_IP_DROP_MEMBERSHIP)
    #check_symbol_exists(SIOCGIFCONF "sys/ioctl.h" HAVE_IOCTL_SIOCGIFCONF)
    if (${HAVE_DECL_IP_MULTICAST_IF} AND ${HAVE_DECL_IP_MULTICAST_TTL} AND ${HAVE_DECL_IP_MULTICAST_LOOP}
        AND ${HAVE_DECL_IP_ADD_MEMBERSHIP} AND ${HAVE_DECL_IP_DROP_MEMBERSHIP})   
        set(HAVE_IPV4_MULTICAST ON)
        message(STATUS "Set HAVE_IPV4_MULTICAST")
    endif()
    if(HAVE_IPV4_MULTICAST AND HOST_OS_LINUX)
        set(USE_MULT_MCAST_TABLES ON)
        message(STATUS "Enabling MULT_MCAST_TABLES logic since we are compiling for Linux.")
    else()
        message(STATUS "Disabling MULT_MCAST_TABLES, host_os: ${HOST_OS}")
    endif()
endif()

if(ENABLE_WARNINGLOG)
    set(L_WARNING ON)
endif()

if(ENABLE_ERRORLOG)
    set(L_ERROR ON)
endif()

if(ENABLE_INFOLOG)
    set(L_INFO ON)
endif()

if(ENABLE_TRACELOG)
    set(L_TRACE ON)
endif()

if(ENABLE_ASSERTLOG)
    set(L_ASSERT ON)
endif()

if(ENABLE_OTHERLOG)
    set(L_OTHER ON)
endif()

if(ENABLE_FATALLOG)
    set(L_FATAL ON)
endif()



# configure_file(config.h.in ${CMAKE_CURRENT_LIST_DIR}/xorp_config.h)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/xorp_config.h)
set(MAIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
include_directories("${CMAKE_CURRENT_BINARY_DIR}/")
include_directories(.)

add_subdirectory(libxorp)
add_subdirectory(cli)
add_subdirectory(libcomm)
add_subdirectory(xrl/targets)
add_subdirectory(xrl/interfaces)
add_subdirectory(libxipc)
add_subdirectory(libproto)
add_subdirectory(libfeaclient)

#add_subdirectory(xrl/templates)
add_subdirectory(fea)
add_subdirectory(fib2mrib)
add_subdirectory(mld6igmp)
add_subdirectory(mrt)
add_subdirectory(pim)
add_subdirectory(policy)
add_subdirectory(rib)
add_subdirectory(rtrmgr)
add_subdirectory(static_routes)
add_subdirectory(utils)
# add_subdirectory(xrl/tests) # Unused now

if(OLSR)
    add_subdirectory(contrib/olsr)
endif()

if(WRAPPER)
    add_subdirectory(contrib/wrapper)
endif()

if(BGP)
    add_subdirectory(bgp)
endif()

if(OSPF)
    add_subdirectory(ospf)
endif()

if(RIP)
    add_subdirectory(rip)
endif()

if(VRRP)
    add_subdirectory(vrrp)
endif()

