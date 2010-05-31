// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/fea/mfea_node_cli.hh,v 1.10 2008/10/02 21:56:49 bms Exp $


#ifndef __FEA_MFEA_NODE_CLI_HH__
#define __FEA_MFEA_NODE_CLI_HH__


//
// MFEA (Multicast Forwarding Engine Abstraction) CLI
//


#include <string>
#include <vector>

#include "libproto/proto_node_cli.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

class MfeaNode;

/**
 * @short The class for @ref MfeaNode CLI access.
 */
class MfeaNodeCli : public ProtoNodeCli {
public:
    /**
     * Constructor for a given MFEA node.
     * 
     * @param mfea_node the @ref MfeaNode this node belongs to.
     */
    MfeaNodeCli(MfeaNode& mfea_node);
    
    /**
     * Destructor
     */
    virtual ~MfeaNodeCli();
    
    /**
     * Start the CLI operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start();

    /**
     * Stop the CLI operation.
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
     * Install all MFEA-related CLI commands to the CLI.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_all_cli_commands();
    
private:
    
    MfeaNode& mfea_node() const { return (_mfea_node); }
    MfeaNode& _mfea_node;
    
    //
    // MFEA CLI commands
    //
    int		cli_show_mfea_dataflow(const vector<string>& argv);
    int		cli_show_mfea_interface(const vector<string>& argv);
    int		cli_show_mfea_interface_address(const vector<string>& argv);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __FEA_MFEA_NODE_CLI_HH__
