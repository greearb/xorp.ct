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

// $XORP: xorp/bgp/attribute_manager.hh,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $

#ifndef __BGP_ATTRIBUTE_MANAGER_HH__
#define __BGP_ATTRIBUTE_MANAGER_HH__

#include <set>
#include "path_attribute_list.hh"

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

/**
 * Att_Ptr_Cmp is needed because we want BGPAttributeManager to use a
 * set of pointers, but when we check to see if something's in the set,
 * we want to compare the data and not the pointers themselves.  This
 * forces the right comparison.
 */
template <class A>
class Att_Ptr_Cmp {
public:
    bool operator() (StoredAttributeList<A> *a,
		     StoredAttributeList<A> *b) const {
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
	return *a < *b;
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
    const PathAttributeList<A> *
       add_attribute_list(const PathAttributeList<A> *attribute_list);
    void delete_attribute_list(const PathAttributeList<A> *attribute_list);
    int number_of_managed_atts() const {
	return _attribute_lists.size();
    }
private:
    // set above for explanation of Att_Ptr_Cmp
    set <StoredAttributeList<A>*, Att_Ptr_Cmp<A> > _attribute_lists;
    int _total_references;
};

#endif // __BGP_ATTRIBUTE_MANAGER_HH__
