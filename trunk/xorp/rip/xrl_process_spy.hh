// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rip/xrl_process_spy.hh,v 1.11 2008/07/23 05:11:38 pavlin Exp $

#ifndef __RIP_XRL_PROCESS_SPY_HH__
#define __RIP_XRL_PROCESS_SPY_HH__

#include "libxorp/service.hh"

class XrlRouter;

/**
 * @short Class that watches remote FEA and RIB processes.
 *
 * This class registers interest with the Finder in the FEA and RIB
 * processes and reports whether the FEA and RIB are running to
 * interested parties.
 */
class XrlProcessSpy : public ServiceBase {
public:
    XrlProcessSpy(XrlRouter& rtr);
    ~XrlProcessSpy();

    /**
     * Register interest in FEA and RIB with Finder.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Deregister interest in FEA and RIB with Finder.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Get indication of whether FEA is present.
     *
     * @return true if FEA is present, false if FEA is not present or
     * @ref run_status() is not in SERVICE_RUNNING state.
     */
    bool fea_present() const;

    /**
     * Get indication of whether RIB is present.
     *
     * @return true if RIB is present, false if RIB is not present or
     * @ref run_status() is not in SERVICE_RUNNING state.
     */
    bool rib_present() const;

    /**
     * Inform instance about the birth of an Xrl Target instance
     * within a class.  Typically called by associated Xrl Target of
     * running RIP.
     *
     * @param class_name class of new born Xrl Target.
     * @param instance_name instance name of new born Xrl Target.
     */
    void birth_event(const string& class_name,
		     const string& instance_name);

    /**
     * Inform instance about the death of a Xrl Target instance within a class.
     * Typically called by associated Xrl Target of running RIP.
     *
     * @param class_name class of recently deceased Xrl Target.
     * @param instance_name instance name of recently deceased Xrl Target.
     */
    void death_event(const string& class_name,
		     const string& instance_name);

protected:
    void send_register(uint32_t idx);
    void register_cb(const XrlError& e, uint32_t idx);
    void schedule_register_retry(uint32_t idx);

    void send_deregister(uint32_t idx);
    void deregister_cb(const XrlError& e, uint32_t idx);
    void schedule_deregister_retry(uint32_t idx);

protected:
    static const uint32_t FEA_IDX = 0;
    static const uint32_t RIB_IDX = 1;
    static const uint32_t END_IDX = 2;

protected:
    XrlRouter&	_rtr;
    string	_cname[END_IDX];
    string	_iname[END_IDX];
    XorpTimer   _retry;
};

#endif // __RIP_XRL_PROCESS_SPY_HH__
