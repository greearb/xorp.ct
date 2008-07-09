// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_transaction.hh,v 1.19 2008/05/05 21:02:01 pavlin Exp $

#ifndef __FEA_IFCONFIG_TRANSACTION_HH__
#define __FEA_IFCONFIG_TRANSACTION_HH__

#include "libxorp/c_format.hh"
#include "libxorp/transaction.hh"

#include "ifconfig.hh"
#include "iftree.hh"


class IfConfigTransactionManager : public TransactionManager {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     */
    IfConfigTransactionManager(EventLoop& eventloop)
	: TransactionManager(eventloop, TIMEOUT_MS, MAX_PENDING)
    {}

    /**
     * Get the string with the first error during commit.
     *
     * @return the string with the first error during commit or an empty
     * string if no error.
     */
    const string& error() const 	{ return _first_error; }

protected:
    /**
     * Pre-commit method that is called before the first operation
     * in a commit.
     *
     * This is an overriding method.
     *
     * @param tid the transaction ID.
     */
    virtual void pre_commit(uint32_t tid);

    /**
     * Method that is called after each operation.
     *
     * This is an overriding method.
     *
     * @param success set to true if the operation succeeded, otherwise false.
     * @param op the operation that has been just called.
     */
    virtual void operation_result(bool success,
				  const TransactionOperation& op);

private:
    /**
     * Reset the string with the error.
     */
    void reset_error()			{ _first_error.erase(); }

    string	_first_error;		// The string with the first error
    uint32_t	_tid_exec;		// The transaction ID

    enum { TIMEOUT_MS = 5000, MAX_PENDING = 10 };
};

/**
 * Base class for Interface related operations acting on an
 * IfTree.
 */
class IfConfigTransactionOperation : public TransactionOperation {
public:
    IfConfigTransactionOperation(IfConfig& ifconfig, const string& ifname)
	: _ifconfig(ifconfig),
	  _iftree(ifconfig.user_config()),	// XXX: The iftree to modify
	  _ifname(ifname) {}

    /**
     * @return space separated path description.
     */
    virtual string path() const 		{ return _ifname; }

    /**
     * Get the interface name this operation applies to.
     *
     * @return the interface name this operation applies to.
     */
    const string& ifname() const		{ return _ifname; }

protected:
    /**
     * Get the corresponding interface manager.
     *
     * @return a reference to the corresponding interface manager.
     */
    IfConfig& ifconfig()			{ return _ifconfig; }

    /**
     * Get the interface tree to modify.
     *
     * @return a reference to the interface tree to modify.
     */
    IfTree& iftree()				{ return _iftree; }

private:
    IfConfig&		_ifconfig;
    IfTree&		_iftree;
    const string	_ifname;
};

/**
 * Class for adding an interface.
 */
class AddInterface : public IfConfigTransactionOperation {
public:
    AddInterface(IfConfig& ifconfig, const string& ifname)
	: IfConfigTransactionOperation(ifconfig, ifname) {}

