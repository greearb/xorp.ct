// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.48 2007/05/01 21:27:40 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__

#include "iftree.hh"
#include "fea/data_plane/control_socket/netlink_socket.hh"

#include <map>

class IfConfig;
class RunCommand;

class IfConfigSet {
public:
    IfConfigSet(IfConfig& ifconfig)
	: _is_running(false),
	  _ifconfig(ifconfig),
	  _is_primary(true)
    {}
    virtual ~IfConfigSet() {}
    
    IfConfig&	ifconfig() { return _ifconfig; }
    
    virtual void set_primary() { _is_primary = true; }
    virtual void set_secondary() { _is_primary = false; }
    virtual bool is_primary() const { return _is_primary; }
    virtual bool is_secondary() const { return !_is_primary; }
    virtual bool is_running() const { return _is_running; }

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;

    /**
     * Push the network interface configuration into the underlying system.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param config the interface tree configuration to push.
     * @return true on success, otherwise false.
     */
    virtual bool push_config(IfTree& config);

protected:
    /**
     * Determine if the interface's underlying provider implements discard
     * semantics natively, or if they are emulated through other means.
     *
     * @param i the interface item to inspect.
     * @return true if discard semantics are emulated.
     */
    virtual bool is_discard_emulated(const IfTreeInterface& i) const = 0;

    /**
     * Start the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_begin(string& error_msg) = 0;

    /**
     * Complete the configuration.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_end(string& error_msg) = 0;

    /**
     * Add an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg) = 0;

    /**
     * Add a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg) = 0;

    /**
     * Configure an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param flags the flags to set on the interface.
     * @param is_up if true, the interface is UP, otherwise is DOWN.
     * @param is_deleted if true, the interface is deleted.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg) = 0;

    /**
     * Configure a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param flags the flags to set on the vif.
     * @param is_up if true, the vif is UP, otherwise is DOWN.
     * @param is_deleted if true, the vif is deleted.
     * @param broadcast if true, the vif is broadcast capable.
     * @param loopback if true, the vif corresponds to the loopback interface.
     * @param point_to_point if true, the vif is a point-to-point interface.
     * @param multicast if true, the vif is multicast capable.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg) = 0;

    /**
     * Set the MAC address of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param ether_addr the Ethernet MAC address to set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg) = 0;

    /**
     * Set the MTU of an interface.
     *
     * @param ifname the interface name.
     * @param if_index the interface index.
     * @param mtu the MTU to set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg) = 0;

    /**
     * Add an address to a vif.
     *
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param is_broadcast true if @ref dst_or_bcast is a broadcast address.
     * @param is_p2p true if dst_or_bcast is a destination/peer address.
     * @param addr the address to add.
     * @param dst_or_bcast the broadcast or the destination/peer address.
     * @param prefix_len the prefix length of the subnet mask.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg) = 0;

    /**
     * Delete an address from a vif. 
     * 
     * @param ifname the interface name.
     * @param vifname the vif name.
     * @param if_index the interface index.
     * @param addr the address to delete.
     * @param prefix_len the prefix length of the subnet mask.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg) = 0;

protected:
    // Misc other state
    bool	_is_running;

private:
    void push_iftree_begin();
    void push_iftree_end();
    void push_interface_begin(const IfTreeInterface& i);
    void push_interface_end(IfTreeInterface& i);
    void push_vif_begin(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_end(const IfTreeInterface& i, const IfTreeVif& v);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr4& a);
    void push_vif_address(const IfTreeInterface& i, const IfTreeVif& v,
			  const IfTreeAddr6& a);

    IfConfig&	_ifconfig;
    bool	_is_primary;	// True -> primary, false -> secondary method
};

class IfConfigSetDummy : public IfConfigSet {
public:
    IfConfigSetDummy(IfConfig& ifconfig);
    virtual ~IfConfigSetDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Push the network interface configuration into the underlying system.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param config the interface tree configuration to push.
     * @return true on success, otherwise false.
     */
    virtual bool push_config(IfTree& config);

private:
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);
};

class IfConfigSetIoctl : public IfConfigSet {
public:
    IfConfigSetIoctl(IfConfig& ifconfig);
    virtual ~IfConfigSetIoctl();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

private:
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);

    virtual int add_vif_address4(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_broadcast,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst_or_bcast,
				 uint32_t prefix_len,
				 string& error_msg);
    virtual int add_vif_address6(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst,
				 uint32_t prefix_len,
				 string& error_msg);

    int _s4;
    int _s6;
};

class IfConfigSetNetlinkSocket : public IfConfigSet,
				 public NetlinkSocket {
public:
    IfConfigSetNetlinkSocket(IfConfig& ifconfig);
    virtual ~IfConfigSetNetlinkSocket();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

private:
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);

    NetlinkSocketReader	_ns_reader;
};

