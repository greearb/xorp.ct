// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/mfea/mfea_unix_comm.cc,v 1.4 2003/03/10 23:20:39 hodson Exp $"


//
// UNIX kernel-access specific implementation.
//


#include "mfea_module.h"
#include "mfea_private.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/utils.hh"
#include "mrt/max_vifs.h"
#include "mrt/multicast_defs.h"
#include "mfea_node.hh"
#include "mfea_vif.hh"
#include "mfea_unix_osdep.hh"
#include "mfea_unix_comm.hh"

// XXX: _PIM_VT is needed if we want the extra features of <netinet/pim.h>
#define _PIM_VT 1
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#else
#include "mrt/include/netinet/pim.h"
#endif


//
// Exported variables
//

//
// Static class members
//
int UnixComm::_rtm_seq = 1;

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//
static void unix_comm_proto_socket_read(int fd, SelectorMask mask,
					void *thunk);


/**
 * UnixComm::UnixComm:
 * @mfea_node: The MfeaNode I belong to.
 * @proto: The protocol number (e.g., %IPPROTO_IGMP, %IPPROTO_PIM, etc).
 **/
UnixComm::UnixComm(MfeaNode& mfea_node, int ipproto, x_module_id module_id)
    : ProtoUnit(mfea_node.family(), mfea_node.module_id()),
      _mfea_node(mfea_node),
      _ipproto(ipproto),
      _module_id(module_id)
{
    // Init Router Alert related option stuff
#ifdef HAVE_IPV6
    _rtalert_code = htons(IP6OPT_RTALERT_MLD); // XXX: used by MLD6 only (?)
#ifndef HAVE_RFC2292BIS
    _raopt[0] = IP6OPT_ROUTER_ALERT;
    _raopt[1] = IP6OPT_RTALERT_LEN - 2;
    memcpy(&_raopt[2], (caddr_t)&_rtalert_code, sizeof(_rtalert_code));
#endif // ! HAVE_RFC2292BIS
#endif // HAVE_IPV6
    
    _mrouter_socket = -1;
    _ioctl_socket = -1;
    _mrib_socket = -1;
    _proto_socket = -1;
    
    
    // recvmsg() and sendmsg() related initialization
    switch (family()) {
    case AF_INET:
	_rcvmh.msg_name		= (caddr_t)&_from4;
	_sndmh.msg_name		= (caddr_t)&_to4;
	_rcvmh.msg_namelen	= sizeof(_from4);
	_sndmh.msg_namelen	= sizeof(_to4);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	_rcvmh.msg_name		= (caddr_t)&_from6;
	_sndmh.msg_name		= (caddr_t)&_to6;
	_rcvmh.msg_namelen	= sizeof(_from6);
	_sndmh.msg_namelen	= sizeof(_to6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	break;
    }
    _rcvmh.msg_iov		= _rcviov;
    _sndmh.msg_iov		= _sndiov;
    _rcvmh.msg_iovlen		= 1;
    _sndmh.msg_iovlen		= 1;
    _rcviov[0].iov_base		= (caddr_t)_rcvbuf0;
    _rcviov[1].iov_base		= (caddr_t)_rcvbuf1;
    _rcviov[0].iov_len		= sizeof(_rcvbuf0);
    _rcviov[1].iov_len		= sizeof(_rcvbuf1);
    _sndiov[0].iov_base		= (caddr_t)_sndbuf0;
    _sndiov[1].iov_base		= (caddr_t)_sndbuf1;
    _sndiov[0].iov_len		= 0;
    _sndiov[1].iov_len		= 0;
    
    _rcvmh.msg_control		= (caddr_t)_rcvcmsgbuf;
    _sndmh.msg_control		= (caddr_t)_sndcmsgbuf;
    _rcvmh.msg_controllen	= sizeof(_rcvcmsgbuf);
    _sndmh.msg_controllen	= 0;
    
    _ignore_my_packets		= false;
    _allow_kernel_signal_messages = false;
    _allow_mrib_messages	= false;
    
    // Routing sockets related initialization
    _pid			= getpid();
    //_rtm_seq			= 1;	// XXX: global, hence don't initialize
    
    _mrt_api_mrt_mfc_flags_disable_wrongvif = false;
    _mrt_api_mrt_mfc_flags_border_vif = false;
    _mrt_api_mrt_mfc_rp = false;
    _mrt_api_mrt_mfc_bw_upcall = false;
}

UnixComm::~UnixComm()
{
    stop();
}

/**
 * UNUXcomm::start:
 * @void: 
 * 
 * Start the UnixComm.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
UnixComm::start(void)
{
    vector<MfeaVif *> mfea_vifs_array;
    vector<MfeaVif *>::iterator mfea_vifs_iter;
    
    // XXX: all UnixComm are automatically enabled by default
    ProtoUnit::enable();
    
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    if (_module_id == X_MODULE_NULL) {
	// XXX: the special UnixComm that takes care of house-keeping
	
	// Open IOCTL socket used for ioctl() access to the kernel
	if (open_ioctl_socket() < 0)
	    return (XORP_ERROR);
	
	//
	// Get the network interface information from the kernel,
	// and use that information to create the MfeaVif array in MfeaNode.
	//
	do {
	    if (get_mcast_vifs(mfea_vifs_array) < 0)
		return (XORP_ERROR);
	    
	    mfea_node().delete_all_vifs();
	    
	    for (mfea_vifs_iter = mfea_vifs_array.begin();
		 mfea_vifs_iter != mfea_vifs_array.end();
		 ++mfea_vifs_iter) {
		MfeaVif *mfea_vif = *mfea_vifs_iter;
		mfea_node().add_vif(*mfea_vif);
	    }
	    
	    //
	    // Create the PIM Register vif if there is a valid IP address
	    //
	    // TODO: check with Linux, Solaris, etc, if we can
	    // use 127.0.0.2 or ::2 as a PIM Register vif address, and use that
	    // address instead (otherwise we may always have to keep track
	    // whether the underlying address has changed).
	    //
	    IPvX pim_register_vif_addr(IPvX::ZERO(family()));
	    uint16_t pif_index = 0;
	    for (uint16_t i = 0; i < mfea_node().maxvifs(); i++) {
		MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(i);
		if (mfea_vif == NULL)
		    continue;
		if (! mfea_vif->is_underlying_vif_up())
		    continue;
		if (mfea_vif->addr_ptr() == NULL)
		    continue;
		if (mfea_vif->is_pim_register())
		    continue;
		if (mfea_vif->is_loopback())
		    continue;
		if (! mfea_vif->is_multicast_capable())
		    continue;
		// Found appropriate local address.
		pim_register_vif_addr = *(mfea_vif->addr_ptr());
		pif_index = mfea_vif->pif_index();
		break;
	    }
	    if (pim_register_vif_addr != IPvX::ZERO(family())) {
		// Add the Register vif
		uint16_t vif_index = mfea_node().maxvifs();	// XXX
		// TODO: XXX: the Register vif name is hardcoded here!
		MfeaVif register_vif(mfea_node(), Vif("register_vif"));
		register_vif.set_vif_index(vif_index);
		register_vif.set_pif_index(pif_index);
		register_vif.set_underlying_vif_up(true); // XXX: 'true' to allow creation
		register_vif.set_pim_register(true);
		register_vif.add_address(pim_register_vif_addr,
					 IPvXNet(pim_register_vif_addr,
						 pim_register_vif_addr.addr_bitlen()),
						 pim_register_vif_addr,
						 IPvX::ZERO(family()));
		mfea_node().add_vif(register_vif);
	    }
	    
	    // Delete the old MfeaVif storage
	    delete_pointers_vector(mfea_vifs_array);
	} while (false);
	
	// Check if we have the necessary permission
	if (geteuid() != 0) {
	    XLOG_ERROR("Must be root");
	    exit (1);
	    //return (XORP_ERROR);
	}
	
	// Open kernel multicast routing access socket
	if (open_mrouter_socket() < 0)
	    return (XORP_ERROR);
	set_other_mrouter_socket();
	
	// Open socket to obtain MRIB information
	if (open_mrib_socket() < 0) {
	    return (XORP_ERROR);
	}
	
	// Start the multicast routing in the kernel.
	if (start_mrt() < 0)
	    return (XORP_ERROR);
    }
    
    if (_ipproto >= 0) {
	set_my_mrouter_socket();	// XXX
	if (open_proto_socket() < 0) {
	    stop();
	    return (XORP_ERROR);
	}
	if (_ipproto == IPPROTO_PIM) {
	    if (start_pim() < 0) {
		stop();
		return (XORP_ERROR);
	    }
	}
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::stop:
 * @void: 
 * 
 * Stop the UnixComm.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::stop(void)
{
    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    if (_ipproto == IPPROTO_PIM) {
	stop_pim();
    }
    close_proto_socket();
    if (_module_id == X_MODULE_NULL) {
	stop_mrt();
	close_mrib_socket();
	close_mrouter_socket();
	set_other_mrouter_socket();
	close_ioctl_socket();
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::get_mcast_vifs:
 * @mfea_vifs_vector: Reference to the vector to store the created vifs.
 * 
 * Query the kernel to find network interfaces that are multicast-capable.
 * 
 * Return value: The number of created vifs on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mcast_vifs(vector<MfeaVif *>& mfea_vifs_vector)
{
    if (get_mcast_vifs_osdep(mfea_vifs_vector) < 0) {
	return (XORP_ERROR);
    }
    
    return (mfea_vifs_vector.size());
}

/**
 * UnixComm::get_mrib:
 * @dest_addr: The destination address to lookup.
 * @mrib: A reference to a #Mrib structure to return the MRIB information.
 * 
 * Get the Multicast Routing Information Base (MRIB) information for a given
 * address.
 * The MRIB information contains the next hop router and the vif to send
 * out packets toward a given destination.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_mrib(const IPvX& dest_addr, Mrib& mrib)
{
    if (get_mrib_osdep(dest_addr, mrib) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::get_mrib_table:
 * @return_mrib_table: A pointer to the routing table array composed
 * of #Mrib elements.
 * 
 * Get all MRIB information from the kernel.
 * 
 * Return value: The number of entries in @return_mrib_table, or %XORP_ERROR
 * if there was an error.
 **/
int
UnixComm::get_mrib_table(Mrib **return_mrib_table)
{
    int ret_value = get_mrib_table_osdep(return_mrib_table);
    
    if (ret_value < 0)
	return (XORP_ERROR);
    
    return (ret_value);
}

/**
 * UnixComm::open_ioctl_socket:
 * @void: 
 * 
 * Open an ioctl socket (used for various ioctl() calls).
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_ioctl_socket(void)
{
    if (_ioctl_socket >= 0)
	return (_ioctl_socket);	// The socket was open already
    
    // Open the socket
    _ioctl_socket = socket(family(), SOCK_DGRAM, 0);
    if (_ioctl_socket < 0) {
	_ioctl_socket = -1;
	XLOG_ERROR("Cannot open ioctl socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    return (_ioctl_socket);
}

/**
 * UnixComm::close_ioctl_socket:
 * @void: 
 * 
 * Close the ioctl socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::close_ioctl_socket(void)
{
    if (_ioctl_socket < 0)
	return (XORP_ERROR);
    
    // Remove it just in case, even though it may not be select()-ed
    mfea_node().event_loop().remove_selector(_ioctl_socket);
    
    // Close the socket and reset it to -1
    if (close(_ioctl_socket) < 0) {
	_ioctl_socket = -1;
	XLOG_ERROR("Cannot close ioctl socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    _ioctl_socket = -1;
    
    return (XORP_OK);
}

/**
 * UnixComm::open_mrib_socket:
 * @void: 
 * 
 * Open a MRIB socket (used for obtaining Multicast Routing Information Base
 * information).
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrib_socket(void)
{
    if (_mrib_socket >= 0)
	return (_mrib_socket);	// The socket was open already
    
    // Open the socket
    _mrib_socket = open_mrib_socket_osdep();
    if (_mrib_socket < 0) {
	_mrib_socket = -1;
	XLOG_ERROR("Cannot open MRIB socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    return (_mrib_socket);
}

/**
 * UnixComm::close_mrib_socket:
 * @void: 
 * 
 * Close the MRIB socket (used for access/lookup the unicast routing table
 * in the kernel to obtain Multicast Routing Information Base information).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::close_mrib_socket(void)
{
    if (_mrib_socket < 0)
	return (XORP_ERROR);
    
    // Remove it just in case, even though it may not be select()-ed
    mfea_node().event_loop().remove_selector(_mrib_socket);

    // Close the socket and reset it to -1
    if (close(_mrib_socket) < 0) {
	_mrib_socket = -1;
	XLOG_ERROR("Cannot close MRIB socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    _mrib_socket = -1;
    
    return (XORP_OK);
}


/**
 * UnixComm::open_mrouter_socket:
 * @void: 
 * 
 * Open the mrouter socket (used for various multicast routing kernel calls).
 * 
 * Return value: The socket value on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_mrouter_socket(void)
{
    int kernel_mrouter_ipproto = -1;
    
    if (_mrouter_socket >= 0)
	return (_mrouter_socket);
    
    // The socket protocol is different for IPv4 and IPv6
    switch (family()) {
    case AF_INET:
	kernel_mrouter_ipproto = IPPROTO_IGMP;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	kernel_mrouter_ipproto = IPPROTO_ICMPV6;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    if ( (_mrouter_socket = socket(family(), SOCK_RAW, kernel_mrouter_ipproto))
	 < 0) {
	XLOG_ERROR("Cannot open mrouter socket: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    return (_mrouter_socket);
}


/**
 * UnixComm::close_mrouter_socket:
 * @void: 
 * 
 * Close the mrouter socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::close_mrouter_socket(void)
{
    if (_mrouter_socket < 0)
	return (XORP_ERROR);
    
    if (_module_id == X_MODULE_NULL) {
	// XXX: the special UnixComm that takes care of house-keeping
	// XXX: this special UnixComm only would open/close the mrouter_socket

	// Remove it just in case, even though it may not be select()-ed
	mfea_node().event_loop().remove_selector(_mrouter_socket);
	
	// Close the socket and reset it to -1
	if (close(_mrouter_socket) < 0) {
	    _mrouter_socket = -1;
	    XLOG_ERROR("Cannot close mrouter socket: %s", strerror(errno));
	    return (XORP_ERROR);
	}
    }
    
    _mrouter_socket = -1;
    
    return (XORP_OK);
}


/**
 * UnixComm::set_my_mrouter_socket:
 * @void: 
 * 
 * Set my mrouter socket by copying the mrouter socket value from the special
 * house-keeping #UnixComm with module_id of %X_MODULE_NULL.
 * 
 * Return value: The value of the copied mrouter socket on success,
 * otherwise %XORP_ERROR.
 **/
int
UnixComm::set_my_mrouter_socket(void)
{
    UnixComm *unix_comm;
    
    if (_module_id == X_MODULE_NULL)
	return (XORP_ERROR);	// XXX: the special house-keeping UnixComm
    
    
    // Copy the value from the special house-keeping UnixComm
    unix_comm = mfea_node().unix_comm_find_by_module_id(X_MODULE_NULL);
    if (unix_comm == NULL)
	return (XORP_ERROR);
    
    // If necessary, close the old mrouter socket
    if ((_mrouter_socket >= 0)
	&& (_mrouter_socket != unix_comm->mrouter_socket())) {
	close_mrouter_socket();
    }
    
    _mrouter_socket = unix_comm->mrouter_socket();
    
    if (_mrouter_socket >= 0)
	return (_mrouter_socket);
    else
	return (XORP_ERROR);
}


/**
 * UnixComm::set_other_mrouter_socket:
 * @void: 
 * 
 * Set the mrouter socket value of the other #UnixComm entries.
 * The mrouter socket value of the other #UnixComm entries is
 * set by copying the mrouter socket value of this entry.
 * Note that this method should be applied only to the special
 * house-keeping @ref UnixComm with module ID (#x_module_id)
 * of %X_MODULE_NULL.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::set_other_mrouter_socket(void)
{
    if (_module_id != X_MODULE_NULL)
	return (XORP_ERROR);	// XXX: not the special house-keeping UnixComm
    
    // Copy my _mrouter_socket value to the other UnixComm
    for (size_t i = 0; i < mfea_node().unix_comms().size(); i++) {
	UnixComm *unix_comm = mfea_node().unix_comms()[i];
	if (unix_comm == NULL)
	    continue;
	if (unix_comm == this)
	    continue;
	unix_comm->set_my_mrouter_socket();
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::set_rcvbuf:
 * @recv_socket: The socket whose receiving buffer size to set.
 * @desired_bufsize: The preferred buffer size.
 * @min_bufsize: The smallest acceptable buffer size.
 * 
 * Set the receiving buffer size of a socket.
 * 
 * Return value: The successfully set buffer size on success,
 * otherwise %XORP_ERROR.
 **/
int
UnixComm::set_rcvbuf(int recv_socket, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;
    
    //
    // Set the socket buffer size.  If we can't set it as large as we
    // want, search around to try to find the highest acceptable
    // value.  The highest acceptable value being smaller than
    // minsize is a fatal error.
    //
    if (setsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF,
		   (void *)&desired_bufsize, sizeof(desired_bufsize)) < 0) {
	desired_bufsize -= delta;
	while (true) {
	    if (delta > 1)
		delta /= 2;
	    
	    if (setsockopt(recv_socket, SOL_SOCKET, SO_RCVBUF,
			   (void *)&desired_bufsize, sizeof(desired_bufsize))
		< 0) {
		desired_bufsize -= delta;
	    } else {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) {
	    XLOG_ERROR("Cannot set receiving buffer size of socket %d: "
		       "desired buffer size %u < minimum allowed %u",
		       recv_socket, desired_bufsize, min_bufsize);
	    return (XORP_ERROR);
	}
    }
    
    return (desired_bufsize);
}


/**
 * UnixComm::ip_hdr_include:
 * @enable_bool: If true, set the option, otherwise reset it.
 * 
 * Set/reset the "Header Included" option (for IPv4) on the protocol socket.
 * If set, the IP header of a raw packet should be created
 * by the application itself, otherwise the kernel will build it.
 * XXX: Used only for IPv4.
 * In post-RFC-2292, IPV6_PKTINFO has similar functions,
 * but because it requires the interface index and outgoing address,
 * it is of little use for our purpose. Also, in RFC-2292 this option
 * was a flag, so for compatibility reasons we better not set it
 * here; instead, we will use sendmsg() to specify the header's field values. 
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::ip_hdr_include(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
    {
#ifdef IP_HDRINCL
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = enable_bool; 
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_HDRINCL,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_HDRINCL, %u) failed: %s", bool_flag,
		       strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IP_HDRINCL
    }
    break;
    
#ifdef HAVE_IPV6
    case AF_INET6:
	break;		// XXX
#endif // HAVE_IPV6
	
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(enable_bool);
}


/**
 * UnixComm::recv_pktinfo:
 * @enable_bool: If true, set the option, otherwise reset it.
 * 
 * Enable/disable receiving information about some of the fields
 * in the IP header on the protocol socket.
 * If enabled, values such as interface index, destination address and
 * IP TTL (a.k.a. hop-limit in IPv6) will be received as well.
 * XXX: used only for IPv6. In IPv4 we don't have this; the whole IP
 * packet is passed to the application listening on a raw socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::recv_pktinfo(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
	break;
	
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = enable_bool;
	
	//
	// Interface index and address
	//
#ifdef IPV6_RECVPKTINFO
	// The new option (applies to receiving only)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVPKTINFO, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	// The old option (see RFC 2292)
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_PKTINFO,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_PKTINFO, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVPKTINFO
	
	//
	// Hop-limit field
	//
#ifdef IPV6_RECVHOPLIMIT
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVHOPLIMIT,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVHOPLIMIT, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#else
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_HOPLIMIT,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_HOPLIMIT, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVHOPLIMIT
	
	//
	// Traffic class value
	//
#ifdef IPV6_RECVTCLASS
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_RECVTCLASS,
		       (void *)&bool_flag, sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_RECVTCLASS, %u) failed: %s",
		       bool_flag, strerror(errno));
	    return (XORP_ERROR);
	}
#endif // IPV6_RECVTCLASS
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(enable_bool);
}


/**
 * UnixComm::set_mcast_ttl:
 * @ttl: The desired IP TTL (a.k.a. hop-limit in IPv6) value.
 * 
 * Set the default TTL (or hop-limit in IPv6) for the outgoing multicast
 * packets on the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::set_mcast_ttl(int ttl)
{
    switch (family()) {
    case AF_INET:
    {
	u_char ip_ttl = ttl; // XXX: In IPv4 the value argument is 'u_char'
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_TTL,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_TTL, %u) failed: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	int ip_ttl = ttl;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_HOPS, %u) failed: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::set_multicast_loop:
 * @enable_bool: If true, set the loopback, otherwise reset it.
 * 
 * Set/reset the "Multicast Loop" flag on the protocol socket.
 * If the multicast loopback flag is set, a multicast datagram sent on
 * that socket will be delivered back to this host (assuming the host
 * is a member of the same multicast group).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::set_multicast_loop(bool enable_bool)
{
    switch (family()) {
    case AF_INET:
    {
	u_char loop = enable_bool;
	
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_LOOP,
		       (void *)&loop, sizeof(loop)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_LOOP, %u) failed: %s",
		       loop, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	uint loop6 = enable_bool;
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       (void *)&loop6, sizeof(loop6)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_LOOP, %u) failed: %s",
		       loop6, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::start_mrt:
 * @void: 
 * 
 * Start/enable the multicast routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::start_mrt(void)
{
    int v = 1;
    
    switch (family()) {
    case AF_INET:
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_INIT,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_INIT, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_INIT,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_INIT, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    //
    // Configure advanced multicast API
    //
#if defined(MRT_API_CONFIG) && defined(ENABLE_ADVANCED_MCAST_API)
    if (family() == AF_INET) {
	uint32_t mrt_api = 0;
	
	//
	// Set the desired API
	//
#ifdef MRT_MFC_FLAGS_DISABLE_WRONGVIF
	// Try to enable the support for disabling WRONGVIF signals per vif
	mrt_api |= MRT_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
#ifdef MRT_MFC_FLAGS_BORDER_VIF
	// Try to enable the border bit flag (per MFC per vif)
	mrt_api |= MRT_MFC_FLAGS_BORDER_VIF;
#endif
#ifdef MRT_MFC_RP
	// Try to enable kernel-level PIM Register encapsulation
	mrt_api |= MRT_MFC_RP;
#endif
#ifdef MRT_MFC_BW_UPCALL
	// Try to enable bandwidth-related upcalls from the kernel
	mrt_api |= MRT_MFC_BW_UPCALL;
#endif
	
	//
	// Try to configure the kernel with the desired API
	//
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_API_CONFIG,
		       (void *)&mrt_api, sizeof(mrt_api)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_API_CONFIG) failed: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	
	//
	// Test which of the desired API support succeded
	//
#ifdef MRT_MFC_FLAGS_DISABLE_WRONGVIF
	// Test the support for disabling WRONGVIF signals per vif
	if (mrt_api & MRT_MFC_FLAGS_DISABLE_WRONGVIF)
	    _mrt_api_mrt_mfc_flags_disable_wrongvif = true;
#endif
#ifdef MRT_MFC_FLAGS_BORDER_VIF
	// Test the support for the border bit flag (per MFC per vif)
	if (mrt_api & MRT_MFC_FLAGS_BORDER_VIF)
	    _mrt_api_mrt_mfc_flags_border_vif = true;
#endif
#ifdef MRT_MFC_RP
	// Test the support for kernel-level PIM Register encapsulation
	if (mrt_api & MRT_MFC_RP)
	    _mrt_api_mrt_mfc_rp = true;
#endif
#ifdef MRT_MFC_BW_UPCALL
	// Test the support for bandwidth-related upcalls from the kernel
	if (mrt_api & MRT_MFC_BW_UPCALL)
	    _mrt_api_mrt_mfc_bw_upcall = true;
#endif
	
    }
#endif // MRT_API_CONFIG && ENABLE_ADVANCED_MCAST_API
    
#if defined(MRT6_API_CONFIG) && defined(ENABLE_ADVANCED_MCAST_API)
#ifdef HAVE_IPV6
    if (family == AF_INET6) {
	uint32_t mrt_api = 0;
	
	//
	// Set the desired API
	//
#ifdef MRT6_MFC_FLAGS_DISABLE_WRONGVIF
	// Try to enable the support for disabling WRONGVIF signals per vif
	mrt_api |= MRT6_MFC_FLAGS_DISABLE_WRONGVIF;
#endif
#ifdef MRT6_MFC_FLAGS_BORDER_VIF
	// Try to enable the border bit flag (per MFC per vif)
	mrt_api |= MRT6_MFC_FLAGS_BORDER_VIF;
#endif
#ifdef MRT6_MFC_RP
	// Try to enable kernel-level PIM Register encapsulation
	mrt_api |= MRT6_MFC_RP;
#endif
#ifdef MRT6_MFC_BW_UPCALL
	// Try to enable bandwidth-related upcalls from the kernel
	mrt_api |= MRT6_MFC_BW_UPCALL;
#endif
	
	//
	// Try to configure the kernel with the desired API
	//
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_API_CONFIG,
		       (void *)&mrt_api, sizeof(mrt_api)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_API_CONFIG) failed: %s",
		       strerror(errno));
	    return (XORP_ERROR);
	}
	
	//
	// Test which of the desired API support succeded
	//
#ifdef MRT6_MFC_FLAGS_DISABLE_WRONGVIF
	// Test the support for disabling WRONGVIF signals per vif
	if (mrt_api & MRT6_MFC_FLAGS_DISABLE_WRONGVIF)
	    _mrt_api_mrt_mfc_flags_disable_wrongvif = true;
#endif
#ifdef MRT6_MFC_FLAGS_BORDER_VIF
	// Test the support for the border bit flag (per MFC per vif)
	if (mrt_api & MRT6_MFC_FLAGS_BORDER_VIF)
	    _mrt_api_mrt_mfc_flags_border_vif = true;
#endif
#ifdef MRT6_MFC_RP
	// Test the support for kernel-level PIM Register encapsulation
	if (mrt_api & MRT6_MFC_RP)
	    _mrt_api_mrt_mfc_rp = true;
#endif
#ifdef MRT6_MFC_BW_UPCALL
	// Test the support for bandwidth-related upcalls from the kernel
	if (mrt_api & MRT6_MFC_BW_UPCALL)
	    _mrt_api_mrt_mfc_bw_upcall = true;
#endif
	
    }
#endif // HAVE_IPV6
#endif // MRT6_API_CONFIG && ENABLE_ADVANCED_MCAST_API
    
    return (XORP_OK);
}


/**
 * UnixComm::stop_mrt:
 * @void: 
 * 
 * Stop/disable the multicast routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::stop_mrt(void)
{
    int v = 0;
    
    _mrt_api_mrt_mfc_flags_disable_wrongvif = false;
    _mrt_api_mrt_mfc_flags_border_vif = false;
    _mrt_api_mrt_mfc_rp = false;
    _mrt_api_mrt_mfc_bw_upcall = false;
    
    if (_mrouter_socket < 0)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_INIT,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_INIT, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_INIT,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_INIT, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::start_pim:
 * @void: 
 * 
 * Start/enable PIM routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::start_pim(void)
{
    int v = 1;
    
    switch (family()) {
    case AF_INET:
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_PIM, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_PIM, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::stop_pim:
 * @void: 
 * 
 * Stop/disable PIM routing in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::stop_pim(void)
{
    int v = 0;
    
    if (_mrouter_socket < 0)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_PIM, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_PIM,
		       (void *)&v, sizeof(v)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_PIM, %u) failed: %s",
		       v, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::set_default_multicast_vif:
 * @vif_index: The vif index of the interface to become the default
 * multicast interface.
 *
 * Set default interface for outgoing multicast on the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::set_default_multicast_vif(uint16_t vif_index)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);

    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
    {
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("set_default_multicast_vif() failed: "
		       "vif %s has no address",
		       mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_MULTICAST_IF,
		       (void *)&in_addr, sizeof(in_addr)) < 0) {
	    XLOG_ERROR("setsockopt(IP_MULTICAST_IF, %s) failed: %s",
		       cstring(*mfea_vif->addr_ptr()), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	u_int pif_index = mfea_vif->pif_index();
	
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		       (void *)&pif_index, sizeof(pif_index)) < 0) {
	    XLOG_ERROR("setsockopt(IPV6_MULTICAST_IF, %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm:join_multicast_group:
 * @vif_index: The vif index of the interface to join the multicast group.
 * @group: The multicast group to join.
 * 
 * Join a multicast group on an interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::join_multicast_group(uint16_t vif_index, const IPvX& group)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
#if 0	// TODO: enable or disable the mfea_vif->is_up() check??
    if (! mfea_vif->is_up()) {
	XLOG_ERROR("Cannot join group %s on vif %s: interface is DOWN",
		   cstring(group), mfea_vif->name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("Cannot join group %s on vif %s: "
		       "interface has no address",
		       cstring(group), mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		       (void *)&mreq, sizeof(mreq)) < 0) {
	    XLOG_ERROR("Cannot join group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = mfea_vif->pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		       (void *)&mreq6, sizeof(mreq6)) < 0) {
	    XLOG_ERROR("Cannot join group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * UnixComm:leave_multicast_group:
 * @vif_index: The vif index of the interface to leave the multicast group.
 * @group: The multicast group to leave.
 * 
 * Leave a multicast group on an interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::leave_multicast_group(uint16_t vif_index, const IPvX& group)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
#if 0	// TODO: enable or disable the mfea_vif->is_up() check??
    if (! mfea_vif->is_up()) {
	XLOG_ERROR("Cannot leave group %s on vif %s: interface is DOWN",
		   cstring(group), mfea_vif->name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1
    
    switch (family()) {
    case AF_INET:
    {
	struct ip_mreq mreq;
	struct in_addr in_addr;
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: "
		       "interface has no address",
		       cstring(group), mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(in_addr);
	group.copy_out(mreq.imr_multiaddr);
	mreq.imr_interface.s_addr = in_addr.s_addr;
	if (setsockopt(_proto_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		       (void *)&mreq, sizeof(mreq)) < 0) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct ipv6_mreq mreq6;
	
	group.copy_out(mreq6.ipv6mr_multiaddr);
	mreq6.ipv6mr_interface = mfea_vif->pif_index();
	if (setsockopt(_proto_socket, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
		       (void *)&mreq6, sizeof(mreq6)) < 0) {
	    XLOG_ERROR("Cannot leave group %s on vif %s: %s",
		       cstring(group), mfea_vif->name().c_str(), 
		       strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::add_multicast_vif:
 * @vif_index: The vif index of the virtual interface to add.
 * 
 * Add a virtual multicast interface to the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::add_multicast_vif(uint16_t vif_index)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
    {
	struct vifctl vc;
	
	memset(&vc, 0, sizeof(vc));
	vc.vifc_vifi = mfea_vif->vif_index();
	// XXX: we don't (need to) support VIFF_TUNNEL; VIFF_SRCRT is obsolete
	vc.vifc_flags = 0;
	if (mfea_vif->is_pim_register())
	    vc.vifc_flags	|= VIFF_REGISTER;
	vc.vifc_threshold	= mfea_vif->min_ttl_threshold();
	vc.vifc_rate_limit	= mfea_vif->max_rate_limit();
	
	if (mfea_vif->addr_ptr() == NULL) {
	    XLOG_ERROR("add_multicast_vif() failed: vif %s has no address",
		       mfea_vif->name().c_str());
	    return (XORP_ERROR);
	}
	mfea_vif->addr_ptr()->copy_out(vc.vifc_lcl_addr);
	// TODO: do we need to copy the remote address if we don't need
	// to support IPIP tunnels?
	// vif_addr->peer_addr().copy_out(vc.vifc_rmt_addr);
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_VIF,
		       (void *)&vc, sizeof(vc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_ADD_VIF, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
#ifdef HOST_OS_LINUX
	// Get the vif index if available
	if (! mfea_vif->is_pim_register()) {
	    struct ifreq ifr;
	    // TODO: fix this for Linux (e.g., make sure that
	    // the ioctl socket is open
	    
	    memset(&ifr, 0, sizeof(struct ifreq));
	    strncpy(ifr.ifr_name, mfea_vif->name().c_str(), IFNAMSIZ);
	    if (ioctl(_ioctl_socket, SIOCGIFINDEX, &ifr) < 0) {
		XLOG_ERROR("ioctl(SIOCGIFINDEX, vif %s) failed: %s",
			    mfea_vif->name().c_str(), strerror(errno));
		return (XORP_ERROR);
	    }
	    mfea_vif->set_pif_index(ifr.ifr_ifindex);
	}
#endif // HOST_OS_LINUX
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct mif6ctl mc;
	
	memset(&mc, 0, sizeof(mc));
	mc.mif6c_mifi = mfea_vif->vif_index();
	mc.mif6c_flags = 0;
	if (mfea_vif->is_pim_register())
	    mc.mif6c_flags |= MIFF_REGISTER;
	mc.mif6c_pifi = mfea_vif->pif_index();
#if 0		// TODO: enable it after KAME's stack defines it
	mc.mif6c_threshold = mfea_vif->min_ttl_threshold();
	mc.mif6c_rate_limit = mfea_vif->max_rate_limit();
#endif // 0
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_ADD_MIF,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_ADD_MIF, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::delete_multicast_vif:
 * @vif_index: The vif index of the interface to delete.
 * 
 * Delete a virtual multicast interface from the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::delete_multicast_vif(uint16_t vif_index)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
    {
	vifi_t vifi = mfea_vif->vif_index();
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_VIF,
		       (void *)&vifi, sizeof(vifi)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DEL_VIF, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	mifi_t vifi = mfea_vif->vif_index();
	
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_DEL_MIF,
		       (void *)&vifi, sizeof(vifi)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DEL_MIF, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::add_mfc:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * @iif_vif_index: The MFC incoming interface index.
 * @oifs_ttl: An array with the min. TTL a packet should have to be forwarded.
 * @oifs_flags: An array with misc. flags for the MFC to install.
 * Note that those flags are supported only by the advanced multicast API.
 * @rp_addr: The RP address.
 * 
 * Install/modify a Multicast Forwarding Cache (MFC) entry in the kernel.
 * If the MFC entry specified by (@source, @group) pair was not
 * installed before, a new MFC entry will be created in the kernel;
 * otherwise, the existing entry's fields will be modified.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::add_mfc(const IPvX& source, const IPvX& group,
		  uint16_t iif_vif_index, uint8_t *oifs_ttl,
		  uint8_t *oifs_flags,
		  const IPvX& rp_addr)
{
    if (iif_vif_index >= mfea_node().maxvifs())
	return (XORP_ERROR);
    
    oifs_ttl[iif_vif_index] = 0;		// Pre-caution
    
    switch (family()) {
    case AF_INET:
    {
#if defined(HAVE_MFCCTL2) && defined(ENABLE_ADVANCED_MCAST_API)
	struct mfcctl2 mc;
#else
	struct mfcctl mc;
#endif
	
	memset(&mc, 0, sizeof(mc));
	source.copy_out(mc.mfcc_origin);
	group.copy_out(mc.mfcc_mcastgrp);
	mc.mfcc_parent = iif_vif_index;
	for (uint16_t i = 0; i < mfea_node().maxvifs(); i++) {
	    mc.mfcc_ttls[i] = oifs_ttl[i];
#if defined(HAVE_MFCC_FLAGS) && defined(ENABLE_ADVANCED_MCAST_API)
	    mc.mfcc_flags[i] = oifs_flags[i];
#endif
	}
#if defined(HAVE_MFCC_RP) && defined(ENABLE_ADVANCED_MCAST_API)
	if (_mrt_api_mrt_mfc_rp)
	    rp_addr.copy_out(mc.mfcc_rp);
#endif
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_ADD_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#if defined(HAVE_MF6CCTL2) && defined(ENABLE_ADVANCED_MCAST_API)
	struct mf6cctl2 mc;
#else
	struct mf6cctl mc;
#endif
	
	memset(&mc, 0, sizeof(mc));
	IF_ZERO(&mc.mf6cc_ifset);	
	source.copy_out(mc.mf6cc_origin);
	group.copy_out(mc.mf6cc_mcastgrp);
	mc.mf6cc_parent = iif_vif_index;
	for (uint16_t i = 0; i < mfea_node().maxvifs(); i++) {
	    if (oifs_ttl[i] > 0)
		IF_SET(i, &mc.mf6cc_ifset);
#if defined(HAVE_MF6CC_FLAGS) && defined(ENABLE_ADVANCED_MCAST_API)
	    mc.mf6cc_flags[i] = oifs_flags[i];
#endif
	}
#if defined(HAVE_MF6CC_RP) && defined(ENABLE_ADVANCED_MCAST_API)
	if (_mrt_api_mrt_mfc_rp)
	    rp_addr.copy_out(mc.mf6cc_rp);
#endif
	
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_ADD_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_ADD_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(oifs_flags);
    UNUSED(rp_addr);
}


/**
 * UnixComm::delete_mfc:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * 
 * Delete a Multicast Forwarding Cache (MFC) entry in the kernel.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::delete_mfc(const IPvX& source, const IPvX& group)
{
    switch (family()) {
    case AF_INET:
    {
	struct mfcctl mc;
	
	source.copy_out(mc.mfcc_origin);
	group.copy_out(mc.mfcc_mcastgrp);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DEL_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct mf6cctl mc;

	source.copy_out(mc.mf6cc_origin);
	group.copy_out(mc.mf6cc_mcastgrp);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IPV6, MRT6_DEL_MFC,
		       (void *)&mc, sizeof(mc)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DEL_MFC, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

/**
 * UnixComm::add_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * 
 * Add a dataflow monitor entry in the kernel.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::add_bw_upcall(const IPvX& source, const IPvX& group,
			const struct timeval& threshold_interval,
			uint32_t threshold_packets,
			uint32_t threshold_bytes,
			bool is_threshold_in_packets,
			bool is_threshold_in_bytes,
			bool is_geq_upcall,
			bool is_leq_upcall)
{
    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	XLOG_ERROR("add_bw_upcall(%s,%s) failed: "
		   "dataflow monitor entry in the kernel is not supported",
		   cstring(source), cstring(group));
	return (XORP_ERROR);
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw_upcall_src);
	group.copy_out(bw_upcall.bw_upcall_dst);
	bw_upcall.bw_upcall_threshold_interval = threshold_interval;
	bw_upcall.bw_upcall_threshold_packets = threshold_packets;
	bw_upcall.bw_upcall_threshold_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bw_upcall_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bw_upcall_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bw_upcall_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bw_upcall_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_ADD_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_ADD_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw6_upcall_src);
	group.copy_out(bw_upcall.bw6_upcall_dst);
	bw_upcall.bw6_upcall_threshold_interval = threshold_interval;
	bw_upcall.bw6_upcall_threshold_packets = threshold_packets;
	bw_upcall.bw6_upcall_threshold_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bw6_upcall_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bw6_upcall_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bw6_upcall_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bw6_upcall_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_ADD_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_ADD_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(threshold_interval);
    UNUSED(threshold_packets);
    UNUSED(threshold_bytes);
    UNUSED(is_threshold_in_packets);
    UNUSED(is_threshold_in_bytes);
    UNUSED(is_geq_upcall);
    UNUSED(is_leq_upcall);
    
    return (XORP_OK);
}

/**
 * UnixComm::delete_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * @threshold_interval: The dataflow threshold interval.
 * @threshold_packets: The threshold (in number of packets) to compare against.
 * @threshold_bytes: The threshold (in number of bytes) to compare against.
 * @is_threshold_in_packets: If true, @threshold_packets is valid.
 * @is_threshold_in_bytes: If true, @threshold_bytes is valid.
 * @is_geq_upcall: If true, the operation for comparison is ">=".
 * @is_leq_upcall: If true, the operation for comparison is "<=".
 * 
 * Delete a dataflow monitor entry from the kernel.
 * Note: either @is_threshold_in_packets or @is_threshold_in_bytes (or both)
 * must be true.
 * Note: either @is_geq_upcall or @is_leq_upcall (but not both) must be true.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::delete_bw_upcall(const IPvX& source, const IPvX& group,
			   const struct timeval& threshold_interval,
			   uint32_t threshold_packets,
			   uint32_t threshold_bytes,
			   bool is_threshold_in_packets,
			   bool is_threshold_in_bytes,
			   bool is_geq_upcall,
			   bool is_leq_upcall)
{
    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	XLOG_ERROR("add_bw_upcall(%s,%s) failed: "
		   "dataflow monitor entry in the kernel is not supported",
		   cstring(source), cstring(group));
	return (XORP_ERROR);
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw_upcall_src);
	group.copy_out(bw_upcall.bw_upcall_dst);
	bw_upcall.bw_upcall_threshold_interval = threshold_interval;
	bw_upcall.bw_upcall_threshold_packets = threshold_packets;
	bw_upcall.bw_upcall_threshold_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bw_upcall_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bw_upcall_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bw_upcall_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bw_upcall_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DEL_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw6_upcall_src);
	group.copy_out(bw_upcall.bw6_upcall_dst);
	bw_upcall.bw6_upcall_threshold_interval = threshold_interval;
	bw_upcall.bw6_upcall_threshold_packets = threshold_packets;
	bw_upcall.bw6_upcall_threshold_bytes = threshold_bytes;
	if (is_threshold_in_packets)
	    bw_upcall.bw6_upcall_flags |= BW_UPCALL_UNIT_PACKETS;
	if (is_threshold_in_bytes)
	    bw_upcall.bw6_upcall_flags |= BW_UPCALL_UNIT_BYTES;
	do {
	    if (is_geq_upcall) {
		bw_upcall.bw6_upcall_flags |= BW_UPCALL_GEQ;
		break;
	    }
	    if (is_leq_upcall) {
		bw_upcall.bw6_upcall_flags |= BW_UPCALL_LEQ;
		break;
	    }
	    return (XORP_ERROR);
	} while (false);
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DEL_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
    
    UNUSED(threshold_interval);
    UNUSED(threshold_packets);
    UNUSED(threshold_bytes);
    UNUSED(is_threshold_in_packets);
    UNUSED(is_threshold_in_bytes);
    UNUSED(is_geq_upcall);
    UNUSED(is_leq_upcall);
    
    return (XORP_OK);
}

/**
 * UnixComm::delete_all_bw_upcall:
 * @source: The source address.
 * @group: The group address.
 * 
 * Delete all dataflow monitor entries from the kernel
 * for a given @source and @group address.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::delete_all_bw_upcall(const IPvX& source, const IPvX& group)
{
    //
    // Check if the kernel supports the bandwidth-upcall mechanism.
    //
    if (! mrt_api_mrt_mfc_bw_upcall()) {
	XLOG_ERROR("add_bw_upcall(%s,%s) failed: "
		   "dataflow monitor entry in the kernel is not supported",
		   cstring(source), cstring(group));
	return (XORP_ERROR);
    }
    
    //
    // Do the job
    //
    switch (family()) {
    case AF_INET:
    {
#if defined(MRT_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw_upcall_src);
	group.copy_out(bw_upcall.bw_upcall_dst);
	bw_upcall.bw_upcall_flags |= BW_UPCALL_DELETE_ALL;
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT_DEL_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
#if defined(MRT6_ADD_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	struct bw6_upcall bw_upcall;
	
	//
	// Set the argument
	//
	memset(&bw_upcall, 0, sizeof(bw_upcall));
	source.copy_out(bw_upcall.bw6_upcall_src);
	group.copy_out(bw_upcall.bw6_upcall_dst);
	bw_upcall.bw6_upcall_flags |= BW_UPCALL_DELETE_ALL;
	
	if (setsockopt(_mrouter_socket, IPPROTO_IP, MRT6_DEL_BW_UPCALL,
		       (void *)&bw_upcall, sizeof(bw_upcall)) < 0) {
	    XLOG_ERROR("setsockopt(MRT6_DEL_BW_UPCALL, (%s, %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    return (XORP_ERROR);
	}
#endif // MRT6_ADD_BW_UPCALL && ENABLE_ADVANCED_MCAST_API
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::get_sg_count:
 * @source: The MFC source address.
 * @group: The MFC group address.
 * @sg_count: A reference to a #SgCount class to place the result.
 * 
 * Get various counters per (S,G) entry.
 * Get the number of packets and bytes forwarded by a particular
 * Multicast Forwarding Cache (MFC) entry in the kernel, and the number
 * of packets arrived on wrong interface for that entry.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_sg_count(const IPvX& source, const IPvX& group,
		       SgCount& sg_count)
{
    switch (family()) {
    case AF_INET:
    {
	struct sioc_sg_req sgreq;
	
	memset(&sgreq, 0, sizeof(sgreq));
	source.copy_out(sgreq.src);
	group.copy_out(sgreq.grp);
	// XXX: some older mcast code has bug in ip_mroute.c, get_sg_cnt():
	// the return code is always 0, so this is why we need to check
	// if all values are 0xffffffffU (the indication for error).
	// TODO: remove the 0xffffffffU check in the future.
	//
	// XXX: Note that here we use ioctl(_mrouter_socket) instead
	// of ioctl(_ioctl_socket). On FreeBSD, either one is fine,
	// but Linux is picky...
	//
	if ((ioctl(_mrouter_socket, SIOCGETSGCNT, &sgreq) < 0)
	    || ((sgreq.pktcnt == 0xffffffffU)
		&& (sgreq.bytecnt == 0xffffffffU)
		&& (sgreq.wrong_if == 0xffffffffU))) {
	    XLOG_ERROR("ioctl(SIOCGETSGCNT, (%s %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    sg_count.set_pktcnt(~0);
	    sg_count.set_bytecnt(~0);
	    sg_count.set_wrong_if(~0);
	    return (XORP_ERROR);
	}
	sg_count.set_pktcnt(sgreq.pktcnt);
	sg_count.set_bytecnt(sgreq.bytecnt);
	sg_count.set_wrong_if(sgreq.wrong_if);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {

#ifndef SIOCGETSGCNT_IN6
	//
	// XXX: Some OS such as MacOS X 10.2.3 don't have SIOCGETSGCNT_IN6
	// TODO: for now, allow the code to compile, but abort at run time
	//
	XLOG_FATAL("The OS doesn't have SIOCGETSGCNT_IN6. Aborting...");
	return (XORP_ERROR);
	
#else // SIOCGETSGCNT_IN6
	
	struct sioc_sg_req6 sgreq;

	memset(&sgreq, 0, sizeof(sgreq));
	source.copy_out(sgreq.src);
	group.copy_out(sgreq.grp);
	//
	// XXX: Note that here we use ioctl(_mrouter_socket) instead
	// of ioctl(_ioctl_socket). On FreeBSD, either one is fine,
	// but Linux is picky...
	//
	if (ioctl(_mrouter_socket, SIOCGETSGCNT_IN6, &sgreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETSGCNT_IN6, (%s %s)) failed: %s",
		       cstring(source), cstring(group), strerror(errno));
	    sg_count.set_pktcnt(~0);
	    sg_count.set_bytecnt(~0);
	    sg_count.set_wrong_if(~0);
	    return (XORP_ERROR);
	}
	sg_count.set_pktcnt(sgreq.pktcnt);
	sg_count.set_bytecnt(sgreq.bytecnt);
	sg_count.set_wrong_if(sgreq.wrong_if);
	break;

#endif // SIOCGETSGCNT_IN6

    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::get_vif_count:
 * @vif_index: The vif index of the virtual multicast interface whose
 * statistics we need.
 * @vif_count: A reference to a #VifCount class to store the result.
 * 
 * Get various counters per virtual interface.
 * Get the number of packets and bytes received on, or forwarded on
 * a particular multicast interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::get_vif_count(uint16_t vif_index, VifCount& vif_count)
{
    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    
    switch (family()) {
    case AF_INET:
    {
	struct sioc_vif_req vreq;

	memset(&vreq, 0, sizeof(vreq));
	vreq.vifi = mfea_vif->vif_index();
	//
	// XXX: Note that here we use ioctl(_mrouter_socket) instead
	// of ioctl(_ioctl_socket). On FreeBSD, either one is fine,
	// but Linux is picky...
	//
	if (ioctl(_mrouter_socket, SIOCGETVIFCNT, &vreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETVIFCNT, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    vif_count.set_icount(~0);
	    vif_count.set_ocount(~0);
	    vif_count.set_ibytes(~0);
	    vif_count.set_obytes(~0);
	    return (XORP_ERROR);
	}
	vif_count.set_icount(vreq.icount);
	vif_count.set_ocount(vreq.ocount);
	vif_count.set_ibytes(vreq.ibytes);
	vif_count.set_obytes(vreq.obytes);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	
#ifndef SIOCGETMIFCNT_IN6
	//
	// XXX: Some OS such as MacOS X 10.2.3 don't have SIOCGETMIFCNT_IN6
	// TODO: for now, allow the code to compile, but abort at run time
	//
	XLOG_FATAL("The OS doesn't have SIOCGETMIFCNT_IN6. Aborting...");
	return (XORP_ERROR);
	
#else // SIOCGETMIFCNT_IN6

	struct sioc_mif_req6 mreq;
	
	memset(&mreq, 0, sizeof(mreq));
	mreq.mifi = mfea_vif->vif_index();
	//
	// XXX: Note that here we use ioctl(_mrouter_socket) instead
	// of ioctl(_ioctl_socket). On FreeBSD, either one is fine,
	// but Linux is picky...
	//
	if (ioctl(_mrouter_socket, SIOCGETMIFCNT_IN6, &mreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGETMIFCNT_IN6, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    vif_count.set_icount(~0);
	    vif_count.set_ocount(~0);
	    vif_count.set_ibytes(~0);
	    vif_count.set_obytes(~0);
	    return (XORP_ERROR);
	}
	vif_count.set_icount(mreq.icount);
	vif_count.set_ocount(mreq.ocount);
	vif_count.set_ibytes(mreq.ibytes);
	vif_count.set_obytes(mreq.obytes);
	break;
	
#endif // SIOCGETMIFCNT_IN6
	
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::is_multicast_capable:
 * @vif_index: The vif index of the interface to test whether is
 * multicast capable.
 * 
 * Test if an interface in the kernel is multicast-capable.
 * 
 * Return value: true if the interface is multicast-capable, otherwise false.
 **/
bool
UnixComm::is_multicast_capable(uint16_t vif_index) const
{
    struct ifreq	ifreq;
    uint16_t		flags;
    const MfeaVif	*mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (false);
    
    flags = 0;
    switch (family()) {
    case AF_INET:
	strncpy(ifreq.ifr_name, mfea_vif->name().c_str(), IFNAMSIZ);
	// Get the interface flags
	if (ioctl(_ioctl_socket, SIOCGIFFLAGS, &ifreq) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFFLAGS, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (false);
	}
	flags = ifreq.ifr_flags;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	
#ifdef HOST_OS_MACOSX
	//
	// XXX: Some OS such as MacOS X 10.2.3 don't have struct in6_ifreq
	// TODO: for now, allow the code to compile, but abort at run time
	//
	XLOG_FATAL("MacOS X doesn't have struct in6_ifreq. Aborting...");
	return (XORP_ERROR);
	
#else // ! HOST_OS_MACOSX
	
	struct in6_ifreq ifreq6;
	
	strncpy(ifreq6.ifr_name, mfea_vif->name().c_str(), IFNAMSIZ);
	// Get the interface flags
	if (ioctl(_ioctl_socket, SIOCGIFFLAGS, &ifreq6) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFFLAGS, vif %s) failed: %s",
		       mfea_vif->name().c_str(), strerror(errno));
	    return (false);
	}
	flags = ifreq6.ifr_ifru.ifru_flags;
	break;
	
#endif // ! HOST_OS_MACOSX
	
    }	
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (false);
    }
    
    // Check misc. interface-specific flags
    if ((flags & IFF_LOOPBACK)
	|| (!(flags & IFF_MULTICAST))
	|| (!(flags & IFF_UP))) {
	return (false);
    }
    
    return (true);
}


