// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_click.cc,v 1.21 2008/07/23 05:10:29 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/run_command.hh"
#include "libxorp/utils.hh"

#include "fea/ifconfig.hh"
#include "fea/nexthop_port_mapper.hh"

#include "ifconfig_set_click.hh"


#ifdef HOST_OS_WINDOWS
#define	unlink(x)	_unlink(x)
#endif


//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is Click:
//   http://www.read.cs.ucla.edu/click/
//

IfConfigSetClick::IfConfigSetClick(FeaDataPlaneManager& fea_data_plane_manager)
    : IfConfigSet(fea_data_plane_manager),
      ClickSocket(fea_data_plane_manager.eventloop()),
      _cs_reader(*(ClickSocket *)this),
      _kernel_click_config_generator(NULL),
      _user_click_config_generator(NULL),
      _has_kernel_click_config(false),
      _has_user_click_config(false)
{
}

IfConfigSetClick::~IfConfigSetClick()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click mechanism to set "
		   "information about network interfaces into the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
IfConfigSetClick::start(string& error_msg)
{
    if (! ClickSocket::is_enabled())
	return (XORP_OK);

    if (_is_running)
	return (XORP_OK);

    if (ClickSocket::start(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: Push the existing merged configuration
    //
    push_config(ifconfig().merged_config());

    return (XORP_OK);
}

int
IfConfigSetClick::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    terminate_click_config_generator();

    ret_value = ClickSocket::stop(error_msg);

    _is_running = false;

    return (ret_value);
}

bool
IfConfigSetClick::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return correct value
}

bool
IfConfigSetClick::is_unreachable_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return correct value
}

