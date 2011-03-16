// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/contrib/mld6igmp_lite/mld6igmp_node_cli.hh,v 1.3 2008/10/02 21:56:32 bms Exp $


#ifndef __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__
#define __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__


//
// MLD6IGMP protocol CLI
//





#include "libproto/proto_node_cli.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

// MLD6IGMP protocol CLI class

class Mld6igmpNode;

/**
 * @short The class for @ref Mld6igmpNode CLI access.
 */
class Mld6igmpNodeCli : public ProtoNodeCli {
public:

    /**
     * Constructor for a given MLD6IGMP node.
     * 
     * @param mld6igmp_node the @ref Mld6igmpNode this node belongs to.
     */
    Mld6igmpNodeCli(Mld6igmpNode& mld6igmp_node);
    
    /**
     * Destructor
     */
    virtual ~Mld6igmpNodeCli();
    
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
     * Install all MLD6IGMP-related CLI commands to the CLI.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		add_all_cli_commands();
    
private:
    
    Mld6igmpNode& mld6igmp_node() const { return (_mld6igmp_node); }
    Mld6igmpNode& _mld6igmp_node;
    
    //
    // MLD6IGMP CLI commands
    //
    int		cli_show_mld6igmp_interface(const vector<string>& argv);
    int		cli_show_mld6igmp_interface_address(const vector<string>& argv);
    int		cli_show_mld6igmp_group(const vector<string>& argv);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__