/**
 * UnixComm::open_proto_socket:
 * @void: 
 * 
 * Register and 'start' a multicast protocol (IGMP, MLD6, PIM, etc)
 * in the kernel.
 * XXX: This function will create the socket to receive the messages,
 * and will assign a function to start listening on that socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::open_proto_socket(void)
{
    if (_ipproto < 0)
	return (XORP_ERROR);
    
    // XXX: The mrouter socket is same as the IPPROTO_{IGMP,ICMPV6} socket
    switch (family()) {
    case AF_INET:
	if (_ipproto == IPPROTO_IGMP)
	    _proto_socket = _mrouter_socket;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (_ipproto == IPPROTO_ICMPV6)
	    _proto_socket = _mrouter_socket;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    if (_proto_socket < 0) {
	if ( (_proto_socket = socket(family(), SOCK_RAW, _ipproto)) < 0) {
	    XLOG_ERROR("Cannot open ipproto %d raw socket stream: %s",
		       _ipproto, strerror(errno));
	    return (XORP_ERROR);
	}
    }
    // Set various socket options
    // Lots of input buffering
    if (set_rcvbuf(_proto_socket, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Include IP header when sending (XXX: doesn't do anything for IPv6)
    if (ip_hdr_include(true) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Show interest in receiving information from IP header (XXX: IPv6 only)
    if (recv_pktinfo(true) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Restrict multicast TTL
    if (set_mcast_ttl(MULTICAST_MIN_TTL_THRESHOLD_DEFAULT)
	< 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Disable mcast loopback
    if (set_multicast_loop(false) < 0) {
	close_proto_socket();
	return (XORP_ERROR);
    }
    // Protocol-specific setup
    switch (family()) {
    case AF_INET:
	if (_ipproto == IPPROTO_IGMP) {
	    _mrouter_socket = _proto_socket; // XXX: for various kernel access
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if (_ipproto == IPPROTO_ICMPV6) {
	    struct icmp6_filter filter;
	    
	    // Filter all non-MLD ICMP messages
	    ICMP6_FILTER_SETBLOCKALL(&filter);
	    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_QUERY, &filter);
	    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REPORT, &filter);
	    ICMP6_FILTER_SETPASS(ICMP6_MEMBERSHIP_REDUCTION, &filter);
	    ICMP6_FILTER_SETPASS(MLD6_MTRACE_RESP, &filter);
	    ICMP6_FILTER_SETPASS(MLD6_MTRACE, &filter);
#ifdef MLDV2_LISTENER_REPORT
	    ICMP6_FILTER_SETPASS(MLD6V2_LISTENER_REPORT, &filter);
#endif
	    if (setsockopt(_proto_socket, _ipproto, ICMP6_FILTER,
			   (void *)&filter, sizeof(filter)) < 0) {
		close_proto_socket();
		XLOG_ERROR("setsockopt(ICMP6_FILTER) failed: %s",
			   strerror(errno));
		return (XORP_ERROR);
	    }
	    
	    _mrouter_socket = _proto_socket; // XXX: for various kernel access
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    // Assign a function to read from this socket
    if (mfea_node().event_loop().add_selector(_proto_socket, SEL_RD,
					      unix_comm_proto_socket_read, this)
	== false) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}


/**
 * UnixComm::close_proto_socket:
 * @void: 
 * 
 * Close the protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::close_proto_socket(void)
{
    bool sock_close_flag = true;
    
    if (_proto_socket < 0)
	return (XORP_ERROR);
    
    //
    // XXX: The mrouter socket is same as the IPPROTO_{IGMP,ICMPV6} socket.
    // Hence, don't close the protocol communication socket for such
    // protocols if the mrouter socket is still in use.
    //
    switch (family()) {
    case AF_INET:
	if ((_ipproto == IPPROTO_IGMP) && (_proto_socket == _mrouter_socket))
	    sock_close_flag = false;
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	if ((_ipproto == IPPROTO_ICMPV6) && (_proto_socket == _mrouter_socket))
	    sock_close_flag = false;
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    if (sock_close_flag) {
	// Remove it just in case, even though it may not be select()-ed
	mfea_node().event_loop().remove_selector(_proto_socket);
	// We can close the socket
	close(_proto_socket);
    }
    _proto_socket = -1;
    
    return (XORP_OK);
}

/**
 * unix_comm_proto_socket_read:
 * @fd: The file descriptor that is ready to read.
 * @mask: The selector event type mask that describes the status of @fd.
 * @thunk: The #UnixComm object that corresponds to the protocol socket that
 * is ready to read.
 * 
 * A wrapper to call the appropriate method to read data
 * from a protocol socket.
 * XXX: This function should not be called directly, but should be assigned to
 * read a socket by mfea_node().event_loop().add_selector().
 * 
 **/
