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

// $XORP: xorp/fea/mfea_mrouter.hh,v 1.6 2003/08/28 16:16:13 pavlin Exp $


#ifndef __FEA_MFEA_MROUTER_HH__
#define __FEA_MFEA_MROUTER_HH__


//
// Multicast routing kernel-access specific definitions.
//

#include <sys/uio.h>

#include "libxorp/eventloop.hh"
#include "libproto/proto_register.hh"
#include "libproto/proto_unit.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class IPvX;
class MfeaNode;
class SgCount;
class TimeVal;
class VifCount;


/**
 * @short A class for multicast routing related I/O communication.
 * 
 * In case of UNIX kernels, we cannot have more than one MfeaMrouter
 * per address family (i.e., one per IPv4, and one per IPv6).
 */
class MfeaMrouter : public ProtoUnit {
public:
    /**
     * Constructor for given MFEA node.
     * 
     * @param mfea_node the MFEA node (@ref MfeaNode) this entry belongs to.
     */
    MfeaMrouter(MfeaNode& mfea_node);
    
    /**
     * Destructor
     */
    virtual ~MfeaMrouter();
    
    /**
     * Start the @ref MfeaMrouter.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the @ref MfeaMrouter.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();
    
    /**
     * Test if the underlying system supports IPv4 multicast routing.
     * 
     * @return true if the underlying system supports IPv4 multicast routing,
     * otherwise false.
     */
    bool have_multicast_routing4() const;
    
    /**
     * Test if the underlying system supports IPv6 multicast routing.
     * 
     * @return true if the underlying system supports IPv6 multicast routing,
     * otherwise false.
     */
    bool have_multicast_routing6() const;
    
    /**
     * Get the protocol that would be used in case of mrouter socket.
     * 
     * Return value: the protocol number on success, otherwise XORP_ERROR.
     **/
    int		kernel_mrouter_ipproto() const;
    
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
     * Adopt control over the mrouter socket.
     * 
     * When the MfeaMrouter adopts control over the mrouter socket,
     * it is the one that will be reading from that socket.
     * 
     * @return the socket value on success, otherwise XORP_ERROR.
     */
    int		adopt_mrouter_socket();
    
    /**
     * Close the mrouter socket.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		close_mrouter_socket();
    
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
			      const TimeVal& threshold_interval,
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
				 const TimeVal& threshold_interval,
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
     * Process a call from the kernel (e.g., "nocache", "wrongiif", "wholepkt")
     * XXX: It is OK for im_src/im6_src to be 0 (for 'nocache' or 'wrongiif'),
     *	just in case the kernel supports (*,G) MFC.
     * 
     * @param databuf the data buffer.
     * @param datalen the length of the data in @ref databuf.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		kernel_call_process(uint8_t *databuf, size_t datalen);
    
private:
    // Private functions
    MfeaNode&	mfea_node() const	{ return (_mfea_node);	}
    
    /**
     * Read data from the multicast router socket.
     *
     * This is called as a SelectorList callback.
     * @param fd file descriptor that with event caused this method to be
     * called.
     * @param m mask representing event type.
     */
    void	mrouter_socket_read(int fd, SelectorMask m);
    
    // Private state
    MfeaNode&	  _mfea_node;	// The MFEA node I belong to
    int		  _mrouter_socket; // The socket for multicast routing access
    uint8_t*	  _rcvbuf0;	// Data buffer0 for receiving
    uint8_t*	  _sndbuf0;	// Data buffer0 for sending
    uint8_t*	  _rcvbuf1;	// Data buffer1 for receiving
    uint8_t*	  _sndbuf1;	// Data buffer1 for sending
    uint8_t*	  _rcvcmsgbuf;	// Control recv info (IPv6 only)
    uint8_t*	  _sndcmsgbuf;	// Control send info (IPv6 only)
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

//
// Global variables
//


//
// Global functions prototypes
//

#endif // __FEA_MFEA_MROUTER_HH__
