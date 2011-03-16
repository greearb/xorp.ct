/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

// Idea from boost::noncopyable

/** Class is not supposed to be coppied, so we don't implement the copy
 * methods, but we do declare them.  This will cause a compile error if anyone
 * ever tries to copy objects descended from xorp_noncopyable.
 */

#ifndef __XORP_NONCOPYABLE__INC_
#define __XORP_NONCOPYABLE__INC_

class xorp_noncopyable {
private:
   xorp_noncopyable(const xorp_noncopyable& rhs);
   xorp_noncopyable& operator=(const xorp_noncopyable& rhs);

public:
   xorp_noncopyable() { }

};

#endif
