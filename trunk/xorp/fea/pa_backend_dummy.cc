// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/pa_backend_dummy.cc,v 1.7 2007/02/16 22:45:47 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"
#include "pa_backend_dummy.hh"

/* ------------------------------------------------------------------------- */
/* Constructor and destructor. */

PaDummyBackend::PaDummyBackend()
    throw(PaInvalidBackendException)
{
}

PaDummyBackend::~PaDummyBackend()
{
}

/* ------------------------------------------------------------------------- */
/* General back-end methods. */

const char*
PaDummyBackend::get_name() const
{
    return ("dummy");
}

const char*
PaDummyBackend::get_version() const
{
    return ("0.1");
}

/* ------------------------------------------------------------------------- */
/* IPv4 ACL back-end methods. */

int
PaDummyBackend::push_entries4(const PaSnapshot4* snap)
{
    // Do nothing.
    UNUSED(snap);
    return (XORP_OK);
}

int
PaDummyBackend::delete_all_entries4()
{
    // Do nothing.
    return (XORP_OK);
}

const PaBackend::Snapshot4Base*
PaDummyBackend::create_snapshot4()
{
    return (new PaDummyBackend::Snapshot4());
}

int
PaDummyBackend::restore_snapshot4(const PaBackend::Snapshot4Base* snap4)
{
    // Simply check if the snapshot is an instance of our derived snapshot.
    // If it is not, we received it in error, and it is of no interest to us.
    const PaDummyBackend::Snapshot4* dsnap4 =
	dynamic_cast<const PaDummyBackend::Snapshot4*>(snap4);
    XLOG_ASSERT(dsnap4 != NULL);
    return (XORP_OK);
}

/* ------------------------------------------------------------------------- */
/* Snapshot scoped classes (Memento pattern) */

PaDummyBackend::Snapshot4::Snapshot4()
    throw(PaInvalidSnapshotException)
{
}

PaDummyBackend::Snapshot4::Snapshot4(
    const PaDummyBackend::Snapshot4& snap4)
    throw(PaInvalidSnapshotException)
    : PaBackend::Snapshot4Base()
{
    // Nothing to do.
    UNUSED(snap4);
}

PaDummyBackend::Snapshot4::~Snapshot4()
{
}

/* ------------------------------------------------------------------------- */
