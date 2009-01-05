// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/xrl_atom_list.hh,v 1.16 2008/10/02 21:57:24 bms Exp $

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
