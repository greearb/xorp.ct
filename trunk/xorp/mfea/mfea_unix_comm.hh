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

// $XORP: xorp/mfea/mfea_unix_comm.hh,v 1.3 2003/03/14 12:20:27 pavlin Exp $


#ifndef __MFEA_MFEA_UNIX_COMM_HH__
#define __MFEA_MFEA_UNIX_COMM_HH__


//
// UNIX kernel-access specific definitions.
//


#include "config.h"

#include <sys/socket.h>
#include <sys/uio.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif
#include "mrt/include/ip_mroute.h"
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif

#include <vector>

#include "libproto/proto_unit.hh"


//
// Constants definitions
//
#define IO_BUF_SIZE		(64*1024)  // I/O buffer(s) size
#define CMSG_BUF_SIZE		(10*1024)  // 'rcvcmsgbuf' and 'sndcmsgbuf'
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  // Min. socket buffer size
#define SO_RCV_BUF_SIZE_MAX	(256*1024) // Desired socket buffer size

//
// XXX: in *BSD there is only MRT_ASSERT, but in Linux there are
// both MRT_ASSERT and MRT_PIM (with MRT_PIM defined as a superset
// of MRT_ASSERT).
//
#ifndef MRT_PIM
#define MRT_PIM MRT_ASSERT
#endif

//
// Structures/classes, typedefs and macros
//

#ifndef CMSG_LEN
#define CMSG_LEN(l) (ALIGN(sizeof(struct cmsghdr)) + (l)) // XXX
#endif


/**
 * @short Class that contains various counters per (S,G) entry.
 * 
 * All counters are related to the multicast data packets per (S,G) entry.
 */
class SgCount {
public:
    /**
     * Default constructor
     */
    SgCount() : _pktcnt(0), _bytecnt(0), _wrong_if(0) {}
    
    /**
     * Assignment Operator
     * 
     * @param sg_count the value to assing to this entry.
     * @return the entry with the new value assigned.
     */
    SgCount& operator=(const SgCount& sg_count) {
	_pktcnt = sg_count.pktcnt();
	_bytecnt = sg_count.bytecnt();
	_wrong_if = sg_count.wrong_if();
	return (*this);
    }
    
    /**
     * Assign-Sum Operator
     * 
     * @param sg_count the value to add to this entry.
     * @return the entry with the new value after @ref sg_count was added.
     */
    SgCount& operator+=(const SgCount& sg_count) {
	_pktcnt += sg_count.pktcnt();
	_bytecnt += sg_count.bytecnt();
	_wrong_if += sg_count.wrong_if();
	return (*this);
    }
    
    /**
     * Assign-Difference Operator
     * 
     * @param sg_count the value to substract from this entry.
     * @return the entry with the new value after @ref sg_count was
     * substracted.
     */
    SgCount& operator-=(const SgCount& sg_count) {
	_pktcnt -= sg_count.pktcnt();
	_bytecnt -= sg_count.bytecnt();
	_wrong_if -= sg_count.wrong_if();
	return (*this);
    }
    
    /**
     * Get the packet count.
     * 
     * @return the packet count.
     */
    size_t	pktcnt()	const	{ return (_pktcnt);	}
    
    /**
     * Get the byte count.
     * 
     * @return the byte count.
     */
    size_t	bytecnt()	const	{ return (_bytecnt);	}
    
    /**
     * Get the number of packets received on wrong interface.
     * 
     * @return the number of packets received on wrong interface.
     */
    size_t	wrong_if()	const	{ return (_wrong_if);	}
    
    /**
     * Set the packet count.
     * 
     * @param v the value to assign to the packet count.
     */
    void	set_pktcnt(size_t v)	{ _pktcnt = v;		}
    
    /**
     * Set the byte count.
     * 
     * @param v the value to assign to the byte count.
     */
    void	set_bytecnt(size_t v)	{ _bytecnt = v;		}
    
    /**
     * Set the wrong-interface packet count.
     * 
     * @param v the value to assign to the wrong-interface packet count.
     */
    void	set_wrong_if(size_t v)	{ _wrong_if = v;	}
    
    /**
     * Reset all counters.
     */
    void	reset() { _pktcnt = 0; _bytecnt = 0; _wrong_if = 0; }
    
