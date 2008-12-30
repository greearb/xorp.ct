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

// $XORP: xorp/policy/process_watch_base.hh,v 1.7 2008/10/01 23:44:25 pavlin Exp $

#ifndef __POLICY_PROCESS_WATCH_BASE_HH__
#define __POLICY_PROCESS_WATCH_BASE_HH__

#include <string>

/**
 * @short Base class for a process watcher.
 *
 * The VarMap registers interest in known protocols. Finally, the filter manager
 * may be informed about which processes are alive.
 */
class ProcessWatchBase {
public:
    virtual ~ProcessWatchBase() {}

    /**
     * @param proto protocol to register interest in.
     */
    virtual void add_interest(const std::string& proto) = 0;
};

#endif // __POLICY_PROCESS_WATCH_BASE_HH__
