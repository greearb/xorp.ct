// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libproto/proto_node.hh,v 1.23 2004/06/10 22:41:03 hodson Exp $


#ifndef __LIBPROTO_PROTO_NODE_HH__
#define __LIBPROTO_PROTO_NODE_HH__


#include <map>
#include <vector>

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/status_codes.h"
#include "libxorp/vif.hh"

#include "proto_unit.hh"


//
// Protocol node generic functionality
//


class EventLoop;
class IPvX;
class IPvXNet;

/**
 * @short Base class for a protocol node.
 */
template<class V>
class ProtoNode : public ProtoUnit {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param init_family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively).
     * @param init_module_id the module ID XORP_MODULE_* (@ref xorp_module_id).
     * @param init_eventloop the event loop to use.
     */
    ProtoNode(int init_family, xorp_module_id init_module_id,
	      EventLoop& init_eventloop)
	: ProtoUnit(init_family, init_module_id),
	  _eventloop(init_eventloop),
	  _is_vif_setup_completed(false),
	  _node_status(PROC_NULL),
	  _startup_requests_n(0),
	  _shutdown_requests_n(0) {}
    
    /**
     * Destructor
     */
    virtual ~ProtoNode() {
	// TODO: free vifs (after they are added, etc.
    }
    
    /**
     * Map a vif name to a vif index.
     * 
     * @param vif_name the vif name to map to a vif index.
     * @return the virtual interface index for vif name @ref vif_name.
     */
    inline uint16_t vif_name2vif_index(const string& vif_name) const;
    
    /**
     * Find an unused vif index.
     * 
     * @return the smallest unused vif index if there is one available,
     * otherwise return @ref Vif::VIF_INDEX_INVALID.
     */
    inline uint16_t find_unused_vif_index() const;
    
    /**
     * Find a virtual interface for a given name.
     * 
     * @param name the name to search for.
     * @return the virtual interface with name @ref name if found,
     * otherwise NULL.
     */
    inline V *vif_find_by_name(const string& name) const;
    
    /**
     * Find a virtual interface for a given address.
     * 
     * Note that the PIM Register virtual interfaces are excluded, because
     * they are special, and because they may share the same address as some
     * of the other virtual interfaces.
     * 
     * @param ipaddr_test the address to search for.
     * @return the virtual interface with address @ref ipaddr_test if found,
     * otherwise NULL.
     */
    inline V *vif_find_by_addr(const IPvX& ipaddr_test) const;
    
    /**
     * Find a virtual interface for a given physical interface index.
     * 
     * @param pif_index the physical interface index to search for.
     * @return the virtual interface with physical interface index @ref
     * pif_index if found, otherwise NULL.
     */
    inline V *vif_find_by_pif_index(uint16_t pif_index) const;
    
    /**
     * Find a virtual interface for a given virtual interface index.
     * 
     * @param vif_index the virtual interface index to search for.
     * @return the vvirtual interface with virtual interface index @ref
     * vif_index if found, otherwise NULL.
     */
    inline V *vif_find_by_vif_index(uint16_t vif_index) const;

    /**
     * Find a virtual interface that belongs to the same subnet
     * or point-to-point link as a given address.
     * 
     * Note that the PIM Register virtual interfaces are excluded, because
     * they are special, and because they may share the same address as some
     * of the other virtual interfaces.
     * 
     * @param ipaddr_test the address to search by.
     * @return the virtual interface that belongs to the same subnet
     * or point-to-point link as address @ref ipaddr_test if found,
     * otherwise NULL.
     */
    inline V *vif_find_same_subnet_or_p2p(const IPvX& ipaddr_test) const;

    /**
     * Test if an address belongs to one of my virtual interfaces.
     * 
     * Note that the PIM Register virtual interfaces are excluded, because
     * they are special, and because they may share the same address as some
     * of the other virtual interfaces.
     * 
     * @param ipaddr_test the address to test.
     * @return true if @ref ipaddr_test belongs to one of my virtual
     * interfaces, otherwise false.
     */
     bool is_my_addr(const IPvX& ipaddr_test) const {
	return (vif_find_by_addr(ipaddr_test) != NULL);
    }

    /**
     * Add a virtual interface.
     * 
     * @param vif a pointer to the virtual interface to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    inline int add_vif(V *vif);
    
    /**
     * Delete a virtual interface.
     * 
     * Note: the @ref vif itself is not deleted, only its place in the
     * array of protocol vifs.
     * 
     * @param vif a pointer to the virtual interface to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    inline int delete_vif(const V *vif);
    
    /**
     * Get the array of pointers to the virtual interfaces.
     * 
     * @return the array of pointers to the virtual interfaces.
     */
    vector<V *>& proto_vifs() { return (_proto_vifs);	}

    /**
     * Get the array of pointers to the virtual interfaces.
     * 
     * @return the array of pointers to the virtual interfaces.
     */
    const vector<V *>& const_proto_vifs() const { return (_proto_vifs);	}
    
    /**
     * Get the maximum number of vifs.
     * 
     * Note: the interfaces that are not configured or are down are
     * also included.
     * 
     * @return the maximum number of vifs we can have.
     */
    uint16_t	maxvifs() const { return (_proto_vifs.size()); }

    /**
     * Get the event loop this node is added to.
     * 
     * @return the event loop this node is added to.
     */
    EventLoop& eventloop() { return (_eventloop); }
    
    /**
     * Test if the vif setup is completed.
     * 
     * @return true if the vif setup is completed, otherwise false.
     */
    bool is_vif_setup_completed() const { return (_is_vif_setup_completed); }
    
    /**
     * Set/reset the flag that indicates whether the vif setup is completed.
     * 
     * @param v if true, set the flag that the vif setup is completed,
     * otherwise reset it.
     */
    void set_vif_setup_completed(bool v) { _is_vif_setup_completed = v; }
    
    
    /**
     * Receive a protocol message.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the particular protocol node class that inherits this base class.
     * 
     * @param src_module_instance_name the module instance name of the
     * module-origin of the message.
     * 
     * @param src_module_id the module ID (@ref xorp_module_id) of the
     * module-origin of the message.
     * 
     * @param vif_index the vif index of the interface used to receive this
     * message.
     * 
     * @param src the source address of the message.
     * 
     * @param dst the destination address of the message.
     * 
     * @param ip_ttl the IP TTL (Time To Live) of the message. If it has
     * a negative value, it should be ignored.
     * 
     * @param ip_tos the IP TOS (Type of Service) of the message. If it has
     * a negative value, it should be ignored.
     * 
     * @param router_alert_bool if true, the Router Alert IP option for the IP
     * packet of the incoming message was set.
     * 
     * @param rcvbuf the data buffer with the received message.
     * 
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	proto_recv(const string& src_module_instance_name,
			   xorp_module_id src_module_id,
			   uint16_t vif_index,
			   const IPvX& src,
			   const IPvX& dst,
			   int ip_ttl,
			   int ip_tos,
			   bool router_alert_bool,
			   const uint8_t *rcvbuf,
			   size_t rcvlen) = 0;
    
    /**
     * Send a protocol message.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the particular protocol node class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @param vif_index the vif index of the interface to send this message.
     * 
     * @param src the source address of the message.
     * 
     * @param dst the destination address of the message.
     * 
     * @param ip_ttl the IP TTL of the message. If it has a negative value,
     * the TTL will be set by the lower layers.
     * 
     * @param ip_tos the IP TOS of the message. If it has a negative value,
     * the TOS will be set by the lower layers.
     * 
     * @param router_alert_bool if true, set the Router Alert IP option for
     * the IP packet of the outgoung message.
     * 
     * @param sndbuf the data buffer with the outgoing message.
     * 
     * @param sndlen the data length in @ref sndbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	proto_send(const string& dst_module_instance_name,
			   xorp_module_id dst_module_id,
			   uint16_t vif_index,
			   const IPvX& src,
			   const IPvX& dst,
			   int ip_ttl,
			   int ip_tos,
			   bool router_alert_bool,
			   const uint8_t *sndbuf,
			   size_t sndlen) = 0;
    
    /**
     * Receive a signal message.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the particular protocol node class that inherits this base class.
     * 
     * @param src_module_instance_name the module instance name of the
     * module-origin of the message.
     * 
     * @param src_module_id the module ID (@ref xorp_module_id) of the
     * module-origin of the message.
     * 
     * @param message_type the message type. The particular values are
     * specific for the origin and recepient of this signal message.
     * 
     * @param vif_index the vif index of the related interface
     * (message-specific relation).
     * 
     * @param src the source address of the message. The exact meaning of
     * this address is message-specific.
     * 
     * @param dst the destination address of the message. The exact meaning of
     * this address is message-specific.
     * 
     * @param rcvbuf the data buffer with the additional information in
     * the message.
     * @param rcvlen the data length in @ref rcvbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	signal_message_recv(const string& src_module_instance_name,
				    xorp_module_id src_module_id,
				    int message_type,
				    uint16_t vif_index,
				    const IPvX& src,
				    const IPvX& dst,
				    const uint8_t *rcvbuf,
				    size_t rcvlen) = 0;
    
    /**
     * Send a signal message.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the particular protocol node class that inherits this base class.
     * 
     * @param dst_module_instance_name the module instance name of the
     * module-recepient of the message.
     * 
     * @param dst_module_id the module ID (@ref xorp_module_id) of the
     * module-recepient of the message.
     * 
     * @param message_type the message type. The particular values are
     * specific for the origin and recepient of this signal message.
     * 
     * @param vif_index the vif index of the related interface
     * (message-specific relation).
     * 
     * @param src the source address of the message. The exact meaning of
     * this address is message-specific.
     * 
     * @param dst the destination address of the message. The exact meaning of
     * this address is message-specific.
     * 
     * @param sndbuf the data buffer with the outgoing message.
     * 
     * @param sndlen the data length in @ref sndbuf.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int	signal_message_send(const string& dst_module_instance_name,
				    xorp_module_id dst_module_id,
				    int message_type,
				    uint16_t vif_index,
				    const IPvX& src,
				    const IPvX& dst,
				    const uint8_t *sndbuf,
				    size_t sndlen) = 0;

    /**
     * Test if the node processing is done.
     *
     * @return true if the node processing is done, otherwise false.
     */
    bool	is_done() const { return (_node_status == PROC_DONE); }

    /**
     * Get the node status (see @ref ProcessStatus).
     * 
     * @return the node status (see @ref ProcessStatus).
     */
    ProcessStatus node_status() const { return (_node_status); }
    
    /**
     * Set the node status (see @ref ProcessStatus).
     * 
     * @param v the value to set the node status to.
     */
    void set_node_status(ProcessStatus v) { _node_status = v; }
    
    /**
     * Start a set of configuration changes.
     * 
     * Note that it may change the node status.
     * 
     * @error_msg: The error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start_config(string& error_msg);
    
    /**
     * End a set of configuration changes.
     * 
     * Note that it may change the node status.
     * 
     * @error_msg: The error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		end_config(string& error_msg);

    /**
     * Find an unused vif index from the set of configured vifs.
     * 
     * @return the smallest unused vif index from the set of configured vifs
     * if there is one available, otherwise return @ref Vif::VIF_INDEX_INVALID.
     */
    inline uint16_t find_unused_config_vif_index() const;

    /**
     * Add a configured vif.
     * 
     * @param vif the vif with the information to add.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_config_vif(const Vif& vif, string& error_msg);
    
    /**
     * Add a configured vif.
     * 
     * @param vif_name the name of the vif to add.
     * @param vif_index the vif index of the vif to add.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_config_vif(const string& vif_name, uint16_t vif_index,
			       string& error_msg);

    /**
     * Delete a configured vif.
     * 
     * @param vif_name the name of the vif to delete.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_config_vif(const string& vif_name, string& error_msg);

    /**
     * Add an address to a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the address to add.
     * @param subnet the subnet address to add.
     * @param broadcast the broadcast address to add.
     * @param peer the peer address to add.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_config_vif_addr(const string& vif_name,
				    const IPvX& addr,
				    const IPvXNet& subnet,
				    const IPvX& broadcast,
				    const IPvX& peer,
				    string& error_msg);
    
    /**
     * Delete an address from a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param addr the address to delete.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		delete_config_vif_addr(const string& vif_name,
				       const IPvX& addr,
				       string& error_msg);
    
    /**
     * Set the pif_index of a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param pif_index the physical interface index.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_pif_index(const string& vif_name,
				     uint16_t pif_index,
				     string& error_msg);
    
    /**
     * Set the vif flags of a configured vif.
     * 
     * @param vif_name the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @error_msg: The error message (if error).
     * @return  XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_config_vif_flags(const string& vif_name,
				     bool is_pim_register,
				     bool is_p2p,
				     bool is_loopback,
				     bool is_multicast,
				     bool is_broadcast,
				     bool is_up,
				     string& error_msg);

    /**
     * Get the map with configured vifs.
     * 
     * @return a reference for the map with configured vifs.
     */
    map<string, Vif>& configured_vifs() { return (_configured_vifs); }

    /**
     * Get the node status (see @ref ProcessStatus).
     * 
     * @param reason_msg return-by-reference string that contains
     * human-readable information about the status.
     * @return the node status (see @ref ProcessStatus).
     */
    ProcessStatus	node_status(string& reason_msg);
    
