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

#ident "$XORP: xorp/fea/ifconfig_click.cc,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $"

#include <fstream>
#include <string>
#include <list>

#include "config.h"
#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipvx.hh"
#include "libxorp/mac.hh"
#include "libxorp/c_format.hh"
#include "ifconfig.hh"
#include "click.hh"
#include "ifconfig_click.hh"

/*
** Get all interfaces on the system.
*/
bool
IfconfigClick::get_all_interface_names(list<string>& names)
{
    debug_msg("\n");

    return propagate(_next.get_all_interface_names(names));
}

/*
** Get all the addresses associated with this vif.
*/
bool
IfconfigClick::get_vif_addresses(const string& vifname, list<IPvX>& addresses)
{
    debug_msg("\n");

    return propagate(_next.get_vif_addresses(vifname, addresses));
}

/*
** Verify that the system has a vif with this name.
*/
bool
IfconfigClick::vif_exists(const string& vifname)
{
    debug_msg("(%s)\n", vifname.c_str());

    return propagate(_next.vif_exists(vifname));
}

/*
** Set the mac address of this interface.
*/
bool
IfconfigClick::set_mac(const string& interface_name, const MAC& mac)
{
    debug_msg("(%s, %s)\n", interface_name.c_str(), mac.str().c_str());

    return propagate(_next.set_mac(interface_name, mac));
}

/*
** Get the mac address on this interface.
*/
bool
IfconfigClick::get_mac(const string& interface_name, MAC& mac)
{
    debug_msg("(%s, %s)\n", interface_name.c_str(), mac.str().c_str());

    return propagate(_next.get_mac(interface_name, mac));
}

/*
** Set the mtu of this interface.
*/
bool
IfconfigClick::set_mtu(const string& interface_name, int mtu)
{
    debug_msg("(%s, %d)\n", interface_name.c_str(), mtu);

    return propagate(_next.set_mtu(interface_name, mtu));
}

/*
** Get the mtu of this interface.
*/
bool
IfconfigClick::get_mtu(const string& interface_name, int& mtu)
{
    debug_msg("(%s, %d)\n", interface_name.c_str(), mtu);

    return propagate(_next.get_mtu(interface_name, mtu));
}

/*
** Get flags associated with this interface
*/
bool
IfconfigClick::get_flags(const string&	interface_name,
			  const IPvX&	address,
			  uint32_t&	flags)
{
    return propagate(_next.get_flags(interface_name, address, flags));
}

/*
** Set the address.
** If the interface already had this address set already to true.
*/
bool
IfconfigClick::add_address(const string& interface_name,
			    const IPvX&   addr)
{
    debug_msg("(%s, %s)\n", interface_name.c_str(), addr.str().c_str());

    if (!propagate(_next.add_address(interface_name, addr)))
	return false;

    return add_interface(interface_name, addr);
}

/*
** Delete this address.
*/
bool
IfconfigClick::delete_address(const string& interface_name, const IPvX& addr)
{
    debug_msg("(%s, %s)\n", interface_name.c_str(), addr.str().c_str());

    if (!propagate(_next.delete_address(interface_name, addr)))
	return false;

    return delete_interface(interface_name, addr);
}