    bool dispatch() {
	if (iftree().add_interface(ifname()) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const { return string("AddInterface: ") + ifname(); }
};

/**
 * Class for removing an interface.
 */
class RemoveInterface : public IfConfigTransactionOperation {
public:
    RemoveInterface(IfConfig& ifconfig, const string& ifname)
	: IfConfigTransactionOperation(ifconfig, ifname) {}

    bool dispatch() {
	if (iftree().remove_interface(ifname()) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return string("RemoveInterface: ") + ifname();
    }
};

/**
 * Class for configuring all interfaces within the FEA by using information
 * from the underlying system.
 */
class ConfigureAllInterfacesFromSystem : public IfConfigTransactionOperation {
public:
    ConfigureAllInterfacesFromSystem(IfConfig& ifconfig, bool enable)
	: IfConfigTransactionOperation(ifconfig, ""),
	  _enable(enable)
    {}

    bool dispatch() {
	if (_enable) {
	    //
	    // Configure all interfaces
	    //
	    const IfTree& dev_config = ifconfig().system_config();
	    IfTree::IfMap::const_iterator iter;
	    for (iter = dev_config.interfaces().begin();
		 iter != dev_config.interfaces().end();
		 ++iter) {
		const IfTreeInterface& iface = *(iter->second);
		if (iftree().update_interface(iface) != XORP_OK)
		    return (false);
	    }
	}

	//
	// Set the "default_system_config" flag for all interfaces
	//
	IfTree::IfMap::iterator iter;
	for (iter = iftree().interfaces().begin();
	     iter != iftree().interfaces().end();
	     ++iter) {
	    IfTreeInterface& iface = *(iter->second);
	    iface.set_default_system_config(_enable);
	}

	return (true);
    }

    string str() const {
	return c_format("ConfigureAllInterfacesFromSystem: %s",
			bool_c_str(_enable));
    }

private:
    bool	_enable;
};

/**
 * Class for configuring an interface within the FEA by using information
 * from the underlying system.
 */
class ConfigureInterfaceFromSystem : public IfConfigTransactionOperation {
public:
    ConfigureInterfaceFromSystem(IfConfig&	ifconfig,
				 const string&	ifname,
				 bool		enable)
	: IfConfigTransactionOperation(ifconfig, ifname),
	  _enable(enable)
    {}

    bool dispatch() {
	IfTreeInterface* ifp = iftree().find_interface(ifname());
	if (ifp == NULL)
	    return (false);
	ifp->set_default_system_config(_enable);
	return (true);
    }

    string str() const {
	return c_format("ConfigureInterfaceFromSystem: %s %s",
			ifname().c_str(), bool_c_str(_enable));
    }

private:
    bool	_enable;
};

/**
 * Base class for interface modifier operations.
 */
class InterfaceModifier : public IfConfigTransactionOperation {
public:
    InterfaceModifier(IfConfig&		ifconfig,
		      const string&	ifname)
	: IfConfigTransactionOperation(ifconfig, ifname) {}

protected:
    IfTreeInterface* interface() {
	IfTreeInterface* ifp = iftree().find_interface(ifname());
	return (ifp);
    }
};

/**
 * Class for setting the enabled state of an interface.
 */
class SetInterfaceEnabled : public InterfaceModifier {
public:
    SetInterfaceEnabled(IfConfig&	ifconfig,
			const string&	ifname,
			bool		enabled)
	: InterfaceModifier(ifconfig, ifname), _enabled(enabled) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_enabled(_enabled);
	return (true);
    }

    string str() const {
	return c_format("SetInterfaceEnabled: %s %s",
			ifname().c_str(), bool_c_str(_enabled));
    }

private:
    bool _enabled;
};

/**
 * Class for setting the discard state of an interface.
 */
class SetInterfaceDiscard : public InterfaceModifier {
public:
    SetInterfaceDiscard(IfConfig&	ifconfig,
			const string&	ifname,
			bool		discard)
	: InterfaceModifier(ifconfig, ifname), _discard(discard) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_discard(_discard);
	return (true);
    }

    string str() const {
	return c_format("SetInterfaceDiscard: %s %s",
			ifname().c_str(), bool_c_str(_discard));
    }

private:
    bool _discard;
};

/**
 * Class for setting the unreachable state of an interface.
 */
class SetInterfaceUnreachable : public InterfaceModifier {
public:
    SetInterfaceUnreachable(IfConfig&		ifconfig,
			    const string&	ifname,
			    bool		unreachable)
	: InterfaceModifier(ifconfig, ifname), _unreachable(unreachable) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_unreachable(_unreachable);
	return (true);
    }

    string str() const {
	return c_format("SetInterfaceUnreachable: %s %s",
			ifname().c_str(), bool_c_str(_unreachable));
    }

private:
    bool _unreachable;
};

/**
 * Class for setting the management state of an interface.
 */
