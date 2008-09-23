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

#ident "$XORP: xorp/libxipc/xrl_atom_list.cc,v 1.14 2008/09/23 19:56:47 abittau Exp $"

#include "xrl_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "xrl_atom.hh"
#include "xrl_atom_list.hh"
#include "xrl_tokens.hh"

XrlAtomList::XrlAtomList() : _size(0) {}

void
XrlAtomList::prepend(const XrlAtom& xa) throw (BadAtomType)
{
    if (_list.empty() == false && _list.front().type() != xa.type()) {
	// Atom being prepended is of different type to head
	xorp_throw(BadAtomType,
		   c_format("Head type = %d, added type %d\n",
			    _list.front().type(), xa.type()));
    }
    _list.push_front(xa);
    _size++;
}

void
XrlAtomList::append(const XrlAtom& xa) throw (BadAtomType)
{
    check_type(xa);
    do_append(xa);
}

void
XrlAtomList::check_type(const XrlAtom& xa) throw (BadAtomType)
{
    if (_list.empty() == false && _list.front().type() != xa.type()) {
	// Atom being appended is of different type to head
	xorp_throw(BadAtomType,
		   c_format("Head type = %d, added type %d\n",
			    _list.front().type(), xa.type()));
    }
}

void
XrlAtomList::do_append(const XrlAtom& xa)
{
    _list.push_back(xa);
    _size++;
}

const XrlAtom&
XrlAtomList::get(size_t itemno) const throw (InvalidIndex)
{
    list<XrlAtom>::const_iterator ci = _list.begin();
    size_t size = _size;

    if (ci == _list.end() || size == 0) {
	xorp_throw(InvalidIndex, "Index out of range: empty list.");
    }
    while (itemno != 0) {
	++ci;
	if (ci == _list.end() || size-- == 0) {
	    xorp_throw(InvalidIndex, "Index out of range.");
	}
	itemno--;
    }
    return *ci;
}

void
XrlAtomList::remove(size_t itemno) throw (InvalidIndex)
{
    list<XrlAtom>::iterator i = _list.begin();
    size_t size = _size;

    if (i == _list.end() || size == 0) {
	xorp_throw(InvalidIndex, "Index out of range: empty list.");
    }
    while (itemno != 0) {
	i++;
	if (i == _list.end() || size-- == 0) {
	    xorp_throw(InvalidIndex, "Index out of range.");
	}
	itemno--;
    }
    _list.erase(i);
    _size--;
}

size_t XrlAtomList::size() const
{
    return _size;
}

bool
XrlAtomList::operator==(const XrlAtomList& other) const
{
    list<XrlAtom>::const_iterator a = _list.begin();
    list<XrlAtom>::const_iterator b = other._list.begin();
    int i = 0;
    size_t size = _size;

    if (_size != other._size)
	return false;

    while (a != _list.end() && size--) {
	if (b == other._list.end()) {
	    debug_msg("lengths differ\n");
	    return false;
	}
	if (*a != *b) {
		debug_msg("%d -> \"%s\" %d \"%s\" != \"%s\" %d \"%s\"\n", i,
		       a->text().c_str(), a->type(), a->name().c_str(),
		       b->text().c_str(), b->type(), b->name().c_str());
		return false;
	}
	a++; b++; i++;
    }
    return true;
}

string
XrlAtomList::str() const
{
    string r;
    list<XrlAtom>::const_iterator ci = _list.begin();
    size_t size = _size;

    while (ci != _list.end() && size--) {
	r += ci->str();
	ci++;
	if (ci != _list.end()) {
	    r += string(XrlToken::LIST_SEP);
	}
    }
    return r;
}

XrlAtomList::XrlAtomList(const string& s) : _size(0)
{
    const char *start, *sep;
    start = s.c_str();

    for (;;) {
	sep = strstr(start, XrlToken::LIST_SEP);
	if (0 == sep) break;
	append(XrlAtom(string(start, sep - start).c_str()));
	start = sep + TOKEN_BYTES(XrlToken::LIST_SEP) - 1;
    }
    if (*start != '\0')
	append(XrlAtom(start));
}

size_t
XrlAtomList::modify(size_t idx, const uint8_t* buf, size_t len)
{
    bool added = false;

    if (_list.size() <= idx) {
	XLOG_ASSERT(idx == _list.size());
	do_append(XrlAtom());
	added = true;

    } else if (size() <= idx) {
	XLOG_ASSERT(idx == size());
	_size++;
    }

    const XrlAtom& a = get(idx);
    XrlAtom& atom = const_cast<XrlAtom&>(a); 

    size_t rc = atom.unpack(buf, len);

    // gotta make sure the type is right, and that it was unpacked correctly
    if (added) {
	if (!rc)
	    remove(idx);
	else {
	    try {
		check_type(atom);
	    } catch (const BadAtomType& ex) {
		remove(idx);

		throw ex;
	    }
	}
    }

    return rc;
}

void
XrlAtomList::set_size(size_t size)
{
    XLOG_ASSERT(size <= _list.size());

    _size = size;
}
