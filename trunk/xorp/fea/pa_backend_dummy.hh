// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/pa_backend_dummy.hh,v 1.7 2007/09/15 19:52:39 pavlin Exp $

#ifndef __FEA_PA_BACKEND_DUMMY_HH__
#define __FEA_PA_BACKEND_DUMMY_HH__

#include "pa_entry.hh"
#include "pa_table.hh"
#include "pa_backend.hh"

/* ------------------------------------------------------------------------- */

/**
 * @short Dummy ACL backend interface.
 *
 * Concrete class defining a backend which does nothing.
 */
class PaDummyBackend : public PaBackend {
    friend class Snapshot4;
public:
    PaDummyBackend() throw(PaInvalidBackendException);
    virtual ~PaDummyBackend();

protected:
    /* --------------------------------------------------------------------- */
    /*
     * @short State snapshot Memento classes.
     *
     * These are provider-specific and abstract. Attempting to instantiate
     * them directly will result in an exception being thrown.
     *
     * Be warned that they might not actually copy all the state in a form
     * which can be marshaled elsewhere. Each provider must implement
     * both of these classes and override the virtuals, and check that
     * snapshots passed to it are its own by using dynamic casts.
     */

    class Snapshot4 : public PaBackend::Snapshot4Base {
    public:
	Snapshot4()
	    throw(PaInvalidSnapshotException);
#ifdef notyet
	Snapshot4(const PaDummyBackend& parent)
	    throw(PaInvalidSnapshotException);
#endif
	Snapshot4(const Snapshot4&)
	    throw(PaInvalidSnapshotException);
	virtual ~Snapshot4();
    };

public:
    /* --------------------------------------------------------------------- */
    /* General back-end methods. */

    const char* get_name() const;
    const char* get_version() const;

    /* --------------------------------------------------------------------- */
    /* IPv4 ACL back-end methods. */

    int push_entries4(const PaSnapshot4* snap);
    int delete_all_entries4();
    const PaBackend::Snapshot4Base* create_snapshot4();
    int restore_snapshot4(const PaBackend::Snapshot4Base* snap);

#ifdef notyet
    /* --------------------------------------------------------------------- */
    /* IPv6 ACL back-end methods. */

    int push_entries6(const PaSnapshot6* snap);
    int delete_all_entries6();
    const PaBackend::Snapshot6Base* create_snapshot6();
    int restore_snapshot6(const PaBackend::Snapshot6Base* snap);
#endif
};

/* ------------------------------------------------------------------------- */

#endif // __FEA_PA_BACKEND_DUMMY_HH__
