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

#ident "$XORP: xorp/fea/click.cc,v 1.2 2003/03/10 23:20:13 hodson Exp $"

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"

#include <sys/param.h>
#include <sys/linker.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <fstream>
#include <string>

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "click.hh"
#include "fticonfig.hh"

const char *Click::MOUNT_POINT = "/click";
const char *Click::_config_file;
string Click::_click_module;
string Click::_fea_module;

bool Click::loaded = false;	/* XXX For when we separate the
				   modules */
Click::Click()
{
}

Click::Click(const char *config_file, const char *click_module,
	     const char *fea_module) throw(FtiConfigError)
{
    _config_file = config_file;
    _click_module = click_module;
    _fea_module = fea_module;

    /* XXX
    ** If there is already a click loaded zero the config before we
    ** try anything. When click is more robust this will not be necessary.
    */
    configure("");

    /*
    ** We need to set loaded to true to force the unload.
    */
    loaded = true;
    unload();
}

bool
Click::load(const string& fname)
{
    string basename = string(fname, fname.find_last_of("/") + 1);

    int cfd;
    if (-1 == (cfd = kldfind(basename.c_str()))) {
	if (-1 == kldload(fname.c_str())) {
	    XLOG_ERROR("Unable to load module: %s", fname.c_str());
	    return false;
	}
    } else {
	debug_msg("%s already loaded\n", fname.c_str());
	close(cfd);
    }

    return true;
}

/*
** Load click.
*/
bool
Click::load()
{
    debug_msg("load\n");

    if (loaded)
	return false;

    /*
    ** Unload first so we can start from scratch.
    */
    unload();

    /*
    ** Load click then fea module
    */
    if (!load(_click_module))
	return false;

    if (!load(_fea_module))
	return false;

    if (!mount())
	return false;

    loaded = true;

    return true;
}

bool
Click::unload(const string& fname)
{
    int cfd;
    string basename = string(fname, fname.find_last_of("/") + 1);

    if (-1 == (cfd = kldfind(basename.c_str()))) {
	return true;
    }

    if (-1 == kldunload(cfd)) {
	XLOG_ERROR("Unable to unload module %s: %s", basename.c_str(),
		   strerror(errno));

	close(cfd);
	return false;
    }

    close(cfd);

    return true;
}

/*
** Unload click.
*/
void
Click::unload()
{
    debug_msg("unload\n");

    if (!loaded)
	return;

    if (!unmount())
	return;

    if (!unload(_fea_module))
	return;

    if (!unload(_click_module))
	return;

    loaded = false;
}

bool
Click::is_loaded()
{
    return loaded;
}

/*
** Mount click.
*/
bool
Click::mount()
{
    debug_msg("mount %s\n", MOUNT_POINT);

    if (-1 == ::mount("click", MOUNT_POINT, 0, 0)) {
	XLOG_ERROR("Failed to mount click on %s", MOUNT_POINT);
	return false;
    }

    return true;
}

/*
** Unmount click.
*/
bool
Click::unmount()
{
    debug_msg("unmount %s\n", MOUNT_POINT);

    /*
    ** Check that its actually mounted before attempting the unmount.
    */
    int num = getfsstat(0, 0, MNT_NOWAIT);

    if (-1 == num)  {
	XLOG_ERROR("getfsstat failed: %s", strerror(errno));
	return false;
    }
    struct statfs *buf = new struct statfs[num];

    if (-1 != getfsstat(buf, sizeof(struct statfs) * num, MNT_NOWAIT)) {
	int i;
	struct statfs *tfs;
	for (i = 0, tfs = buf; i < num; i++, tfs++) {
	    if (0 == strcmp(tfs->f_mntonname, MOUNT_POINT)) {
		if (-1 == ::unmount(MOUNT_POINT, 0)) {
		    XLOG_ERROR("Failed to unmount click on %s: %s",
			       MOUNT_POINT, strerror(errno));
		    delete buf;
		    return false;
		}
		delete buf;
		return true;
	    }
	}
	delete buf;
	return true;
    } else {
	XLOG_ERROR("getfsstat failed: %s", strerror(errno));
	return false;
    }

    return true;
}

/*
** Install a new click configuration.
*/
bool
Click::configure(const string& config, bool sleep)
{
    /* XXX
    ** Click on FreeBSD is flakey loading a new config can cause a
    ** crash. So unload and load click. This code should be removed
    ** when click is more stable.
    **
    ** What should happen is that we should clear the last config
    ** first.
    ** bash$ > /click/config
    */
#if	0
    debug_msg("Unloading then loading click\n");

    unload();
    load();
#else
    debug_msg("Zapping current click config\n");

    {
	std::ofstream zap(_config_file);
	if (!zap) {
	    XLOG_ERROR("Could not open click config file %s: %s",
		       _config_file, strerror(errno));
	    return false;
	}
	zap.close();
	if (sleep) {
	    debug_msg("Sleeping\n");
	    ::sleep(1);
	}
    }
#endif

    if (0 == config.length()) {
	debug_msg("Zero length config\n");
	return true;
    }

    /*
    ** Open the config file and write in the config.
    */
    std::ofstream conf(_config_file);
    if (!conf) {
	XLOG_ERROR("Could not open click config file %s: %s",
		   _config_file, strerror(errno));
	return false;
    }

    /*
    ** Write out the config.
    */
    conf << config;
    if (!conf.good()) {
	XLOG_ERROR("Write of click config to %s failed: %s",
		   _config_file, strerror(errno));
	return false;
    }

    conf.close();
    if (!conf.good()) {
	XLOG_ERROR("Bad Click Configuration %s", config.c_str());
	return false;
    }

    return true;
}
