// -*- c-basic-offset: 8; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2004 International Computer Science Institute
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

// $XORP: xorp/fea/firewall_dummy.hh,v 1.2 2004/09/01 23:47:15 bms Exp $

#ifndef __FEA_FIREWALL_DUMMY_HH__
#define __FEA_FIREWALL_DUMMY_HH__

#include "fea/firewall.hh"

/**
 * @short Dummy firewall provider.
 *
 * Concrete class defining a firewall provider which does nothing.
 */
class DummyFwProvider : public FwProvider {
public:
	DummyFwProvider(FirewallManager &m)
	    throw(InvalidFwProvider)
	    : FwProvider(m),
	      _initialized(true), _enabled(false),
	      _numrules4(0), _numrules6(0) {}

	virtual ~DummyFwProvider() {
		_enabled = false;
	}

	// General provider interface

	inline bool get_enabled() const {
		return (_enabled);
	}

	inline int set_enabled(bool enabled) {
		_enabled = enabled;
		return (XORP_OK);
	}

	inline const char* get_provider_name() const {
		return ("dummy");
	}

	inline const char* get_provider_version() const {
		return ("0.1");
	}

	// IPv4 firewall provider interface

	inline int add_rule4(FwRule4& rule) {
		UNUSED(rule);
		_numrules4++;
		return (XORP_OK);
	}

	inline int delete_rule4(FwRule4& rule) {
		UNUSED(rule);
		--_numrules4;
		return (XORP_OK);
	}

	inline uint32_t get_num_xorp_rules4() const {
		return (_numrules4);
	}

	inline uint32_t get_num_system_rules4() const {
		// Always return 0, as there are no system rules
		// managed or provided by a dummy implementation.
		return (0);
	}

	inline int add_rule6(FwRule6& rule) {
		UNUSED(rule);
		_numrules6++;
		return (XORP_OK);
	}

	inline int delete_rule6(FwRule6& rule) {
		UNUSED(rule);
		--_numrules6;
		return (XORP_OK);
	}

	inline uint32_t get_num_xorp_rules6() const {
		return (_numrules6);
	}

	inline uint32_t get_num_system_rules6() const {
		// Always return 0, as there are no system rules
		// managed or provided by a dummy implementation.
		return (0);
	}

	// XXX: What about rule retrieval?

private:
	bool		_initialized;
	bool		_enabled;
	uint32_t	_numrules4;
	uint32_t	_numrules6;
};

#endif // __FEA_FIREWALL_DUMMY_HH__
