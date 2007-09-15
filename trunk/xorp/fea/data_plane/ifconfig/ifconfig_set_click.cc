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

#ident "$XORP: xorp/fea/data_plane/ifconfig/ifconfig_set_click.cc,v 1.9 2007/07/11 22:18:15 pavlin Exp $"

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

    if (ClickSocket::start(error_msg) < 0)
	return (XORP_ERROR);

    _is_running = true;

    //
    // XXX: Push the existing configuration
    //
    push_config(ifconfig().pushed_config());

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

int
IfConfigSetClick::config_begin(string& error_msg)
{
    debug_msg("config_begin\n");

    UNUSED(error_msg);

    return (XORP_OK);
}

int
IfConfigSetClick::config_end(string& error_msg)
{
    debug_msg("config_end\n");

    //
    // Trigger the generation of the configuration
    //
    if (execute_click_config_generator(error_msg) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

bool
IfConfigSetClick::is_discard_emulated(const IfTreeInterface& i) const
{
    UNUSED(i);

    return (false);	// TODO: return correct value
}

int
IfConfigSetClick::add_interface(const string& ifname,
				uint32_t if_index,
				string& error_msg)
{
    IfTreeInterface* ifp;

    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	//
	// Add the new interface
	//
	if (_iftree.add_interface(ifname) != XORP_OK) {
	    error_msg = c_format("Cannot add interface '%s'", ifname.c_str());
	    return (XORP_ERROR);
	}
	ifp = _iftree.find_interface(ifname);
	XLOG_ASSERT(ifp != NULL);
    }

    // Update the interface
    if (ifp->pif_index() != if_index)
	ifp->set_pif_index(if_index);

    return (XORP_OK);
}

int
IfConfigSetClick::add_vif(const string& ifname,
			  const string& vifname,
			  uint32_t if_index,
			  string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;

    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    UNUSED(if_index);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot add interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	//
	// Add the new vif
	//
	if (ifp->add_vif(vifname) != XORP_OK) {
	    error_msg = c_format("Cannot add interface '%s' vif '%s'",
				 ifname.c_str(), vifname.c_str());
	    return (XORP_ERROR);
	}
	vifp = ifp->find_vif(vifname);
	XLOG_ASSERT(vifp != NULL);
    }

    return (XORP_OK);
}

int
IfConfigSetClick::config_interface(const string& ifname,
				   uint32_t if_index,
				   uint32_t flags,
				   bool is_up,
				   bool is_deleted,
				   string& error_msg)
{
    IfTreeInterface* ifp;

    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), XORP_UINT_CAST(if_index),
	      XORP_UINT_CAST(flags), bool_c_str(is_up),
	      bool_c_str(is_deleted));

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Update the interface
    //
    if (ifp->pif_index() != if_index)
	ifp->set_pif_index(if_index);

    if (ifp->interface_flags() != flags)
	ifp->set_interface_flags(flags);

    if (ifp->enabled() != is_up)
	ifp->set_enabled(is_up);

    if (is_deleted)
	_iftree.remove_interface(ifname);

    return (XORP_OK);
}

int
IfConfigSetClick::config_vif(const string& ifname,
			     const string& vifname,
			     uint32_t if_index,
			     uint32_t flags,
			     bool is_up,
			     bool is_deleted,
			     bool broadcast,
			     bool loopback,
			     bool point_to_point,
			     bool multicast,
			     string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;

    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(),
	      if_index, XORP_UINT_CAST(flags),
	      bool_c_str(is_up),
	      bool_c_str(is_deleted),
	      bool_c_str(broadcast),
	      bool_c_str(loopback),
	      bool_c_str(point_to_point),
	      bool_c_str(multicast));

    UNUSED(if_index);
    UNUSED(flags);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	error_msg = c_format("Cannot configure interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Update the vif
    //
    if (vifp->enabled() != is_up)
	vifp->set_enabled(is_up);

    if (vifp->broadcast() != broadcast)
	vifp->set_broadcast(broadcast);

    if (vifp->loopback() != loopback)
	vifp->set_loopback(loopback);

    if (vifp->point_to_point() != point_to_point)
	vifp->set_point_to_point(point_to_point);

    if (vifp->multicast() != multicast)
	vifp->set_multicast(multicast);

    if (is_deleted) {
	ifp->remove_vif(vifname);
	ifconfig().nexthop_port_mapper().delete_interface(ifname, vifname);
    }

    return (XORP_OK);
}

