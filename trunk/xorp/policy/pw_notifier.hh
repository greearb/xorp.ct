// vim:set sts=4 ts=8:

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

// $XORP$

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
