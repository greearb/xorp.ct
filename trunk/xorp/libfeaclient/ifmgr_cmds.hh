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

// $XORP: xorp/libfeaclient/ifmgr_cmds.hh,v 1.2 2003/09/03 23:20:17 hodson Exp $

#ifndef __LIBFEACLIENT_IFMGR_CMDS_HH__
#define __LIBFEACLIENT_IFMGR_CMDS_HH__

#include "ifmgr_cmd_base.hh"

/**
 * @short Base class for interface state manipulator commands.
 */
class IfMgrIfCommandBase : public IfMgrCommandBase {
public:
    inline IfMgrIfCommandBase(const string& ifname)
	: _ifname(ifname)
    {}

    /**
     * @return Interface name command relates to.
     */
    inline const string& ifname() const			{ return _ifname; }

protected:
    string _ifname;
};

/**
 * @short Command to add an interface.
 */
class IfMgrIfAdd : public IfMgrIfCommandBase {
public:
    inline IfMgrIfAdd(const string& ifname)
	: IfMgrIfCommandBase(ifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an interface.
 */
class IfMgrIfRemove : public IfMgrIfCommandBase {
public:
    inline IfMgrIfRemove(const string& ifname)
	: IfMgrIfCommandBase(ifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set enabled condition on interface.
 */
class IfMgrIfSetEnabled : public IfMgrIfCommandBase {
public:
    inline IfMgrIfSetEnabled(const string& ifname, bool en)
	: IfMgrIfCommandBase(ifname), _en(en)
    {}

    inline bool en() const				{ return _en; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _en;
};

/**
 * @short Command to set MTU of interface.
 */
class IfMgrIfSetMtu : public IfMgrIfCommandBase {
public:
    inline IfMgrIfSetMtu(const string& ifname, uint32_t mtu_bytes)
	: IfMgrIfCommandBase(ifname), _mtu(mtu_bytes) {}

    inline uint32_t mtu_bytes() const			{ return _mtu; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t _mtu;
};

/**
 * @short Command to set MAC address of interface.
 */
class IfMgrIfSetMac : public IfMgrIfCommandBase {
public:
    inline IfMgrIfSetMac(const string& ifname, const Mac& mac)
	: IfMgrIfCommandBase(ifname), _mac(mac) {}

    inline const Mac& mac() const			{ return _mac; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    Mac _mac;
};


/**
 * @short Base class for virtual interface state manipulation commands.
 */
class IfMgrVifCommandBase : public IfMgrIfCommandBase {
public:
    inline IfMgrVifCommandBase(const string& ifname, const string& vifname)
	: IfMgrIfCommandBase(ifname), _vifname(vifname)
    {}

    /**
     * @return Virtual interface name command relates to.
     */
    inline const string& vifname() const		{ return _vifname; }

protected:
    string _vifname;
};

/**
 * @short Command to add a virtual interface to an interface.
 */
class IfMgrVifAdd : public IfMgrVifCommandBase {
public:
    inline IfMgrVifAdd(const string& ifname, const string& vifname)
	: IfMgrVifCommandBase(ifname, vifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove a virtual interface to an interface.
 */
class IfMgrVifRemove : public IfMgrVifCommandBase {
public:
    inline IfMgrVifRemove(const string& ifname, const string& vifname)
	: IfMgrVifCommandBase(ifname, vifname)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set enabled condition on a virtual interface.
 */
class IfMgrVifSetEnabled : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetEnabled(const string& ifname,
			      const string& vifname,
			      bool	    en)
	: IfMgrVifCommandBase(ifname, vifname), _en(en)
    {}

    inline bool en() const 				{ return _en; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _en;
};

/**
 * @short Command to mark virtual interface as multicast capable.
 */
class IfMgrVifSetMulticastCapable : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetMulticastCapable(const string& ifname,
				       const string& vifname,
				       bool	     cap)
	: IfMgrVifCommandBase(ifname, vifname), _cap(cap)
    {}

    inline bool capable() const 			{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to mark virtual interface as broadcast capable.
 */
class IfMgrVifSetBroadcastCapable : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetBroadcastCapable(const string& ifname,
				       const string& vifname,
				       bool	     cap)
	: IfMgrVifCommandBase(ifname, vifname), _cap(cap)
    {}

    inline bool capable() const 			{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to mark virtual interface as point-to-point capable.
 */
class IfMgrVifSetP2PCapable : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetP2PCapable(const string& ifname,
				 const string& vifname,
				 bool	       cap)
	: IfMgrVifCommandBase(ifname, vifname), _cap(cap)
    {}

    inline bool capable() const 			{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to mark virtual interface as loopback capable.
 */
class IfMgrVifSetLoopbackCapable : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetLoopbackCapable(const string& ifname,
				      const string& vifname,
				      bool	    cap)
	: IfMgrVifCommandBase(ifname, vifname), _cap(cap)
    {}

    inline bool capable() const 			{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to associate a physical interface id with a virtual
 * interface.
 */
class IfMgrVifSetPifIndex : public IfMgrVifCommandBase {
public:
    inline IfMgrVifSetPifIndex(const string& ifname,
			       const string& vifname,
			       uint32_t	     pif_index)
	: IfMgrVifCommandBase(ifname, vifname), _pi(pif_index)
    {}

    inline uint32_t pif_index() const 			{ return _pi; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t _pi;
};


/**
 * @short Base class for interface IPv4 address data manipulation.
 */
class IfMgrIPv4CommandBase : public IfMgrVifCommandBase {
public:
    inline IfMgrIPv4CommandBase(const string& ifname,
				const string& vifname,
				IPv4	      addr)
	: IfMgrVifCommandBase(ifname, vifname), _addr(addr)
    {}

    /**
     * @return IPv4 address command relates to.
     */
    inline IPv4 addr() const 				{ return _addr; }

protected:
    IPv4   _addr;
};

/**
 * @short Command to add an address to a virtual interface.
 */
class IfMgrIPv4Add : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4Add(const string& ifname,
			const string& vifname,
			IPv4	      addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an address to a virtual interface.
 */
class IfMgrIPv4Remove : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4Remove(const string& ifname,
			   const string& vifname,
			   IPv4		 addr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set prefix of a virtual interface interface address.
 */
class IfMgrIPv4SetPrefix : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetPrefix(const string& ifname,
			      const string& vifname,
			      IPv4	    addr,
			      uint32_t	    prefix)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _prefix(prefix)
    {}

    inline uint32_t prefix() const			{ return _prefix; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t _prefix;
};

/**
 * @short Command to set enabled flag of a virtual interface interface address.
 */
class IfMgrIPv4SetEnabled : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetEnabled(const string& ifname,
			       const string& vifname,
			       IPv4	     addr,
			       bool	     en)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _en(en)
    {}

    inline bool en() const				{ return _en; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _en;
};

/**
 * @short Command to mark virtual interface address as multicast capable.
 */
class IfMgrIPv4SetMulticastCapable : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetMulticastCapable(const string& ifname,
					const string& vifname,
					IPv4	      addr,
					bool	      cap)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _cap(cap)
    {}

    inline bool capable() const				{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to mark virtual interface address as a loopback address.
 */
class IfMgrIPv4SetLoopback : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetLoopback(const string& ifname,
				const string& vifname,
				IPv4	      addr,
				bool	      loop)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _loop(loop)
    {}

    inline bool loopback() const			{ return _loop; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _loop;
};

/**
 * @short Command to set broadcast address associated with a virtual
 * interface address.
 */
class IfMgrIPv4SetBroadcast : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetBroadcast(const string& ifname,
				 const string& vifname,
				 IPv4	       addr,
				 IPv4	       oaddr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _oaddr(oaddr)
    {}

    inline const IPv4 oaddr() const			{ return _oaddr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv4 _oaddr;
};

/**
 * @short Command to set endpoint address associated with a virtual
 * interface address.
 */
class IfMgrIPv4SetEndpoint : public IfMgrIPv4CommandBase {
public:
    inline IfMgrIPv4SetEndpoint(const string& ifname,
				const string& vifname,
				IPv4	      addr,
				IPv4	      oaddr)
	: IfMgrIPv4CommandBase(ifname, vifname, addr), _oaddr(oaddr)
    {}

    inline const IPv4 oaddr() const			{ return _oaddr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv4 _oaddr;
};


/**
 * @short Base class for interface IPv6 address data manipulation.
 */
class IfMgrIPv6CommandBase : public IfMgrVifCommandBase {
public:
    inline IfMgrIPv6CommandBase(const string& ifname,
				const string& vifname,
				IPv6	      addr)
	: IfMgrVifCommandBase(ifname, vifname), _addr(addr)
    {}

    /**
     * @return IPv6 address command relates to.
     */
    inline const IPv6& addr() const 			{ return _addr; }

protected:
    IPv6   _addr;
};

/**
 * @short Command to add an address to a virtual interface.
 */
class IfMgrIPv6Add : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6Add(const string& ifname,
			const string& vifname,
			IPv6	      addr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to remove an address to a virtual interface.
 */
class IfMgrIPv6Remove : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6Remove(const string& ifname,
			   const string& vifname,
			   IPv6		 addr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr)
    {}

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		  const string&		xrl_target,
		  const IfMgrXrlSendCB&	xscb) const;

    string str() const;
};

/**
 * @short Command to set prefix of a virtual interface interface address.
 */
class IfMgrIPv6SetPrefix : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6SetPrefix(const string& ifname,
			      const string& vifname,
			      IPv6	    addr,
			      uint32_t	    prefix)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _prefix(prefix)
    {}

    inline uint32_t prefix() const			{ return _prefix; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    uint32_t _prefix;
};

/**
 * @short Command to set enabled flag of a virtual interface interface address.
 */
class IfMgrIPv6SetEnabled : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6SetEnabled(const string& ifname,
			       const string& vifname,
			       IPv6	     addr,
			       bool	     en)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _en(en)
    {}

    inline bool en() const				{ return _en; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _en;
};

/**
 * @short Command to mark virtual interface address as multicast capable.
 */
class IfMgrIPv6SetMulticastCapable : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6SetMulticastCapable(const string& ifname,
					const string& vifname,
					IPv6	      addr,
					bool	      cap)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _cap(cap)
    {}

    inline bool capable() const				{ return _cap; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _cap;
};

/**
 * @short Command to mark virtual interface address as a loopback address.
 */
class IfMgrIPv6SetLoopback : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6SetLoopback(const string& ifname,
				const string& vifname,
				IPv6	      addr,
				bool	      loop)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _loop(loop)
    {}

    inline bool loopback() const			{ return _loop; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    bool _loop;
};

/**
 * @short Command to set endpoint address associated with a virtual
 * interface address.
 */
class IfMgrIPv6SetEndpoint : public IfMgrIPv6CommandBase {
public:
    inline IfMgrIPv6SetEndpoint(const string&	ifname,
				const string&	vifname,
				IPv6		addr,
				IPv6		oaddr)
	: IfMgrIPv6CommandBase(ifname, vifname, addr), _oaddr(oaddr)
    {}

    inline const IPv6& oaddr() const			{ return _oaddr; }

    bool execute(IfMgrIfTree& tree) const;

    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;

    string str() const;

protected:
    IPv6 _oaddr;
};


/**
 * @short Base class for configuration events.
 *
 * These commands serve as hints to remote command recipients, eg complete
 * config tree sent, changes made.
 */
class IfMgrHintCommandBase : public IfMgrCommandBase {
public:
    /**
     * Apply command to local tree.  This is a no-op for this class
     * and its derivatives.
     *
     * @return success indication, always true.
     */
    bool execute(IfMgrIfTree& tree) const;
};

class IfMgrHintTreeComplete : public IfMgrHintCommandBase {
public:
    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;
    string str() const;
};

class IfMgrHintUpdatesMade : public IfMgrHintCommandBase {
public:
    bool forward(XrlSender&		sender,
		 const string&		xrl_target,
		 const IfMgrXrlSendCB&	xscb) const;
    string str() const;
};

#endif // __LIBFEACLIENT_IFMGR_CMDS_HH__















