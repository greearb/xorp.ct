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

// $XORP: xorp/pim/pim_node_cli.hh,v 1.15 2008/10/02 21:57:54 bms Exp $


#ifndef __PIM_PIM_NODE_CLI_HH__
#define __PIM_PIM_NODE_CLI_HH__


//
// PIM protocol CLI
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

// PIM protocol CLI class

class PimNode;

class PimNodeCli : public ProtoNodeCli {
public:
    PimNodeCli(PimNode& pim_node);
    virtual ~PimNodeCli();

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

    int		add_all_cli_commands();
    
private:
    
    PimNode& pim_node() const { return (_pim_node); }
    PimNode& _pim_node;

    //
    // Misc. other methods
    //
    string	mifset_str(const Mifset& mifset) const {
	string res;
	for (uint32_t i = 0; i < _pim_node.maxvifs(); i++) {
	    if (mifset.test(i))
		res += "O";
	    else
		res += ".";
	}
	return res;
    }
    
    //
    // PIM CLI commands
    //
    int		cli_show_pim_bootstrap(const vector<string>& argv);
    int		cli_show_pim_bootstrap_rps(const vector<string>& argv);
    int		cli_show_pim_interface(const vector<string>& argv);
    int		cli_show_pim_interface_address(const vector<string>& argv);
    int		cli_show_pim_join(const vector<string>& argv);
    int		cli_show_pim_join_all(const vector<string>& argv);
    int		cli_show_pim_mfc(const vector<string>& argv);
    int		cli_show_pim_neighbors(const vector<string>& argv);
    int		cli_show_pim_mrib(const vector<string>& argv);
    int		cli_show_pim_rps(const vector<string>& argv);
    int		cli_show_pim_scope(const vector<string>& argv);
    
    //
    // Methods used by the PIM CLI commands
    //
    void	cli_print_pim_mre_entries(const IPvXNet& group_range,
					  bool is_print_all);
    void	cli_print_pim_mfc_entries(const IPvXNet& group_range);
    void	cli_print_pim_mre(const PimMre *pim_mre);
    void	cli_print_pim_mfc(const PimMfc *pim_mfc);
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_NODE_CLI_HH__
