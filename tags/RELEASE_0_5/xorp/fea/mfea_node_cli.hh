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

// $XORP: xorp/mfea/mfea_node_cli.hh,v 1.3 2003/03/10 23:20:39 hodson Exp $


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
    int		cli_show_mfea_mrib(const vector<string>& argv);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __FEA_MFEA_NODE_CLI_HH__
