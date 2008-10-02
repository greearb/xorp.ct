// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/attribute_manager.cc,v 1.16 2008/07/23 05:09:31 pavlin Exp $"

//#define DEBUG_LOGGING
#include "bgp_module.h"
#include "attribute_manager.hh"

template <class A>
bool
StoredAttributeList<A>::operator<(const StoredAttributeList<A>& them) const
{
    return (memcmp(hash(), them.hash(), 16) < 0);
}

template <class A>
AttributeManager<A>::AttributeManager()
{
    _total_references = 0;
}

template <class A>
const PathAttributeList<A> *
AttributeManager<A>::add_attribute_list(
    const PathAttributeList<A> *attribute_list)
{
    debug_msg("AttributeManager<A>::add_attribute_list\n");
    StoredAttributeList<A> *new_att =
	new StoredAttributeList<A>(attribute_list);
    typedef typename set<StoredAttributeList<A>*, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(new_att);

    if (i == _attribute_lists.end()) {
	new_att->clone_data();
	_attribute_lists.insert(new_att);
	return new_att->attribute();
    }

    (*i)->increase();
    delete new_att;
    debug_msg("** (+) ref count for %p now %d\n",
	      (*i)->attribute(), (*i)->references());
    debug_msg("done\n");

    return (*i)->attribute();
}

template <class A>
void
AttributeManager<A>::delete_attribute_list(
    const PathAttributeList<A> *attribute_list)
{
    StoredAttributeList<A> *del_att =
	new StoredAttributeList<A>(attribute_list);
    debug_msg("AttributeManager<A>::delete_attribute_list %p\n",
	      del_att->attribute());
    typedef typename set<StoredAttributeList<A>*, Att_Ptr_Cmp<A> >::iterator Iter;
    Iter i = _attribute_lists.find(del_att);
    assert(i != _attribute_lists.end());
    delete del_att;
    (*i)->decrease();
    debug_msg("** (-) ref count for %p now %d\n",
	      (*i)->attribute(), (*i)->references());
    if ((*i)->references() == 0) {
	del_att = (*i);
	_attribute_lists.erase(i);
	delete del_att;
    }

}

template class AttributeManager<IPv4>;
template class AttributeManager<IPv6>;