protected:
    void incr_startup_requests_n();
    void decr_startup_requests_n();
    void incr_shutdown_requests_n();
    void decr_shutdown_requests_n();
    void update_status();
    
private:
    // TODO: add vifs, etc
    
    vector<V *> _proto_vifs;	// The array with all protocol vifs
    EventLoop&	_eventloop;	// The event loop to use
    
    map<string, uint16_t> _vif_name2vif_index_map;
    
    bool	_is_vif_setup_completed;	// True if the vifs are setup
    
    ProcessStatus	_node_status;		// The node status

    //
    // Status-related state
    //
    size_t		_startup_requests_n;
    size_t		_shutdown_requests_n;
    
    //
    // Config-related state
    //
    map<string, Vif>	_configured_vifs;	// Configured vifs
};

//
// Deferred definitions
//

template<class V>
ProcessStatus
ProtoNode<V>::node_status(string& reason_msg)
{
    ProcessStatus status = _node_status;

    // Set the return message with the reason
    reason_msg = "";
    switch (status) {
    case PROC_NULL:
	// Can't be running and in this state
	XLOG_UNREACHABLE();
	break;
    case PROC_STARTUP:
	// Get the message about the startup progress
	reason_msg = c_format("Waiting for %u startup events",
			      static_cast<uint32_t>(_startup_requests_n));
	break;
    case PROC_NOT_READY:
	reason_msg = c_format("Waiting for configuration completion");
	break;
    case PROC_READY:
	reason_msg = c_format("Node is READY");
	break;
    case PROC_SHUTDOWN:
	// Get the message about the shutdown progress
	reason_msg = c_format("Waiting for %u shutdown events",
			      static_cast<uint32_t>(_shutdown_requests_n));
	break;
    case PROC_FAILED:
	reason_msg = c_format("Node is PROC_FAILED");
	break;
    case PROC_DONE:
	// Process has completed operation
	break;
    default:
	// Unknown status
	XLOG_UNREACHABLE();
	break;
    }
    
    return (status);
}

