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

// $XORP: xorp/libproto/proto_node.hh,v 1.4 2003/03/10 23:20:20 hodson Exp $


#ifndef __LIBPROTO_PROTO_NODE_HH__
#define __LIBPROTO_PROTO_NODE_HH__


#include <map>
#include <vector>

#include "libxorp/xorp.h"
#include "libxorp/callback.hh"
#include "proto_unit.hh"


//
// Protocol node generic functionality
//


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class EventLoop;
class IPvX;
class Vif;

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
     * @param init_event_loop the event loop to use.
     */
    ProtoNode(int init_family, xorp_module_id init_module_id,
	      EventLoop& init_event_loop)
	: ProtoUnit(init_family, init_module_id),
	  _event_loop(init_event_loop),
	  _is_vif_setup_completed(false) {}
    
    /**
     * Destructor
     */
    virtual ~ProtoNode() {
	// TODO: free vifs (after they are added, etc.
    }
    
#if 0		// TODO: add it later to start/stop all vifs, etc??
    int		start();
    int		stop();
    void	enable();
    void	disable();
#endif
    
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
     * Find a virtual interface that is directly connected to the given
     * address.
     * 
     * @param ipaddr_test the address to search by.
     * @return the virtual interface that is directly connected to
     * address @ref ipaddr_test if found, otherwise NULL.
     */
    inline V *vif_find_direct(const IPvX& ipaddr_test) const;
    
    /**
     * Test if an address is directly connected to one of my virtual
     * interfaces.
     * 
     * @param ipaddr_test the address to test.
     * @return true if @ref ipaddr_test is directly connected to one of
     * my virtual interfaces, otherwise false.
     */
    bool is_directly_connected(const IPvX& ipaddr_test) const {
	return (vif_find_direct(ipaddr_test) != NULL);
    }
    
    /**
     * Test if an address belongs to one of my virtual interfaces.
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
    EventLoop& event_loop() { return (_event_loop); }
    
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
     * @param router_alert_bool if true, the ROUTER_ALERT IP option for the IP
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
     * @param router_alert_bool if true, set the ROUTER_ALERT IP option for
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
    
private:
    // TODO: add vifs, etc
    
    vector<V *> _proto_vifs;	// The array with all protocol vifs
    EventLoop&	_event_loop;	// The event loop to use
    
    map<string, uint16_t> _vif_name2vif_index_map;
    
    bool	_is_vif_setup_completed; // True if the vifs are setup
};

//
// Deferred definitions
//
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
ProtoNode<V>::vif_find_direct(const IPvX& ipaddr_test) const
{
    typename vector<V *>::const_iterator iter;
    
    for (iter = _proto_vifs.begin(); iter != _proto_vifs.end(); ++iter) {
	V *vif = *iter;
	if (vif->is_directly_connected(ipaddr_test))
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


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __LIBPROTO_PROTO_NODE_HH__
