// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_OBSERVER_RTMV2_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_OBSERVER_RTMV2_HH__

#include "fea/fibconfig_table_observer.hh"
#include "fea/data_plane/control_socket/windows_rtm_pipe.hh"

class WinRtmPipe;
class RtmV2Observer;


class FibConfigTableObserverRtmV2 : public FibConfigTableObserver {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigTableObserverRtmV2(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigTableObserverRtmV2();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);
    
    /**
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(vector<uint8_t>& buffer);

    virtual int notify_table_id_change(uint32_t new_tbl) {
	UNUSED(new_tbl);
	return 0;
    }

private:
    class RtmV2Observer : public WinRtmPipeObserver {
    public:
    	RtmV2Observer(WinRtmPipe& rs, int af,
		      FibConfigTableObserverRtmV2& rtmo)
	    : WinRtmPipeObserver(rs), _af(af), _rtmo(rtmo) {}
    	virtual ~RtmV2Observer() {}
	void routing_socket_data(vector<uint8_t>& buffer) {
	    _rtmo.receive_data(buffer);
	}
    private:
	int _af;
    	FibConfigTableObserverRtmV2& _rtmo;
    };

    WinRtmPipe*		_rs4;
    RtmV2Observer*	_rso4;
    WinRtmPipe*		_rs6;
    RtmV2Observer*	_rso6;
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_OBSERVER_RTMV2_HH__
