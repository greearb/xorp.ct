// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __LIBXIPC_FINDER_NG_CLIENT_XRL_TARGET_HH__
#define __LIBXIPC_FINDER_NG_CLIENT_XRL_TARGET_HH__

#include "finder_client_base.hh"

class FinderNGClientXrlCommandInterface; 

class FinderNGClientXrlTarget : public XrlFinderclientTargetBase {
public:
    FinderNGClientXrlTarget(FinderNGClientXrlCommandInterface* client,
			    XrlCmdMap* cmds);

    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);

    XrlCmdError finder_client_0_1_hello();
    XrlCmdError finder_client_0_1_remove_xrl_from_cache(const string& xrl);
    XrlCmdError finder_client_0_1_remove_xrls_for_target_from_cache(const string& target);
    
protected:
    FinderNGClientXrlCommandInterface* _client;
};

#endif // __LIBXIPC_FINDER_NG_CLIENT_XRL_TARGET_HH__