int
IfConfigSetClick::config_begin(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetClick::config_end(string& error_msg)
{
    //
    // Trigger the generation of the configuration
    //
    if (execute_click_config_generator(error_msg) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
IfConfigSetClick::config_interface_begin(const IfTreeInterface* pulled_ifp,
					 IfTreeInterface& config_iface,
					 string& error_msg)
{
    IfTreeInterface* ifp;

    UNUSED(pulled_ifp);

    //
    // Find or add the interface
    //
    ifp = _iftree.find_interface(config_iface.ifname());
    if (ifp == NULL) {
	if (_iftree.add_interface(config_iface.ifname()) != XORP_OK) {
	    error_msg = c_format("Cannot add interface '%s'",
				 config_iface.ifname().c_str());
	    return (XORP_ERROR);
	}
	ifp = _iftree.find_interface(config_iface.ifname());
	XLOG_ASSERT(ifp != NULL);
    }

    //
    // Update the interface state
    //
    // Note that we postpone the setting of the "enabled" and interface flags
    // for the end of the vif configuration.
    //
    ifp->set_pif_index(config_iface.pif_index());
    ifp->set_discard(config_iface.discard());
    ifp->set_unreachable(config_iface.unreachable());
    ifp->set_management(config_iface.management());
    ifp->set_mtu(config_iface.mtu());
    ifp->set_mac(config_iface.mac());
    ifp->set_no_carrier(config_iface.no_carrier());
    ifp->set_baudrate(config_iface.baudrate());

    return (XORP_OK);
}

int
IfConfigSetClick::config_interface_end(const IfTreeInterface* pulled_ifp,
				       const IfTreeInterface& config_iface,
				       string& error_msg)
{
    IfTreeInterface* ifp;
    bool is_deleted = false;

    UNUSED(pulled_ifp);

    if (config_iface.is_marked(IfTreeItem::DELETED)) {
	is_deleted = true;
    }

    //
    // Find the interface
    //
    ifp = _iftree.find_interface(config_iface.ifname());
    if (ifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s': "
			     "no such interface in the interface tree",
			     config_iface.ifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the interface if marked for deletion
    //
    if (is_deleted) {
	_iftree.remove_interface(config_iface.ifname());
	return (XORP_OK);
    }

    //
    // Update the remaining interface state
    //
    ifp->set_interface_flags(config_iface.interface_flags());
    ifp->set_enabled(config_iface.enabled());

    return (XORP_OK);
}

int
IfConfigSetClick::config_vif_begin(const IfTreeInterface* pulled_ifp,
				   const IfTreeVif* pulled_vifp,
				   const IfTreeInterface& config_iface,
				   const IfTreeVif& config_vif,
				   string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;

    UNUSED(pulled_ifp);

    //
    // Find the interface
    //
    ifp = _iftree.find_interface(config_iface.ifname());
    if (ifp == NULL) {
	error_msg = c_format("Cannot add interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Find or add the vif
    //
    vifp = ifp->find_vif(config_vif.vifname());
    if (vifp == NULL) {
	if (ifp->add_vif(config_vif.vifname()) != XORP_OK) {
	    error_msg = c_format("Cannot add interface '%s' vif '%s'",
				 config_iface.ifname().c_str(),
				 config_vif.vifname().c_str());
	    return (XORP_ERROR);
	}
	vifp = ifp->find_vif(config_vif.vifname());
	XLOG_ASSERT(vifp != NULL);
    }

    //
    // Update the vif state
    //
    // Note that we postpone the setting of the "enabled" flag for the end
    // of the vif configuration.
    //
    if (pulled_vifp != NULL) {
	vifp->set_pif_index(pulled_vifp->pif_index());
	vifp->set_broadcast(pulled_vifp->broadcast());
	vifp->set_loopback(pulled_vifp->loopback());
	vifp->set_point_to_point(pulled_vifp->point_to_point());
	vifp->set_multicast(pulled_vifp->multicast());
	vifp->set_vlan(pulled_vifp->is_vlan());
	vifp->set_vlan_id(pulled_vifp->vlan_id());
    }

    return (XORP_OK);
}

int
IfConfigSetClick::config_vif_end(const IfTreeInterface* pulled_ifp,
				 const IfTreeVif* pulled_vifp,
				 const IfTreeInterface& config_iface,
				 const IfTreeVif& config_vif,
				 string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;
    bool is_deleted = false;

    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);

    if (config_vif.is_marked(IfTreeItem::DELETED)) {
	is_deleted = true;
    }

    //
    // Find the interface
    //
    ifp = _iftree.find_interface(config_iface.ifname());
    if (ifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Find the vif
    //
    vifp = ifp->find_vif(config_vif.vifname());
    if (vifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the vif if marked for deletion
    //
    if (is_deleted) {
	ifp->remove_vif(config_vif.vifname());
	ifconfig().nexthop_port_mapper().delete_interface(
	    config_iface.ifname(), config_vif.vifname());
	return (XORP_OK);
    }

    //
    // Update the remaining vif state
    //
    vifp->set_vif_flags(config_vif.vif_flags());
    vifp->set_enabled(config_vif.enabled());

    return (XORP_OK);
}

int
IfConfigSetClick::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr4* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr4& config_addr,
				     string& error_msg)
{
    IfTreeVif* vifp;
    IfTreeAddr4* ap;

    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    vifp = _iftree.find_vif(config_iface.ifname(), config_vif.vifname());
    if (vifp == NULL) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Find or add the address
    //
    ap = vifp->find_addr(config_addr.addr());
    if (ap == NULL) {
	if (vifp->add_addr(config_addr.addr()) != XORP_OK) {
	    error_msg = c_format("Cannot add address '%s' "
				 "to interface '%s' vif '%s'",
				 config_addr.addr().str().c_str(),
				 config_iface.ifname().c_str(),
				 config_vif.vifname().c_str());
	    return (XORP_ERROR);
	}
	ap = vifp->find_addr(config_addr.addr());
	XLOG_ASSERT(ap != NULL);
    }

    //
    // Update the address state
    //
    ap->set_broadcast(config_addr.broadcast());
    ap->set_loopback(config_addr.loopback());
    ap->set_point_to_point(config_addr.point_to_point());
    ap->set_multicast(config_addr.multicast());
    if (ap->broadcast())
	ap->set_bcast(config_addr.bcast());
    if (ap->point_to_point())
	ap->set_endpoint(config_addr.endpoint());
    ap->set_prefix_len(config_addr.prefix_len());
    ap->set_enabled(config_addr.enabled());

    return (XORP_OK);
}

int
IfConfigSetClick::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr4* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr4& config_addr,
					string& error_msg)
{
    IfTreeVif* vifp;
    IfTreeAddr4* ap;

    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    vifp = _iftree.find_vif(config_iface.ifname(), config_vif.vifname());
    if (vifp == NULL) {
	error_msg = c_format("Cannot delete address on interface '%s' "
			     "vif '%s': no such vif in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the address
    //
    const IPv4& addr = config_addr.addr();
    ap = vifp->find_addr(addr);
    if (ap == NULL) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': "
			     "no such address",
			     addr.str().c_str(),
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }
    vifp->remove_addr(addr);
    ifconfig().nexthop_port_mapper().delete_ipv4(addr);

    return (XORP_OK);
}

int
IfConfigSetClick::config_add_address(const IfTreeInterface* pulled_ifp,
				     const IfTreeVif* pulled_vifp,
				     const IfTreeAddr6* pulled_addrp,
				     const IfTreeInterface& config_iface,
				     const IfTreeVif& config_vif,
				     const IfTreeAddr6& config_addr,
				     string& error_msg)
{
    IfTreeVif* vifp;
    IfTreeAddr6* ap;

    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

    vifp = _iftree.find_vif(config_iface.ifname(), config_vif.vifname());
    if (vifp == NULL) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Find or add the address
    //
    ap = vifp->find_addr(config_addr.addr());
    if (ap == NULL) {
	if (vifp->add_addr(config_addr.addr()) != XORP_OK) {
	    error_msg = c_format("Cannot add address '%s' "
				 "to interface '%s' vif '%s'",
				 config_addr.addr().str().c_str(),
				 config_iface.ifname().c_str(),
				 config_vif.vifname().c_str());
	    return (XORP_ERROR);
	}
	ap = vifp->find_addr(config_addr.addr());
	XLOG_ASSERT(ap != NULL);
    }

    //
    // Update the address state
    //
    ap->set_loopback(config_addr.loopback());
    ap->set_point_to_point(config_addr.point_to_point());
    ap->set_multicast(config_addr.multicast());
    if (ap->point_to_point())
	ap->set_endpoint(config_addr.endpoint());
    ap->set_prefix_len(config_addr.prefix_len());
    ap->set_enabled(config_addr.enabled());

    return (XORP_OK);
}

int
IfConfigSetClick::config_delete_address(const IfTreeInterface* pulled_ifp,
					const IfTreeVif* pulled_vifp,
					const IfTreeAddr6* pulled_addrp,
					const IfTreeInterface& config_iface,
					const IfTreeVif& config_vif,
					const IfTreeAddr6& config_addr,
					string& error_msg)
{
    IfTreeVif* vifp;
    IfTreeAddr6* ap;
 
    UNUSED(pulled_ifp);
    UNUSED(pulled_vifp);
    UNUSED(pulled_addrp);

     vifp = _iftree.find_vif(config_iface.ifname(), config_vif.vifname());
    if (vifp == NULL) {
	error_msg = c_format("Cannot delete address on interface '%s' "
			     "vif '%s': no such vif in the interface tree",
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the address
    //
    const IPv6& addr = config_addr.addr();
    ap = vifp->find_addr(addr);
    if (ap == NULL) {
	error_msg = c_format("Cannot delete address '%s' "
			     "on interface '%s' vif '%s': "
			     "no such address",
			     addr.str().c_str(),
			     config_iface.ifname().c_str(),
			     config_vif.vifname().c_str());
	return (XORP_ERROR);
    }
    vifp->remove_addr(addr);
    ifconfig().nexthop_port_mapper().delete_ipv6(addr);

    return (XORP_OK);
}

int
IfConfigSetClick::execute_click_config_generator(string& error_msg)
{
    string kernel_generator_file = ClickSocket::kernel_click_config_generator_file();
    string user_generator_file = ClickSocket::user_click_config_generator_file();
    string arguments;

    //
    // Test the Click configuration generator filenames are valid
    //
    if (ClickSocket::is_kernel_click()) {
	if (kernel_generator_file.empty()) {
	    error_msg = c_format(
		"Cannot execute the kernel-level Click configuration "
		"generator: "
		"empty generator file name");
	    return (XORP_ERROR);
	}
    }
    if (ClickSocket::is_user_click()) {
	if (user_generator_file.empty()) {
	    error_msg = c_format(
		"Cannot execute the user-level Click configuration "
		"generator: "
		"empty generator file name");
	    return (XORP_ERROR);
	}
    }

    //
    // Re-generate the XORP fea/click configuration and
    // pass it to the Click config generator.
    //
    // TODO: XXX: the internal re-generation is a temporary solution
    // that should go away after the xorpsh takes over the functionality
    // of calling the Click config generator.
    //
    string xorp_config = regenerate_xorp_iftree_config();
    xorp_config += regenerate_xorp_fea_click_config();

    //
    // XXX: kill any previously running instances
    //
    if (_kernel_click_config_generator != NULL) {
	delete _kernel_click_config_generator;
	_kernel_click_config_generator = NULL;
    }
    if (_user_click_config_generator != NULL) {
	delete _user_click_config_generator;
	_user_click_config_generator = NULL;
    }

    //
    // Cleanup
    //
    _has_kernel_click_config = false;
    _has_user_click_config = false;
    _generated_kernel_click_config.erase();
    _generated_user_click_config.erase();

    //
    // Execute the Click configuration generators
    //
    if (ClickSocket::is_kernel_click()) {
	_kernel_click_config_generator = new ClickConfigGenerator(
	    *this,
	    kernel_generator_file);
	if (_kernel_click_config_generator->execute(xorp_config, error_msg)
	    != XORP_OK) {
	    delete _kernel_click_config_generator;
	    _kernel_click_config_generator = NULL;
	    return (XORP_ERROR);
	}
    }
    if (ClickSocket::is_user_click()) {
	_user_click_config_generator = new ClickConfigGenerator(
	    *this,
	    user_generator_file);
	if (_user_click_config_generator->execute(xorp_config, error_msg)
	    != XORP_OK) {
	    delete _user_click_config_generator;
	    _user_click_config_generator = NULL;
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

void
IfConfigSetClick::terminate_click_config_generator()
{
    if (_kernel_click_config_generator != NULL) {
	delete _kernel_click_config_generator;
	_kernel_click_config_generator = NULL;
    }
    if (_user_click_config_generator != NULL) {
	delete _user_click_config_generator;
	_user_click_config_generator = NULL;
    }
    _has_kernel_click_config = false;
    _has_user_click_config = false;
    _generated_kernel_click_config.erase();
    _generated_user_click_config.erase();
}

void
IfConfigSetClick::click_config_generator_done(
    IfConfigSetClick::ClickConfigGenerator* click_config_generator,
    bool success,
    const string& error_msg)
{
    // Check for errors
    XLOG_ASSERT((click_config_generator == _kernel_click_config_generator)
		|| (click_config_generator == _user_click_config_generator));
    if (! success) {
	XLOG_ERROR("External Click configuration generator (%s) failed: %s",
		   click_config_generator->command_name().c_str(),
		   error_msg.c_str());
    }

    string command_stdout = click_config_generator->command_stdout();

    if (click_config_generator == _kernel_click_config_generator) {
	if (success) {
	    _has_kernel_click_config = true;
	    _generated_kernel_click_config = command_stdout;
	}
	_kernel_click_config_generator = NULL;
    }
    if (click_config_generator == _user_click_config_generator) {
	if (success) {
	    _generated_user_click_config = command_stdout;
	    _has_user_click_config = true;
	}
	_user_click_config_generator = NULL;
    }
    delete click_config_generator;

    if (! success)
	return;

    if ((_kernel_click_config_generator != NULL)
	|| (_user_click_config_generator != NULL)) {
	// XXX: we are still waiting for some output
	return;
    }

    string write_error_msg;
    if (write_generated_config(_has_kernel_click_config,
			       _generated_kernel_click_config,
			       _has_user_click_config,
			       _generated_user_click_config,
			       write_error_msg) != XORP_OK) {
	XLOG_ERROR("Failed to write the Click configuration: %s",
		   write_error_msg.c_str());
    }
}

int
IfConfigSetClick::write_generated_config(bool has_kernel_config,
					 const string& kernel_config,
					 bool has_user_config,
					 const string& user_config,
					 string& error_msg)
{
    string element = "";
    string handler = "hotconfig";

    if (ClickSocket::write_config(element, handler,
				  has_kernel_config, kernel_config,
				  has_user_config, user_config,
				  error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    //
    // Generate the new port mapping
    //
    generate_nexthop_to_port_mapping();

    //
    // Notify the NextHopPortMapper observers about any port mapping changes
    //
    ifconfig().nexthop_port_mapper().notify_observers();

    return (XORP_OK);
}

string
IfConfigSetClick::regenerate_xorp_iftree_config() const
{
    string config, preamble;
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::IPv4Map::const_iterator ai4;
    IfTreeVif::IPv6Map::const_iterator ai6;

    //
    // Configuration section: "interfaces{}"
    //
    preamble = "";
    config += preamble + c_format("interfaces {\n");
    for (ii = _iftree.interfaces().begin(); ii != _iftree.interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	preamble = "    ";
	config += preamble + c_format("interface %s {\n",
				      fi.ifname().c_str());
	preamble = "\t";
	config += preamble + c_format("disable: %s\n",
				      bool_c_str(! fi.enabled()));
	config += preamble + c_format("discard: %s\n",
				      bool_c_str(fi.discard()));
	config += preamble + c_format("unreachable: %s\n",
				      bool_c_str(fi.unreachable()));
	config += preamble + c_format("management: %s\n",
				      bool_c_str(fi.management()));
	config += preamble + c_format("mac: %s\n", fi.mac().str().c_str());
	config += preamble + c_format("mtu: %u\n", XORP_UINT_CAST(fi.mtu()));
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = *(vi->second);
	    preamble = "\t";
	    config += preamble + c_format("vif %s {\n", fv.vifname().c_str());
	    preamble = "\t    ";
	    config += preamble + c_format("disable: %s\n",
					  bool_c_str(! fv.enabled()));
	    for (ai4 = fv.ipv4addrs().begin(); ai4 != fv.ipv4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = *(ai4->second);
		preamble = "\t    ";
		config += preamble + c_format("address %s {\n",
					      fa4.addr().str().c_str());
		preamble = "\t\t";
		config += preamble + c_format("prefix-length: %u\n",
					      XORP_UINT_CAST(fa4.prefix_len()));
		if (fa4.broadcast()) {
		    config += preamble + c_format("broadcast: %s\n",
						  fa4.bcast().str().c_str());
		}
		if (fa4.point_to_point()) {
		    config += preamble + c_format("destination: %s\n",
						  fa4.endpoint().str().c_str());
		}
		config += preamble + c_format("multicast-capable: %s\n",
					      bool_c_str(fa4.multicast()));
		config += preamble + c_format("point-to-point: %s\n",
					      bool_c_str(fa4.point_to_point()));
		config += preamble + c_format("loopback: %s\n",
					      bool_c_str(fa4.loopback()));
		config += preamble + c_format("disable: %s\n",
					      bool_c_str(! fa4.enabled()));
		preamble = "\t    ";
		config += preamble + c_format("}\n");
	    }
	    for (ai6 = fv.ipv6addrs().begin(); ai6 != fv.ipv6addrs().end(); ++ai6) {
		const IfTreeAddr6& fa6 = *(ai6->second);
		preamble = "\t    ";
		config += preamble + c_format("address %s {\n",
					      fa6.addr().str().c_str());
		preamble = "\t\t";
		config += preamble + c_format("prefix-length: %u\n",
					      XORP_UINT_CAST(fa6.prefix_len()));
		if (fa6.point_to_point()) {
		    config += preamble + c_format("destination: %s\n",
						  fa6.endpoint().str().c_str());
		}
		config += preamble + c_format("multicast-capable: %s\n",
					      bool_c_str(fa6.multicast()));
		config += preamble + c_format("point-to-point: %s\n",
					      bool_c_str(fa6.point_to_point()));
		config += preamble + c_format("loopback: %s\n",
					      bool_c_str(fa6.loopback()));
		config += preamble + c_format("disable: %s\n",
					      bool_c_str(! fa6.enabled()));
		preamble = "\t    ";
		config += preamble + c_format("}\n");
	    }
	    preamble = "\t";
	    config += preamble + c_format("}\n");
	}
	preamble = "    ";
	config += preamble + c_format("}\n");
    }
    preamble = "";
    config += preamble + c_format("}\n");

    return (config);
}

string
IfConfigSetClick::regenerate_xorp_fea_click_config() const
{
    string config, preamble;

    //
    // Configuration section: "fea{}"
    //
    // XXX: note that we write only a partial configuration: those
    // that is related to enabling/disabling kernel or user-level Click.
    //
    preamble = "";
    config += preamble + c_format("fea {\n");
    do {
	preamble = "    ";
	config += preamble + c_format("click {\n");
	do {
	    preamble = "\t";
	    config += preamble + c_format("disable: %s\n",
					  bool_c_str(! ClickSocket::is_enabled()));
	    do {
		config += preamble + c_format("kernel-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("disable: %s\n",
					      bool_c_str(! ClickSocket::is_kernel_click()));
		preamble = "\t";
		config += preamble + c_format("}\n");
	    } while (false);
	    do {
		config += preamble + c_format("user-click {\n");
		preamble = "\t    ";
		config += preamble + c_format("disable: %s\n",
					      bool_c_str(! ClickSocket::is_user_click()));
		preamble = "\t";
		config += preamble + c_format("}\n");
	    } while (false);
	} while (false);
	preamble = "    ";
	config += preamble + c_format("}\n");
    } while (false);
    preamble = "";
    config += preamble + c_format("}\n");

    return (config);
}

/**
 * Generate the next-hop to port mapping.
 *
 * @return the number of generated ports.
 */
int
IfConfigSetClick::generate_nexthop_to_port_mapping()
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::IPv4Map::const_iterator ai4;
    IfTreeVif::IPv6Map::const_iterator ai6;
    int xorp_rt_port, local_xorp_rt_port;

    //
    // Calculate the port for local delivery.
    // XXX: The last port in xorp_rt is reserved for local delivery
    //
    local_xorp_rt_port = 0;
    for (ii = _iftree.interfaces().begin(); ii != _iftree.interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    local_xorp_rt_port++;
	}
    }

    //
    // Generate the next-hop to port mapping.
    // Note that all local IP addresses are mapped to the port
    // designated for local delivery.
    //
    ifconfig().nexthop_port_mapper().clear();
    xorp_rt_port = 0;
    for (ii = _iftree.interfaces().begin(); ii != _iftree.interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = *(vi->second);
	    ifconfig().nexthop_port_mapper().add_interface(fi.ifname(),
							   fv.vifname(),
							   xorp_rt_port);
	    for (ai4 = fv.ipv4addrs().begin(); ai4 != fv.ipv4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = *(ai4->second);
		ifconfig().nexthop_port_mapper().add_ipv4(fa4.addr(),
							  local_xorp_rt_port);
		IPv4Net ipv4net(fa4.addr(), fa4.prefix_len());
		ifconfig().nexthop_port_mapper().add_ipv4net(ipv4net,
							     xorp_rt_port);
		if (fa4.point_to_point())
		    ifconfig().nexthop_port_mapper().add_ipv4(fa4.endpoint(),
							      xorp_rt_port);
	    }
	    for (ai6 = fv.ipv6addrs().begin(); ai6 != fv.ipv6addrs().end(); ++ai6) {
		const IfTreeAddr6& fa6 = *(ai6->second);
		ifconfig().nexthop_port_mapper().add_ipv6(fa6.addr(),
							  local_xorp_rt_port);
		IPv6Net ipv6net(fa6.addr(), fa6.prefix_len());
		ifconfig().nexthop_port_mapper().add_ipv6net(ipv6net,
							     xorp_rt_port);
		if (fa6.point_to_point())
		    ifconfig().nexthop_port_mapper().add_ipv6(fa6.endpoint(),
							      xorp_rt_port);
	    }
	    xorp_rt_port++;
	}
    }

    return (xorp_rt_port);
}

IfConfigSetClick::ClickConfigGenerator::ClickConfigGenerator(
    IfConfigSetClick& ifconfig_set_click,
    const string& command_name)
    : _ifconfig_set_click(ifconfig_set_click),
      _eventloop(ifconfig_set_click.ifconfig().eventloop()),
      _command_name(command_name),
      _run_command(NULL)
{
}

IfConfigSetClick::ClickConfigGenerator::~ClickConfigGenerator()
{
#ifndef HOST_OS_WINDOWS
    if (_run_command != NULL)
	delete _run_command;
    if (! _tmp_filename.empty())
	unlink(_tmp_filename.c_str());
#endif
}

int
IfConfigSetClick::ClickConfigGenerator::execute(const string& xorp_config,
						string& error_msg)
{
    XLOG_ASSERT(_tmp_filename.empty());

    //
    // Create a temporary file
    //
    FILE* fp = xorp_make_temporary_file("", "xorp_fea_click",
					_tmp_filename, error_msg);
    if (fp == NULL) {
	error_msg = c_format("Cannot create a temporary file: %s",
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    if (fwrite(xorp_config.c_str(), sizeof(char), xorp_config.size(), fp) !=
	static_cast<size_t>(xorp_config.size())) {
	error_msg = c_format("Error writing to temporary file: %s",
			     strerror(errno));
	fclose(fp);
	return (XORP_ERROR);
    }
    fclose(fp);

    // XXX: the filename is the argument
    _command_argument_list.clear();
    _command_argument_list.push_back(_tmp_filename);

    _run_command = new RunCommand(
	_eventloop,
	_command_name,
	_command_argument_list,
	callback(this, &IfConfigSetClick::ClickConfigGenerator::stdout_cb),
	callback(this, &IfConfigSetClick::ClickConfigGenerator::stderr_cb),
	callback(this, &IfConfigSetClick::ClickConfigGenerator::done_cb),
	false /* redirect_stderr_to_stdout */);
    if (_run_command->execute() != XORP_OK) {
	delete _run_command;
	_run_command = NULL;
	unlink(_tmp_filename.c_str());
	error_msg = c_format("Could not execute the Click "
			     "configuration generator");
	return (XORP_ERROR);
    }
    return (XORP_OK);
}

void
IfConfigSetClick::ClickConfigGenerator::stdout_cb(RunCommand* run_command,
						  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
IfConfigSetClick::ClickConfigGenerator::stderr_cb(RunCommand* run_command,
						  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    XLOG_ERROR("External Click configuration generator (%s) stderr output: %s",
	       run_command->command().c_str(),
	       output.c_str());
}

void
IfConfigSetClick::ClickConfigGenerator::done_cb(RunCommand* run_command,
						bool success,
						const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);
    _ifconfig_set_click.click_config_generator_done(this, success, error_msg);
}