    /**
     * Test if this entry contains valid counters.
     * 
     * @return true if this entry contains valid counters, otherwise false.
     */
    bool	is_valid() const { return (! ((_pktcnt == (size_t)~0)
					      && (_bytecnt == (size_t)~0)
					      && (_wrong_if == (size_t)~0))); }
    
private:
    size_t	_pktcnt;	// Number of multicast data packets received
    size_t	_bytecnt;	// Number of multicast data bytes received
    size_t	_wrong_if;	// Number of multicast data packets received
				// on wrong iif
};

//
// A class that contains information about a vif in the kernel.
//

/**
 * @short Class that contains various counters per virtual interface.
 * 
 * All counters are related to the multicast data packets per virtual
 * interface.
 */
class VifCount {
public:
    /**
     * Default constructor
     */
    VifCount() : _icount(0), _ocount(0), _ibytes(0), _obytes(0) {}
    
    /**
     * Assignment Operator
     * 
     * @param vif_count the value to assign to this entry.
     * @return the entry with the new value assigned.
     */
    VifCount& operator=(const VifCount& vif_count) {
	_icount = vif_count.icount();
	_ocount = vif_count.ocount();
	_ibytes = vif_count.ibytes();
	_obytes = vif_count.obytes();
	return (*this);
    }
    
    /**
     * Get the input packet count.
     * 
     * @return the input packet count.
     */
    size_t	icount()	const	{ return (_icount);	}
    
    /**
     * Get the output packet count.
     * 
     * @return the output packet count.
     */
    size_t	ocount()	const	{ return (_ocount);	}
    
    /**
     * Get the input byte count.
     * 
     * @return the input byte count.
     */
    size_t	ibytes()	const	{ return (_ibytes);	}
    
    /**
     * Get the output byte count.
     * 
     * @return the output byte count.
     */
    size_t	obytes()	const	{ return (_obytes);	}
    
    /**
     * Set the input packet count.
     * 
     * @param v the value to assign to the data packet count.
     */
    void	set_icount(size_t v)	{ _icount = v;		}

    /**
     * Set the output packet count.
     * 
     * @param v the value to assign to the output packet count.
     */
    void	set_ocount(size_t v)	{ _ocount = v;		}

    /**
     * Set the input byte count.
     * 
     * @param v the value to assign to the data byte count.
     */
    void	set_ibytes(size_t v)	{ _ibytes = v;		}
    
    /**
     * Set the output byte count.
     * 
     * @param v the value to assign to the output byte count.
     */
    void	set_obytes(size_t v)	{ _obytes = v;		}
    
    /**
     * Test if this entry contains valid counters.
     * 
     * @return true if this entry contains valid counters, otherwise false.
     */
    bool	is_valid() const { return (! ((_icount == (size_t)~0)
					      && (_ocount == (size_t)~0)
					      && (_ibytes == (size_t)~0)
					      && (_obytes == (size_t)~0))); }
    
private:
    size_t	_icount;	// Number of input multicast data packets
    size_t	_ocount;	// Number of output multicast data packets
    size_t	_ibytes;	// Number of input multicast data bytes
    size_t	_obytes;	// Number of output multicast data bytes
};

class MfeaNode;
class MfeaVif;
class Mrib;
class EventLoop;




/**
 * @short A class for socket I/O communication.
 * 
 * Each protocol 'registers' for socket I/O and gets assigned one object
 * of this class.
 */
class UnixComm : public ProtoUnit {
public:
    /**
     * Constructor for given MFEA node, IP protocol, and module ID
     * (@ref xorp_module_id).
     * 
     * @param mfea_node the MFEA node (@ref MfeaNode) this entry belongs to.
     * @param ipproto the IP protocol number (e.g., IPPROTO_PIM for PIM).
     * @param module_id the module ID (@ref xorp_module_id) for the protocol.
     */
    UnixComm(MfeaNode& mfea_node, int ipproto, xorp_module_id module_id);
    
    /**
     * Destructor
     */
    virtual ~UnixComm();
    
