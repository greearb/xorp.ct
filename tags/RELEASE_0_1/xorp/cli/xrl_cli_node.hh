// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/cli/xrl_cli_node.hh,v 1.12 2002/12/09 18:28:54 hodson Exp $

#ifndef __CLI_XRL_CLI_NODE_HH__
#define __CLI_XRL_CLI_NODE_HH__

#include <string>

#include "libxorp/xlog.h"
#include "xrl/targets/cli_base.hh"
#include "xrl/interfaces/cli_processor_xif.hh"
#include "cli_node.hh"


//
// TODO: XrlCliProcessorV1p0Client should NOT be a base class. Temp. solution..
//
class XrlCliNode : public XrlCliTargetBase, public XrlCliProcessorV0p1Client {
public:
    XrlCliNode(XrlRouter* r, CliNode& cli_node);
    virtual ~XrlCliNode() {}
    
protected:
    // Methods to be implemented by derived classes supporting this interface.
    virtual XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    virtual XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    virtual XrlCmdError cli_manager_0_1_add_cli_command(
	// Input values, 
	const string&	processor_name, 
	const string&	command_name, 
	const string&	command_help, 
	const bool&	is_command_cd, 
	const string&	command_cd_prompt, 
	const bool&	is_command_processor, 
	// Output values, 
	bool&	fail, 
	string&	reason);

    virtual XrlCmdError cli_manager_0_1_delete_cli_command(
	// Input values, 
	const string&	processor_name, 
	const string&	command_name, 
	// Output values, 
	bool&	fail, 
	string&	reason);
    
    //
    // The CLI client-side (i.e., the CLI sending XRLs)
    //
    void send_process_command(const char *target,
			      const string& processor_name,
			      const string& cli_term_name,
			      uint32_t cli_session_id,
			      const string& command_name,
			      const string& command_args);
    void recv_process_command_output(const XrlError& xrl_error,
				     const string *processor_name,
				     const string *cli_term_name,
				     const uint32_t *cli_session_id,
				     const string *command_output);
private:
    CliNode&	cli_node() const { return (_cli_node); }
    
    CliNode&	_cli_node;
    
};

#endif // __CLI_XRL_CLI_NODE_HH__
