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

#ident "$XORP: xorp/devnotes/template_gpl.cc,v 1.1 2008/10/02 21:56:43 bms Exp $"
/*
 * verify that the .hh file makes sense.
 */

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
	theone.AddUser("george", 111);
	theone.AddUser("hermoine", 106);
	theone.AddUser("phillip", 106);
	theone.AddUser("andrew", 106);
	theone.AddGroup("group44", 106);
	theone.AddGroup("fubarz", 106);
	theone.AddGroup("muumuus", 106);
	theone.AddGroup("tekeleks", 106);
	theone.AddUser("geromino", 106);
	theone.AddUser("vasili", 106);
	theone.AddGroup("vinieski", 106);
	theone.AddUser("ocupine", 106);

	theone.describe();

	
	/*
	 * delete some of the users and groups.
	 */
	theone.DelGroup("fubarz");
	theone.DelUser("andrew");
	theone.DelUser("george");
	theone.DelUser("ocupine");

	theone.describe();

	return 0;
}
