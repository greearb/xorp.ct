// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/bgp/subnet_route.hh,v 1.2 2002/12/16 21:48:34 mjh Exp $

#ifndef __BGP_SUBNET_ROUTE_HH__
#define __BGP_SUBNET_ROUTE_HH__

#include "path_attribute_list.hh"
#include "attribute_manager.hh"
#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#define SRF_IN_USE 0x0001
#define SRF_WINNER 0x0002
#define SRF_FILTERED 0x0004

template<class A>
class SubnetRoute
{
public:
    SubnetRoute(const SubnetRoute<A>& route_to_clone);
    SubnetRoute(const IPNet<A> &net, 
		const PathAttributeList<A> *attributes,
		const SubnetRoute<A>* parent_route);
    SubnetRoute(const IPNet<A> &net, 
		const PathAttributeList<A> *attributes,
		const SubnetRoute<A>* parent_route,
		uint32_t igp_metric);
    ~SubnetRoute();
    bool operator==(const SubnetRoute<A>& them) const;
    const IPNet<A>& net() const {return _net;}
    const A& nexthop() const {return _attributes->nexthop();}
    const PathAttributeList<A> *attributes() const {return _attributes;}
    bool in_use() const {return (_flags & SRF_IN_USE) != 0;}
    void set_in_use(bool used) const;

    /* the winner methods are used to set the route flag indicating
       that the route won the decision process.  These methods should
       NOT be called by anything other than the DecisionTable because
       caching means that they may not do the right thing anywhere
       else */
    bool is_winner() const {return (_flags & SRF_WINNER) != 0;}
    void set_is_winner(uint32_t igp_metric) const;
    void set_is_not_winner() const;

    bool is_filtered() const {return (_flags & SRF_FILTERED) != 0;}
    void set_filtered(bool filtered) const;
    string str() const;

    int number_of_managed_atts() const {
	return _att_mgr.number_of_managed_atts();
    }
    uint32_t igp_metric() const {return _igp_metric;}

    const SubnetRoute<A> *original_route() const {
	if (_parent_route)
	    return _parent_route;
	else
	    return this;
    }

    void set_parent_route(const SubnetRoute<A> *parent) 
    {
	_parent_route = parent;
    }
protected:
private:
    static AttributeManager<A> _att_mgr;
    IPNet<A> _net;
    const PathAttributeList<A> *_attributes;

    const SubnetRoute<A> *_parent_route;

    /*Flags: */
    /* SRF_IN_USE indicates whether this route is currently used for
       anything.  The route might not be used if it wasn't chosen by
       the BGP decision mechanism, or if a downstream filter caused a
       modified version of this route to be installed in a cache table */

    /* SRF_WINNER indicates that the route won the decision process.  */

    /* SRF_FILTERED indicates that the route was filtered downstream.
       Currently this is only used for RIB-IN routes that are filtered
       in the inbound filter bank */
    mutable uint32_t _flags;

    /* If the route is a winner (SRF_WINNER is set), then
       DecisionTable will fill in the IGP metric that was used in
       deciding the route was a winner */
    mutable uint32_t _igp_metric;
};

#endif // __BGP_SUBNET_ROUTE_HH__