class IfConfigSetClick : public IfConfigSet,
			 public ClickSocket {
private:
    class ClickConfigGenerator;

public:
    IfConfigSetClick(IfConfig& ifconfig);
    virtual ~IfConfigSetClick();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Get a reference to the @ref IfTree instance.
     *
     * @return a reference to the @ref IfTree instance.
     */
    const IfTree& iftree() const { return _iftree; }

    /**
     * Receive a signal the work of a Click configuration generator is done.
     *
     * @param click_config_generator a pointer to the @ref
     * IfConfigSetClick::ClickConfigGenerator instance that has completed
     * its work.
     * @param success if true, then the generator has successfully finished
     * its work, otherwise false.
     * @param error_msg the error message (if error).
     */
    void click_config_generator_done(
	IfConfigSetClick::ClickConfigGenerator* click_config_generator,
	bool success,
	const string& error_msg);

private:
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);

    int execute_click_config_generator(string& error_msg);
    void terminate_click_config_generator();
    int write_generated_config(bool is_kernel_click,
			       const string& kernel_config,
			       bool is_user_click,
			       const string& user_config,
			       string& error_msg);
    string regenerate_xorp_iftree_config() const;
    string regenerate_xorp_fea_click_config() const;

    /**
     * Generate the next-hop to port mapping.
     *
     * @return the number of generated ports.
     */
    int generate_nexthop_to_port_mapping();

    ClickSocketReader	_cs_reader;
    IfTree		_iftree;

    class ClickConfigGenerator {
    public:
	ClickConfigGenerator(IfConfigSetClick& ifconfig_set_click,
			     const string& command_name);
	~ClickConfigGenerator();
	int execute(const string& xorp_config, string& error_msg);

	const string& command_name() const { return _command_name; }
	const string& command_stdout() const { return _command_stdout; }

    private:
	void stdout_cb(RunCommand* run_command, const string& output);
	void stderr_cb(RunCommand* run_command, const string& output);
	void done_cb(RunCommand* run_command, bool success,
		     const string& error_msg);

	IfConfigSetClick& _ifconfig_set_click;
	EventLoop&	_eventloop;
	string		_command_name;
	list<string>	_command_argument_list;
	RunCommand*	_run_command;
	string		_command_stdout;
	string		_tmp_filename;
    };

    ClickConfigGenerator*	_kernel_click_config_generator;
    ClickConfigGenerator*	_user_click_config_generator;
    bool			_has_kernel_click_config;
    bool			_has_user_click_config;
    string			_generated_kernel_click_config;
    string			_generated_user_click_config;
};

class IfConfigSetIPHelper : public IfConfigSet {
public:
    IfConfigSetIPHelper(IfConfig& ifconfig);
    virtual ~IfConfigSetIPHelper();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);

    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

private:
    virtual bool is_discard_emulated(const IfTreeInterface& i) const;
    virtual int config_begin(string& error_msg);
    virtual int config_end(string& error_msg);
    virtual int add_interface(const string& ifname,
			      uint32_t if_index,
			      string& error_msg);
    virtual int add_vif(const string& ifname,
			const string& vifname,
			uint32_t if_index,
			string& error_msg);
    virtual int config_interface(const string& ifname,
				 uint32_t if_index,
				 uint32_t flags,
				 bool is_up,
				 bool is_deleted,
				 string& error_msg);
    virtual int config_vif(const string& ifname,
			   const string& vifname,
			   uint32_t if_index,
			   uint32_t flags,
			   bool is_up,
			   bool is_deleted,
			   bool broadcast,
			   bool loopback,
			   bool point_to_point,
			   bool multicast,
			   string& error_msg);
    virtual int set_interface_mac_address(const string& ifname,
					  uint32_t if_index,
					  const struct ether_addr& ether_addr,
					  string& error_msg);
    virtual int set_interface_mtu(const string& ifname,
				  uint32_t if_index,
				  uint32_t mtu,
				  string& error_msg);
    virtual int add_vif_address(const string& ifname,
				const string& vifname,
				uint32_t if_index,
				bool is_broadcast,
				bool is_p2p,
				const IPvX& addr,
				const IPvX& dst_or_bcast,
				uint32_t prefix_len,
				string& error_msg);
    virtual int delete_vif_address(const string& ifname,
				   const string& vifname,
				   uint32_t if_index,
				   const IPvX& addr,
				   uint32_t prefix_len,
				   string& error_msg);
    virtual int add_vif_address4(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_broadcast,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst_or_bcast,
				 uint32_t prefix_len,
				 string& error_msg);
    virtual int add_vif_address6(const string& ifname,
				 const string& vifname,
				 uint32_t if_index,
				 bool is_p2p,
				 const IPvX& addr,
				 const IPvX& dst,
				 uint32_t prefix_len,
				 string& error_msg);

#ifdef HOST_OS_WINDOWS
private:
    map<pair<uint32_t, IPAddr>, ULONG>	_nte_map;
#endif
};

#endif // __FEA_IFCONFIG_SET_HH__
