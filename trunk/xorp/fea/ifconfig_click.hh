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

// $XORP: xorp/fea/ifconfig_click.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_IFCONFIG_CLICK_HH__
#define __FEA_IFCONFIG_CLICK_HH__

#include "libxorp/ipvx.hh"
#include "click.hh"

/*
** This class allows snooping on device configuration operations.
** Therefore allowing the "click" configuration to stay in step when
** changes occur.
*/

class IfconfigClick : public Ifconfig {
public:
    /*
    ** The output port number is allocated from 1 as 0 is used for
    ** sending packets to the host.
    */
    IfconfigClick(Ifconfig& i) :
	_next(i), _port(1) {
    }

    /*
    ** Get all interfaces on the system.
    */
    bool get_all_interface_names(list<string>& names);

    /*
    ** Get all the addresses associated with this vif.
    */
    bool get_vif_addresses(const string& vifname, list<IPvX>& addresses);

    /*
    ** Verify that the system has a vif with this name.
    */
    bool vif_exists(const string& vifname);

    /*
    ** Set the mac address of this interface.
    */
    bool set_mac(const string& interface_name, const MAC& mac);

    /*
    ** Get the mac address on this interface.
    */
    bool get_mac(const string& interface_name, MAC& mac);

    /*
    ** Set the mtu of this interface.
    */
    bool set_mtu(const string& interface_name, int mtu);

    /*
    ** Get the mtu of this interface.
    */
    bool get_mtu(const string& interface_name, int& mtu);

    /*
    ** Get flags associated with interface.
    */
    bool get_flags(const string& interface_name,
		   const IPvX&	 address,
		   uint32_t&	 flags);

    /*
    ** Set the address.
    */
    bool add_address(const string& interface_name, const IPvX& addr);

    /*
    ** Delete this address.
    */
    bool delete_address(const string& interface_name, const IPvX& addr);

    /*
    ** set netmask prefix.
    */
    bool set_prefix(const string& interface_name, const IPvX& addr, int prefix);

    /*
    ** get netmask prefix.
    */
    bool get_prefix(const string& interface_name, const IPvX& addr, int& prefix);

    /*
    ** set the broadcast address.
    */
    bool set_broadcast(const string& interface_name, const IPvX& addr,
		       const IPvX& broadcast);

    /*
    ** get the broadcast address.
    */
    bool get_broadcast(const string& interface_name, const IPvX& addr,
		       IPvX& broadcast);

    /*
    ** set the endpoint address (on point-to-point links).
    */
    bool set_endpoint(const string& interface_name, const IPvX& addr,
		      const IPvX& endpoint);

    /*
    ** get the endpoint address (on point-to-point links).
    */
    bool get_endpoint(const string& interface_name, const IPvX& addr,
		      IPvX& endpoint);

    /*
    ** set callback associated with interface/vif/address events.
    */
    void set_interface_event_callback(const InterfaceEventCallback&);
    void set_vif_event_callback(const IfTreeVifEventCallback&);
    void set_address_event_callback(const AddressEventCallback&);

    /*
    ** Interface to click port mapping specific to click.
    */
    int ifname_to_port(const string& ifname) const;
    string port_to_ifname(int port) const;
private:
    /*
    ** If an error has occured propagate the error string.
    */
    bool propagate(bool good);

    /*
    ** Add an interface.
    */
    bool add_interface(const string& interface_name, const IPvX& address);

    /*
    ** Interface changed.
    */
    bool interface_changed(const string& interface_name, const IPvX& address);

    /*
    ** Delete an interface.
    */
    bool delete_interface(const string& interface_name, const IPvX& address);

    /*
    ** Update the entries in an Interface structure.
    */
    struct Interface;
    bool update(Interface& i);

    /*
    ** Generate a click config.
    */
    bool config();

    /*
    ** Utility methods for config.
    */
    const string tohost();
    const string fromdevice(const Interface& i);
    const string todevice(const Interface& i);
    const int port(const Interface& i);
    const string arp(const Interface& i, string text);

    /*
    ** The next ifconfig class.
    */
    Ifconfig& _next;

    /*
    ** Used to generate a click configuration.
    */
    struct Interface {
	explicit Interface(const string& ifname, IPvX address) :
	    interface_name(ifname), address(address) {

	    in_use = true;
	    port = INVALID;
	}

	static const int INVALID = -1;

	string interface_name;
	bool polling;
	IPvX	address;
	IPvX	mask;
	MAC	mac;

	int port;
	bool in_use;
    };

    list<Interface> _ifs;

    /*
    ** Next available port number.
    */
    int _port;
};

#endif // __FEA_IFCONFIG_CLICK_HH__
