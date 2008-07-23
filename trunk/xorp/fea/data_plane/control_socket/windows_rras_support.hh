// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/control_socket/windows_rras_support.hh,v 1.3 2008/01/04 03:15:56 pavlin Exp $

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