    /**
     * Start the @ref UnixComm.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the @ref UnixComm.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Query the kernel to find network interfaces that are multicast-capable.
     * 
     * @param mfea_vifs_vector reference to the vector to store the created
     * MFEA-specific vifs (@ref MfeaVif).
     * @return the number of created vifs on success, otherwise %XORP_ERROR.
     */
    int		get_mcast_vifs(vector<MfeaVif *>& mfea_vifs_vector);
    
    /**
     * Get the ioctl socket.
     * 
     * The ioctl socket is used for various ioctl() calls.
     * 
     * @return the socket value if valid, otherwise XORP_ERROR.
     */
    int		ioctl_socket() const { return (_ioctl_socket); }
    
    /**
     * Open an ioctl socket.
     * 
     * The ioctl socket is used for various ioctl() calls.
     * 
     * @return the socket value on success, otherwise XORP_ERROR.
     */
    int		open_ioctl_socket();
    
    /**
     * Close the ioctl socket.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_ioctl_socket();
    
    /**
     * Get the MRIB socket.
     * 
     * The MRIB socket is used for obtaining Multicast Routing Information
     * Base information.
     * 
     * @return the socket value if valid, otherwise XORP_ERROR.
     */
    int		mrib_socket() const { return (_mrib_socket); }
    
    /**
     * Open an MRIB socket.
     * 
     * The MRIB socket is used for obtaining Multicast Routing Information
     * Base information.
     * 
     * @return the socket value on success, otherwise XORP_ERROR.
     */
    int		open_mrib_socket();
    
    /**
     * Close the MRIB socket.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_mrib_socket();
    
    /**
     * Get the mrouter socket.
     * 
     * The mrouter socket is used for various multicast-related access.
     * 
     * @return the socket value if valid, otherwise XORP_ERROR.
     */
    int		mrouter_socket() const { return (_mrouter_socket); }
    
    /**
     * Open an mrouter socket.
     * 
     * The mrouter socket is used for various multicast-related access.
     * Note that no more than one mrouter socket (per address family)
     * should be open at a time.
     * 
     * @return the socket value on success, otherwise XORP_ERROR.
     */
    int		open_mrouter_socket();
    
    /**
     * Close the mrouter socket.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_mrouter_socket();
    
    /**
     * Set the mrouter socket value of this entry.
     * 
     * The mrouter socket value of this entry is set by copying the mrouter
     * socket value from the special house-keeping @ref UnixComm
     * with module ID (@ref xorp_module_id) of XORP_MODULE_NULL.
     * 
     * @return the value of the copied mrouter socket on success,
     * otherwise XORP_ERROR.
     */
    int		set_my_mrouter_socket();
    
    /**
     * Set the mrouter socket value of the other @ref UnixComm entries.
     * 
     * The mrouter socket value of the other @ref UnixComm entries is
     * set by copying the mrouter socket value of this entry.
     * Note that this method should be applied only to the special
     * house-keeping @ref UnixComm with module ID (@ref xorp_module_id)
     * of XORP_MODULE_NULL.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_other_mrouter_socket();
    
    /**
     * Get the protocol socket.
     * 
     * The protocol socket is specific to the particular protocol of
     * this entry.
     * 
     * @return the socket value if valid, otherwise XORP_ERROR.
     */
    int		proto_socket() const { return (_proto_socket); }
    
    /**
     * Open an protocol socket.
     * 
     * The protocol socket is specific to the particular protocol of
     * this entry.
     * 
     * @return the socket value on success, otherwise XORP_ERROR.
     */
    int		open_proto_socket();
    
    /**
     * Close the protocol socket.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_proto_socket();
    
    /**
     * Set the receiving buffer size of a socket.
     * 
     * @param recv_socket the socket whose receiving buffer size to set.
     * @param desired_bufsize the preferred buffer size.
     * @param min_bufsize the smallest acceptable buffer size.
     * @return the successfully set buffer size on success,
     * otherwise XORP_ERROR
     */
    int		set_rcvbuf(int recv_socket, int desired_bufsize,
			   int min_bufsize);
    
