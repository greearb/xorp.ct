// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/cli/cli_node.hh,v 1.33 2008/06/19 09:56:02 bms Exp $


#ifndef __CLI_CLI_NODE_HH__
#define __CLI_CLI_NODE_HH__


//
// CLI node definition.
//


#include <list>

#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libproto/proto_node.hh"
#include "cli_command.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class EventLoop;
class CliClient;
class CliPipe;
class IPvXNet;

/**
 * @short The class for the CLI node.
 * 
 * There should one node per CLI instance. There should be
 * one CLI instance per router.
 */
class CliNode : public ProtoNode<Vif> {
public:
    /**
     * Constructor for a given address family, module ID, and event loop.
     * 
     * @param init_family the address family (AF_INET or AF_INET6 for
     * IPv4 and IPv6 respectively). Note that this argument may disappear
     * in the future, and a single Cli node would provide access for
     * both IPv4 and IPv6.
     * @param init_module_id the module ID (@ref xorp_module_id). Should be
     * equal to XORP_MODULE_CLI.
     * @param init_eventloop the event loop to use.
     */
    CliNode(int init_family, xorp_module_id init_module_id,
	    EventLoop& init_eventloop);

    /**
     * Destructor
     */
    virtual ~CliNode();
    
    /**
     * Start the node operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();
    
    /**
     * Stop the node operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop();

    /**
     * Enable node operation.
     * 
     * If an unit is not enabled, it cannot be start, or pending-start.
     */
    void	enable();
    
    /**
     * Disable node operation.
     * 
     * If an unit is disabled, it cannot be start or pending-start.
     * If the unit was runnning, it will be stop first.
     */
    void	disable();

    /**
     * Set the CLI access port.
     * 
     * The access port is the TCP port the CLI node listens to for
     * network access (e.g., telnet xorp_host <port_number>).
     * 
     * @param v the access port number (in host order).
     */
    void	set_cli_port(unsigned short v) { _cli_port = htons(v); }
    
    /**
     * Add a subnet address to the list of subnet addresses enabled
     * for CLI access.
     * 
     * This method can be called more than once to add a number of
     * subnet addresses.
     * 
     * @param subnet_addr the subnet address to add.
     */
    void	add_enable_cli_access_from_subnet(const IPvXNet& subnet_addr);
    
    /**
     * Delete a subnet address from the list of subnet addresses enabled
     * for CLI access.
     * 
     * @param subnet_addr the subnet address to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR (e.g., if the subnet
     * address was not added before).
     */
    int		delete_enable_cli_access_from_subnet(const IPvXNet& subnet_addr);
    
    /**
     * Add a subnet address to the list of subnet addresses disabled
     * for CLI access.
     * 
     * This method can be called more than once to add a number of
     * subnet addresses.
     * 
     * @param subnet_addr the subnet address to add.
     */
    void	add_disable_cli_access_from_subnet(const IPvXNet& subnet_addr);
    
    /**
     * Delete a subnet address from the list of subnet addresses disabled
     * for CLI access.
     * 
     * @param subnet_addr the subnet address to delete.
     * @return XORP_OK on success, otherwise XORP_ERROR (e.g., if the subnet
     * address was not added before).
     */
    int		delete_disable_cli_access_from_subnet(const IPvXNet& subnet_addr);
    
    /**
     * Get the @ref CliCommand entry for the CLI root command.
     * 
     * @return a pointer to the @ref CliCommand entry for the CLI root command.
     */
    CliCommand	*cli_command_root() { return (&_cli_command_root); }
    
    /**
     * Output a log message to a @ref CliClient object.
     * 
     * @param obj the @ref CliClient object to apply this method to.
     * @param level the XLOG level.
     * @param msg a C-style string with the message to output.
     * @return on success, the number of characters printed,
     * otherwise %XORP_ERROR.
     */
    static int	xlog_output(void *obj, xlog_level_t level, const char *msg);
    
    /**
     * Find a CLI client @ref CliClient for a given terminal name.
     * 
     * @param term_name the CLI terminal name to search for.
     * @return the CLI client @ref CliClient with name of @ref term_name
     * on success, otherwise NULL.
     */
    CliClient	*find_cli_by_term_name(const string& term_name) const;
    
    /**
     * Find a CLI client @ref CliClient for a given session ID.
     * 
     * @param session_id the CLI session ID to search for.
     * @return the CLI client @ref CliClient with session ID of @ref session_id
     * on success, otherwise NULL.
     */
    CliClient	*find_cli_by_session_id(uint32_t session_id) const;
    
