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

// $XORP: xorp/bgp/attribute_manager.hh,v 1.12 2008/11/08 06:14:36 mjh Exp $

#ifndef __BGP_ATTRIBUTE_MANAGER_HH__
#define __BGP_ATTRIBUTE_MANAGER_HH__

#include <set>
#include "path_attribute.hh"

template <class A>
class PathAttributeList;

#if 0
template <class A>
class StoredAttributeList {
public:
    StoredAttributeList(const PathAttributeList<A> *att) {
	_attribute_list = att;
	_references = 1;
    }

    ~StoredAttributeList() {
	if (_references == 0) {
	    delete _attribute_list;
	}
    }

    int references() const { return _references; }
    void increase() { _references++; }
    void decrease() { _references--; }
    void clone_data() {
	_attribute_list = new PathAttributeList<A>(*_attribute_list);
    }
    const PathAttributeList<A> *attribute() const { return _attribute_list; }
    const uint8_t* hash() const { return _attribute_list->hash(); }
    bool operator<(const StoredAttributeList<A>& them) const;
private:
    const PathAttributeList<A> *_attribute_list;
    int _references;
};

#endif

/**
 * Att_Ptr_Cmp is needed because we want BGPAttributeManager to use a
 * set of pointers, but when we check to see if something's in the set,
 * we want to compare the data and not the pointers themselves.  This
 * forces the right comparison.
 */
template <class A>
class Att_Ptr_Cmp {
public:
    bool operator() (const PAListRef<A>& a, const PAListRef<A>& b) const {
/*	printf("Att_Ptr_Cmp [%s] < [%s] ?\n", a->attribute()->str().c_str(),
	b->attribute()->str().c_str());*/
#ifdef OLDWAY
	if (*(a->attribute()) < *(b->attribute())) {
	    //	    printf("true\n");
	    return true;
	} else {
	    //	    printf("false\n");
	    return false;
	}
#else
	return a < b;
#endif
    }
};

/**
 * AttributeManager manages the storage of PathAttributeLists, so
 * that we don't store the same attribute list more than once.  The
 * interface is simple: to store something, you give it a pointer, and
 * it gives you back a pointer to where it stored it.  To unstore
 * something, you just tell it to delete it, and the undeletion is
 * handled for you if no-one else is still referencing a copy.
 */
template <class A>
class AttributeManager {
public:
    AttributeManager();
    PAListRef<A> add_attribute_list(PAListRef<A>& attribute_list);
    void delete_attribute_list(PAListRef<A>& attribute_list);
    int number_of_managed_atts() const {
	return _attribute_lists.size();
    }
private:
    // set above for explanation of Att_Ptr_Cmp
    set <PAListRef<A>, Att_Ptr_Cmp<A> > _attribute_lists;
    int _total_references;
};

#endif // __BGP_ATTRIBUTE_MANAGER_HH__
