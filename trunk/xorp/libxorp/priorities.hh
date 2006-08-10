// Copyright (c) 2006 International Computer Science Institute
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

#ifndef __LIBXORP_PRIORITIES_HH__
#define __LIBXORP_PRIORITIES_HH__

// Task/Timer priorities.  You don't need to stick with these, but
// they are suggested values.  
#define HIGHEST_PRIORITY 0
#define XRL_KEEPALIVE_PRIORITY 1
#define HIGH_PRIORITY 2
#define DEFAULT_PRIORITY 4
#define BACKGROUND_PRIORITY 7
#define LOWEST_PRIORITY 9
#define INFINITY_PRIORITY 255


#endif
