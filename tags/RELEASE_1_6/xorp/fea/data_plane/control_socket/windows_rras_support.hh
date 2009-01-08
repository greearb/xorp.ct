// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/control_socket/windows_rras_support.hh,v 1.5 2008/10/02 21:56:54 bms Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_WINDOWS_RRAS_SUPPORT_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_WINDOWS_RRAS_SUPPORT_HH__

class WinSupport {
public:
    static bool is_rras_running();
    static int restart_rras();
    static int add_protocol_to_rras(int family);
    static int add_protocol_to_registry(int family);
private:
};

#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_WINDOWS_RRAS_SUPPORT_HH__
