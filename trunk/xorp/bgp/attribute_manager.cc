// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



//#define DEBUG_LOGGING
#include "bgp_module.h"
#include "attribute_manager.hh"

#if 0
template <class A>
bool
StoredAttributeList<A>::operator<(const StoredAttributeList<A>& them) const
{
    return (memcmp(hash(), them.hash(), 16) < 0);
}
#endif

template <class A>
AttributeManager<A>::AttributeManager()
{
    _total_references = 0;
}

template <class A>
PAListRef<A> 
AttributeManager<A>::add_attribute_list(PAListRef<A>& palist)
{
    debug_msg("AttributeManager<A>::add_attribute_list\n");
    typedef typename set<PAListRef<A>, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(palist);

    if (i == _attribute_lists.end()) {
	_attribute_lists.insert(palist);
	palist->incr_managed_refcount(1);
	debug_msg("** new att list\n");
	debug_msg("** (+) ref count for %p now %u\n",
		  palist.attributes(), palist->managed_references());
	return palist;
    }

    (*i)->incr_managed_refcount(1);
    debug_msg("** old att list\n");
    debug_msg("** (+) ref count for %p now %u\n",
	      (*i).attributes(), (*i)->managed_references());
    debug_msg("done\n");

    return *i;
}

template <class A>
void
AttributeManager<A>::delete_attribute_list(PAListRef<A>& palist)
{
    debug_msg("AttributeManager<A>::delete_attribute_list %p\n",
	      palist.attributes());
    typedef typename set<PAListRef<A>, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(palist);
    assert(i != _attribute_lists.end());

    XLOG_ASSERT((*i)->managed_references()>=1);
    (*i)->decr_managed_refcount(1);

    debug_msg("** (-) ref count for %p now %u\n",
	      (*i).attributes(), (*i)->managed_references());

    if ((*i)->managed_references() < 1) {
	_attribute_lists.erase(i);
    }
}

template class AttributeManager<IPv4>;
template class AttributeManager<IPv6>;









