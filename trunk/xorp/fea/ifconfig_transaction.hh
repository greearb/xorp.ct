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

// $XORP: xorp/fea/ifconfig_transaction.hh,v 1.7 2007/05/23 12:12:34 pavlin Exp $

#ifndef __FEA_IFCONFIG_TRANSACTION_HH__
#define __FEA_IFCONFIG_TRANSACTION_HH__

#include "libxorp/c_format.hh"
#include "libxorp/transaction.hh"

#include "ifconfig.hh"
#include "iftree.hh"


class IfConfigTransactionManager : public TransactionManager {
public:
    IfConfigTransactionManager(EventLoop& eventloop)
	: TransactionManager(eventloop, TIMEOUT_MS, MAX_PENDING)
    {}

    void reset_error()			{ _first_error.erase(); }

    const string& error() const 	{ return _first_error; }

    virtual void pre_commit(uint32_t tid);

    virtual void operation_result(bool success,
				  const TransactionOperation& op);

protected:
    string   _first_error;
    uint32_t _tid_exec;

private:
    enum { TIMEOUT_MS = 5000, MAX_PENDING = 10 };
};

/**
 * Base class for Interface related operations acting on an
 * IfTree.
 */
class IfConfigTransactionOperation : public TransactionOperation {
public:
    IfConfigTransactionOperation(IfTree& it, const string& ifname)
	: _it(it), _ifname(ifname) {}

    /**
     * The interface / vif / address stored in operation exist.
     */
    virtual bool path_valid() const = 0;

    /**
     * @return space separated path description.
     */
    virtual string path() const 		{ return _ifname; }

    const string& ifname() const		{ return _ifname; }

    const IfTree& iftree() const		{ return _it; }

protected:
    inline IfTree& iftree() 			{ return _it; }

private:
    IfTree& _it;
    const string    	   _ifname;
};

/**
 * Class for adding an interface.
 */
class AddInterface : public IfConfigTransactionOperation {
public:
    AddInterface(IfTree& it, const string& ifname)
	: IfConfigTransactionOperation(it, ifname) {}

    bool dispatch() 	{ iftree().add_interface(ifname()); return true; }

    string str() const 		{ return string("AddInterface: ") + ifname(); }

    bool path_valid() const	{ return true; }
};

/**
 * Class for removing an interface.
 */
class RemoveInterface : public IfConfigTransactionOperation {
public:
    RemoveInterface(IfTree& it, const string& ifname)
	: IfConfigTransactionOperation(it, ifname) {}

    bool dispatch() 		{ return iftree().remove_interface(ifname()); }

    string str() const {
	return string("RemoveInterface: ") + ifname();
    }

    bool path_valid() const	{ return true; }
};

/**
 * Class for configuring an interface within the FEA by using information
 * from the underlying system.
 */
class ConfigureInterfaceFromSystem : public IfConfigTransactionOperation {
public:
    ConfigureInterfaceFromSystem(IfConfig& ifconfig, IfTree& it,
				 const string& ifname)
	: IfConfigTransactionOperation(it, ifname), _ifconfig(ifconfig) {}

    bool dispatch() {
	const IfTree& dev_config = _ifconfig.pulled_config();
	const IfTreeInterface* ifp = dev_config.find_interface(ifname());
	if (ifp == NULL)
	    return false;
	
	return iftree().update_interface(*ifp);
    }

    string str() const {
	return string("ConfigureInterfaceFromSystem: ") + ifname();
    }

    bool path_valid() const	{ return true; }

private:
    IfConfig& _ifconfig;
};

/**
 * Base class for interface modifier operations.
 */
class InterfaceModifier : public IfConfigTransactionOperation {
public:
    InterfaceModifier(IfTree& it, const string& ifname)
	: IfConfigTransactionOperation(it, ifname) {}

    bool path_valid() const	{ return interface() != 0; }

protected:
    IfTreeInterface* interface() {
	IfTreeInterface* ifp = iftree().find_interface(ifname());
	return (ifp);
    }
    const IfTreeInterface* interface() const {
	const IfTreeInterface* ifp = iftree().find_interface(ifname());
	return (ifp);
    }
};

/**
 * Class for setting the enabled state of an interface.
 */
class SetInterfaceEnabled : public InterfaceModifier {
public:
    SetInterfaceEnabled(IfTree&		it,
			const string&	ifname,
			bool		en)
	: InterfaceModifier(it, ifname), _en(en) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;
	fi->set_enabled(_en);
	return true;
    }

    string str() const {
	return c_format("SetInterfaceEnabled: %s %s",
			ifname().c_str(), bool_c_str(_en));
    }

private:
    bool _en;
};

/**
 * Class for setting the discard state of an interface.
 */
