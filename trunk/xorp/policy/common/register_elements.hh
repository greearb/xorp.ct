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

#ifndef __POLICY_COMMON_REGISTER_ELEMENTS_HH__
#define __POLICY_COMMON_REGISTER_ELEMENTS_HH__

/**
 * @short Does the initial registration of ElementFactory callbacks.
 *
 * This class only has a constructor which registers callbacks. It is used to
 * ensure that callbacks are registered once, and at the correct time.
 */
class RegisterElements {
public:
    /**
     * Contains callback registration routines.
     */
    RegisterElements();

};

#endif // __POLICY_COMMON_REGISTER_ELEMENTS_HH__