static void
unix_comm_proto_socket_read(int fd, SelectorMask mask, void *thunk)
{
    UnixComm	*unix_comm = reinterpret_cast<UnixComm*>(thunk);
    
    unix_comm->proto_socket_read();
    
    UNUSED(fd);
    UNUSED(mask);
}

/**
 * UnixComm::proto_socket_read:
 * @void: 
 * 
 * Read data from a protocol socket, and then call the appropriate protocol
 * module to process it.
 * XXX: This function should not be called directly, but should be called
 * by its wrapper: unix_comm_proto_socket_read().
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::proto_socket_read(void)
{
    int		nbytes;
    int		iphdrlen = 0;
    int		ipdatalen = 0;
    IPvX	src(family());
    IPvX	dst(family());
    int		ip_ttl = -1;		// a.k.a. Hop-Limit in IPv6
    int		ip_tos = -1;
    bool	router_alert_bool = false; // Router Alert option received
    int		pif_index = 0;
    MfeaVif	*mfea_vif = NULL;
    
    // Zero and reset various fields
    _rcvmh.msg_controllen = sizeof(_rcvcmsgbuf);
    // TODO: when resetting _from4 and _from6 do we need to set the address
    // family and the sockaddr len?
    switch (family()) {
    case AF_INET:
	memset(&_from4, 0, sizeof(_from4));
	_rcvmh.msg_namelen = sizeof(_from4);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	memset(&_from6, 0, sizeof(_from6));
	_rcvmh.msg_namelen = sizeof(_from6);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    // Read from the socket
    nbytes = recvmsg(_proto_socket, &_rcvmh, 0);
    if (nbytes < 0) {
	if (errno == EINTR)
	    return (XORP_OK);		// XXX
	XLOG_ERROR("recvmsg() failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    
    // Check if it is a signal from the kernel to the user-level
    switch (_ipproto) {
    case IPPROTO_IGMP:
    {
	struct igmpmsg *igmpmsg;
	
	igmpmsg = (struct igmpmsg *)_rcvbuf0;
	if (nbytes < (int)sizeof(*igmpmsg)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal packet size %d is smaller than minimum size %u",
			 nbytes, (uint32_t)sizeof(*igmpmsg));
	    return (XORP_ERROR);
	}
	if (igmpmsg->im_mbz == 0) {
	    //
	    // XXX: Packets sent up from kernel to daemon have
	    //      igmpmsg->im_mbz = ip->ip_p = 0
	    //
	    return (kernel_call_process(_rcvbuf0, nbytes));
	}
	break;
    }
#ifdef HAVE_IPV6
    case IPPROTO_ICMPV6:
    {
	struct mrt6msg *mrt6msg;
	
	mrt6msg = (struct mrt6msg *)_rcvbuf0;
	if ((nbytes < (int)sizeof(*mrt6msg))
	    && (nbytes < (int)sizeof(struct mld6_hdr))) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "kernel signal packet size %d is smaller than minimum size %u",
			 nbytes,
			 min((uint32_t)sizeof(*mrt6msg), (uint32_t)sizeof(struct mld6_hdr)));
	    return (XORP_ERROR);
	}
	if ((mrt6msg->im6_mbz == 0) || (_rcvmh.msg_controllen == 0)) {
	    //
	    // XXX: Packets sent up from kernel to daemon have
	    //      mrt6msg->im6_mbz = icmp6_hdr->icmp6_type = 0
	    // Because we set ICMP6 filters on the socket,
	    // we should never see a real ICMPv6 packet
	    // with icmp6_type = 0 .
	    //
	    //
	    // TODO: XXX: (msg_controllen == 0) is presumably
	    // true for older IPv6 systems (e.g. KAME circa
	    // April 2000, FreeBSD-4.0) which don't have the
	    //     'icmp6_type = 0' mechanism.
	    //
	    return (kernel_call_process(_rcvbuf0, nbytes));
	}
	break;
    }
#endif // HAVE_IPV6
    default:
	break;
    }
    
    //
    // Not a kernel signal. Pass to the registered processing function
    //
    
    // Input check.
    // Get source and destination address, IP TTL (a.k.a. hop-limit),
    // and (eventually) interface address (IPv6 only).
    // TODO: check whether Router Alert option has been received.
    switch (family()) {
    case AF_INET:
    {
	struct ip *ip;
	struct cmsghdr *cm;
	bool is_datalen_error = false;
	
	// Input check
	ip = (struct ip *)_rcvbuf0;
	if (nbytes < (int)sizeof(*ip)) {
	    XLOG_WARNING("proto_socket_read() failed: "
			 "packet size %d is smaller than minimum size %u",
			 nbytes, (uint32_t)sizeof(*ip));
	    return (XORP_ERROR);
	}
	// TODO: check for Router Alert option
	src.copy_in(_from4);
	dst.copy_in(ip->ip_dst);
	ip_ttl = ip->ip_ttl;
	ip_tos = ip->ip_tos;
	
	iphdrlen  = ip->ip_hl << 2;
#ifdef IPV4_RAW_INPUT_IS_RAW
	ipdatalen = ntohs(ip->ip_len) - iphdrlen;
#else
	ipdatalen = ip->ip_len;
#endif // ! IPV4_RAW_INPUT_IS_RAW
	// Check length
	is_datalen_error = false;
	do {
	    if (iphdrlen + ipdatalen == nbytes) {
		is_datalen_error = false;
		break;		// OK
	    }
	    if (nbytes < iphdrlen) {
		is_datalen_error = true;
		break;
	    }
	    if (ip->ip_p == IPPROTO_PIM) {
		struct pim *pim;
		if (nbytes < iphdrlen + PIM_REG_MINLEN) {
		    is_datalen_error = true;
		    break;
		}
		pim = (struct pim *)((uint8_t *)ip + iphdrlen);
		if (PIM_VT_T(pim->pim_vt) != PIM_REGISTER) {
		    is_datalen_error = true;
		    break;
		}
		// XXX: the *BSD kernel might truncate the encapsulated data
		// in PIM Register messages before passing the message up
		// to user level. The reason for the truncation is to reduce
		// the bandwidth overhead, but the price for this is
		// messages with weird IP header. Sigh...
		is_datalen_error = false;
		break;
	    }
	} while (false);
	if (is_datalen_error) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet size from %s to %s with %d bytes instead of "
		       "hdr+datalen=%d+%d=%d",
		       cstring(src), cstring(dst), nbytes,
		       iphdrlen, ipdatalen, iphdrlen+ipdatalen);
	    return (XORP_ERROR);
	}
	
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&_rcvmh);
	     cm != NULL;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&_rcvmh, cm)) {
	    if (cm->cmsg_level != IPPROTO_IP)
		continue;
	    switch (cm->cmsg_type) {
#ifdef IP_RECVIF
	    case IP_RECVIF:
	    {
		struct sockaddr_dl *sdl = NULL;
		if (cm->cmsg_len != CMSG_LEN(sizeof(struct sockaddr_dl)))
		    continue;
		sdl = (struct sockaddr_dl *)CMSG_DATA(cm);
		pif_index = sdl->sdl_index;
		break;
	    }
#endif // IP_RECVIF
	    default:
		break;
	    }
	}
	
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct cmsghdr *cm;
	struct in6_pktinfo *pi = NULL;
	
	// TODO: check for Router Alert option
	src.copy_in(_from6);
	if (_rcvmh.msg_flags & MSG_CTRUNC) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s with size of %d bytes is truncated",
		       cstring(src), nbytes);
	    return (XORP_ERROR);
	}
	if (_rcvmh.msg_controllen < sizeof(struct cmsghdr)) {
	    XLOG_ERROR("proto_socket_read() failed: "
		       "RX packet from %s has too short msg_controllen "
		       "(%d instead of %u)",
		       cstring(src),
		       _rcvmh.msg_controllen,
		       (uint32_t)sizeof(struct cmsghdr));
	    return (XORP_ERROR);
	}
	
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&_rcvmh);
	     cm != NULL;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&_rcvmh, cm)) {
	    if (cm->cmsg_level != IPPROTO_IPV6)
		continue;
	    switch (cm->cmsg_type) {
	    case IPV6_PKTINFO:
		if (cm->cmsg_len != CMSG_LEN(sizeof(struct in6_pktinfo)))
		    continue;
		pi = (struct in6_pktinfo *)CMSG_DATA(cm);
		pif_index = pi->ipi6_ifindex;
		dst.copy_in(pi->ipi6_addr);
		break;
	    case IPV6_HOPLIMIT:
		if (cm->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		ip_ttl = *((int *)CMSG_DATA(cm));
		break;
#ifdef IPV6_TCLASS
		if (cm->cmsg_len != CMSG_LEN(sizeof(int)))
		    continue;
		ip_tos = *((int *)CMSG_DATA(cm));
		break;
#endif // IPV6_TCLASS
	    default:
		break;
	    }
	}
	
	iphdrlen = 0;
	ipdatalen = nbytes;
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    // Various checks
    if (! src.is_unicast()) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid unicast sender address: %s", cstring(src));
	return (XORP_ERROR);
    }
    if (! (dst.is_multicast() || dst.is_unicast())) {
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid destination address: %s", cstring(dst));
	return (XORP_ERROR);
    }
    if (ip_ttl < 0) {
	// TODO: what about ip_ttl = 0? Is it OK?
	XLOG_ERROR("proto_socket_read() failed: "
		   "invalid Hop-Limit (TTL) from %s to %s: %d",
		   cstring(src), cstring(dst), ip_ttl);
	return (XORP_ERROR);
    }
    if (pif_index == 0) {
	switch (family()) {
	case AF_INET:
	    // TODO: take care of Linux??
	    break;	// XXX: in IPv4 (except Linux?) there is no pif_index
#ifdef HAVE_IPV6
	case AF_INET6:
	    XLOG_ERROR("proto_socket_read() failed: "
		       "invalid interface pif_index from %s to %s: %d",
		       cstring(src), cstring(dst), pif_index);
	    return (XORP_ERROR);
#endif // HAVE_IPV6
	default:
	    XLOG_ASSERT(false);
	    return (XORP_ERROR);
	}
    }
    
    // XXX: silently ignore the messages originated by myself
    // TODO: this search is probably too much overhead?
    if (ignore_my_packets()) {
	if (mfea_node().vif_find_by_addr(src) != NULL)
	    return (XORP_ERROR);
    }
    
    // Find the vif this message was received on.
    // XXX: note that in case of IPv4 (except Linux?) we may be able
    // only to "guess" the vif by using the sender or the destination address.
    mfea_vif = NULL;
    do {
	//
	// Find vif based on src address, if a directly connected sender,
	// or based on dst address if unicast and one of my local addresses.
	// However, check first whether we can use 'pif_index' instead.
	if (pif_index != 0) {
	    mfea_vif = mfea_node().vif_find_by_pif_index(pif_index);
	    break;
	}
	
	if (dst.is_multicast()) {
	    // TODO: use VIF_LOCAL_DISABLE flag
	    mfea_vif = mfea_node().vif_find_direct(src);
	} else {
	    mfea_vif = mfea_node().vif_find_by_addr(dst);
	}
	break;
    } while (false);
    
    if (mfea_vif == NULL) {
	// No vif found. Ignore this packet.
	XLOG_WARNING("proto_socket_read() failed: "
		     "RX packet from %s to %s: no vif found",
		     cstring(src), cstring(dst));
	return (XORP_ERROR);
    }
    if (! mfea_vif->is_up()) {
	// This vif is down. Silently ignore this packet.
	return (XORP_ERROR);
    }
    
    // Process the result
    if (mfea_node().unix_comm_recv(module_id(),
				   mfea_vif->vif_index(),
				   src, dst, ip_ttl, ip_tos, router_alert_bool,
				   _rcvbuf0 + iphdrlen, nbytes - iphdrlen)
	== XORP_OK) {
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}


/**
 * UnixComm::proto_socket_write:
 * @vif_index: The vif index of the vif that will be used to send-out
 * the packet.
 * @src: The source address of the packet.
 * @dst: The destination address of the packet.
 * @ip_ttl: The TTL (a.k.a. Hop-limit in IPv6) of the packet. If it has a
 * negative value, the TTL will be set here or by the lower layers.
 * @ip_tos: The TOS (a.k.a. Traffic Class in IPv6) of the packet. If it has a
 * negative value, the TOS will be set here or by the lower layers.
 * @router_alert_bool: If true, then the IP packet with the data should
 * have the Router Alert option included.
 * @databuf: The data buffer.
 * @datalen: The length of the data in @databuf.
 * 
 * Send a packet on a protocol socket.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::proto_socket_write(uint16_t vif_index,
			     const IPvX& src, const IPvX& dst,
			     int ip_ttl, int ip_tos, bool router_alert_bool,
			     const uint8_t *databuf, size_t datalen)
{
    struct ip	*ip;
    uint32_t	ip_option;
    uint32_t	*ip_option_p;
    int		ip_hdr_len = 0;
    int		setloop = false;
    int		ret;
    MfeaVif	*mfea_vif = mfea_node().vif_find_by_vif_index(vif_index);
    
    if (mfea_vif == NULL)
	return (XORP_ERROR);
    if (! mfea_vif->is_up()) {
	// The vif is DOWN. Silently ignore the packet
	return (XORP_ERROR);
    }
    
    //
    // Assign the TTL and TOS if they were not specified
    //
    switch (family()) {
    case AF_INET:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (router_alert_bool)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (router_alert_bool)
		ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
	    else
		ip_tos = 0;
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	// Assign the TTL
	if (ip_ttl < 0) {
	    if (router_alert_bool)
		ip_ttl = MINTTL;
	    else
		ip_ttl = IPDEFTTL;
	}
	// Assign the TOS
	if (ip_tos < 0) {
	    if (router_alert_bool) {
		// TODO: XXX: IPTOS for IPv6 is bogus??
		// ip_tos = IPTOS_PREC_INTERNETCONTROL;  // Internet Control
		ip_tos = 0;
	    } else {
		ip_tos = 0;
	    }
	}
	break;
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    
    // Setup the IP header (including the Router Alert option, if specified)
    // In case of IPv4, the IP header and the data are specified as
    // two entries to the sndiov[] scatter/gatter array.
    // In case of IPv6, the IP header information is specified as
    // ancillary data.
    switch (family()) {
    case AF_INET:
	ip = (struct ip *)_sndbuf0;
	if (router_alert_bool) {
	    // Include the Router Alert option
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNETCONTROL	0xc0
#endif
#ifndef IPOPT_RA
#define IPOPT_RA			148	/* 0x94 */
#endif
	    ip_option	= htonl((IPOPT_RA << 24) | (0x04 << 16));
	    ip_option_p	= (uint32_t *)(ip + 1);
	    *ip_option_p = ip_option;
	    ip_hdr_len	= (sizeof(*ip) + sizeof(*ip_option_p));
	} else {
	    ip_hdr_len	= sizeof(*ip);
	}
	ip->ip_hl	= ip_hdr_len >> 2;
	ip->ip_v	= IPVERSION;
	ip->ip_id	= 0;				// Let kernel fill in
	ip->ip_off	= 0x0;
	ip->ip_p	= _ipproto;
	ip->ip_ttl	= ip_ttl;
	ip->ip_tos	= ip_tos;
	
	src.copy_out(ip->ip_src);
	dst.copy_out(ip->ip_dst);
	ip->ip_len = (ip->ip_hl << 2) + datalen;
