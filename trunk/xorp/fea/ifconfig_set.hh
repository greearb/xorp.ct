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

// $XORP: xorp/fea/ifconfig_set.hh,v 1.3 2003/05/14 01:13:43 pavlin Exp $

#ifndef __FEA_IFCONFIG_SET_HH__
#define __FEA_IFCONFIG_SET_HH__


class IfConfig;
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IfTreeAddr4;
class IfTreeAddr6;

class IfConfigSet {
public:
    IfConfigSet(IfConfig& ifc);
    
    virtual ~IfConfigSet();
    
    IfConfig&	ifc() { return _ifc; }
    
    virtual void register_ifc();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start() = 0;
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop() = 0;
    
    virtual bool push_config(const IfTree& config) = 0;
    
private:
    IfConfig&	_ifc;
};

class IfConfigSetDummy : public IfConfigSet {
public:
    IfConfigSetDummy(IfConfig& ifc);
    virtual ~IfConfigSetDummy();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    virtual bool push_config(const IfTree& config);
    
private:
};

class IfConfigSetIoctl : public IfConfigSet {
public:
    IfConfigSetIoctl(IfConfig& ifc);
    virtual ~IfConfigSetIoctl();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    virtual bool push_config(const IfTree& config);
    
private:
    void push_if(const IfTreeInterface& i);
    void push_vif(const IfTreeInterface& i, const IfTreeVif& v);
    void push_addr(const IfTreeInterface& i, const IfTreeVif& v,
		   const IfTreeAddr4& a);
    void push_addr(const IfTreeInterface& i, const IfTreeVif& v,
		   const IfTreeAddr6& a);
    
    int _s4;
    int _s6;
};

class IfConfigSetNetlink : public IfConfigSet,
			   public NetlinkSocket4,
			   public NetlinkSocket6 {
public:
    IfConfigSetNetlink(IfConfig& ifc);
    virtual ~IfConfigSetNetlink();

    /**
     * Start operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start();
    
    /**
     * Stop operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop();
    
    virtual bool push_config(const IfTree& config);
    
private:
    void push_if(const IfTreeInterface& i);
    void push_vif(const IfTreeInterface& i, const IfTreeVif& v);
    void push_addr(const IfTreeInterface& i, const IfTreeVif& v,
		   const IfTreeAddr4& a);
    void push_addr(const IfTreeInterface& i, const IfTreeVif& v,
		   const IfTreeAddr6& a);

    int _s4;
    int _s6;
};


#endif // __FEA_IFCONFIG_SET_HH__