class SetInterfaceManagement : public InterfaceModifier {
public:
    SetInterfaceManagement(IfConfig&		ifconfig,
			   const string&	ifname,
			   bool			management)
	: InterfaceModifier(ifconfig, ifname), _management(management) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_management(_management);
	return (true);
    }

    string str() const {
	return c_format("SetInterfaceManagement: %s %s",
			ifname().c_str(), bool_c_str(_management));
    }

private:
    bool _management;
};

/**
 * Class for setting an interface MTU.
 */
class SetInterfaceMtu : public InterfaceModifier {
public:
    SetInterfaceMtu(IfConfig&		ifconfig,
		    const string&	ifname,
		    uint32_t		mtu)
	: InterfaceModifier(ifconfig, ifname), _mtu(mtu) {}

    // Minimum and maximum MTU (as defined in RFC 791 and RFC 1191)
    static const uint32_t MIN_MTU = 68;
    static const uint32_t MAX_MTU = 65536;
    
    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);

	if (_mtu < MIN_MTU || _mtu > MAX_MTU)
	    return (false);
	
	fi->set_mtu(_mtu);
	return (true);
    }

    string str() const {
	string s = c_format("SetInterfaceMtu: %s %u", ifname().c_str(),
			    XORP_UINT_CAST(_mtu));
	if (_mtu < MIN_MTU || _mtu > MAX_MTU) {
	    s += c_format(" (valid range %u--%u)",
			  XORP_UINT_CAST(MIN_MTU), XORP_UINT_CAST(MAX_MTU));
	}
	return s;
    }

private:
    uint32_t _mtu;
};

/**
 * Class for restoring an interface MTU.
 */
class RestoreInterfaceMtu : public InterfaceModifier {
public:
    RestoreInterfaceMtu(IfConfig&	ifconfig,
			const string&	ifname)
	: InterfaceModifier(ifconfig, ifname) {}

    bool dispatch() {
	// Get the original MTU
	const IfTree& orig_iftree = ifconfig().original_config();
	const IfTreeInterface* orig_fi = orig_iftree.find_interface(ifname());
	if (orig_fi == NULL)
	    return (false);
	uint32_t orig_mtu = orig_fi->mtu();

	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_mtu(orig_mtu);
	return (true);
    }

    string str() const {
	return c_format("RestoreInterfaceMtu: %s", ifname().c_str());
    }

private:
};

/**
 * Class for setting an interface MAC.
 */
class SetInterfaceMac : public InterfaceModifier {
public:
    SetInterfaceMac(IfConfig&		ifconfig,
		    const string&	ifname,
		    const Mac&		mac)
	: InterfaceModifier(ifconfig, ifname), _mac(mac) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_mac(_mac);
	return (true);
    }

    string str() const {
	return c_format("SetInterfaceMac: %s %s",
			ifname().c_str(), _mac.str().c_str());
    }
private:
    Mac _mac;
};

/**
 * Class for restoring an interface MAC.
 */
class RestoreInterfaceMac : public InterfaceModifier {
public:
    RestoreInterfaceMac(IfConfig&	ifconfig,
		    const string&	ifname)
	: InterfaceModifier(ifconfig, ifname) {}

    bool dispatch() {
	// Get the original MAC
	const IfTree& orig_iftree = ifconfig().original_config();
	const IfTreeInterface* orig_fi = orig_iftree.find_interface(ifname());
	if (orig_fi == NULL)
	    return (false);
	const Mac& orig_mac = orig_fi->mac();

	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->set_mac(orig_mac);
	return (true);
    }

    string str() const {
	return c_format("RestoreInterfaceMac: %s", ifname().c_str());
    }

private:
};

/**
 * Class for adding a VIF to an interface.
 */
class AddInterfaceVif : public InterfaceModifier {
public:
    AddInterfaceVif(IfConfig&		ifconfig,
		    const string&	ifname,
		    const string&	vifname)
	: InterfaceModifier(ifconfig, ifname), _vifname(vifname) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	fi->add_vif(_vifname);
	return (true);
    }

    string str() const {
	return c_format("AddInterfaceVif: %s %s",
			ifname().c_str(), _vifname.c_str());
    }