template<class V>
inline void
ProtoNode<V>::incr_startup_requests_n()
{
    _startup_requests_n++;
    XLOG_ASSERT(_startup_requests_n > 0);
}

template<class V>
inline void
ProtoNode<V>::decr_startup_requests_n()
{
    XLOG_ASSERT(_startup_requests_n > 0);
    _startup_requests_n--;

    update_status();
}

template<class V>
inline void
ProtoNode<V>::incr_shutdown_requests_n()
{
    _shutdown_requests_n++;
    XLOG_ASSERT(_shutdown_requests_n > 0);
}

template<class V>
inline void
ProtoNode<V>::decr_shutdown_requests_n()
{
    XLOG_ASSERT(_shutdown_requests_n > 0);
    _shutdown_requests_n--;

    update_status();
}

template<class V>
inline void
ProtoNode<V>::update_status()
{
    //
    // Test if the startup process has completed
    //
    if (ServiceBase::status() == STARTING) {
	if (_startup_requests_n > 0)
	    return;

	// The startup process has completed
	ServiceBase::set_status(RUNNING);
	_node_status = PROC_READY;
	return;
    }

    //
    // Test if the shutdown process has completed
    //
    if (ServiceBase::status() == SHUTTING_DOWN) {
	if (_shutdown_requests_n > 0)
	    return;

	// The shutdown process has completed
	ServiceBase::set_status(SHUTDOWN);
	// Set the node status
	_node_status = PROC_DONE;
	return;
    }

    //
    // Test if we have failed
    //
    if (ServiceBase::status() == FAILED) {
	// Set the node status
	_node_status = PROC_DONE;
	return;
    }
}

