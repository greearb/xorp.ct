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

// $XORP: xorp/fea/ifconfig_rtsock.hh,v 1.1.1.1 2002/12/11 23:56:02 hodson Exp $

#ifndef __FEA_IFCONFIG_RTSOCK_HH__
#define __FEA_IFCONFIG_RTSOCK_HH__

#include "rtsock.hh"
#include "ifconfig.hh"

class IfConfigRoutingSocket : public RoutingSocketObserver, public IfConfig {
public:
    IfConfigRoutingSocket(RoutingSocket&		rs,
			  IfConfigUpdateReporterBase&	ur,
			  SimpleIfConfigErrorReporter&	er);

    ~IfConfigRoutingSocket();

    bool push_config(const IfTree& config);

    const IfTree& pull_config(IfTree& config);

    void rtsock_data(const uint8_t* data, size_t n_bytes);

protected:
    void flush_config(IfTree& it);

    bool read_config(IfTree& it);

    /**
     * @return true if one or more messages in buffer handled by parsing.
     */
    bool parse_buffer(IfTree&		it,
		      const uint8_t*	buf,
		      size_t		buf_bytes);

    void push_if(const IfTreeInterface&	i);

    void push_vif(const IfTreeInterface&	i,
		  const IfTreeVif&	 	v);

    void push_addr(const IfTreeInterface&	i,
		   const IfTreeVif&		v,
		   const IfTreeAddr4&		a);

    void push_addr(const IfTreeInterface&	i,
		   const IfTreeVif&		v,
		   const IfTreeAddr6&		a);

protected:
    IfTree _live_config;
    int       _s4; 		/* socket used for IPv4 configuration */
    int       _s6; 		/* socket used for IPv6 configuration */
};

#endif //  __FEA_IFCONFIG_RTSOCK_HH__
