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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "rib_notifier_base.hh"

// ----------------------------------------------------------------------------
// RibNotifierBase<A> implementation

template <typename A>
RibNotifierBase<A>::RibNotifierBase(EventLoop&	 e,
				 UpdateQueue<A>& uq,
				 uint32_t	 ms)
    : _e(e), _uq(uq), _poll_ms(ms)
{
    _ri = _uq.create_reader();
}

template <typename A>
RibNotifierBase<A>::~RibNotifierBase()
{
    _uq.destroy_reader(_ri);
}

template <typename A>
void
RibNotifierBase<A>::start_polling()
{
    _t = _e.new_periodic(_poll_ms,
			 callback(this, &RibNotifierBase<A>::poll_updates));
}

template <typename A>
void
RibNotifierBase<A>::stop_polling()
{
    _t.unschedule();
}

template <typename A>
bool
RibNotifierBase<A>::poll_updates()
{
    if (_uq.get(_ri)) {
	updates_available();
    }
    return true;
}

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

template class RibNotifierBase<IPv4>;
template class RibNotifierBase<IPv6>;