class SetInterfaceDiscard : public InterfaceModifier {
public:
    SetInterfaceDiscard(IfTree&		it,
			const string&	ifname,
			bool		discard)
	: InterfaceModifier(it, ifname), _discard(discard) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;
	fi->set_discard(_discard);
	return true;
    }

    string str() const {
	return c_format("SetInterfaceDiscard: %s %s",
			ifname().c_str(), bool_c_str(_discard));
    }

private:
    bool _discard;
};

/**
 * Class for setting an interface mtu.
 */
class SetInterfaceMTU : public InterfaceModifier {
public:
    SetInterfaceMTU(IfTree&		it,
		    const string&	ifname,
		    uint32_t		mtu)
	: InterfaceModifier(it, ifname), _mtu(mtu) {}

    // Minimum and maximum MTU (as defined in RFC 791 and RFC 1191)
    static const uint32_t MIN_MTU = 68;
    static const uint32_t MAX_MTU = 65536;
    
    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;

	if (_mtu < MIN_MTU || _mtu > MAX_MTU)
	    return false;
	
	fi->set_mtu(_mtu);
	return true;
    }

    string str() const {
	string s = c_format("SetInterfaceMTU: %s %u", ifname().c_str(),
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
 * Class for setting an interface mac.
 */
class SetInterfaceMAC : public InterfaceModifier {
public:
    SetInterfaceMAC(IfTree&		it,
		    const string&	ifname,
		    const Mac&		mac)
	: InterfaceModifier(it, ifname), _mac(mac) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;
	fi->set_mac(_mac);
	return true;
    }

    string str() const {
	return c_format("SetInterfaceMAC: %s %s",
			ifname().c_str(), _mac.str().c_str());
    }
private:
    Mac _mac;
};

/**
 * Class for adding a VIF to an interface.
 */
class AddInterfaceVif : public InterfaceModifier {
public:
    AddInterfaceVif(IfTree&		it,
		    const string&	ifname,
		    const string&	vifname)
	: InterfaceModifier(it, ifname), _vifname(vifname) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;
	fi->add_vif(_vifname);
	return true;
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
    RemoveInterfaceVif(IfTree&		it,
		    const string&	ifname,
		    const string&	vifname)
	: InterfaceModifier(it, ifname), _vifname(vifname) {}

    bool dispatch() {
	IfTreeInterface* fi = interface();
	if (fi == 0) return false;
	return fi->remove_vif(_vifname);
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
    VifModifier(IfTree&		it,
		const string&	ifname,
		const string&	vifname)
	: InterfaceModifier(it, ifname), _vifname(vifname) {}

    bool path_valid() const			{ return vif() != 0; }

    string path() const {
	return InterfaceModifier::path() + string(" ") + vifname();
    }

    const string& vifname() const 		{ return _vifname; }

protected:
    IfTreeVif* vif() {
	IfTreeVif* vifp = iftree().find_vif(ifname(), _vifname);
	return (vifp);
    }
    const IfTreeVif* vif() const {
	const IfTreeVif* vifp = iftree().find_vif(ifname(), _vifname);
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
    SetVifEnabled(IfTree& 	it,
		  const string&	ifname,
		  const string&	vifname,
		  bool		en)
	: VifModifier(it, ifname, vifname), _en(en) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == 0) return false;
	fv->set_enabled(_en);
	return true;
    }

    string str() const {
	return c_format("SetVifEnabled: %s %s",
			path().c_str(), bool_c_str(_en));
    }

private:
    bool _en;
};

/**
 * Class for adding an IPv4 address to a VIF.
 */
class AddAddr4 : public VifModifier {
public:
    AddAddr4(IfTree&		it,
	     const string&	ifname,
	     const string&	vifname,
	     const IPv4&	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == 0) return false;
	fv->add_addr(_addr);
	return true;
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
    RemoveAddr4(IfTree&		it,
		const string&	ifname,
		const string&	vifname,
		const IPv4&	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == 0) return false;
	return fv->remove_addr(_addr);
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
    AddAddr6(IfTree&		it,
	     const string&	ifname,
	     const string&	vifname,
	     const IPv6&	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == 0) return false;
	fv->add_addr(_addr);
	return true;
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
    RemoveAddr6(IfTree&		it,
		const string&	ifname,
		const string&	vifname,
		const IPv6& 	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool dispatch() {
	IfTreeVif* fv = vif();
	if (fv == 0) return false;
	return fv->remove_addr(_addr);
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
    Addr4Modifier(IfTree& 	it,
		  const string&	ifname,
		  const string&	vifname,
		  const IPv4&	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool path_valid() const	{ return addr() != 0; }

    string path() const {
	return VifModifier::path() + string(" ") + _addr.str();
    }

protected:
    IfTreeAddr4* addr() {
	IfTreeAddr4* ap = iftree().find_addr(ifname(), vifname(), _addr);
	return (ap);
    }
    const IfTreeAddr4* addr() const {
	const IfTreeAddr4* ap = iftree().find_addr(ifname(), vifname(), _addr);
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
    SetAddr4Enabled(IfTree&		it,
		    const string&	ifname,
		    const string&	vifname,
		    const IPv4&		addr,
		    bool		en)
	: Addr4Modifier(it, ifname, vifname, addr), _en(en) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == 0) return false;
	fa->set_enabled(_en);
	return true;
    }

    string str() const {
	return c_format("SetAddr4Enabled: %s %s",
			path().c_str(), bool_c_str(_en));
    }

protected:
    bool _en;
};

/**
 * Class to set the prefix of an IPv4 address associated with a vif.
 */
class SetAddr4Prefix : public Addr4Modifier {
public:
    SetAddr4Prefix(IfTree&		it,
		   const string&	ifname,
		   const string&	vifname,
		   const IPv4&		addr,
		   uint32_t		prefix_len)
	: Addr4Modifier(it, ifname, vifname, addr), _prefix_len(prefix_len) {}

    static const uint32_t MAX_PREFIX_LEN = 32;
    
    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == 0 || _prefix_len > MAX_PREFIX_LEN)
	    return false;
	return fa->set_prefix_len(_prefix_len);
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
    SetAddr4Endpoint(IfTree&	 	it,
		     const string&	ifname,
		     const string&	vifname,
		     const IPv4&	addr,
		     const IPv4&	endpoint)
	: Addr4Modifier(it, ifname, vifname, addr), _endpoint(endpoint) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == 0) return false;
	fa->set_endpoint(_endpoint);
	fa->set_point_to_point(true);
	return true;
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
    SetAddr4Broadcast(IfTree&		it,
		      const string&	ifname,
		      const string&	vifname,
		      const IPv4&	addr,
		      const IPv4&	bcast)
	: Addr4Modifier(it, ifname, vifname, addr), _bcast(bcast) {}

    bool dispatch() {
	IfTreeAddr4* fa = addr();
	if (fa == 0) return false;
	fa->set_bcast(_bcast);
	fa->set_broadcast(true);
	return true;
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
    Addr6Modifier(IfTree&	it,
		  const string&	ifname,
		  const string&	vifname,
		  const IPv6&	addr)
	: VifModifier(it, ifname, vifname), _addr(addr) {}

    bool path_valid() const	{ return addr() != 0; }

    string path() const {
	return VifModifier::path() + string(" ") + _addr.str();
    }

protected:
    IfTreeAddr6* addr() {
	IfTreeAddr6* ap = iftree().find_addr(ifname(), vifname(), _addr);
	return (ap);
    }

    const IfTreeAddr6* addr() const {
	const IfTreeAddr6* ap = iftree().find_addr(ifname(), vifname(), _addr);
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
    SetAddr6Enabled(IfTree& 		it,
		    const string&	ifname,
		    const string&	vifname,
		    const IPv6&		addr,
		    bool		en)
	: Addr6Modifier(it, ifname, vifname, addr), _en(en) {}

    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == 0) return false;
	fa->set_enabled(_en);
	return true;
    }

    string str() const {
	return c_format("SetAddr6Enabled: %s %s",
			path().c_str(), bool_c_str(_en));
    }

protected:
    bool _en;
};

/**
 * Class to set the prefix of an IPv6 address associated with a vif.
 */
class SetAddr6Prefix : public Addr6Modifier {
public:
    SetAddr6Prefix(IfTree&		it,
		   const string&	ifname,
		   const string&	vifname,
		   const IPv6&		addr,
		   uint32_t		prefix_len)
	: Addr6Modifier(it, ifname, vifname, addr), _prefix_len(prefix_len) {}

    static const uint32_t MAX_PREFIX_LEN = 128;
    
    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == 0 || _prefix_len > MAX_PREFIX_LEN)
	    return false;
	return fa->set_prefix_len(_prefix_len);
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
    SetAddr6Endpoint(IfTree& 		it,
		     const string&	ifname,
		     const string&	vifname,
		     const IPv6&	addr,
		     const IPv6&	endpoint)
	: Addr6Modifier(it, ifname, vifname, addr), _endpoint(endpoint) {}

    bool dispatch() {
	IfTreeAddr6* fa = addr();
	if (fa == 0) return false;
	fa->set_endpoint(_endpoint);
	fa->set_point_to_point(true);
	return true;
    }

    string str() const {
	return c_format("SetAddr6Endpoint: %s %s",
			path().c_str(), _endpoint.str().c_str());
    }

protected:
    IPv6 _endpoint;
};

#endif // __FEA_IFCONFIG_TRANSACTION_HH__
