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

// $XORP: xorp/fea/click.hh,v 1.2 2003/03/10 23:20:13 hodson Exp $

#ifndef __FEA_CLICK_HH__
#define __FEA_CLICK_HH__

#include <string>
#include "fticonfig.hh"

class Click {
public:
    Click();
    Click(const char *config_file, const char *click_module,
	  const char *fea_module) throw(FtiConfigError);

    static bool load();
    static void unload();

    static bool configure(const string& config, bool sleep = true);
    static bool is_loaded();
private:
    static bool load(const string& fname);
    static bool unload(const string& fname);

    static bool mount();
    static bool unmount();

    static const char *MOUNT_POINT;
    static const char *_config_file;
    static string _click_module;
    static string _fea_module;

    static bool loaded;
};

#endif // __FEA_CLICK_HH__