    /**
     * Set/reset the "Header Included" option (for IPv4) on the protocol
     * socket.
     * 
     * If set, the IP header of a raw packet should be created
     * by the application itself, otherwise the kernel will build it.
     * Note: used only for IPv4.
     * In post-RFC-2292, IPV6_PKTINFO has similar functions,
     * but because it requires the interface index and outgoing address,
     * it is of little use for our purpose. Also, in RFC-2292 this option
     * was a flag, so for compatibility reasons we better not set it
     * here; instead, we will use sendmsg() to specify the header's field
     * values.
     * 
     * @param enable_bool if true, set the option, otherwise reset it.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		ip_hdr_include(bool enable_bool);
    
    /**
     * Enable/disable receiving information about some of the fields
     * in the IP header on the protocol socket.
     * 
     * If enabled, values such as interface index, destination address and
     * IP TTL (a.k.a. hop-limit in IPv6), and hop-by-hop options will be
     * received as well.
     * Note: used only for IPv6. In IPv4 we don't have this; the whole IP
     *  packet is passed to the application listening on a raw socket.
     * 
     * @param enable_bool if true, set the option, otherwise reset it.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		recv_pktinfo(bool enable_bool);
    
    /**
     * Set the default TTL (or hop-limit in IPv6) for the outgoing multicast
     * packets on the protocol socket.
     * 
     * @param ttl the desired IP TTL (a.k.a. hop-limit in IPv6) value.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_mcast_ttl(int ttl);
    
    /**
     * Set/reset the "Multicast Loop" flag on the protocol socket.
     * 
     * If the multicast loopback flag is set, a multicast datagram sent on
     * that socket will be delivered back to this host (assuming the host
     * is a member of the same multicast group).
     * 
     * @param enable_bool if true, set the loopback, otherwise reset it.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_multicast_loop(bool enable_bool);
    
    /**
     * Start/enable the multicast routing in the kernel.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_mrt();
    
    /**
     * Stop/disable the multicast routing in the kernel.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_mrt();
    
    /**
     * Start/enable PIM routing in the kernel.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_pim();
    
    /**
     * Stop/disable PIM routing in the kernel.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop_pim();
    
    /**
     * Set default interface for outgoing multicast on the protocol socket.
     * 
     * 
     * @param vif_index the vif index of the interface to become the default
     * multicast interface.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_default_multicast_vif(uint16_t vif_index);
    
    /**
     * Join a multicast group on an interface.
     * 
     * @param vif_index the vif index of the interface to join the multicast
     * group.
     * @param group the multicast group to join.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(uint16_t vif_index, const IPvX& group);
    
    /**
     * Leave a multicast group on an interface.
     * 
     * @param vif_index the vif index of the interface to leave the multicast
     * group.
     * @param group the multicast group to leave.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(uint16_t vif_index, const IPvX& group);
    
    /**
     * Add a virtual multicast interface to the kernel.
     * 
     * @param vif_index the vif index of the virtual interface to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_multicast_vif(uint16_t vif_index);
    
    /**
     * Delete a virtual multicast interface from the kernel.
     * 
     * @param vif_index the vif index of the interface to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_multicast_vif(uint16_t vif_index);
    
    /**
     * Install/modify a Multicast Forwarding Cache (MFC) entry in the kernel.
     * 
     * If the MFC entry specified by (@source, @group) pair was not
     * installed before, a new MFC entry will be created in the kernel;
     * otherwise, the existing entry's fields will be modified.
     * 
     * @param source the MFC source address.
     * @param group the MFC group address.
     * @param iif_vif_index the MFC incoming interface index.
     * @param oifs_ttl an array with the min. TTL a packet should have to be
     * forwarded.
     * @param oifs_flags an array with misc. flags for the MFC to install.
     * Note that those flags are supported only by the advanced multicast API.
     * @param rp_addr the RP address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_mfc(const IPvX& source, const IPvX& group,
			uint16_t iif_vif_index, uint8_t *oifs_ttl,
			uint8_t *oifs_flags,
			const IPvX& rp_addr);
    
    /**
     * Delete a Multicast Forwarding Cache (MFC) entry in the kernel.
     * 
     * @param source the MFC source address.
     * @param group the MFC group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_mfc(const IPvX& source, const IPvX& group);
    
    /**
     * Add a dataflow monitor entry in the kernel.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param source the source address.
     * @param group the group address.
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets) to
     * compare against.
     * @param threshold_bytes the threshold (in number of bytes) to
     * compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_bw_upcall(const IPvX& source, const IPvX& group,
			      const struct timeval& threshold_interval,
			      uint32_t threshold_packets,
			      uint32_t threshold_bytes,
			      bool is_threshold_in_packets,
			      bool is_threshold_in_bytes,
			      bool is_geq_upcall,
			      bool is_leq_upcall);
    
    /**
     * Delete a dataflow monitor entry from the kernel.
     * 
     * Note: either @ref is_threshold_in_packets or @ref is_threshold_in_bytes
     * (or both) must be true.
     * Note: either @ref is_geq_upcall or @ref is_leq_upcall
     * (but not both) must be true.
     * 
     * @param source the source address.
     * @param group the group address.
     * @param threshold_interval the dataflow threshold interval.
     * @param threshold_packets the threshold (in number of packets) to
     * compare against.
     * @param threshold_bytes the threshold (in number of bytes) to
     * compare against.
     * @param is_threshold_in_packets if true, @ref threshold_packets is valid.
     * @param is_threshold_in_bytes if true, @ref threshold_bytes is valid.
     * @param is_geq_upcall if true, the operation for comparison is ">=".
     * @param is_leq_upcall if true, the operation for comparison is "<=".
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_bw_upcall(const IPvX& source, const IPvX& group,
				 const struct timeval& threshold_interval,
				 uint32_t threshold_packets,
				 uint32_t threshold_bytes,
				 bool is_threshold_in_packets,
				 bool is_threshold_in_bytes,
				 bool is_geq_upcall,
				 bool is_leq_upcall);
    
    /**
     * Delete all dataflow monitor entries from the kernel
     * for a given source and group address.
     * 
     * @param source the source address.
     * @param group the group address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_all_bw_upcall(const IPvX& source, const IPvX& group);
    
    /**
     * Get various counters per (S,G) entry.
     * 
     * Get the number of packets and bytes forwarded by a particular
     * Multicast Forwarding Cache (MFC) entry in the kernel, and the number
     * of packets arrived on wrong interface for that entry.
     * 
     * @param source the MFC source address.
     * @param group the MFC group address.
     * @param sg_count a reference to a @ref SgCount class to place the result.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_sg_count(const IPvX& source, const IPvX& group,
			     SgCount& sg_count);
    
    /**
     * Get various counters per virtual interface.
     * 
     * Get the number of packets and bytes received on, or forwarded on
     * a particular multicast interface.
     * 
     * @param vif_index the vif index of the virtual multicast interface whose
     * statistics we need.
     * @param vif_count a reference to a @ref VifCount class to store
     * the result.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_vif_count(uint16_t vif_index, VifCount& vif_count);
    
    /**
     * Test if an interface in the kernel is multicast-capable.
     * 
     * @param vif_index the vif index of the interface to test whether is
     * multicast capable.
     * @return true if the interface is multicast-capable, otherwise false.
     */
    bool	is_multicast_capable(uint16_t vif_index) const;
    