template<class V>
inline uint16_t
ProtoNode<V>::vif_name2vif_index(const string& vif_name) const
{
    map<string, uint16_t>::const_iterator iter;
    
    iter = find(_vif_name2vif_index_map.find(vif_name));
    if (iter != _vif_name2vif_index_map.end())
	return (iter->second);
    return (Vif::VIF_INDEX_INVALID);
}

template<class V>
inline uint16_t
ProtoNode<V>::find_unused_vif_index() const
{
    for (uint16_t i = 0; i < _proto_vifs.size(); i++) {
	if (_proto_vifs[i] == NULL)
	    return (i);
    }
    
    if (maxvifs() + 1 >= Vif::VIF_INDEX_MAX)
	return (Vif::VIF_INDEX_INVALID);
    
    return (maxvifs());
}

template<class V>
inline V *
ProtoNode<V>::vif_find_by_name(const string& name) const
{
    typename vector<V *>::const_iterator iter;
    
    for (iter = _proto_vifs.begin(); iter != _proto_vifs.end(); ++iter) {
	V *vif = *iter;
	if (vif == NULL)
	    continue;
	if (vif->name() == name)
	    return (vif);
    }
    
    return (NULL);
}

template<class V>
inline V *
ProtoNode<V>::vif_find_by_addr(const IPvX& ipaddr_test) const
{
    typename vector<V *>::const_iterator iter;
    
    for (iter = _proto_vifs.begin(); iter != _proto_vifs.end(); ++iter) {
	V *vif = *iter;
	if (vif == NULL)
	    continue;
	//
	// XXX: exclude the PIM Register vifs, because they are special
	//
	if (vif->is_pim_register())
	    continue;
	if (vif->is_my_addr(ipaddr_test))
	    return (vif);
    }
    
    return (NULL);
}