/*
** set netmask prefix.
*/
bool
IfconfigClick::set_prefix(const string& interface_name, const IPvX& addr, int prefix)
{
    debug_msg("(%s, %s, %d)\n", interface_name.c_str(), addr.str().c_str(),
	      prefix);

    if (!propagate(_next.set_prefix(interface_name, addr, prefix)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
** get netmask prefix.
*/
bool
IfconfigClick::get_prefix(const string& interface_name, const IPvX& addr,
			   int& prefix)
{
    debug_msg("(%s, %s, %d)\n", interface_name.c_str(), addr.str().c_str(),
	      prefix);

    if (!propagate(_next.get_prefix(interface_name, addr, prefix)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
** set the broadcast address.
*/
bool
IfconfigClick::set_broadcast(const string& interface_name,
			      const IPvX&   addr,
			      const IPvX&   broadcast)
{
    debug_msg("(%s, %s, %s)\n", interface_name.c_str(), addr.str().c_str(),
	      broadcast.str().c_str());

    if (!propagate(_next.set_broadcast(interface_name, addr, broadcast)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
** get the broadcast address.
*/
bool
IfconfigClick::get_broadcast(const string& interface_name, const IPvX& addr,
			      IPvX& broadcast)
{
    debug_msg("(%s, %s, %s)\n", interface_name.c_str(), addr.str().c_str(),
	      broadcast.str().c_str());

    if (!propagate(_next.get_broadcast(interface_name, addr, broadcast)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
** set the endpoint address.
*/
bool
IfconfigClick::set_endpoint(const string& interface_name,
			     const IPvX&   addr,
			     const IPvX&   endpoint)
{
    debug_msg("(%s, %s, %s)\n", interface_name.c_str(), addr.str().c_str(),
	      endpoint.str().c_str());

    if (!propagate(_next.set_endpoint(interface_name, addr, endpoint)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
** get the endpoint address.
*/
bool
IfconfigClick::get_endpoint(const string& interface_name, const IPvX& addr,
			      IPvX& endpoint)
{
    debug_msg("(%s, %s, %s)\n", interface_name.c_str(), addr.str().c_str(),
	      endpoint.str().c_str());

    if (!propagate(_next.get_endpoint(interface_name, addr, endpoint)))
	return false;

    return interface_changed(interface_name, addr);
}

/*
-------------------- Private methods --------------------
*/

/*
** Proogate the error string.
*/
bool
IfconfigClick::propagate(bool good)
{
    if (!good)
	error(_next.error());

    return good;
}

/*
** Add an interface to the click configuration.
*/
bool
IfconfigClick::add_interface(const string& interface_name, const IPvX& address)
{
    if (address.is_IPv6())
	XLOG_FATAL("Can't handle IPv6 yet: %s %s",
		       interface_name.c_str(), address.str().c_str());

    /*
    ** Need to add ourselves to the list of configured interfaces.
    **
    ** Click port numbers must be contiguous. When an interface is
    ** deleted the port is connected to the "discard"
    ** interface. Totally rebuilding the configuration would change
    ** the mapping of interfaces to ports, hence totally screw up
    ** forwarding, this would be bad.
    **
    ** If may be the case that an interface that was previously
    ** "deleted" is being "added". In this case it will already be in
    ** the table with a port number. So revive the entry.
    */

    /*
    ** Search for this interface and address not in use.
    */
    bool found = false;
    list<Interface>::iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	if (i->interface_name == interface_name && i->address == address) {
	    found = true;
	    break;
	}
    }

    if (true == found) {
	/*
	** If its already in use something bad has happened.
	*/
	if (i->in_use) {
	    IFCONFIG_ERROR("This interface is already configured: %s",
			   interface_name.c_str());
	    return false;
	}
    } else {
	_ifs.push_back(Interface(interface_name, address));
	i = _ifs.end();
	--i;
    }

    /*
    ** XXX
    ** Do magic at some point to figure out if this device supports
    ** polling. Note on BSD there is only one type of device it will
    ** do polling if it can.
    */
    i->polling = false;

    /*
    ** At this point this interface and address should be configured
    ** in the system. It should not be possible for update to fail.
    */
    if (!update(*i))
	XLOG_FATAL("Could not update the configuration for %s %s",
		   interface_name.c_str(), address.str().c_str());

    return config();
}

/*
** Interface changed.
** An attribute of this interface has changed e.g. prefix or broadcast
** address. Therefore get the new values build a new config.
*/
bool
IfconfigClick::interface_changed(const string& interface_name, const IPvX& address)
{
    list<Interface>::iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	if (i->interface_name == interface_name && i->address == address) {
	    if (!update(*i))
		XLOG_FATAL("Could not update the configuration for %s %s",
			   interface_name.c_str(), address.str().c_str());
	    return config();
	}
    }

    IFCONFIG_ERROR("Interface not found: %s", interface_name.c_str());

    return false;
}

/*
** Delete an interface.
*/
bool
IfconfigClick::delete_interface(const string& interface_name, const IPvX& address)
{
    list<Interface>::iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	if (i->interface_name == interface_name && i->address == address) {
	    i->in_use = false;
	    return config();
	}
    }

    IFCONFIG_ERROR("Interface not found: %s", interface_name.c_str());

    return false;
}

/*
** Update the entries in an Interface structure.
*/
bool
IfconfigClick::update(Interface& i)
{
    int prefix;
    if (false == _next.get_prefix(i.interface_name, i.address, prefix))
	return false;

    i.mask = IPvX::make_prefix(i.address.af(), prefix);

    if (false == _next.get_mac(i.interface_name, i.mac))
	return false;

    if (Interface::INVALID == i.port)
	i.port = _port++;

    return true;
}

const
string
IfconfigClick::tohost()
{
    /*
    ** FreeBSD requires a device name if available to "ToHost".
    */
    if (0 == _ifs.size())
	return "Discard";

    return "ToHost(" + _ifs.begin()->interface_name + ")";
}

const
string
IfconfigClick::fromdevice(const Interface& i)
{
    if (!i.in_use)
	return "NullDevice";

    string ret;
    if (i.polling)
	ret = "PollDevice";
    else
	ret = "FromDevice";

    return ret + "(" + i.interface_name + ")";
}

const
string
IfconfigClick::todevice(const Interface& i)
{
    if (i.in_use)
	return "ToDevice";
    else
	return "Discard";
}

const
int
IfconfigClick::port(const Interface& i)
{
    return i.port;
}

const
string
IfconfigClick::arp(const Interface& i, string text)
{
    return text + "(" + i.address.str() + " " + i.mac.str() + ")";
}

static
string
itos(int i)
{
    char buf[1024];
    snprintf(buf, 1024, "%d", i);
    return string(buf);
}

/*
** Generate a click config.
*/
bool
IfconfigClick::config()
{
    std::string os;
    const int num_ifs = _ifs.size();

    /*
    ** Preamble.
    */

    os = "// Generated by FEA\n";

    list<Interface>::iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	os += "// \t" + i->interface_name + " " +
	    i->address.str() + " " + i->mac.str() + "\n";
    }

    os += "\n";

    /*
    ** We need the xorp module to be loaded.
    */
    os += "require(xorp)\n\n";

    /*
    ** Send packets to the host.
    */
    os += "tohost :: " + tohost() + ";\n";
    os += "t :: Tee(" +  itos(num_ifs + 1) + ");\n";
    os += "t[" + itos(num_ifs) + "] -> tohost;\n" ;

    os += "\n";

    /*
    ** Connect up the inputs and outputs.
    */

    int port;
    for (port = 0, i = _ifs.begin(); i != _ifs.end(); i++, port++) {
	if (!i->in_use)
	    continue;

	os += "c" + itos(port) + " :: " +
	    "Classifier(12/0806 20/0001, 12/0806 20/0002, 12/0800, -);\n";

	os += fromdevice(*i) + " -> " + "[0]c" + itos(port) + "\n";

	os += c_format("out%d :: Queue(200) -> todevice%d :: ToDevice(%s);\n",
		       port, port, i->interface_name.c_str());
	os += c_format("arpq%d :: ARPQuerier(%s, %s);\n",
		       port,
		       i->address.str().c_str(),
		       i->mac.str().c_str());
	os += "c" + itos(port) + " [1] -> t;\n";
	os += c_format("t[%d] -> [1]arpq%d;\n", port, port);
	os += "arpq" + itos(port) + " -> out" + itos(port) + ";\n";
	os += "ar" + itos(port) + " :: " + arp(*i, "ARPResponder") + ";\n";
	os += "c" + itos(port) + " [0] -> " + "ar" + itos(port) + " -> out"
	   + itos(port) + ";\n";

	os += "\n";
    }

    /*
    ******************************************
    */
    /*
    ** Setup the routing table.
    */
    os += "rt :: Forward2(\n";

    /*
    ** Deliver packets to the host for our addresses.
    */
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	os += i->address.str() + "/32 0,\n";	// My address.
	IPvX subnet = i->address.mask_by_prefix(i->mask.prefix_length())
		       | ~i->mask;
	os += subnet.str() + "/32 0,\n";	// Subnet broadcast correct.
	subnet = i->address.mask_by_prefix(i->mask.prefix_length());
	os += subnet.str() + "/32 0,\n";	// Subnet broadcast old compat.
    }

    /*
    ** Routes to our neighbours.
    */
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	int port = this->port(*i) - 1;
	os += i->address.mask_by_prefix(i->mask.prefix_length()).str() +
	    "/" + i->mask.str() + " " + itos(port + 1) + ",\n";
    }

    /*
    ** Broadcast. Both kinds country and western.
    */
    os += "255.255.255.255/32 0.0.0.0 0,\n";
    os += "0.0.0.0/32 0);\n";

    /* XXX
    ** No default route to MIT.
    */

    /*
    ******************************************
    */

    /*
    ** To host.
    */
    os += "rt[0] -> EtherEncap(0x0800, 1:1:1:1:1:1, 2:2:2:2:2:2) -> tohost;\n";

    /*
    ** Check the packet is for us.
    */
    os += "ip :: Strip(14)\n";
    os += "    -> CheckIPHeader(";
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	IPvX subnet = i->address.mask_by_prefix(i->mask.prefix_length())
		       | ~i->mask;
	os += subnet.str() + " ";	// Subnet broadcast correct.
    }
    os += ")\n";
    os += "    -> GetIPAddress(16)\n";
    os += "    -> [0]rt;\n";

    /*
    ** Connect ifmgr to input path.
    */
    for (int i = 0; i < num_ifs; i++) {
	os += c_format("c%d [2] -> Paint(%d) -> ip;\n", i, i + 1);
    }

    os += "\n";

    /*
    ** Connect the output paths.
    */
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	int port = this->port(*i) - 1;
	const char *address = i->address.str().c_str();

	if (!i->in_use) {
	    os += c_format("rt[%d] -> Discard\n", port + 1);
	    continue;
	}

	os += c_format("rt[%d] -> DropBroadcasts\n", port + 1);
        os += c_format("       -> cp%d :: PaintTee(%d)\n", port, port + 1);
        os += c_format("       -> gio%d :: IPGWOptions(%s)\n", port, address);
        os += c_format("       -> FixIPSrc(%s)\n", address);
        os += c_format("       -> dt%d :: DecIPTTL\n", port);
        os += c_format("       -> fr%d :: IPFragmenter(1500)\n", port);
        os += c_format("       -> [0]arpq%d\n", port);
 	os += c_format("dt%d [1] -> ICMPError(%s, 11, 0) -> [0]rt;\n",
 		       port, i->address.str().c_str());
	os += c_format("fr%d [1] -> ICMPError(%s, 3, 4) -> [0]rt;\n",
		       port, address);
	os += c_format("gio%d [1] -> ICMPError(%s, 12, 1) -> [0]rt;\n",
		       port, address);
	os += c_format("cp%d [1] -> ICMPError(%s, 5, 1) -> [0]rt;\n",
		       port, address);
	os += c_format("c%d [3] -> Print(xx%d) -> Discard;\n",
		       port, port);
    }

    return Click::configure(os);
}

int
IfconfigClick::ifname_to_port(const string& interface_name) const
{
    list<Interface>::const_iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	if (i->interface_name == interface_name && i->in_use) {
	    return i->port;
	}
    }

    return -1;
}

string
IfconfigClick::port_to_ifname(int port) const
{
    debug_msg("(%d)\n", port);

    if (0 == port)
	return "tobsd";

    list<Interface>::const_iterator i;
    for (i = _ifs.begin(); i != _ifs.end(); i++) {
	debug_msg("interface_name %s port %d in use %s\n",
		  i->interface_name.c_str(), i->port,
		  i->in_use ? "true" : "false");
	if (i->port == port && i->in_use) {
	    return i->interface_name;
	}
    }

    return "badness";
}

void
IfconfigClick::set_interface_event_callback(const InterfaceEventCallback&)
{
    XLOG_ERROR("IfconfigClick::set_interface_event_callback unimplemented");
}

void
IfconfigClick::set_vif_event_callback(const IfTreeVifEventCallback&)
{
    XLOG_ERROR("IfconfigClick::set_vif_event_callback unimplemented");
}

void
IfconfigClick::set_address_event_callback(const AddressEventCallback&)
{
    XLOG_ERROR("IfconfigClick::set_address_event_callback unimplemented");
}