    /**
     * Read data from a protocol socket, and then call the appropriate protocol
     * module to process it.
     * 
     * Note: this function should not be called directly, but should be called
     * by its wrapper: unix_comm_proto_socket_read().
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_socket_read();
    
    /**
     * Send a packet on a protocol socket.
     * 
     * @param vif_index the vif index of the vif that will be used to send-out
     * the packet.
     * @param src the source address of the packet.
     * @param dst the destination address of the packet.
     * @param ip_ttl the TTL (a.k.a. Hop-limit in IPv6) of the packet.
     * If it has a negative value, the TTL will be set here or by the
     * lower layers.
     * @param ip_tos the TOS (a.k.a. Traffic Class in IPv6) of the packet.
     * If it has a negative value, the TOS will be set here or by the lower
     * layers.
     * @param router_alert_bool if true, then the IP packet with the data
     * should have the Router Alert option included.
     * @param databuf the data buffer.
     * @param datalen the length of the data in @ref databuf.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		proto_socket_write(uint16_t vif_index,
				   const IPvX& src, const IPvX& dst,
				   int ip_ttl, int ip_tos,
				   bool router_alert_bool,
				   const uint8_t *databuf, size_t datalen);
    
    /**
     * Get the Multicast Routing Information Base (MRIB) information
     * for a given address (@ref Mrib).
     * 
     * The MRIB information contains the next hop router and the vif to send
     * out packets toward a given destination.
     * 
     * @param dest_addr the destination address to lookup.
     * @param mrib a reference to a @ref Mrib structure to return the
     * MRIB information.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		get_mrib(const IPvX& dest_addr, Mrib& mrib);
    
    /**
     *  Get all MRIB (@ref Mrib) information from the kernel.
     * 
     * @param mrib_table a pointer to the routing table array composed
     * of @ref Mrib elements.
     * @return the number of entries in @ref return_mrib_table, or XORP_ERROR
     * if there was an error
     */
    int		get_mrib_table(Mrib **mrib_table);
    