#ifdef IPV4_RAW_OUTPUT_IS_RAW
	ip->ip_len = htons(ip->ip_len);
#endif
	ip->ip_sum = 0;					// Let kernel fill in
	
	// Now hook the data
	dst.copy_out(_to4);
	_sndmh.msg_namelen	= sizeof(_to4);
	_sndmh.msg_iovlen	= 2;
	_sndiov[0].iov_len	= ip_hdr_len;
	if (datalen > sizeof(_sndbuf1)) {
	    XLOG_ERROR("proto_socket_write() failed: "
		       "cannot send packet on vif %s from %s to %s: "
		       "too much data: %u octets (max = %u)",
		       mfea_vif->name().c_str(),
		       src.str().c_str(),
		       dst.str().c_str(),
		       (uint32_t)datalen,
		       (uint32_t)sizeof(_sndbuf1));
	    return (XORP_ERROR);
	}
    	memcpy(_sndbuf1, databuf, datalen); // XXX: goes to _sndiov[1].iov_base
	// _sndiov[1].iov_base	= (caddr_t)databuf;
	_sndiov[1].iov_len	= datalen;
	_sndmh.msg_control	= NULL;
	_sndmh.msg_controllen	= 0;
	
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	int ctllen = 0;
	int hbhlen = 0;
	struct cmsghdr *cmsgp;
	struct in6_pktinfo *sndpktinfo;
	
	//
	// XXX: unlikely IPv4, in IPv6 the 'header' is specified as
	// ancillary data.
	//
	
	//
	// First, estimate total length of ancillary data
	//
	// Space for IPV6_PKTINFO
	ctllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
	
	if (router_alert_bool) {
	    // Space for Router Alert option
#ifdef HAVE_RFC2292BIS
	    if ((hbhlen = inet6_opt_init(NULL, 0)) == -1)
		XLOG_ERROR("inet6_opt_init(NULL) failed");
	    if ((hbhlen = inet6_opt_append(NULL, 0, hbhlen,
					   IP6OPT_ROUTER_ALERT, 2,
					   2, NULL)) == -1) {
		XLOG_ERROR("inet6_opt_append(NULL) failed");
		return (XORP_ERROR);
	    }
	    if ((hbhlen = inet6_opt_finish(NULL, 0, hbhlen)) == -1) {
		XLOG_ERROR("inet6_opt_finish(NULL) failed");
		return (XORP_ERROR);
	    }
	    ctllen += CMSG_SPACE(hbhlen);
#else
	    hbhlen = inet6_option_space(sizeof(_raopt));
	    ctllen += hbhlen;
#endif // ! HAVE_RFC2292BIS
	}
	// Space for IPV6_TCLASS
