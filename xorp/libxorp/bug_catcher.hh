// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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


#ifndef __XORP_BUG_CATCHER_INC__
#define __XORP_BUG_CATCHER_INC__

#include <assert.h>

class BugCatcher {
private:
   unsigned int magic;
   static unsigned int _cnt;

public:
#define X_BUG_CATCHER_MAGIC 0x1234543
   BugCatcher() { magic = X_BUG_CATCHER_MAGIC; _cnt++; }
   BugCatcher(const BugCatcher& rhs) { magic = rhs.magic; _cnt++; }
   virtual ~BugCatcher() { assert_not_deleted(); magic = 0xdeadbeef; _cnt--; }

   virtual void assert_not_deleted() const {
      assert(magic == X_BUG_CATCHER_MAGIC);
   }

   static int get_instance_count() { return _cnt; }
};

#endif