    /**
     * Get the list of CLI clients (see @ref CliClient).
     * 
     * @return a reference to the list of pointers to CLI clients
     * (see @ref CliClient).
     */
    list<CliClient *>& client_list() { return (_client_list); }
    
    /**
     * Add a CLI command to the CLI manager.
     * 
     * @param processor_name the name of the module that will process
     * that command.
     * @param command_name the name of the command to add.
     * @param command_help the help for the command to add.
     * @param is_command_cd if true, this is a command that allows
     * "change directory" inside the CLI command-tree.
     * @param command_cd_prompt if @ref is_command_cd is true,
     * the string that will replace the CLI prompt after we
     * "cd" to that level of the CLI command-tree.
     * @param is_command_processor if true, this is a processing command
     * that would be performed by @processor_name.
     * @param error_msg the error message (if error).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_cli_command(
	// Input values,
	const string&	processor_name,
	const string&	command_name,
	const string&	command_help,
	const bool&	is_command_cd,
	const string&	command_cd_prompt,
	const bool&	is_command_processor,
	// Output values,
	string&		error_msg);

    /**
     * Delete a CLI command from the CLI manager.
     * 
     * @param processor_name the name of the module that is processing
     * that command.
     * @param command_name the name of the command to delete.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_cli_command(
	// Input values,
	const string&	processor_name,
	const string&	command_name,
	// Output values,
	string&		error_msg);

    /**
     * Process the response of a command processed by a remote node.
     * 
     * @param processor_name the name of the module that has processed
     * that command.
     * @param cli_term_name the terminal name the command was entered from.
     * @param cli_session_id the CLI session ID the command was entered from.
     * @param command_output the command output to process.
     */
    void recv_process_command_output(const string *processor_name,
				     const string *cli_term_name,
				     const uint32_t *cli_session_id,
				     const string *command_output);
    
    //
    // Protocol message and kernel signal send/recv: not used by the CLI.
    //
    /**
     * UNUSED
     */
    int proto_recv(const string&	, // if_name,
		   const string&	, // vif_name,
		   const IPvX&		, // src_address,
		   const IPvX&		, // dst_address,
		   uint8_t		, // ip_protocol,
		   int32_t		, // ip_ttl,
		   int32_t		, // ip_tos,
		   bool			, // ip_router_alert,
		   bool			, // ip_internet_control,
		   const vector<uint8_t>& , // payload,
		   string&		  // error_msg
	) { assert (false); return (XORP_ERROR); }

    /**
     * UNUSED
     */
    int	proto_send(const string&	, // if_name,
		   const string&	, // vif_name,
		   const IPvX&		, // src_address,
		   const IPvX&		, // dst_address,
		   uint8_t		, // ip_protocol,
		   int32_t		, // ip_ttl,
		   int32_t		, // ip_tos,
		   bool			, // ip_router_alert,
		   bool			, // ip_internet_control,
		   const uint8_t*	, // sndbuf,
		   size_t		, // sndlen,
		   string&		  // error_msg
	) { assert (false); return (XORP_ERROR); }

    /**
     * UNUSED
     */
    int	signal_message_recv(const string&	, // src_module_instance_name,
			    int			, // message_type,
			    uint32_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // rcvbuf,
			    size_t		  // rcvlen
	) { assert (false); return (XORP_ERROR); }
    /**
     * UNUSED
     */
    int	signal_message_send(const string&	, // dst_module_instance_name,
			    int			, // message_type,
			    uint32_t		, // vif_index,
			    const IPvX&		, // src,
			    const IPvX&		, // dst,
			    const uint8_t *	, // sndbuf,
			    size_t		  // sndlen
	) { assert (false); return (XORP_ERROR); }
    
    typedef XorpCallback6<void,
	const string&,		// target
	const string&,		// server_name
	const string&,		// cli_term_name
	uint32_t,		// cli_session_id
	const vector<string>&,	// command_global_name
	const vector<string>&	// argv
    >::RefPtr SenderProcessCallback;
    
    /**
     * Set a callback to send a CLI command to a processing module.
     * 
     * @param v the @ref SenderProcessCallback callback to set.
     */
    void set_send_process_command_callback(const SenderProcessCallback& v) {
	_send_process_command_callback = v;
    }
    
