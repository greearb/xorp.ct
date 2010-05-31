// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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
    _t = _e.new_periodic_ms(_poll_ms,
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


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
#include "libxorp/ipv4.hh"
template class RibNotifierBase<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
#include "libxorp/ipv6.hh"
template class RibNotifierBase<IPv6>;
#endif
