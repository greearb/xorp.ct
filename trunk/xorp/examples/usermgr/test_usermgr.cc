// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#ident "$XORP: xorp/examples/usermgr/test_usermgr.cc,v 1.1 2008/10/18 02:41:50 paulz Exp $"

/*
 * Test the UserDB implementation.
 */

#include "libxorp/xorp.h"
#include "usermgr_module.h"
#include "usermgr.hh"

int
main (int argc, char ** argv )
{
    UNUSED(argc);
    UNUSED(argv);
    UserDB theone;

    /*
     * create some users and groups.
     */
    theone.add_user("george", 111);
    theone.add_user("hermoine", 106);
    theone.add_user("phillip", 126);
    theone.add_user("andrew", 136);
    theone.add_group("group44", 146);
    theone.add_group("fubarz", 156);
    theone.add_group("muumuus", 105);
    theone.add_group("tekeleks", 155);
    theone.add_user("geromino", 166);
    theone.add_user("vasili", 606);
    theone.add_group("vinieski", 666);
    theone.add_user("ocupine", 166);

    theone.describe();

    
    /*
     * delete some of the users and groups.
     */
    theone.del_group("fubarz");
    theone.del_user("andrew");
    theone.del_user("george");
    theone.del_user("ocupine");

    theone.describe();

    return 0;
}