#ifdef IPV6_TCLASS 
	ctllen += CMSG_SPACE(sizeof(int));
#endif
	// Space for IPV6_HOPLIMIT
	ctllen += CMSG_SPACE(sizeof(int));
	XLOG_ASSERT(ctllen <= CMSG_BUF_SIZE);   // XXX
	
	//
	// Now setup the ancillary data
	//
	_sndmh.msg_controllen = ctllen;
	cmsgp = CMSG_FIRSTHDR(&_sndmh);
	
	// Add the IPV6_PKTINFO ancillary data
	cmsgp->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type  = IPV6_PKTINFO;
	sndpktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsgp);
	memset(sndpktinfo, 0, sizeof(*sndpktinfo));
	if (mfea_vif->pif_index() > 0)
	    sndpktinfo->ipi6_ifindex = mfea_vif->pif_index();
	else
	    sndpktinfo->ipi6_ifindex = 0;		// Let kernel fill in
	src.copy_out(sndpktinfo->ipi6_addr);
	cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	
	//
	// Include the Router Alert option if needed
	//
	if (router_alert_bool) {
#ifdef HAVE_RFC2292BIS
	    int currentlen;
	    void *hbhbuf, *optp = NULL;
	    
	    cmsgp->cmsg_len   = CMSG_LEN(hbhlen);
	    cmsgp->cmsg_level = IPPROTO_IPV6;
	    cmsgp->cmsg_type  = IPV6_HOPOPTS;
	    hbhbuf = CMSG_DATA(cmsgp);
	    currentlen = inet6_opt_init(hbhbuf, hbhlen);
	    if (currentlen == -1) {
		XLOG_ERROR("inet6_opt_init(len = %d) failed", hbhlen);
		return (XORP_ERROR);
	    }
	    currentlen = inet6_opt_append(hbhbuf, hbhlen, currentlen,
					  IP6OPT_ROUTER_ALERT, 2, 2, &optp);
	    if (currentlen == -1) {
		XLOG_ERROR("inet6_opt_append(len = %d) failed", currentlen);
		return (XORP_ERROR);
	    }
	    inet6_opt_set_val(optp, 0, &rtalert_code, sizeof(rtalert_code));
	    if (inet6_opt_finish(hbhbuf, hbhlen, currentlen) == -1) {
		XLOG_ERROR("inet6_opt_finish(len = %d) failed", currentlen);
		return (XORP_ERROR);
	    }
	    
#else  // ! HAVE_RFC2292BIS (i.e., the old advanced API)
	    
	    if (inet6_option_init((void *)cmsgp, &cmsgp, IPV6_HOPOPTS)) {
		XLOG_ERROR("inet6_option_init(IPV6_HOPOPTS) failed");
		return (XORP_ERROR);
	    }
	    if (inet6_option_append(cmsgp, _raopt, 4, 0)) {
		XLOG_ERROR("inet6_option_append(Router Alert) failed");
		return (XORP_ERROR);
	    }
	    
#endif // ! HAVE_RFC2292BIS
	    
	    cmsgp = CMSG_NXTHDR(&_sndmh, cmsgp);
	}
	
	//
	// Set the TTL
	//
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_HOPLIMIT;
	*(int *)(CMSG_DATA(cmsgp)) = ip_ttl;
	
	//
	// Set the TOS
	//