    /**
     * Add a CLI client (@ref CliClient) to the CLI with enabled access
     * from a file descriptor.
     * 
     * @param input_fd the file descriptor for the CLI client to read
     * data from.
     * @param output_fd the file descriptor for the CLI client to write
     * data to.
     * @param is_network if true, this client is associated with a
     * network connection.
     * @param startup_cli_prompt the startup CLI prompt.
     * @param error_msg the error message (if error).
     * @return a pointer to the CLI client (@ref CliClient) with enabled
     * CLI access on success, otherwise NULL.
     */
    CliClient *add_client(XorpFd input_fd, XorpFd output_fd, bool is_network,
			  const string& startup_cli_prompt, string& error_msg);

    /**
     * Remove a CLI client (@ref CliClient) from the CLI.
     * 
     * Note that the CLI client object itself is not deleted.
     * 
     * @param cli_client the CLI client (@ref CliClient) to remove.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_client(CliClient *cli_client, string& error_msg);

    typedef XorpCallback1<void,
	CliClient*		// CLI client to delete
    >::RefPtr CliClientDeleteCallback;
    
    /**
     * Set the callback method that is invoked whenever a CliClient is deleted
     * 
     * @param v the @ref CliClientDeleteCallback callback to set.
     */
    void set_cli_client_delete_callback(const CliClientDeleteCallback& v) {
	_cli_client_delete_callback = v;
    }

    //
    // Debug-related methods
    //
    
    /**
     * Test if trace log is enabled.
     * 
     * This method is used to test whether to output trace log debug messges.
     * 
     * @return true if trace log is enabled, otherwise false.
     */
    bool	is_log_trace() const { return (_is_log_trace); }
    
    /**
     * Enable/disable trace log.
     * 
     * This method is used to enable/disable trace log debug messages output.
     * 
     * @param is_enabled if true, trace log is enabled, otherwise is disabled.
     */
    void	set_log_trace(bool is_enabled) { _is_log_trace = is_enabled; }
    
private:
    friend class CliClient;
    
    //
    // Internal CLI commands
    //
    int		add_internal_cli_commands(string& error_msg);
    int		cli_show_log(const string& server_name,
			     const string& cli_term_name,
			     uint32_t cli_session_id,
			     const vector<string>& command_global_name,
			     const vector<string>& argv);
    int		cli_show_log_user(const string& server_name,
				  const string& cli_term_name,
				  uint32_t cli_session_id,
				  const vector<string>& command_global_name,
				  const vector<string>& argv);
    int		cli_set_log_output_cli(const string& server_name,
				       const string& cli_term_name,
				       uint32_t cli_session_id,
				       const vector<string>& command_global_name,
				       const vector<string>& argv);
    int		cli_set_log_output_file(const string& server_name,
					const string& cli_term_name,
					uint32_t cli_session_id,
					const vector<string>& command_global_name,
					const vector<string>& argv);
    int		cli_set_log_output_remove_cli(const string& server_name,
					      const string& cli_term_name,
					      uint32_t cli_session_id,
					      const vector<string>& command_global_name,
					      const vector<string>& argv);
    int		cli_set_log_output_remove_file(const string& server_name,
					       const string& cli_term_name,
					       uint32_t cli_session_id,
					       const vector<string>& command_global_name,
					       const vector<string>& argv);
    
    int send_process_command(const string& server_name,
			     const string& cli_term_name,
			     const uint32_t cli_session_id,
			     const vector<string>& command_global_name,
			     const vector<string>& argv);
    
    XorpFd	sock_serv_open();
    int		sock_serv_close();
    CliClient	*add_connection(XorpFd input_fd, XorpFd output_fd,
				bool is_network,
				const string& startup_cli_prompt,
				string& error_msg);
    int		delete_connection(CliClient *cli_client, string& error_msg);
    void	accept_connection(XorpFd fd, IoEventType type);
    
    bool	is_allow_cli_access(const IPvX& ipvx) const;
    
    XorpFd		_cli_socket;	// The CLI listening socket
    unsigned short	_cli_port;	// The CLI port (network-order) to
					//   listen on for new connections.
    list<CliClient *>	_client_list;	// The list with the CLI clients
    uint32_t		_next_session_id; // Used to assign unique session IDs.
    string		_startup_cli_prompt; // The CLI prompt on startup
    
    CliCommand		_cli_command_root; // The root of the CLI commands
    
    SenderProcessCallback _send_process_command_callback;
    
    // The callback pethod that is invoked whenever a CliClient is deleted
    CliClientDeleteCallback _cli_client_delete_callback;
    
    list<IPvXNet>	_enable_cli_access_subnet_list;
    list<IPvXNet>	_disable_cli_access_subnet_list;

    //
    // Debug and test-related state
    //
    bool	_is_log_trace;		// If true, enable XLOG_TRACE()
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_NODE_HH__