    /**
     * Set/reset the flag whether to ignore the receiving my own packets.
     * 
     * @param v if true, ignore my own packets on receiving, otherwise don't
     * ignore them.
     */
    void	set_ignore_my_packets(bool v) { _ignore_my_packets = v; }
    
    /**
     * Set/reset the flag whether to allow signal messages from the kernel.
     * 
     * @param v if true, allow delivery of signal messages from the kernel
     * for this protocol.
     */
    void	set_allow_kernel_signal_messages(bool v) { _allow_kernel_signal_messages = v; }
    
    /**
     * Test if the the kernel signal messages are allowed for this protocol.
     * 
     * @return true if the kernel signal messages are allowed, otherwise false.
     */
    bool	is_allow_kernel_signal_messages() const { return (_allow_kernel_signal_messages); }
    
    /**
     * Set/reset the flag whether to allow MRIB (@ref Mrib) messages for
     * this protocol.
     * 
     * @param v if true, allow delivery of MRIB (@ref Mrib) messages for
     * this protocol.
     */
    void	set_allow_mrib_messages(bool v) { _allow_mrib_messages = v; }
    
    /**
     * Test if the the delivery of MRIB (@ref Mrib) messages are allowed
     * for this protocol.
     * 
     * @return true if the delivery is allowed.
     */
    bool	is_allow_mrib_messages() const { return (_allow_mrib_messages); }
    
    /**
     * Get the flag that indicates whether the kernel supports disabling of
     * WRONGVIF signal per (S,G) per interface.
     * 
     * @return true if the kernel supports disabling of WRONGVIF signal
     * per (S,G) per interface, otherwise false.
     */
    bool	mrt_api_mrt_mfc_flags_disable_wrongvif() const {
	return (_mrt_api_mrt_mfc_flags_disable_wrongvif);
    }
    
    /**
     * Get the flag that indicates whether the kernel supports setting of
     * the Border bit flag per (S,G) per interface.
     * 
     * The Border bit flag is used for PIM-SM Register encapsulation in
     * the kernel.
     * 
     * @return true if the kernel supports setting of the Border bit flag
     * per (S,G) per interface, otherwise false.
     */
    bool	mrt_api_mrt_mfc_flags_border_vif() const {
	return (_mrt_api_mrt_mfc_flags_border_vif);
    }
    
    /**
     * Get the flag that indicates whether the kernel supports adding
     * the RP address to the kernel.
     * 
     * The RP address is used for PIM-SM Register encapsulation in
     * the kernel.
     * 
     * @return true if the kernel supports adding the RP address to the kernel,
     * otherwise false.
     */
    bool	mrt_api_mrt_mfc_rp() const {
	return (_mrt_api_mrt_mfc_rp);
    }
    
    /**
     * Get the flag that indicates whether the kernel supports the bandwidth
     * upcall mechanism.
     * 
     * @return true if the kernel supports the bandwidth upcall mechanism.
     */
    bool	mrt_api_mrt_mfc_bw_upcall() const {
	return (_mrt_api_mrt_mfc_bw_upcall);
    }
    
