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

#ident "$XORP: xorp/fea/libfeaclient_bridge.cc,v 1.2 2003/10/17 21:03:26 hodson Exp $"

/*
#define DEBUG_LOGGING
*/

#include "fea_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "libfeaclient/ifmgr_atoms.hh"
#include "libfeaclient/ifmgr_cmds.hh"
#include "libfeaclient/ifmgr_xrl_replicator.hh"

#include "libfeaclient_bridge.hh"

// ----------------------------------------------------------------------------
// Debug helpers

static const char*
update_name(IfConfigUpdateReporterBase::Update u)
{
    switch (u) {
    case IfConfigUpdateReporterBase::CREATED:
	return "Creation";
    case IfConfigUpdateReporterBase::DELETED:
	return "Deletion";
    case IfConfigUpdateReporterBase::CHANGED:
	;					// FALLTHROUGH
    }
    return "Change";
}

static const char*
truth_of(bool v)
{
    return v ? "true" : "false";
}


// ----------------------------------------------------------------------------
// LibFeaClientBridge implementation

LibFeaClientBridge::LibFeaClientBridge(XrlRouter& rtr)
    : _iftree(0)
{
    _rm = new IfMgrXrlReplicationManager(rtr);
}

LibFeaClientBridge::~LibFeaClientBridge()
{
    delete _rm;
}

void
LibFeaClientBridge::set_iftree(const IfTree* tree)
{
    _iftree = tree;
}

bool
LibFeaClientBridge::add_libfeaclient_mirror(const string& m)
{
    return _rm->add_mirror(m);
}

bool
LibFeaClientBridge::remove_libfeaclient_mirror(const string& m)
{
    return _rm->remove_mirror(m);
}

const IfMgrIfTree&
LibFeaClientBridge::libfeaclient_iftree() const
{
    return _rm->iftree();
}

const IfTree*
LibFeaClientBridge::fea_iftree() const
{
    return _iftree;
}

