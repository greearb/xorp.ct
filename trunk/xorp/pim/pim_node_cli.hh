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

// $XORP: xorp/pim/pim_node_cli.hh,v 1.5 2003/08/07 01:09:10 pavlin Exp $


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
    
    int		start();
    int		stop();

    int		add_all_cli_commands();
    
private:
    
    PimNode& pim_node() const { return (_pim_node); }
    PimNode& _pim_node;

    //
    // Misc. other methods
    //
    string	mifset_str(const Mifset& mifset) const {
	string res;
	for (uint16_t i = 0; i < _pim_node.maxvifs(); i++) {
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
