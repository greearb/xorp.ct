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

// $XORP: xorp/rip/xrl_process_spy.hh,v 1.2 2004/04/22 01:11:52 pavlin Exp $

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
     * @return true on success, false on failure.
     */
    bool startup();

    /**
     * Deregister interest in FEA and RIB with Finder.
     *
     * @return true on success, false on failure.
     */
    bool shutdown();

    /**
     * Get indication of whether FEA is present.
     *
     * @return true if FEA is present, false if FEA is not present or
     * @ref run_status() is not in RUNNING state.
     */
    bool fea_present() const;

    /**
     * Get indication of whether RIB is present.
     *
     * @return true if RIB is present, false if RIB is not present or
     * @ref run_status() is not in RUNNING state.
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