void
LibFeaClientBridge::interface_update(const string& ifname,
				     const Update& update,
				     bool	   all)
{
    debug_msg("%s update for interface %s (%s)\n",
	      update_name(update), ifname.c_str(), truth_of(all));

    XLOG_ASSERT(_iftree != 0);

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIfAdd(ifname));
	return;

    case DELETED:
	_rm->push(new IfMgrIfRemove(ifname));
	return;

    case CHANGED:
	;					// FALLTHROUGH
    }

    //
    // Validate interface is in the FEA iftree we're using and
    // in libfeaclient's equivalent.
    //
    const IfMgrIfAtom* ifa = _rm->iftree().find_if(ifname);
    if (ifa == 0) {
	XLOG_WARNING("Got update for interface not in libfeaclient tree: "
		     "%s\n", ifname.c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree->get_if(ifname);
    if (ii == _iftree->ifs().end()) {
	XLOG_WARNING("Got update for interface not in FEA tree: %s\n",
		     ifname.c_str());
	return;
    }

    //
    // Copy interface state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new IfMgrIfSetEnabled(ifname, ii->second.enabled()));
    _rm->push(new IfMgrIfSetMtu(ifname, ii->second.mtu()));
    _rm->push(new IfMgrIfSetMac(ifname, ii->second.mac()));
    _rm->push(new IfMgrIfSetPifIndex(ifname, ii->second.pif_index()));

    //
    // XXX TODO / TBD if need doing...
    // _rm->push(new IfMgrIfSetIfFlags(ifname, ii- if_flags - relevant ????
    //
}


void
LibFeaClientBridge::vif_update(const string& ifname,
			       const string& vifname,
			       const Update& update,
			       bool	     all)
{
    debug_msg("%s update for vif %s/%s (%s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      truth_of(all));

    XLOG_ASSERT(_iftree != 0);

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrVifAdd(ifname, vifname));
	return;

    case DELETED:
	_rm->push(new IfMgrVifRemove(ifname, vifname));
	return;

    case CHANGED:
	;					// FALLTHROUGH
    }

    //
    // Validate vif is in the FEA iftree we're using and in
    // libfeaclient's equivalent.
    //
    const IfMgrVifAtom* ifa = _rm->iftree().find_vif(ifname, vifname);
    if (ifa == 0) {
	XLOG_WARNING("Got update for vif not in libfeaclient tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree->get_if(ifname);
    if (ii == _iftree->ifs().end()) {
	XLOG_WARNING("Got update for vif on interface not in tree:"
		     "%s/(%s)", ifname.c_str(), vifname.c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for vif not in FEA tree: %s/%s",
		     ifname.c_str(), vifname.c_str());
	return;
    }

    //
    // Copy vif state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    const IfTreeVif& vif = vi->second;
    _rm->push(new
	      IfMgrVifSetEnabled(ifname, vifname, vif.enabled())
	      );
    _rm->push(new
	      IfMgrVifSetBroadcastCapable(ifname, vifname, vif.broadcast())
	      );
    _rm->push(new
	      IfMgrVifSetLoopbackCapable(ifname, vifname, vif.loopback())
	      );
    _rm->push(new
	      IfMgrVifSetP2PCapable(ifname, vifname, vif.point_to_point())
	      );
    _rm->push(new
	      IfMgrVifSetMulticastCapable(ifname, vifname, vif.multicast())
	      );
    _rm->push(new
	      IfMgrVifSetPifIndex(ifname, vifname, vif.pif_index())
	      );
}


void
LibFeaClientBridge::vifaddr4_update(const string& ifname,
				    const string& vifname,
				    const IPv4&   addr,
				    const Update& update,
				    bool	  all)
{
    debug_msg("%s update for address %s/%s/%s (%s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str(), truth_of(all));

    XLOG_ASSERT(_iftree != 0);

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIPv4Add(ifname, vifname, addr));
	return;

    case DELETED:
	_rm->push(new IfMgrIPv4Remove(ifname, vifname, addr));
	return;

    case CHANGED:
	;					// FALLTHROUGH
    }

    //
    // Validate vif address is in the FEA iftree we're using and in
    // libfeaclient's equivalent
    //
    const IfMgrIPv4Atom* ifa = _rm->iftree().find_addr(ifname,
						       vifname,
						       addr);
    if (ifa == 0) {
	XLOG_WARNING("Got update for address no in libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree->get_if(ifname);
    if (ii == _iftree->ifs().end()) {
	XLOG_WARNING("Got update for address on interface not in tree: "
		     "%s/(%s/%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for address on vif not in FEA tree: "
		     "%s/%s/(%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif& vif = vi->second;
    IfTreeVif::V4Map::const_iterator ai = vif.get_addr(addr);
    if (ai == vif.v4addrs().end()) {
	XLOG_WARNING("Got update for address not in FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr4& a4 = ai->second;

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv4SetEnabled(ifname, vifname, addr, a4.enabled())
	      );
    _rm->push(new
	      IfMgrIPv4SetLoopback(ifname, vifname, addr, a4.loopback())
	      );
    _rm->push(new
	      IfMgrIPv4SetMulticastCapable(ifname, vifname, addr,
					   a4.multicast())
	      );
    _rm->push(new
	      IfMgrIPv4SetPrefix(ifname, vifname, addr, a4.prefix_len())
	      );
    if (a4.point_to_point()) {
	const IPv4& end = a4.endpoint();
	_rm->push(new IfMgrIPv4SetEndpoint(ifname, vifname, addr, end));
    } else {
	// NB if IfTreeAddr4::bcast() will return IPv4::ZERO() if
	// broadcast is not supported.  This happens to be the
	// correct argument for libfeaclient to signify broadcast
	// is not supported.
	const IPv4& bcast = a4.bcast();
	_rm->push(new IfMgrIPv4SetBroadcast(ifname, vifname, addr, bcast));
    }
}


void
LibFeaClientBridge::vifaddr6_update(const string& ifname,
				    const string& vifname,
				    const IPv6&	  addr,
				    const Update& update,
				    bool	  all)
{
    debug_msg("%s update for address %s/%s/%s (%s)\n",
	      update_name(update), ifname.c_str(), vifname.c_str(),
	      addr.str().c_str(), truth_of(all));

    XLOG_ASSERT(_iftree != 0);

    switch (update) {
    case CREATED:
	_rm->push(new IfMgrIPv6Add(ifname, vifname, addr));
	return;

    case DELETED:
	_rm->push(new IfMgrIPv6Remove(ifname, vifname, addr));
	return;

    case CHANGED:
	; 					// FALLTHROUGH
    }

    //
    // Validate vif address is in the FEA iftree we're using and in
    // libfeaclient's equivalent
    //
    const IfMgrIPv6Atom* ifa = _rm->iftree().find_addr(ifname,
						       vifname,
						       addr);
    if (ifa == 0) {
	XLOG_WARNING("Got update for address no in libfeaclient tree: "
		     "%s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    IfTree::IfMap::const_iterator ii = _iftree->get_if(ifname);
    if (ii == _iftree->ifs().end()) {
	XLOG_WARNING("Got update for address on interface not in tree: "
		     "%s/(%s/%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeInterface& iface = ii->second;
    IfTreeInterface::VifMap::const_iterator vi = iface.get_vif(vifname);
    if (vi == iface.vifs().end()) {
	XLOG_WARNING("Got update for address on vif not in FEA tree: "
		     "%s/%s/(%s)",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeVif& vif = vi->second;
    IfTreeVif::V6Map::const_iterator ai = vif.get_addr(addr);
    if (ai == vif.v6addrs().end()) {
	XLOG_WARNING("Got update for address not in FEA tree: %s/%s/%s",
		     ifname.c_str(), vifname.c_str(), addr.str().c_str());
	return;
    }

    const IfTreeAddr6& a6 = ai->second;

    //
    // Copy address state out of FEA iftree into libfeaclient's
    // equivalent.
    //
    _rm->push(new
	      IfMgrIPv6SetEnabled(ifname, vifname, addr, a6.enabled())
	      );
    _rm->push(new
	      IfMgrIPv6SetLoopback(ifname, vifname, addr, a6.loopback())
	      );
    _rm->push(new
	      IfMgrIPv6SetMulticastCapable(ifname, vifname, addr,
					   a6.multicast())
	      );
    _rm->push(new
	      IfMgrIPv6SetPrefix(ifname, vifname, addr, a6.prefix_len())
	      );
    _rm->push(new
	      IfMgrIPv6SetEndpoint(ifname, vifname, addr, a6.endpoint()));
}


// ----------------------------------------------------------------------------
// Sanity check / verification code (expensive)

#include <algorithm>

class AppendFeaIfNames {
public:
    AppendFeaIfNames(string& out) : _out(out)
    {}

    void
    operator() (const IfTree::IfMap::value_type& v)
    {
	_out += v.first + " ";
    }

private:
    string& _out;
};

class AppendFeaVifNames {
public:
    AppendFeaVifNames(string& out) : _out(out)
    {}

    void
    operator() (const IfTreeInterface::VifMap::value_type& v)
    {
	_out += v.first + " ";
    }

private:
    string& _out;
};

class AppendFeaV4Addrs {
public:
    AppendFeaV4Addrs(string& out) : _out(out)
    {}

    void
    operator() (const IfTreeVif::V4Map::value_type& v)
    {
	_out += v.first.str() + " ";
    }

private:
    string& _out;
};

class AppendFeaV6Addrs {
public:
    AppendFeaV6Addrs(string& out) : _out(out)
    {}

    void
    operator() (const IfTreeVif::V6Map::value_type& v)
    {
	_out += v.first.str() + " ";
    }

private:
    string& _out;
};

class AppendLibFeaClientIfNames {
public:
    AppendLibFeaClientIfNames(string& out) : _out(out)
    {}

    void
    operator() (const IfMgrIfTree::IfMap::value_type& v)
    {
	_out += v.first + " ";
    }

private:
    string& _out;
};

class AppendLibFeaClientVifNames {
public:
    AppendLibFeaClientVifNames(string& out) : _out(out)
    {}

    void
    operator() (const IfMgrIfAtom::VifMap::value_type& v)
    {
	_out += v.first + " ";
    }

private:
    string& _out;
};

class AppendLibFeaClientV4Addrs {
public:
    AppendLibFeaClientV4Addrs(string& out) : _out(out)
    {}

    void
    operator() (const IfMgrVifAtom::V4Map::value_type& v)
    {
	_out += v.first.str() + " ";
    }

private:
    string& _out;
};

class AppendLibFeaClientV6Addrs {
public:
    AppendLibFeaClientV6Addrs(string& out) : _out(out)
    {}

    void
    operator() (const IfMgrVifAtom::V6Map::value_type& v)
    {
	_out += v.first.str() + " ";
    }

private:
    string& _out;
};


/**
 * Check IPv4 addresses.
 */
class CheckV4Addrs {
public:
    CheckV4Addrs(const IfMgrVifAtom::V4Map&	lfc_addrs,
		 string&			errlog)
	: _lfc_addrs(lfc_addrs), _errlog(errlog)
    {}

    void
    operator() (const IfTreeVif::V4Map::value_type& v)
    {
	const IfTreeAddr4& ita = v.second;

	IfMgrVifAtom::V4Map::const_iterator i = _lfc_addrs.find(ita.addr());
	if (i == _lfc_addrs.end()) {
	    return;
	}

	const IfMgrIPv4Atom& ima = i->second;
	if (ita.addr() != ima.addr()) {
	    _errlog += c_format("+   Addr4 %s mismatch %s != %s\n",
				ita.addr().str().c_str(),
				ita.addr().str().c_str(),
				ima.addr().str().c_str());
	}
	if (ita.prefix_len() != ima.prefix_len()) {
	    _errlog += c_format("+   Addr4 %s prefix len mismatch %u != %u\n",
				ita.addr().str().c_str(),
				ita.prefix_len(), ima.prefix_len());
	}
	if (ita.enabled() != ima.enabled()) {
	    _errlog += c_format("+   Addr4 %s enabled mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.enabled()),
				truth_of(ima.enabled()));
	}
	if (ita.broadcast() != ima.has_broadcast()) {
	    _errlog += c_format("+   Addr4 %s broadcast mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.broadcast()),
				truth_of(ima.has_broadcast()));
	}
	if (ita.broadcast() && ita.bcast() != ima.broadcast_addr()) {
	    _errlog += c_format("+   Addr4 %s bcast addr mismatch %s != %s\n",
				ita.addr().str().c_str(),
				ita.bcast().str().c_str(),
				ima.broadcast_addr().str().c_str());
	}
	if (ita.point_to_point() != ima.has_endpoint()) {
	    _errlog += c_format("+   Addr4 %s p2p mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.point_to_point()),
				truth_of(ima.has_endpoint()));
	}
	if (ita.point_to_point() &&
	    ita.endpoint() != ima.endpoint_addr()) {
	    _errlog += c_format("+   Addr4 %s endpoint mismatch %s != %s\n",
				ita.addr().str().c_str(),
				ita.endpoint().str().c_str(),
				ima.endpoint_addr().str().c_str());
	}
	if (ita.loopback() != ima.loopback()) {
	    _errlog += c_format("+   Addr4 %s loopback mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.loopback()),
				truth_of(ima.loopback()));
	}
	if (ita.multicast() != ima.multicast_capable()) {
	    _errlog += c_format("+   Addr4 %s multicast mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.multicast()),
				truth_of(ima.multicast_capable()));
	}
    }

private:
    const IfMgrVifAtom::V4Map&	_lfc_addrs;
    string&			_errlog;
};

/**
 * Check IPv6 addresses.
 */
class CheckV6Addrs {
public:
    CheckV6Addrs(const IfMgrVifAtom::V6Map&	lfc_addrs,
		 string&			errlog)
	: _lfc_addrs(lfc_addrs), _errlog(errlog)
    {}

    void
    operator() (const IfTreeVif::V6Map::value_type& v)
    {
	const IfTreeAddr6& ita = v.second;

	IfMgrVifAtom::V6Map::const_iterator i = _lfc_addrs.find(ita.addr());
	if (i == _lfc_addrs.end()) {
	    return;
	}

	const IfMgrIPv6Atom& ima = i->second;
	if (ita.addr() != ima.addr()) {
	    _errlog += c_format("+   Addr6 %s mismatch %s != %s\n",
				ita.addr().str().c_str(),
				ita.addr().str().c_str(),
				ima.addr().str().c_str());
	}
	if (ita.prefix_len() != ima.prefix_len()) {
	    _errlog += c_format("+   Addr6 %s prefix len mismatch %u != %u\n",
				ita.addr().str().c_str(),
				ita.prefix_len(), ima.prefix_len());
	}
	if (ita.enabled() != ima.enabled()) {
	    _errlog += c_format("+   Addr6 %s enabled mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.enabled()),
				truth_of(ima.enabled()));
	}
	if (ita.point_to_point() != ima.has_endpoint()) {
	    _errlog += c_format("+   Addr6 %s p2p mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.point_to_point()),
				truth_of(ima.has_endpoint()));
	}
	if (ita.point_to_point() &&
	    ita.endpoint() != ima.endpoint_addr()) {
	    _errlog += c_format("+   Addr6 %s endpoint mismatch %s != %s\n",
				ita.addr().str().c_str(),
				ita.endpoint().str().c_str(),
				ima.endpoint_addr().str().c_str());
	}
	if (ita.loopback() != ima.loopback()) {
	    _errlog += c_format("+   Addr6 %s loopback mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.loopback()),
				truth_of(ima.loopback()));
	}
	if (ita.multicast() != ima.multicast_capable()) {
	    _errlog += c_format("+   Addr6 %s multicast mismatch %s != %s\n",
				ita.addr().str().c_str(),
				truth_of(ita.multicast()),
				truth_of(ima.multicast_capable()));
	}
    }

private:
    const IfMgrVifAtom::V6Map&	_lfc_addrs;
    string&			_errlog;
};


/**
 * Check Virtual Interfaces class.
 */
class CheckVifs {
public:
    CheckVifs(const IfMgrIfAtom::VifMap& lfc_vifs,
	      string&			 errlog)
	: _lfc_vifs(lfc_vifs), _errlog(errlog)
    {}

    void
    operator() (const IfTreeInterface::VifMap::value_type& v)
    {
	const IfTreeVif& itv = v.second;

	IfMgrIfAtom::VifMap::const_iterator i = _lfc_vifs.find(itv.vifname());
	if (i == _lfc_vifs.end()) {
	    return;
	}

	const IfMgrVifAtom& imv = i->second;
	if (itv.vifname() != imv.name()) {
	    _errlog += c_format("+  Vif %s name mismatch %s != %s\n",
				itv.vifname().c_str(),
				itv.vifname().c_str(),
				imv.name().c_str());
	}
	if (itv.enabled() != imv.enabled()) {
	    _errlog += c_format("+  Vif %s enabled mismatch %s != %s\n",
				itv.vifname().c_str(),
				truth_of(itv.enabled()),
				truth_of(imv.enabled()));
	}
	if (itv.pif_index() != imv.pif_index()) {
	    _errlog += c_format("+  Vif %s pif_index mismatch %u != %u\n",
				itv.vifname().c_str(),
				itv.pif_index(),
				imv.pif_index());
	}
	if (itv.multicast() != imv.multicast_capable()) {
	    _errlog += c_format("+  Vif %s multicast mismatch %s != %s\n",
				itv.vifname().c_str(),
				truth_of(itv.multicast()),
				truth_of(imv.multicast_capable()));
	}
	if (itv.broadcast() != imv.broadcast_capable()) {
	    _errlog += c_format("+  Vif %s broadcast mismatch %s != %s\n",
				itv.vifname().c_str(),
				truth_of(itv.broadcast()),
				truth_of(imv.broadcast_capable()));
	}
	if (itv.point_to_point() != imv.p2p_capable()) {
	    _errlog += c_format("+  Vif %s point_to_point mismatch %s != %s\n",
				itv.vifname().c_str(),
				truth_of(itv.point_to_point()),
				truth_of(imv.p2p_capable()));
	}
	if (itv.loopback() != imv.loopback()) {
	    _errlog += c_format("+  Vif %s loopback mismatch %s != %s\n",
				itv.vifname().c_str(),
				truth_of(itv.loopback()),
				truth_of(imv.loopback()));
	}

	if (itv.v4addrs().size() != imv.ipv4addrs().size()) {
	    _errlog += "Vif %s IPv4 address count mismatch\n";

	    string fv4a;
	    for_each(itv.v4addrs().begin(), itv.v4addrs().end(),
		     AppendFeaV4Addrs(fv4a));
	    _errlog += "  Fea Iftree v4addrs          = " + fv4a + "\n";

	    string lfv4a;
	    for_each(imv.ipv4addrs().begin(), imv.ipv4addrs().end(),
		     AppendLibFeaClientV4Addrs(lfv4a));
	    _errlog += "  LibFeaClient Iftree v4addrs = " + lfv4a + "\n";
	}

	if (itv.v6addrs().size() != imv.ipv6addrs().size()) {
	    _errlog += "Vif %s IPv6 address count mismatch\n";

	    string fv6a;
	    for_each(itv.v6addrs().begin(), itv.v6addrs().end(),
		     AppendFeaV6Addrs(fv6a));
	    _errlog += "  Fea Iftree v6addrs          = " + fv6a + "\n";

	    string lfv6a;
	    for_each(imv.ipv6addrs().begin(), imv.ipv6addrs().end(),
		     AppendLibFeaClientV6Addrs(lfv6a));
	    _errlog += "  LibFeaClient Iftree v6addrs = " + lfv6a + "\n";
	}

	for_each(itv.v4addrs().begin(), itv.v4addrs().end(),
		 CheckV4Addrs(imv.ipv4addrs(), _errlog));
    }

private:
    const IfMgrIfAtom::VifMap&	_lfc_vifs;
    string&			_errlog;
};


/**
 * Check Interfaces class.
 */
class CheckIfs {
public:
    CheckIfs(const IfMgrIfTree::IfMap&	lfc_ifs,
	     string&			errlog)
	: _lfc_ifs(lfc_ifs), _errlog(errlog)
    {}

    void
    operator() (const IfTree::IfMap::value_type& v)
    {
	const IfTreeInterface& ifi = v.second;

	IfMgrIfTree::IfMap::const_iterator i = _lfc_ifs.find(ifi.name());
	if (i == _lfc_ifs.end()) {
	    return;
	}

	const IfMgrIfAtom& imi = i->second;

	if (ifi.enabled() != imi.enabled()) {
	    _errlog += c_format("+ Interface %s enabled %d != %d\n",
				ifi.name().c_str(),
				ifi.enabled(), imi.enabled());
	}
	if (ifi.mtu() != imi.mtu_bytes()) {
	    _errlog += c_format("+ Interface %s mtu %u != %u\n",
				ifi.name().c_str(),
				ifi.mtu(), imi.mtu_bytes());
	}
	if (ifi.mac() != imi.mac()) {
	    _errlog += c_format("+ Interface %s mac %s != %s\n",
				ifi.name().c_str(),
				ifi.mac().str().c_str(),
				imi.mac().str().c_str());
	}
	if (ifi.pif_index() != imi.pif_index()) {
	    _errlog += c_format("+ Interface %s pif_index %u != %u\n",
				ifi.name().c_str(),
				ifi.pif_index(),
				imi.pif_index());
	}

	if (ifi.vifs().size() != imi.vifs().size()) {
	    _errlog += "Interface %s vif count mismatch\n";
	    string fvifs;
	    for_each(ifi.vifs().begin(), ifi.vifs().end(),
		     AppendFeaVifNames(fvifs));
	    _errlog += "  Fea IfTree vifs          = " + fvifs + "\n";

	    string lvifs;
	    for_each(imi.vifs().begin(), imi.vifs().end(),
		     AppendLibFeaClientVifNames(lvifs));
	    _errlog += "  LibFeaClient IfTree vifs = " + lvifs + "\n";
	}
	for_each(ifi.vifs().begin(), ifi.vifs().end(),
	     CheckVifs(imi.vifs(), _errlog));
    }

private:
    const IfMgrIfTree::IfMap&	_lfc_ifs;
    string&			_errlog;
};

bool
equivalent(const IfTree&	fea_iftree,
	   const IfMgrIfTree&	lfc_iftree,
	   string&		errlog)
{
    errlog.erase();

    if (fea_iftree.ifs().size() != lfc_iftree.ifs().size()) {
	string fifs, lifs;

	for_each(fea_iftree.ifs().begin(), fea_iftree.ifs().end(),
		 AppendFeaIfNames(fifs));

	for_each(lfc_iftree.ifs().begin(), lfc_iftree.ifs().end(),
		 AppendLibFeaClientIfNames(fifs));

	errlog += "Interface count mismatch\n";
	errlog += "  Fea IfTree interfaces          = " + fifs + "\n";
	errlog += "  LibFeaClient IfTree interfaces = " + lifs + "\n";
    }
    for_each(fea_iftree.ifs().begin(), fea_iftree.ifs().end(),
	     CheckIfs(lfc_iftree.ifs(), errlog));

    return errlog.empty();
}
