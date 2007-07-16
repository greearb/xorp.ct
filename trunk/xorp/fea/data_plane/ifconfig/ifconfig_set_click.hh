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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_click.hh,v 1.4 2007/07/11 22:18:15 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__

#include "fea/ifconfig_set.hh"
#include "fea/data_plane/control_socket/click_socket.hh"

class RunCommand;


class IfConfigSetClick : public IfConfigSet,
			 public ClickSocket {
private:
    class ClickConfigGenerator;

public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     */
    IfConfigSetClick(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
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

    /**
     * Test whether the provider mirrors the pulled interface configuration
     * from the system.
     *
     * Note that this method is a short-term solution and might disappear
     * in the future.
     *
     * @return true if the provider mirrors the pulled interface configuration
     * from the system.
     */
    virtual bool is_pulled_config_mirror() const { return (true); }

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

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_SET_CLICK_HH__
