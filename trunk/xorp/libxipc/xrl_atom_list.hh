// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_atom_list.hh,v 1.12 2008/09/23 08:02:10 abittau Exp $

#ifndef __LIBXIPC_XRL_ATOM_LIST_HH__
#define __LIBXIPC_XRL_ATOM_LIST_HH__

#include <list>
#include "libxorp/exceptions.hh"

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
    struct BadAtomType : public XorpReasonedException {
	BadAtomType(const char* file, size_t line, const string& init_why)
	    : XorpReasonedException("BadAtomType", file, line, init_why)
	{}
    };
    struct InvalidIndex : public XorpReasonedException {
	InvalidIndex(const char* file, size_t line, const string& init_why)
	    : XorpReasonedException("InvalidIndex", file, line, init_why)
	{}
    };

public:
    XrlAtomList();

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

    size_t  modify(size_t item, const uint8_t* buf, size_t len);
    void    set_size(size_t size);

private:
    void    check_type(const XrlAtom& xa) throw (BadAtomType);
    void    do_append(const XrlAtom& xa);

    list<XrlAtom> _list;
    size_t	  _size;
};

#endif // __LIBXIPC_XRL_ATOM_LIST_HH__
