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

// $XORP: xorp/mld6igmp/mld6igmp_node_cli.hh,v 1.2 2003/03/10 23:20:43 hodson Exp $


#ifndef __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__
#define __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__


//
// MLD6IGMP protocol CLI
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
    int		cli_show_mld6igmp_group(const vector<string>& argv);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __MLD6IGMP_MLD6IGMP_NODE_CLI_HH__