private:
    const string _vifname;
};

/**
 * Class for removing a VIF from an interface.
 */
class RemoveInterfaceVif : public InterfaceModifier {
public:
    RemoveInterfaceVif(IfConfig&	ifconfig,
		    const string&	ifname,
		    const string&	vifname)
	: InterfaceModifier(ifconfig, ifname), _vifname(vifname) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == NULL)
	    return (false);
	if (fi->remove_vif(_vifname) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("RemoveInterfaceVif: %s %s",
			ifname().c_str(), _vifname.c_str());
    }

private:
    const string _vifname;
};

/**
 * Base class for vif modifier operations.
 */
class VifModifier : public InterfaceModifier {
public:
    VifModifier(IfConfig&	ifconfig,
		const string&	ifname,
		const string&	vifname)
	: InterfaceModifier(ifconfig, ifname), _vifname(vifname) {}

    string path() const {
	return InterfaceModifier::path() + string(" ") + vifname();
    }

    const string& vifname() const 		{ return _vifname; }

protected:
    IfTreeVif* vif() {
	IfTreeVif* vifp = iftree().find_vif(ifname(), _vifname);
	return (vifp);
    }

protected:
    const string _vifname;
};

/**
 * Class for setting the enabled state of a vif.
 */
class SetVifEnabled : public VifModifier {
public:
    SetVifEnabled(IfConfig& 	ifconfig,
		  const string&	ifname,
		  const string&	vifname,
		  bool		enabled)
	: VifModifier(ifconfig, ifname, vifname), _enabled(enabled) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	fv->set_enabled(_enabled);
	return (true);
    }

    string str() const {
	return c_format("SetVifEnabled: %s %s",
			path().c_str(), bool_c_str(_enabled));
    }

private:
    bool _enabled;
};

/**
 * Class for setting the VLAN state of a vif.
 */
class SetVifVlan : public VifModifier {
public:
    SetVifVlan(IfConfig& 	ifconfig,
	       const string&	ifname,
	       const string&	vifname,
	       uint32_t		vlan_id)
	: VifModifier(ifconfig, ifname, vifname), _vlan_id(vlan_id) {}

    // Maximum VLAN ID
    static const uint32_t MAX_VLAN_ID = 4095;

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	if (_vlan_id > MAX_VLAN_ID)
	    return (false);
	fv->set_vlan(true);
	fv->set_vlan_id(_vlan_id);
	return (true);
    }

    string str() const {
	return c_format("SetVifVlan: %s %u",
			path().c_str(), _vlan_id);
    }

private:
    uint32_t _vlan_id;
};

/**
 * Class for adding an IPv4 address to a VIF.
 */
class AddAddr4 : public VifModifier {
public:
    AddAddr4(IfConfig&		ifconfig,
	     const string&	ifname,
	     const string&	vifname,
	     const IPv4&	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	fv->add_addr(_addr);
	return (true);
    }

    string str() const {
	return c_format("AddAddr4: %s %s",
			path().c_str(), _addr.str().c_str());
    }

private:
    IPv4 _addr;
};

/**
 * Class for removing an IPv4 address to a VIF.
 */
