// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/libxorp/utils.hh,v 1.2 2003/03/10 23:20:37 hodson Exp $

#ifndef __LIBXORP_UTILS_HH__
#define __LIBXORP_UTILS_HH__

#include <list>
#include <vector>

//
// Set of utilities
//

/**
 * Template to delete a list of pointers, and the objects pointed to.
 * 
 * @param delete_list the list of pointers to objects to delete.
 */
template<class T> void
delete_pointers_list(list<T *>& delete_list)
{
    list<T *> tmp_list;
    
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_list, delete_list);
    
    for (typename list<T *>::iterator iter = tmp_list.begin();
	 iter != tmp_list.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_list.clear();
}

/**
 * Template to delete an array of pointers, and the objects pointed to.
 * 
 * @param delete_vector the vector of pointers to objects to delete.
 */
template<class T> void
delete_pointers_vector(vector<T *>& delete_vector)
{
    vector<T *> tmp_vector;
    // Swap the elements, so the original container never contains
    // entries that point to deleted elements.
    swap(tmp_vector, delete_vector);
    
    for (typename vector<T *>::iterator iter = tmp_vector.begin();
	 iter != tmp_vector.end();
	 ++iter) {
	T *elem = (*iter);
	delete elem;
    }
    tmp_vector.clear();
}

#endif // __LIBXORP_UTILS_HH__