#ifdef IPV6_TCLASS
	cmsgp->cmsg_len = CMSG_LEN(sizeof(int));
	cmsgp->cmsg_level = IPPROTO_IPV6;
	cmsgp->cmsg_type = IPV6_TCLASS;
	*(int *)(CMSG_DATA(cmsgp)) = ip_tos;
#endif // IPV6_TCLASS
	
	//
	// Now hook the data
	//
	dst.copy_out(_to6);
	_sndmh.msg_namelen  = sizeof(_to6);
	_sndmh.msg_iovlen   = 1;
	if (datalen > sizeof(_sndbuf0)) {
	    XLOG_ERROR("proto_socket_write() failed: "
		       "error sending packet on vif %s from %s to %s: "
		       "too much data: %u octets (max = %u)",
		       mfea_vif->name().c_str(),
		       src.str().c_str(),
		       dst.str().c_str(),
		       (uint32_t)datalen,
		       (uint32_t)sizeof(_sndbuf0));
	    return (XORP_ERROR);
	}
	memcpy(_sndbuf0, databuf, datalen); // XXX: goes to _sndiov[0].iov_base
	// _sndiov[0].iov_base	= (caddr_t)databuf;
	_sndiov[0].iov_len  = datalen;
	
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    setloop = false;
    if (dst.is_multicast()) {
	set_default_multicast_vif(mfea_vif->vif_index());
	set_multicast_loop(true);
	setloop = true;
    }
    
    ret = XORP_OK;
    if (sendmsg(_proto_socket, &_sndmh, 0) < 0) {
	ret = XORP_ERROR;
	if (errno == ENETDOWN) {
	    // TODO: check the interface status. E.g. vif_state_check(family);
	} else {
	    XLOG_ERROR("sendmsg(proto %d from %s to %s on vif %s) failed: %s",
		       _ipproto, cstring(src), cstring(dst),
		       mfea_vif->name().c_str(), strerror(errno));
	}
    }
    if (setloop) {
	set_multicast_loop(false);
    }
    
    return (ret);
}