template<class V>
inline V *
ProtoNode<V>::vif_find_by_pif_index(uint16_t pif_index) const
{
    typename vector<V *>::const_iterator iter;
    
    for (iter = _proto_vifs.begin(); iter != _proto_vifs.end(); ++iter) {
	V *vif = *iter;
	if (vif == NULL)
	    continue;
	if (vif->pif_index() == pif_index)
	    return (vif);
    }
    
    return (NULL);
}

template<class V>
inline V *
ProtoNode<V>::vif_find_by_vif_index(uint16_t vif_index) const
{
    if (vif_index < _proto_vifs.size()) {
	// XXX: if vif_index becomes signed, we must check (vif_index >= 0)
	return (_proto_vifs[vif_index]);
    }
    return (NULL);
}

template<class V>
inline V *
ProtoNode<V>::vif_find_same_subnet_or_p2p(const IPvX& ipaddr_test) const
{
    typename vector<V *>::const_iterator iter;
    
    for (iter = _proto_vifs.begin(); iter != _proto_vifs.end(); ++iter) {
	V *vif = *iter;
	if (vif == NULL)
	    continue;
	//
	// XXX: exclude the PIM Register vifs, because they are special
	//
	if (vif->is_pim_register())
	    continue;
	if (vif->is_same_subnet(ipaddr_test) || vif->is_same_p2p(ipaddr_test))
	    return (vif);
    }
    
    return (NULL);
}