    /**
     * Get the module ID (@ref xorp_module_id) for the protocol that created
     * this entry.
     * 
     * @return the module ID (@ref xorp_module_id) of the protocol that created
     * this entry.
     */
    xorp_module_id module_id() const { return (_module_id); }
    
    
private:
    // Private functions
    MfeaNode&	mfea_node() const	{ return (_mfea_node);	}
    bool	ignore_my_packets() const { return (_ignore_my_packets); }
    int		get_mcast_vifs_osdep(vector<MfeaVif *>& mfea_vifs_vector);
    int		open_mrib_socket_osdep();
    int		open_mrib_table_socket_osdep();
    int		get_mrib_osdep(const IPvX& dest_addr, Mrib& mrib);
    int		get_mrib_table_osdep(Mrib **mrib_table);
    int		kernel_call_process(uint8_t *databuf, size_t datalen);
    
    
    // Private state
    MfeaNode&	  _mfea_node;	// The MFEA node I belong to
    int		  _ipproto;	// The protocol number (IPPROTO_*)
    xorp_module_id _module_id;	// The corresponding module id (XORP_MODULE_*)
    int		  _mrouter_socket; // The socket for multicast routing access
    int		  _ioctl_socket;   // The socket for ioctl() access
    int		  _mrib_socket;	   // The socket for Mrib access
    int		  _proto_socket;   // The socket for protocol message
    uint8_t	  _rcvbuf0[IO_BUF_SIZE];	// Data buffer0 for receiving
    uint8_t	  _sndbuf0[IO_BUF_SIZE];	// Data buffer0 for sending
    uint8_t	  _rcvbuf1[IO_BUF_SIZE];	// Data buffer1 for receiving
    uint8_t	  _sndbuf1[IO_BUF_SIZE];	// Data buffer1 for sending
    uint8_t	  _rcvcmsgbuf[CMSG_BUF_SIZE]; // Control recv info (IPv6 only)
    uint8_t	  _sndcmsgbuf[CMSG_BUF_SIZE]; // Control send info (IPv6 only)
    struct msghdr _rcvmh;	// The msghdr structure used by recvmsg()
    struct msghdr _sndmh;	// The msghdr structure used by sendmsg()
    struct iovec  _rcviov[2];	// The rcvmh scatter/gatter array
    struct iovec  _sndiov[2];	// The sndmh scatter/gatter array
    struct sockaddr_in  _from4;	// The source addr of recvmsg() msg (IPv4)
    struct sockaddr_in  _to4;	// The dest.  addr of sendmsg() msg (IPv4)
#ifdef HAVE_IPV6
    struct sockaddr_in6 _from6;	// The source addr of recvmsg() msg (IPv6)
    struct sockaddr_in6 _to6;	// The dest.  addr of sendmsg() msg (IPv6)
#endif
    
    // IPv4 Router Alert stuff
#ifndef IPTOS_PREC_INTERNETCONTROL
#define IPTOS_PREC_INTERNETCONTROL	0xc0
#endif
#ifndef IPOPT_RA
#define IPOPT_RA			148	/* 0x94 */
#endif
    
    // IPv6 Router Alert stuff
#ifdef HAVE_IPV6
#ifndef IP6OPT_ROUTER_ALERT	// XXX: for compatibility with older systems
#define IP6OPT_ROUTER_ALERT IP6OPT_RTALERT
#endif
    uint16_t	_rtalert_code;
#ifndef HAVE_RFC2292BIS
    uint8_t	_raopt[IP6OPT_RTALERT_LEN];
#endif
#endif // HAVE_IPV6
    
    bool	_ignore_my_packets; // If true, ignore packets originated by me
    bool	_allow_kernel_signal_messages;	// If true, allow kernel signal
						// messages
    bool	_allow_mrib_messages;		// If true, allow MRIB messages
    pid_t	_pid;		// My process ID. Used by the routing sockets.
    static int	_rtm_seq;	// The unique sequence ID for routing requests
				//   to the kernel. XXX: global per process.
    
    //
    // Flags about various support by the advanced kernel multicast API:
    //  - support for disabling WRONGVIF signals per vif
    //  - support for the border bit flag (per MFC per vif)
    //  - support for kernel-level PIM Register encapsulation
    //  - support for bandwidth-related upcalls from the kernel
    //
    bool	_mrt_api_mrt_mfc_flags_disable_wrongvif;
    bool	_mrt_api_mrt_mfc_flags_border_vif;
    bool	_mrt_api_mrt_mfc_rp;
    bool	_mrt_api_mrt_mfc_bw_upcall;
};

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __MFEA_MFEA_UNIX_COMM_HH__