/**
 * kernel_call_process:
 * @family: The address family.
 * @databuf: The data buffer.
 * @datalen: The length of the data in 'databuf'.
 * 
 * Process a call from the kernel (e.g. "nocache", "wrongiif", "wholepkt")
 * XXX: It is OK for im_src/im6_src to be 0 (for 'nocache' or 'wrongiif'),
 *	just in case the kernel supports (*,G) MFC.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
UnixComm::kernel_call_process(uint8_t *databuf, size_t datalen)
{
    uint16_t	iif_vif_index;
    int		message_type;
    IPvX	src(family()), dst(family());
    
    switch (family()) {
    case AF_INET:
    {
	struct in_addr src_addr, dst_addr;
	struct igmpmsg *igmpmsg = (struct igmpmsg *)databuf;
	
	//
	// Get the source and destination address, the iif, and message type
	//
	src_addr.s_addr = htonl(igmpmsg->im_src.s_addr);
	dst_addr.s_addr = htonl(igmpmsg->im_dst.s_addr);
	src.copy_in(igmpmsg->im_src);
	dst.copy_in(igmpmsg->im_dst);
	iif_vif_index = igmpmsg->im_vif;
	message_type = igmpmsg->im_msgtype;
	
	//
	// Check if the iif is valid and UP
	//
	switch (message_type) {
	case IGMPMSG_NOCACHE:
	case IGMPMSG_WRONGVIF:
	case IGMPMSG_WHOLEPKT:
	{
	    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(iif_vif_index);
	    if ((mfea_vif == NULL) || (! mfea_vif->is_up())) {
		// Silently ignore the packet
		return (XORP_ERROR);
	    }
	}
	break;
#if defined(IGMPMSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	case IGMPMSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	
	//
	// Check 'src' and 'dst' addresses
	//
	switch (message_type) {
	case IGMPMSG_NOCACHE:
	case IGMPMSG_WRONGVIF:
	    if ((! src.is_unicast())
		|| (! dst.is_multicast())
		|| (dst_addr.s_addr <= INADDR_MAX_LOCAL_GROUP)) {
		// XXX: LAN-scoped addresses are not routed
		return (XORP_ERROR);
	    }
	    break;
	case IGMPMSG_WHOLEPKT:
	    // XXX: 'src' and 'dst' are not used, hence we don't check them
	    break;
#if defined(IGMPMSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	case IGMPMSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	//
	// Deliver the signal
	//
	mfea_node().signal_message_recv("",
					module_id(),
					message_type,
					iif_vif_index, src, dst,
					databuf + sizeof(*igmpmsg),
					datalen - sizeof(*igmpmsg));
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct mrt6msg *mrt6msg = (struct mrt6msg *)databuf;
	
	//
	// Get the source and destination address, the iif, and message type
	//
	src.copy_in(mrt6msg->im6_src);
	dst.copy_in(mrt6msg->im6_dst);
	iif_vif_index = mrt6msg->im6_mif;
	message_type = mrt6msg->im6_msgtype;
	
	//
	// Check if the iif is valid and UP
	//
	switch (message_type) {
	case MRT6MSG_NOCACHE:
	case MRT6MSG_WRONGMIF:
	case MRT6MSG_WHOLEPKT:
	{
	    // Check if the iif is valid and UP
	    MfeaVif *mfea_vif = mfea_node().vif_find_by_vif_index(iif_vif_index);
	    if ((mfea_vif == NULL) || (! mfea_vif->is_up())) {
		// Silently ignore the packet
		return (XORP_ERROR);
	    }
	}
	break;
#if defined(MRT6MSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	case MRT6MSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	
	//
	// Check 'src' and 'dst' addresses
	//
	switch (message_type) {
	case MRT6MSG_NOCACHE:
	case MRT6MSG_WRONGMIF:
	    if ((! src.is_unicast())
		|| (! dst.is_multicast())
		|| IN6_IS_ADDR_LINKLOCAL(&mrt6msg->im6_dst)) {
		// XXX: LAN-scoped addresses are not routed
		return (XORP_ERROR);
	    }
	    break;
	case MRT6MSG_WHOLEPKT:
	    // XXX: 'src' and 'dst' are not used, hence we don't check them
	    break;
#if defined(MRT6MSG_BW_UPCALL) && defined(ENABLE_ADVANCED_MCAST_API)
	case MRT6MSG_BW_UPCALL:
	    break;
#endif
	default:
	    break;
	}
	//
	// Deliver the signal
	//
	mfea_node().signal_message_recv("",
					module_id(),
					message_type,
					iif_vif_index, src, dst,
					databuf + sizeof(*mrt6msg),
					datalen - sizeof(*mrt6msg));
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

#if 0	// TODO: use it

//
// The complementary file for the UN*X kernel access module.
// Implements some of the API used to obtain information from the kernel
// about network interfaces, MRIB information, and unicast forwarding table.
//

/**
 * kernel_get_mrib_vif:
 * @ipaddr: The address of the destination whose MRIB we need.
 * @mrib: A pointer to a #Mbib structure to use to return the MRIB information.
 * 
 * Get the Multicast Routing Information Base (MRIB) information
 * (the next hop router and the vif to send out packets toward a given
 * destination) for a given address.
 * XXX: the information is obtained by directly quering the kernel.
 * XXX: 'mrib' must point to data already allocated by the caller.
 * 
 * Return value: The virtual interface toward 'ipaddr', or NULL if error.
 **/
vif_t *
kernel_get_mrib_vif(ipaddr_t *ipaddr, Mrib *mrib)
{
    if (k_get_mrib(ipaddr, mrib) == XORP_OK)
	return (mrib->vif);
    else
	return (NULL);
}
#endif // 0

