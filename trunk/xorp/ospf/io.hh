// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __OSPF_IO_HH__
#define __OSPF_IO_HH__

/**
 * An abstract class that defines packet reception and
 * transmission. The details of how packets are received or transmitted
 * are therefore hidden from the internals of the OSPF code.
 */
template <typename A>
class IO {
 public:
    virtual ~IO() {}

    /**
     * Send Raw frames.
     */
    virtual bool send(const string& interface, const string& vif,
		      A dst, A src,
		      uint8_t* data, uint32_t len) = 0;

    typedef typename XorpCallback6<void, const string&, const string&,
				   A, A,
				   uint8_t*,
				   uint32_t>::RefPtr ReceiveCallback;
    /**
     * Register for receiving raw frames.
     */
    virtual bool register_receive(ReceiveCallback) = 0;

    /**
     * Enable the interface/vif to receive frames.
     */
    virtual bool enable_interface_vif(const string& interface,
				      const string& vif) = 0;

    /**
     * Disable this interface/vif from receiving frames.
     */
    virtual bool disable_interface_vif(const string& interface,
				       const string& vif) = 0;
    
    /**
     * Add route
     */
    virtual bool add_route() = 0;

    /**
     * Delete route
     */
    virtual bool delete_route() = 0;
};
#endif // __OSPF_IO_HH__