class RemoveAddr4 : public VifModifier {
public:
    RemoveAddr4(IfConfig&	ifconfig,
		const string&	ifname,
		const string&	vifname,
		const IPv4&	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	if (fv->remove_addr(_addr) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("RemoveAddr4: %s %s",
			path().c_str(), _addr.str().c_str());
    }

private:
    IPv4 _addr;
};

/**
 * Class for adding an IPv6 address to a VIF.
 */
class AddAddr6 : public VifModifier {
public:
    AddAddr6(IfConfig&		ifconfig,
	     const string&	ifname,
	     const string&	vifname,
	     const IPv6&	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	if (fv->add_addr(_addr) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("AddAddr6: %s %s",
			path().c_str(), _addr.str().c_str());
    }

private:
    IPv6 _addr;
};

/**
 * Class for removing an IPv6 address to a VIF.
 */
class RemoveAddr6 : public VifModifier {
public:
    RemoveAddr6(IfConfig&	ifconfig,
		const string&	ifname,
		const string&	vifname,
		const IPv6& 	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == NULL)
	    return (false);
	if (fv->remove_addr(_addr) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	return c_format("RemoveAddr6: %s %s",
			path().c_str(), _addr.str().c_str());
    }

private:
    IPv6 _addr;
};

/**
 * Base class for IPv4vif address modifier operations.
 */
class Addr4Modifier : public VifModifier {
public:
    Addr4Modifier(IfConfig& 	ifconfig,
		  const string&	ifname,
		  const string&	vifname,
		  const IPv4&	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    string path() const {
	return VifModifier::path() + string(" ") + _addr.str();
    }

protected:
    IfTreeAddr4* addr() {
	IfTreeAddr4* ap = iftree().find_addr(ifname(), vifname(), _addr);
	return (ap);
    }

protected:
    const IPv4 _addr;
};

/**
 * Class to set enable state of an IPv4 address associated with a vif.
 */
class SetAddr4Enabled : public Addr4Modifier {
public:
    SetAddr4Enabled(IfConfig&		ifconfig,
		    const string&	ifname,
		    const string&	vifname,
		    const IPv4&		addr,
		    bool		enabled)
	: Addr4Modifier(ifconfig, ifname, vifname, addr), _enabled(enabled) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == NULL)
	    return (false);
	fa->set_enabled(_enabled);
	return (true);
    }

    string str() const {
	return c_format("SetAddr4Enabled: %s %s",
			path().c_str(), bool_c_str(_enabled));
    }

protected:
    bool _enabled;
};

/**
 * Class to set the prefix of an IPv4 address associated with a vif.
 */
class SetAddr4Prefix : public Addr4Modifier {
public:
    SetAddr4Prefix(IfConfig&		ifconfig,
		   const string&	ifname,
		   const string&	vifname,
		   const IPv4&		addr,
		   uint32_t		prefix_len)
	: Addr4Modifier(ifconfig, ifname, vifname, addr),
	  _prefix_len(prefix_len) {}

    static const uint32_t MAX_PREFIX_LEN = 32;
    
    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == NULL || _prefix_len > MAX_PREFIX_LEN)
	    return (false);
	if (fa->set_prefix_len(_prefix_len) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	string s = c_format("SetAddr4Prefix: %s %u", path().c_str(),
			    XORP_UINT_CAST(_prefix_len));
	if (_prefix_len > MAX_PREFIX_LEN)
	    s += c_format(" (valid range 0--%u)",
			  XORP_UINT_CAST(MAX_PREFIX_LEN));
	return s;
    }

protected:
    uint32_t _prefix_len;
};

/**
 * Class to set the endpoint IPv4 address associated with a vif.
 */
class SetAddr4Endpoint : public Addr4Modifier {
public:
    SetAddr4Endpoint(IfConfig&	 	ifconfig,
		     const string&	ifname,
		     const string&	vifname,
		     const IPv4&	addr,
		     const IPv4&	endpoint)
	: Addr4Modifier(ifconfig, ifname, vifname, addr), _endpoint(endpoint) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == NULL)
	    return (false);
	fa->set_endpoint(_endpoint);
	fa->set_point_to_point(true);
	return (true);
    }

    string str() const {
	return c_format("SetAddr4Endpoint: %s %s",
			path().c_str(), _endpoint.str().c_str());
    }

protected:
    IPv4 _endpoint;
};

/**
 * Class to set the broadcast address IPv4 address associated with a vif.
 */
