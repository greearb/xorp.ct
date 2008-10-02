// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/pw_notifier.hh,v 1.7 2008/10/01 23:44:25 pavlin Exp $

#ifndef __POLICY_PW_NOTIFIER_HH__
#define __POLICY_PW_NOTIFIER_HH__

#include <string>

/**
 * @short Interface which receives notification events from ProcessWatch.
 *
 * An object may register to receive notification events with a process watch.
 * This will enable the object to receive announcements for the death and birth
 * of a XORP process.
 */
class PWNotifier {
public:
    virtual ~PWNotifier() {}
  
    /**
     * Method called when a XORP process comes to life.
     *
     * @param process process name which was born.
     */
    virtual void birth(const std::string& process) = 0;

    /**
     * Method called when a XORP process dies.
     *
     * @param process process name which died.
     */
    virtual void death(const std::string& process) = 0;
};

#endif // __POLICY_PW_NOTIFIER_HH__
