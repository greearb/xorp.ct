// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rib/protocol.cc,v 1.5 2004/02/11 08:48:46 pavlin Exp $"

#include "rib_module.h"
#include "protocol.hh"


Protocol::Protocol(const string& name, ProtocolType protocol_type,
		   uint32_t genid)
    : _name(name),
      _protocol_type(protocol_type),
      _genid(genid)
{

}