int
IfConfigSetClick::set_interface_mac_address(const string& ifname,
					    uint32_t if_index,
					    const struct ether_addr& ether_addr,
					    string& error_msg)
{
    IfTreeInterface* ifp;

    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    UNUSED(if_index);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot set MAC address on interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Set the MAC address
    //
    try {
	EtherMac new_ether_mac(ether_addr);
	Mac new_mac = new_ether_mac;

	if (ifp->mac() != new_mac)
	    ifp->set_mac(new_mac);
    } catch (BadMac) {
	error_msg = c_format("Cannot set MAC address on interface '%s' "
			     "to '%s': "
			     "invalid MAC address",
			     ifname.c_str(),
			     ether_ntoa(const_cast<struct ether_addr *>(&ether_addr)));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetClick::set_interface_mtu(const string& ifname,
				    uint32_t if_index,
				    uint32_t mtu,
				    string& error_msg)
{
    IfTreeInterface* ifp;

    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, XORP_UINT_CAST(mtu));

    UNUSED(if_index);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot set MTU on interface '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Set the MTU
    //
    if (ifp->mtu() != mtu)
	ifp->set_mtu(mtu);

    return (XORP_OK);
}

int
IfConfigSetClick::add_vif_address(const string& ifname,
				  const string& vifname,
				  uint32_t if_index,
				  bool is_broadcast,
				  bool is_p2p,
				  const IPvX& addr,
				  const IPvX& dst_or_bcast,
				  uint32_t prefix_len,
				  string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;

    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      bool_c_str(is_broadcast), bool_c_str(is_p2p),
	      addr.str().c_str(), dst_or_bcast.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(if_index);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	error_msg = c_format("Cannot add address to interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Add the address
    //
    if (addr.is_ipv4()) {
	IPv4 addr4 = addr.get_ipv4();
	IPv4 dst_or_bcast4 = dst_or_bcast.get_ipv4();
	IfTreeAddr4* ap = vifp->find_addr(addr4);

	if (ap == NULL) {
	    if (vifp->add_addr(addr4) != XORP_OK) {
		error_msg = c_format("Cannot add address '%s' "
				     "to interface '%s' vif '%s'",
				     addr4.str().c_str(),
				     ifname.c_str(),
				     vifname.c_str());
		return (XORP_ERROR);
	    }
	    ap = vifp->find_addr(addr4);
	    XLOG_ASSERT(ap != NULL);
	}

	//
	// Update the address
	//
	if (ap->broadcast() != is_broadcast)
	    ap->set_broadcast(is_broadcast);

	if (ap->point_to_point() != is_p2p)
	    ap->set_point_to_point(is_p2p);

	if (ap->broadcast()) {
	    if (ap->bcast() != dst_or_bcast4)
		ap->set_bcast(dst_or_bcast4);
	}

	if (ap->point_to_point()) {
	    if (ap->endpoint() != dst_or_bcast4)
		ap->set_endpoint(dst_or_bcast4);
	}

	if (ap->prefix_len() != prefix_len)
	    ap->set_prefix_len(prefix_len);

	if (! ap->enabled())
	    ap->set_enabled(true);
    }

    if (addr.is_ipv6()) {
	IPv6 addr6 = addr.get_ipv6();
	IPv6 dst_or_bcast6 = dst_or_bcast.get_ipv6();
	IfTreeAddr6* ap = vifp->find_addr(addr6);

	if (ap == NULL) {
	    if (vifp->add_addr(addr6) != XORP_OK) {
		error_msg = c_format("Cannot add address '%s' "
				     "to interface '%s' vif '%s'",
				     addr6.str().c_str(),
				     ifname.c_str(),
				     vifname.c_str());
		return (XORP_ERROR);
	    }
	    ap = vifp->find_addr(addr6);
	    XLOG_ASSERT(ap != NULL);
	}

	//
	// Update the address
	//
	if (ap->point_to_point() != is_p2p)
	    ap->set_point_to_point(is_p2p);

	if (ap->point_to_point()) {
	    if (ap->endpoint() != dst_or_bcast6)
		ap->set_endpoint(dst_or_bcast6);
	}

	if (ap->prefix_len() != prefix_len)
	    ap->set_prefix_len(prefix_len);

	if (! ap->enabled())
	    ap->set_enabled(true);
    }

    return (XORP_OK);
}

int
IfConfigSetClick::delete_vif_address(const string& ifname,
				     const string& vifname,
				     uint32_t if_index,
				     const IPvX& addr,
				     uint32_t prefix_len,
				     string& error_msg)
{
    IfTreeInterface* ifp;
    IfTreeVif* vifp;

    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      XORP_UINT_CAST(prefix_len));

    UNUSED(if_index);
    UNUSED(prefix_len);

    ifp = _iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot delete address "
			     "on interface '%s' vif '%s': "
			     "no such interface in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	error_msg = c_format("Cannot delete address "
			     "on interface '%s' vif '%s': "
			     "no such vif in the interface tree",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the address
    //
    if (addr.is_ipv4()) {
	IPv4 addr4 = addr.get_ipv4();
	IfTreeAddr4* ap = vifp->find_addr(addr4);

	if (ap == NULL) {
	    error_msg = c_format("Cannot delete address '%s' "
				 "on interface '%s' vif '%s': "
				 "no such address",
				 addr4.str().c_str(),
				 ifname.c_str(),
				 vifname.c_str());
	    return (XORP_ERROR);
	}
	vifp->remove_addr(addr4);
	ifconfig().nexthop_port_mapper().delete_ipv4(addr4);
    }

    if (addr.is_ipv6()) {
	IPv6 addr6 = addr.get_ipv6();
	IfTreeAddr6* ap = vifp->find_addr(addr6);
	if (ap == NULL) {
	    error_msg = c_format("Cannot delete address '%s' "
				 "on interface '%s' vif '%s': "
				 "no such address",
				 addr6.str().c_str(),
				 ifname.c_str(),
				 vifname.c_str());
	    return (XORP_ERROR);
	}
	vifp->remove_addr(addr6);
	ifconfig().nexthop_port_mapper().delete_ipv6(addr6);
    }

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
	const IfTreeInterface& fi = ii->second;
	preamble = "    ";
	config += preamble + c_format("interface %s {\n",
				      fi.ifname().c_str());
	preamble = "\t";
	config += preamble + c_format("disable: %s\n",
				      bool_c_str(! fi.enabled()));
	config += preamble + c_format("discard: %s\n",
				      bool_c_str(fi.discard()));
	config += preamble + c_format("mac: %s\n", fi.mac().str().c_str());
	config += preamble + c_format("mtu: %u\n", XORP_UINT_CAST(fi.mtu()));
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    preamble = "\t";
	    config += preamble + c_format("vif %s {\n", fv.vifname().c_str());
	    preamble = "\t    ";
	    config += preamble + c_format("disable: %s\n",
					  bool_c_str(! fv.enabled()));
	    for (ai4 = fv.ipv4addrs().begin(); ai4 != fv.ipv4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
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
		const IfTreeAddr6& fa6 = ai6->second
;		preamble = "\t    ";
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
	const IfTreeInterface& fi = ii->second;
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
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    ifconfig().nexthop_port_mapper().add_interface(fi.ifname(),
							   fv.vifname(),
							   xorp_rt_port);
	    for (ai4 = fv.ipv4addrs().begin(); ai4 != fv.ipv4addrs().end(); ++ai4) {
		const IfTreeAddr4& fa4 = ai4->second;
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
		const IfTreeAddr6& fa6 = ai6->second;
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
