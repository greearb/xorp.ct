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

// $XORP: xorp/fea/ifmanager.hh,v 1.3 2003/05/02 07:50:48 pavlin Exp $

#ifndef __FEA_IFMANAGER_HH__
#define __FEA_IFMANAGER_HH__

#include "iftree.hh"
#include "ifconfig.hh"

/**
 * InterfaceManager is the interface that is exposed by the fea via XRL's.
 */
class InterfaceManager {
public:
    InterfaceManager(IfConfig& ifc) : _ifc(ifc) {}

    IfConfig&  ifc() const				{ return _ifc; }
    IfTree& iftree()					{ return _iftree; }
    IfTree& old_iftree()				{ return _old_iftree; }

protected:
    IfConfig&	_ifc;
    IfTree	_iftree;
    IfTree	_old_iftree;
};

#endif // __FEA_IFMANAGER_HH__
