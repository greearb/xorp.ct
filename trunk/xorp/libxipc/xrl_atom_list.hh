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

// $XORP: xorp/libxipc/xrl_atom_list.hh,v 1.3 2003/03/10 23:20:27 hodson Exp $

#ifndef __LIBXIPC_XRL_ATOM_LIST_HH__
#define __LIBXIPC_XRL_ATOM_LIST_HH__

#include <list>

class XrlAtom;

/**
 * @short List class to contain XrlAtom's of one type
 *
 * The XrlAtomList class is used to pass lists of XrlAtoms of a given
 * type in Xrl calls.  The XrlAtom type that the list supports is
 * determined by the first XrlAtom added to the list via the
 * prepend() and append() methods.
*/
class XrlAtomList {
public:
    XrlAtomList();

    class BadAtomType {};
    class InvalidIndex {};

    /**
     * Insert an XrlAtom at the front of the list.
     *
     * @param xa the XrlAtom to be inserted.
     */
    void prepend(const XrlAtom& xa) throw (BadAtomType);

    /**
     * Insert an XrlAtom at the tail of the list.
     *
     * @param xa the XrlAtom to be inserted.
     */
    void append(const XrlAtom& xa) throw (BadAtomType);


    /**
     * Retrieve an XrlAtom from list.
     *
     * @param itemno the index of the atom in the list.
     * @return the itemno - 1 XrlAtom in the list.
     */
    const XrlAtom& get(size_t itemno) const throw (InvalidIndex);

    /**
     * Removes an XrlAtom from list.
     *
     * @param itemno the index of the atom in the list to be removed.
     */
    void remove(size_t itemno) throw (InvalidIndex);

    /**
     * @return number of XrlAtoms in the list.
     */
    size_t size() const;

    /**
     * Test equality of with another XrlAtomList.
     *
     * @return true if XrlAtomList's are the same.
     */
    bool operator==(const XrlAtomList& x) const;

    /**
     * @return a C++ string with the human-readable representation of
     * the list.
     */
    string str() const;

    /**
     * Construct from human-readable representation.
     */
    XrlAtomList(const string& s);

private:
    list<XrlAtom> _list;
};

#endif // __LIBXIPC_XRL_ATOM_LIST_HH__