template<class V>
inline int
ProtoNode<V>::add_vif(V *vif)
{
    // XXX: vif setup is not completed, if we are trying to add a vif
    _is_vif_setup_completed = false;
    
    if (vif == NULL) {
	XLOG_ERROR("Cannot add NULL vif");
	return (XORP_ERROR);
    }
    
    if (vif_find_by_name(vif->name()) != NULL) {
	XLOG_ERROR("Cannot add vif %s: already exist",
		   vif->name().c_str());
	return (XORP_ERROR);
    }
    if (vif_find_by_vif_index(vif->vif_index()) != NULL) {
	XLOG_ERROR("Cannot add vif %s with vif_index = %d: "
		   "already exist vif with such vif_index",
		   vif->name().c_str(), vif->vif_index());
	return (XORP_ERROR);
    }
    // XXX: we should check for the pif_index as well, but on older
    // systems the kernel doesn't assign pif_index to the interfaces
    
    //
    // Add enough empty entries for the new vif
    //
    while (vif->vif_index() >= maxvifs()) {
	_proto_vifs.push_back(NULL);
    }
    XLOG_ASSERT(_proto_vifs[vif->vif_index()] == NULL);
    
    //
    // Add the new vif
    //
    _proto_vifs[vif->vif_index()] = vif;
    
    // Add the entry to the vif_name2vif_index map
    _vif_name2vif_index_map.insert(
	pair<string, uint16_t>(vif->name(), vif->vif_index()));
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::delete_vif(const V *vif)
{
    // XXX: vif setup is not completed, if we are trying to delete a vif
    _is_vif_setup_completed = false;
    
    if (vif == NULL) {
	XLOG_ERROR("Cannot delete NULL vif");
	return (XORP_ERROR);
    }
    
    if (vif_find_by_name(vif->name()) != vif) {
	XLOG_ERROR("Cannot delete vif %s: inconsistent data pointers",
		   vif->name().c_str());
	return (XORP_ERROR);
    }
    if (vif_find_by_vif_index(vif->vif_index()) != vif) {
	XLOG_ERROR("Cannot delete vif %s with vif_index = %d: "
		   "inconsistent data pointers",
		   vif->name().c_str(), vif->vif_index());
	return (XORP_ERROR);
    }
    
    XLOG_ASSERT(vif->vif_index() < maxvifs());
    XLOG_ASSERT(_proto_vifs[vif->vif_index()] == vif);
    
    _proto_vifs[vif->vif_index()] = NULL;
    
    //
    // Remove unused vif pointers from the back of the vif array
    //
    while (_proto_vifs.size()) {
	size_t i = _proto_vifs.size() - 1;
	if (_proto_vifs[i] != NULL)
	    break;
	_proto_vifs.pop_back();
    }
    
    // Remove the entry from the vif_name2vif_index map
    map<string, uint16_t>::iterator iter;
    iter = _vif_name2vif_index_map.find(vif->name());
    XLOG_ASSERT(iter != _vif_name2vif_index_map.end());
    _vif_name2vif_index_map.erase(iter);
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::start_config(string& error_msg)
{
    switch (node_status()) {
    case PROC_NOT_READY:
	break;	// OK, probably the first set of configuration changes,
		// or a batch of configuration changes that call end_config()
		// at the end.
    case PROC_READY:
	set_node_status(PROC_NOT_READY);
	break;	// OK, start a set of configuration changes
    case PROC_STARTUP:
	break;	// OK, we are still in the startup state
    case PROC_SHUTDOWN:
	error_msg = "invalid start config in PROC_SHUTDOWN state";
	return (XORP_ERROR);
    case PROC_FAILED:
	error_msg = "invalid start config in PROC_FAILED state";
	return (XORP_ERROR);
    case PROC_DONE:
	error_msg = "invalid start config in PROC_DONE state";
	return (XORP_ERROR);
    case PROC_NULL:
	// FALLTHROUGH
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::end_config(string& error_msg)
{
    switch (node_status()) {
    case PROC_NOT_READY:
	set_node_status(PROC_READY);
	break;	// OK, end a set of configuration changes
    case PROC_READY:
	break;	// OK, maybe we got into PROC_READY directly from PROC_STARTUP
    case PROC_STARTUP:
	break;	// OK, we are still in the startup state
    case PROC_SHUTDOWN:
	error_msg = "invalid end config in PROC_SHUTDOWN state";
	return (XORP_ERROR);
    case PROC_FAILED:
	error_msg = "invalid end config in PROC_FAILED state";
	return (XORP_ERROR);
    case PROC_DONE:
	error_msg = "invalid end config in PROC_DONE state";
	return (XORP_ERROR);
    case PROC_NULL:
	// FALLTHROUGH
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

template<class V>
inline uint16_t
ProtoNode<V>::find_unused_config_vif_index() const
{
    map<string, Vif>::const_iterator iter;
    
    for (uint16_t i = 0; i < Vif::VIF_INDEX_INVALID; i++) {
	bool is_avail = true;
	// Check if this vif index is in use
	for (iter = _configured_vifs.begin();
	     iter != _configured_vifs.end();
	     ++iter) {
	    const Vif& vif = iter->second;
	    if (vif.vif_index() == i) {
		is_avail = false;
		break;
	    }
	}
	if (is_avail)
	    return (i);
    }
    
    return (Vif::VIF_INDEX_INVALID);
}

template<class V>
inline int
ProtoNode<V>::add_config_vif(const Vif& vif, string& error_msg)
{
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    if (add_config_vif(vif.name(), vif.vif_index(), error_msg) < 0)
	return (XORP_ERROR);
    
    list<VifAddr>::const_iterator vif_addr_iter;
    for (vif_addr_iter = vif.addr_list().begin();
	 vif_addr_iter != vif.addr_list().end();
	 ++vif_addr_iter) {
	const VifAddr& vif_addr = *vif_addr_iter;
	if (add_config_vif_addr(vif.name(),
				vif_addr.addr(),
				vif_addr.subnet_addr(),
				vif_addr.broadcast_addr(),
				vif_addr.peer_addr(),
				error_msg) < 0) {
	    string dummy_error_msg;
	    delete_config_vif(vif.name(), dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    
    if (set_config_pif_index(vif.name(),
			     vif.pif_index(),
			     error_msg) < 0) {
	string dummy_error_msg;
	delete_config_vif(vif.name(), dummy_error_msg);
	return (XORP_ERROR);
    }
    
    if (set_config_vif_flags(vif.name(),
			     vif.is_pim_register(),
			     vif.is_p2p(),
			     vif.is_loopback(),
			     vif.is_multicast_capable(),
			     vif.is_broadcast_capable(),
			     vif.is_underlying_vif_up(),
			     error_msg) < 0) {
	string dummy_error_msg;
	delete_config_vif(vif.name(), dummy_error_msg);
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::add_config_vif(const string& vif_name, uint16_t vif_index,
			     string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    // Check whether we have vif with same name
    iter = _configured_vifs.find(vif_name);
    if (iter != _configured_vifs.end()) {
	error_msg = c_format("Cannot add vif %s: already have such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // Check whether we have vif with same vif_index
    for (iter = _configured_vifs.begin();
	 iter != _configured_vifs.end();
	 ++iter) {
	Vif* tmp_vif = &iter->second;
	if (tmp_vif->vif_index() == vif_index) {
	    error_msg = c_format("Cannot add vif %s with vif_index %d: "
				 "already have vif %s with same vif_index",
				 vif_name.c_str(), vif_index,
				 tmp_vif->name().c_str());
	    XLOG_ERROR(error_msg.c_str());
	    return (XORP_ERROR);
	}
    }
    
    // Insert the new vif
    Vif vif(vif_name);
    vif.set_vif_index(vif_index);
    _configured_vifs.insert(make_pair(vif_name, vif));
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::delete_config_vif(const string& vif_name, string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    // Find the vif
    iter = _configured_vifs.find(vif_name);
    if (iter == _configured_vifs.end()) {
	error_msg = c_format("Cannot delete vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // Delete the vif
    _configured_vifs.erase(iter);
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::add_config_vif_addr(const string& vif_name, const IPvX& addr,
				  const IPvXNet& subnet, const IPvX& broadcast,
				  const IPvX& peer, string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    // Find the vif
    iter = _configured_vifs.find(vif_name);
    if (iter == _configured_vifs.end()) {
	error_msg = c_format("Cannot add address to vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    Vif* vif = &iter->second;
    
    // Test if we have same address
    if (vif->find_address(addr) != NULL) {
	error_msg = c_format("Cannot add address %s to vif %s: "
			     "already have such address",
			     cstring(addr), vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // Add the address
    vif->add_address(addr, subnet, broadcast, peer);
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::delete_config_vif_addr(const string& vif_name, const IPvX& addr,
				     string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    // Find the vif
    iter = _configured_vifs.find(vif_name);
    if (iter == _configured_vifs.end()) {
	error_msg = c_format("Cannot delete address from vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    Vif* vif = &iter->second;
    
    // Test if we have this address
    if (vif->find_address(addr) == NULL) {
	error_msg = c_format("Cannot delete address %s from vif %s: "
			     "no such address",
			     cstring(addr), vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
    }
    
    // Delete the address
    vif->delete_address(addr);
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::set_config_pif_index(const string& vif_name,
				   uint16_t pif_index,
				   string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // Find the vif
    iter = _configured_vifs.find(vif_name);
    if (iter == _configured_vifs.end()) {
	error_msg = c_format("Cannot set pif_index for vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    Vif* vif = &iter->second;
    
    vif->set_pif_index(pif_index);
    
    return (XORP_OK);
}

template<class V>
inline int
ProtoNode<V>::set_config_vif_flags(const string& vif_name,
				   bool is_pim_register,
				   bool is_p2p,
				   bool is_loopback,
				   bool is_multicast,
				   bool is_broadcast,
				   bool is_up,
				   string& error_msg)
{
    map<string, Vif>::iterator iter;
    
    if (start_config(error_msg) != XORP_OK)
	return (XORP_ERROR);
    
    // Find the vif
    iter = _configured_vifs.find(vif_name);
    if (iter == _configured_vifs.end()) {
	error_msg = c_format("Cannot set flags for vif %s: no such vif",
			     vif_name.c_str());
	XLOG_ERROR(error_msg.c_str());
	return (XORP_ERROR);
    }
    
    Vif* vif = &iter->second;
    
    vif->set_pim_register(is_pim_register);
    vif->set_p2p(is_p2p);
    vif->set_loopback(is_loopback);
    vif->set_multicast_capable(is_multicast);
    vif->set_broadcast_capable(is_broadcast);
    vif->set_underlying_vif_up(is_up);
    
    return (XORP_OK);
}


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __LIBPROTO_PROTO_NODE_HH__