class SetAddr4Broadcast : public Addr4Modifier {
public:
    SetAddr4Broadcast(IfConfig&		ifconfig,
		      const string&	ifname,
		      const string&	vifname,
		      const IPv4&	addr,
		      const IPv4&	bcast)
	: Addr4Modifier(ifconfig, ifname, vifname, addr), _bcast(bcast) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == NULL)
	    return (false);
	fa->set_bcast(_bcast);
	fa->set_broadcast(true);
	return (true);
    }

    string str() const {
	return c_format("SetAddr4Broadcast: %s %s",
			path().c_str(), _bcast.str().c_str());
    }

protected:
    IPv4 _bcast;
};

/**
 * Base class for IPv6vif address modifier operations.
 */
class Addr6Modifier : public VifModifier {
public:
    Addr6Modifier(IfConfig&	ifconfig,
		  const string&	ifname,
		  const string&	vifname,
		  const IPv6&	addr)
	: VifModifier(ifconfig, ifname, vifname), _addr(addr) {}

    string path() const {
	return VifModifier::path() + string(" ") + _addr.str();
    }

protected:
    IfTreeAddr6* addr() {
	IfTreeAddr6* ap = iftree().find_addr(ifname(), vifname(), _addr);
	return (ap);
    }

protected:
    const IPv6 _addr;
};

/**
 * Class to set the enabled state of an IPv6 address associated with a vif.
 */
class SetAddr6Enabled : public Addr6Modifier {
public:
    SetAddr6Enabled(IfConfig& 		ifconfig,
		    const string&	ifname,
		    const string&	vifname,
		    const IPv6&		addr,
		    bool		enabled)
	: Addr6Modifier(ifconfig, ifname, vifname, addr), _enabled(enabled) {}

    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == NULL)
	    return (false);
	fa->set_enabled(_enabled);
	return (true);
    }

    string str() const {
	return c_format("SetAddr6Enabled: %s %s",
			path().c_str(), bool_c_str(_enabled));
    }

protected:
    bool _enabled;
};

/**
 * Class to set the prefix of an IPv6 address associated with a vif.
 */
class SetAddr6Prefix : public Addr6Modifier {
public:
    SetAddr6Prefix(IfConfig&		ifconfig,
		   const string&	ifname,
		   const string&	vifname,
		   const IPv6&		addr,
		   uint32_t		prefix_len)
	: Addr6Modifier(ifconfig, ifname, vifname, addr),
	  _prefix_len(prefix_len) {}

    static const uint32_t MAX_PREFIX_LEN = 128;
    
    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == NULL || _prefix_len > MAX_PREFIX_LEN)
	    return (false);
	if (fa->set_prefix_len(_prefix_len) != XORP_OK)
	    return (false);
	return (true);
    }

    string str() const {
	string s = c_format("SetAddr6Prefix: %s %u", path().c_str(),
			    XORP_UINT_CAST(_prefix_len));
	if (_prefix_len > MAX_PREFIX_LEN)
	    s += c_format(" (valid range 0--%u)",
			  XORP_UINT_CAST(MAX_PREFIX_LEN));
	return s;
    }

protected:
    uint32_t _prefix_len;
};

/**
 * Class to set the endpoint IPv6 address associated with a vif.
 */
class SetAddr6Endpoint : public Addr6Modifier {
public:
    SetAddr6Endpoint(IfConfig& 		ifconfig,
		     const string&	ifname,
		     const string&	vifname,
		     const IPv6&	addr,
		     const IPv6&	endpoint)
	: Addr6Modifier(ifconfig, ifname, vifname, addr), _endpoint(endpoint) {}

    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == NULL)
	    return (false);
	fa->set_endpoint(_endpoint);
	fa->set_point_to_point(true);
	return (true);
    }

    string str() const {
	return c_format("SetAddr6Endpoint: %s %s",
			path().c_str(), _endpoint.str().c_str());
    }

protected:
    IPv6 _endpoint;
};

#endif // __FEA_IFCONFIG_TRANSACTION_HH__
