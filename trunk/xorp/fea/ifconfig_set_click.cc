// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "ifconfig.hh"
#include "ifconfig_set.hh"


// TODO: XXX: PAVPAVPAV: temporary here
// #define DEBUG_CLICK

//
// Set information about network interfaces configuration with the
// underlying system.
//
// The mechanism to set the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//

IfConfigSetClick::IfConfigSetClick(IfConfig& ifc)
    : IfConfigSet(ifc),
      ClickSocket(ifc.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
#ifdef DEBUG_CLICK	// TODO: XXX: PAVPAVPAV
    register_ifc_secondary();
    ClickSocket::enable_user_click(true);
#endif
}

IfConfigSetClick::~IfConfigSetClick()
{
    stop();
}

int
IfConfigSetClick::start()
{
    if (_is_running)
	return (XORP_OK);

#ifndef DEBUG_CLICK	// TODO: XXX: PAVPAVPAV
    register_ifc_secondary();
#endif

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
IfConfigSetClick::stop()
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop();

    _is_running = false;

    return (ret_value);
}

int
IfConfigSetClick::config_begin(string& errmsg)
{
    debug_msg("config_begin\n");

    UNUSED(errmsg);

    return (XORP_OK);
}

int
IfConfigSetClick::config_end(string& errmsg)
{
    string config;

    debug_msg("config_end\n");

    UNUSED(errmsg);

    config = generate_config();

    FILE *f = fopen("foo.click", "r");
    char buf[100000];
    fread(&buf[0], sizeof(buf), 1, f);

    printf("FOO_BEGIN\n");
    // printf("%s", config.c_str());
    config = string(buf);
    printf("%s", config.c_str());
    printf("FOO_END\n");

    string write_config = c_format("WRITEDATA hotconfig %u\n",
				   static_cast<uint32_t>(config.size()));
    write_config += config;
    if (ClickSocket::write(write_config.c_str(), write_config.size())
	!= static_cast<ssize_t>(write_config.size())) {
	errmsg = c_format("Error writing to Click socket: %s",
			  strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Check the command status
    //
    bool is_warning, is_error;
    string command_warning, command_error;
    if (ClickSocket::check_command_status(is_warning, command_warning,
					  is_error, command_error,
					  errmsg) != XORP_OK) {
	errmsg = c_format("Error verifying the command status after writing to Click socket: %s",
			  errmsg.c_str());
	return (XORP_ERROR);
    }

    if (is_warning) {
	XLOG_WARNING("Click command warning: %s", command_warning.c_str());
    }
    if (is_error) {
	XLOG_ERROR("Click command error: %s", command_error.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfigSetClick::add_interface(const string& ifname,
				uint16_t if_index,
				string& errmsg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("add_interface "
	      "(ifname = %s if_index = %u)\n",
	      ifname.c_str(), if_index);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	//
	// Add the new interface
	//
	if (_iftree.add_if(ifname) != true) {
	    errmsg = c_format("Cannot add interface '%s'", ifname.c_str());
	    return (XORP_ERROR);
	}
	ii = _iftree.get_if(ifname);
	XLOG_ASSERT(ii != _iftree.ifs().end());
    }

    // Update the interface
    if (ii->second.pif_index() != if_index)
	ii->second.set_pif_index(if_index);

    return (XORP_OK);
}

int
IfConfigSetClick::add_vif(const string& ifname,
			  const string& vifname,
			  uint16_t if_index,
			  string& errmsg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("add_vif "
	      "(ifname = %s vifname = %s if_index = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot add interface '%s' vif '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);
    if (vi == fi.vifs().end()) {
	//
	// Add the new vif
	//
	if (fi.add_vif(vifname) != true) {
	    errmsg = c_format("Cannot add interface '%s' vif '%s'",
			      ifname.c_str(), vifname.c_str());
	    return (XORP_ERROR);
	}
	vi = fi.get_vif(vifname);
	XLOG_ASSERT(vi != fi.vifs().end());
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::config_interface(const string& ifname,
				   uint16_t if_index,
				   uint32_t flags,
				   bool is_up,
				   bool is_deleted,
				   string& errmsg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("config_interface "
	      "(ifname = %s if_index = %u flags = 0x%x is_up = %s "
	      "is_deleted = %s)\n",
	      ifname.c_str(), if_index, flags, (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false");

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot configure interface '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Update the interface
    //
    if (fi.pif_index() != if_index)
	fi.set_pif_index(if_index);

    if (fi.if_flags() != flags)
	fi.set_if_flags(flags);

    if (fi.enabled() != is_up)
	fi.set_enabled(is_up);

    if (is_deleted)
	_iftree.remove_if(ifname);

    return (XORP_OK);
}

int
IfConfigSetClick::config_vif(const string& ifname,
			     const string& vifname,
			     uint16_t if_index,
			     uint32_t flags,
			     bool is_up,
			     bool is_deleted,
			     bool broadcast,
			     bool loopback,
			     bool point_to_point,
			     bool multicast,
			     string& errmsg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("config_vif "
	      "(ifname = %s vifname = %s if_index = %u flags = 0x%x "
	      "is_up = %s is_deleted = %s broadcast = %s loopback = %s "
	      "point_to_point = %s multicast = %s)\n",
	      ifname.c_str(), vifname.c_str(), if_index, flags,
	      (is_up)? "true" : "false",
	      (is_deleted)? "true" : "false",
	      (broadcast)? "true" : "false",
	      (loopback)? "true" : "false",
	      (point_to_point)? "true" : "false",
	      (multicast)? "true" : "false");

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot configure interface '%s' vif '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);
    if (vi == fi.vifs().end()) {
	errmsg = c_format("Cannot configure interface '%s' vif '%s': "
			  "no such vif in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Update the vif
    //
    if (fv.enabled() != is_up)
	fv.set_enabled(is_up);

    if (fv.broadcast() != broadcast)
	fv.set_broadcast(broadcast);

    if (fv.loopback() != loopback)
	fv.set_loopback(loopback);

    if (fv.point_to_point() != point_to_point)
	fv.set_point_to_point(point_to_point);

    if (fv.multicast() != multicast)
	fv.set_multicast(multicast);

    if (is_deleted)
	fi.remove_vif(vifname);

    return (XORP_OK);

    UNUSED(if_index);
    UNUSED(flags);
}

int
IfConfigSetClick::set_interface_mac_address(const string& ifname,
					    uint16_t if_index,
					    const struct ether_addr& ether_addr,
					    string& errmsg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("set_interface_mac "
	      "(ifname = %s if_index = %u mac = %s)\n",
	      ifname.c_str(), if_index, EtherMac(ether_addr).str().c_str());

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot set MAC address on interface '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Set the MAC address
    //
    try {
	EtherMac new_ether_mac(ether_addr);
	Mac new_mac = new_ether_mac;

	if (fi.mac() != new_mac)
	    fi.set_mac(new_mac);
    } catch (BadMac) {
	errmsg = c_format("Cannot set MAC address on interface '%s' to '%s': "
			  "invalid MAC address",
			  ifname.c_str(),
			  ether_ntoa(const_cast<struct ether_addr *>(&ether_addr)));
	return (XORP_ERROR);
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::set_interface_mtu(const string& ifname,
				    uint16_t if_index,
				    uint32_t mtu,
				    string& errmsg)
{
    IfTree::IfMap::iterator ii;

    debug_msg("set_interface_mtu "
	      "(ifname = %s if_index = %u mtu = %u)\n",
	      ifname.c_str(), if_index, mtu);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot set MTU on interface '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    //
    // Set the MTU
    //
    if (fi.mtu() != mtu)
	fi.set_mtu(mtu);

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::add_vif_address(const string& ifname,
				  const string& vifname,
				  uint16_t if_index,
				  bool is_broadcast,
				  bool is_p2p,
				  const IPvX& addr,
				  const IPvX& dst_or_bcast,
				  uint32_t prefix_len,
				  string& errmsg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("add_vif_address "
	      "(ifname = %s vifname = %s if_index = %u is_broadcast = %s "
	      "is_p2p = %s addr = %s dst/bcast = %s prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index,
	      (is_broadcast)? "true" : "false",
	      (is_p2p)? "true" : "false", addr.str().c_str(),
	      dst_or_bcast.str().c_str(), prefix_len);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot add address to interface '%s' vif '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);

    if (vi == fi.vifs().end()) {
	errmsg = c_format("Cannot add address to interface '%s' vif '%s': "
			  "no such vif in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Add the address
    //
    if (addr.is_ipv4()) {
	IfTreeVif::V4Map::iterator ai;
	IPv4 addr4 = addr.get_ipv4();
	IPv4 dst_or_bcast4 = dst_or_bcast.get_ipv4();

	ai = fv.get_addr(addr4);
	if (ai == fv.v4addrs().end()) {
	    if (fv.add_addr(addr4) != true) {
		errmsg = c_format("Cannot add address '%s' "
				  "to interface '%s' vif '%s'",
				  addr4.str().c_str(),
				  ifname.c_str(),
				  vifname.c_str());
		return (XORP_ERROR);
	    }
	    ai = fv.get_addr(addr4);
	    XLOG_ASSERT(ai != fv.v4addrs().end());
	}
	IfTreeAddr4& fa = ai->second;

	//
	// Update the address
	//
	if (fa.broadcast() != is_broadcast)
	    fa.set_broadcast(is_broadcast);

	if (fa.point_to_point() != is_p2p)
	    fa.set_point_to_point(is_p2p);

	if (fa.broadcast()) {
	    if (fa.bcast() != dst_or_bcast4)
		fa.set_bcast(dst_or_bcast4);
	}

	if (fa.point_to_point()) {
	    if (fa.endpoint() != dst_or_bcast4)
		fa.set_endpoint(dst_or_bcast4);
	}

	if (fa.prefix_len() != prefix_len)
	    fa.set_prefix_len(prefix_len);

	if (! fa.enabled())
	    fa.set_enabled(true);
    }

    if (addr.is_ipv6()) {
	IfTreeVif::V6Map::iterator ai;
	IPv6 addr6 = addr.get_ipv6();
	IPv6 dst_or_bcast6 = dst_or_bcast.get_ipv6();

	ai = fv.get_addr(addr6);
	if (ai == fv.v6addrs().end()) {
	    if (fv.add_addr(addr6) != true) {
		errmsg = c_format("Cannot add address '%s' "
				  "to interface '%s' vif '%s'",
				  addr6.str().c_str(),
				  ifname.c_str(),
				  vifname.c_str());
		return (XORP_ERROR);
	    }
	    ai = fv.get_addr(addr6);
	    XLOG_ASSERT(ai != fv.v6addrs().end());
	}
	IfTreeAddr6& fa = ai->second;

	//
	// Update the address
	//
	if (fa.point_to_point() != is_p2p)
	    fa.set_point_to_point(is_p2p);

	if (fa.point_to_point()) {
	    if (fa.endpoint() != dst_or_bcast6)
		fa.set_endpoint(dst_or_bcast6);
	}

	if (fa.prefix_len() != prefix_len)
	    fa.set_prefix_len(prefix_len);

	if (! fa.enabled())
	    fa.set_enabled(true);
    }

    return (XORP_OK);

    UNUSED(if_index);
}

int
IfConfigSetClick::delete_vif_address(const string& ifname,
				     const string& vifname,
				     uint16_t if_index,
				     const IPvX& addr,
				     uint32_t prefix_len,
				     string& errmsg)
{
    IfTree::IfMap::iterator ii;
    IfTreeInterface::VifMap::iterator vi;

    debug_msg("delete_vif_address "
	      "(ifname = %s vifname = %s if_index = %u addr = %s "
	      "prefix_len = %u)\n",
	      ifname.c_str(), vifname.c_str(), if_index, addr.str().c_str(),
	      prefix_len);

    ii = _iftree.get_if(ifname);
    if (ii == _iftree.ifs().end()) {
	errmsg = c_format("Cannot delete address on interface '%s' vif '%s': "
			  "no such interface in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeInterface& fi = ii->second;

    vi = fi.get_vif(vifname);

    if (vi == fi.vifs().end()) {
	errmsg = c_format("Cannot delete address on interface '%s' vif '%s': "
			  "no such vif in the interface tree",
			  ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    IfTreeVif& fv = vi->second;

    //
    // Delete the address
    //
    if (addr.is_ipv4()) {
	IfTreeVif::V4Map::iterator ai;
	IPv4 addr4 = addr.get_ipv4();

	ai = fv.get_addr(addr4);
	if (ai == fv.v4addrs().end()) {
	    errmsg = c_format("Cannot delete address '%s' "
			      "on interface '%s' vif '%s': "
			      "no such address",
			      addr4.str().c_str(),
			      ifname.c_str(),
			      vifname.c_str());
	    return (XORP_ERROR);
	}
	fv.remove_addr(addr4);
    }

    if (addr.is_ipv6()) {
	IfTreeVif::V6Map::iterator ai;
	IPv6 addr6 = addr.get_ipv6();

	ai = fv.get_addr(addr6);
	if (ai == fv.v6addrs().end()) {
	    errmsg = c_format("Cannot delete address '%s' "
			      "on interface '%s' vif '%s': "
			      "no such address",
			      addr6.str().c_str(),
			      ifname.c_str(),
			      vifname.c_str());
	    return (XORP_ERROR);
	}
	fv.remove_addr(addr6);
    }

    return (XORP_OK);

    UNUSED(if_index);
    UNUSED(prefix_len);
}

string
IfConfigSetClick::generate_config()
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::V4Map::const_iterator ai;
    uint32_t port, max_vifs;

    string config;

    //
    // Finalize IfTree state
    //
    _iftree.finalize_state();

    //
    // Calculate the number of vifs
    //
    max_vifs = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    max_vifs++;
	}
    }


    config += "//\n";
    config += "// Generated by XORP FEA\n";
    config += "//\n";
    config += "\n";
    config += "//\n";
    config += "// Configured interfaces:\n";
    config += "//\n";
    config += "/*\n";
    config += _iftree.str();
    config += "*/\n";

    config += "\n";
    config += "// Shared IP input path and routing table\n";
    string xorp_ip = "_xorp_ip";
    string xorp_rt = "_xorp_rt";
    string check_ip_header = "CheckIPHeader(INTERFACES";
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    for (ai = fv.v4addrs().begin(); ai != fv.v4addrs().end(); ++ai) {
		const IfTreeAddr4& fa = ai->second;
		check_ip_header += c_format(" %s/%u",
					    fa.addr().str().c_str(),
					    fa.prefix_len());
	    }
	}
    }
    check_ip_header += ")";
	    
    config += c_format("%s :: Strip(14)\n", xorp_ip.c_str());
    config += c_format("    -> %s\n", check_ip_header.c_str());
    config += c_format("    -> %s :: TrieIPLookup();\n", xorp_rt.c_str());

    config += "\n";
    config += "// ARP responses are copied to each ARPQuerier and the host.\n";
    string xorp_arpt = "_xorp_arpt";
    config += c_format("%s :: Tee(%u);\n", xorp_arpt.c_str(), max_vifs + 1);

    port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi, ++port) {
	    const IfTreeVif& fv = vi->second;
	    string xorp_c = c_format("_xorp_c%u", port);
	    string xorp_out = c_format("_xorp_out%u", port);
	    string xorp_to_device = c_format("_xorp_to_device%u", port);
	    string xorp_ar = c_format("_xorp_ar%u", port);
	    string xorp_arpq = c_format("_xorp_arpq%u", port);

	    string from_device, to_device, arp_responder, arp_querier;
	    string paint, print;

	    if (! fv.enabled()) {
		from_device = "NullDevice";
	    } else {
		//
		// TODO: XXX: we should find a way to configure polling
		// devices. Such devices should use "PollDevice" instead
		// of "FromDevice".
		//
		from_device = c_format("FromDevice(%s)", fv.vifname().c_str());
	    }

	    if (! fv.enabled()) {
		to_device = "Discard";
	    } else {
		to_device = c_format("ToDevice(%s)", fv.vifname().c_str());
	    }

	    arp_responder = c_format("ARPResponder(");
	    for (ai = fv.v4addrs().begin(); ai != fv.v4addrs().end(); ++ai) {
		const IfTreeAddr4& fa = ai->second;
		if (ai != fv.v4addrs().begin())
		    arp_responder += " ";
		arp_responder += fa.addr().str();
	    }
	    arp_responder += c_format(" %s)", fi.mac().str().c_str());

	    IPv4 first_addr4 = IPv4::ZERO();
	    ai = fv.v4addrs().begin();
	    if (ai != fv.v4addrs().end()) {
		first_addr4 = ai->second.addr();
	    }
	    arp_querier = c_format("ARPQuerier(%s, %s)",
				   first_addr4.str().c_str(),
				   fi.mac().str().c_str());

	    paint = c_format("Paint(%u)", port + 1);
	    print = c_format("Print(\"%s non-IP\")",
			     fv.vifname().c_str());

	    config += "\n";
	    config += c_format("// Input and output paths for %s\n",
			       fv.vifname().c_str());
	    config += c_format("%s :: Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);\n",
			       xorp_c.c_str());
	    config += c_format("%s -> %s;\n",
			       from_device.c_str(),
			       xorp_c.c_str());
	    config += c_format("%s :: Queue(200) -> %s :: %s;\n",
			       xorp_out.c_str(),
			       xorp_to_device.c_str(),
			       to_device.c_str());
	    config += c_format("%s[0] -> %s :: %s -> %s;\n",
			       xorp_c.c_str(),
			       xorp_ar.c_str(),
			       arp_responder.c_str(),
			       xorp_out.c_str());
	    config += c_format("%s :: %s -> %s;\n",
			       xorp_arpq.c_str(),
			       arp_querier.c_str(),
			       xorp_out.c_str());
	    config += c_format("%s[1] -> %s;\n",
			       xorp_c.c_str(),
			       xorp_arpt.c_str());
	    config += c_format("%s[%u] -> [1]%s;\n",
			       xorp_arpt.c_str(),
			       port,
			       xorp_arpq.c_str());
	    config += c_format("%s[2] -> %s -> %s;\n",
			       xorp_c.c_str(),
			       paint.c_str(),
			       xorp_ip.c_str());
	    config += c_format("%s[3] -> %s -> Discard;\n",
			       xorp_c.c_str(),
			       print.c_str());
	}
    }

    //
    // Send packets to the host
    //
    config += "\n";
    config += "// Local delivery\n";
    string xorp_toh = "_xorp_toh";
    // TODO: XXX: PAVPAVPAV: fix ether_encap
    string ether_encap = c_format("EtherEncap(0x0800, 1:1:1:1:1:1, 2:2:2:2:2:2)");
    // TODO: XXX: PAVPAVPAV: Fix the Local delivery for *BSD and/or user-level Click
    if (ClickSocket::is_kernel_click()) {
	config += c_format("%s :: ToHost;\n", xorp_toh.c_str());
    } else {
	config += c_format("%s :: Discard;\n", xorp_toh.c_str());
    }
    config += c_format("%s[%u] -> %s;\n",
		       xorp_arpt.c_str(),
		       max_vifs,
		       xorp_toh.c_str());
    config += c_format("%s[0] -> %s -> %s;\n",
		       xorp_rt.c_str(),
		       ether_encap.c_str(),
		       xorp_toh.c_str());

    port = 0;
    for (ii = _iftree.ifs().begin(); ii != _iftree.ifs().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi, ++port) {
	    const IfTreeVif& fv = vi->second;
	    string xorp_cp = c_format("_xorp_cp%u", port);
	    string paint_tee = c_format("PaintTee(%u)", port + 1);
	    string xorp_gio = c_format("_xorp_gio%u", port);
	    string xorp_dt = c_format("_xorp_dt%u", port);
	    string xorp_fr = c_format("_xorp_fr%u", port);
	    string xorp_arpq = c_format("_xorp_arpq%u", port);	    

	    string ipgw_options = c_format("IPGWOptions(");
	    for (ai = fv.v4addrs().begin(); ai != fv.v4addrs().end(); ++ai) {
		const IfTreeAddr4& fa = ai->second;
		//
		// TODO: XXX: PAVPAVPAV: check whether the syntax is to use
		// spaces or commas.
		//
		if (ai != fv.v4addrs().begin())
		    ipgw_options += " ";
		ipgw_options += c_format("%s", fa.addr().str().c_str());
	    }
	    ipgw_options += ")";

	    IPv4 first_addr4 = IPv4::ZERO();
	    ai = fv.v4addrs().begin();
	    if (ai != fv.v4addrs().end()) {
		first_addr4 = ai->second.addr();
	    }
	    string fix_ip_src = c_format("FixIPSrc(%s)",
					 first_addr4.str().c_str());
	    string ip_fragmenter = c_format("IPFragmenter(%u)",
					    fi.mtu());

	    config += "\n";
	    config += c_format("// Forwarding path for %s\n",
			       fv.vifname().c_str());
	    config += c_format("%s[%u] -> DropBroadcasts\n",
			       xorp_rt.c_str(),
			       port + 1);
	    config += c_format("    -> %s :: %s\n",
			       xorp_cp.c_str(),
			       paint_tee.c_str());
	    config += c_format("    -> %s :: %s\n",
			       xorp_gio.c_str(),
			       ipgw_options.c_str());
	    config += c_format("    -> %s\n",
			       fix_ip_src.c_str());
	    config += c_format("    -> %s :: DecIPTTL\n",
			       xorp_dt.c_str());
	    config += c_format("    -> %s :: %s\n",
			       xorp_fr.c_str(),
			       ip_fragmenter.c_str());
	    config += c_format("    -> [0]%s;\n",
			       xorp_arpq.c_str());

	    config += c_format("%s[1] -> ICMPError(%s, timeexceeded) -> %s;\n",
			       xorp_dt.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, unreachable, needfrag) -> %s;\n",
			       xorp_fr.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, parameterproblem) -> %s;\n",
			       xorp_gio.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	    config += c_format("%s[1] -> ICMPError(%s, redirect, host) -> %s;\n",
			       xorp_cp.c_str(),
			       first_addr4.str().c_str(),
			       xorp_rt.c_str());
	}
    }
    return (config);
}
