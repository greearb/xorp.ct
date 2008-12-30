/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/contrib/win32/xorprtm/xorprtm.c,v 1.6 2008/10/02 02:37:43 pavlin Exp $"

/* XXX: SORT FUNCTIONS IN THIS FILE */

/* XXX: RATIONALIZE INCLUDES */

#include "pchsample.h"
#include <iphlpapi.h>
#pragma hdrstop

/* XXX: move to headers */
typedef union _sockunion_t {
    struct sockaddr	    sa;
    struct sockaddr_in	    sin;
    struct sockaddr_in6	    sin6;
    struct sockaddr_storage ss;
} sockunion_t;

/* XXX: move to headers */
#define XORPRTM_PIPETIMEOUT 2000 /* 2 seconds */
#define XORPRTM_RI_PREF	    1
#define XORPRTM_RI_METRIC   20

/* XXX: run cproto */
/* XXX: move to headers */
pipe_instance_t *pipe_new(void);
void pipe_destroy(pipe_instance_t *pp);
int pipe_listen(pipe_instance_t *pp);
void pipe_disconnect(pipe_instance_t *pp);
void CALLBACK pipe_connect_cb(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
void CALLBACK pipe_read_cb(PVOID lpParameter, BOOLEAN TimerOrWaitFired);
void WINAPI pipe_reread_cb(void *ctx);
void WINAPI pipe_relisten_cb(void *ctx);
DWORD APIENTRY RTM_CallbackEvent (RTM_ENTITY_HANDLE hRtmHandle, RTM_EVENT_TYPE   retEvent, PVOID pvContext1, PVOID pvContext2);
int rtm_add_route(struct rt_msghdr *rtm, int msgsize);
void broadcast_pipe_message(void *msg, int msgsize);

/*
 * XXX: The only global variable.
 */
/* XXX: move to headers */
CONFIGURATION_ENTRY g_ce;

/*
 * Issue a routing socket message for a single changed destination.
 */
DWORD
rtm_send_dest_change(RTM_ENTITY_HANDLE reh, PRTM_DEST_INFO prdi)
{
#ifdef IPV6_DLL
    struct in6_addr dst;
    struct in6_addr ip;
    struct in6_addr nhip;
#else
    struct in_addr dst;
    struct in_addr ip;
    struct in_addr nhip;
#endif
    int i;
    int	dstprefix;
    int nhprefix;
    int type;
    DWORD result;

    if (!prdi)
        return NO_ERROR;

    TRACE1(NETWORK, "RtmDestInfo Destination %p", prdi);

#ifdef IPV6_DLL
    RTM_IPV6_GET_ADDR_AND_LEN(dst.s6_addr, dstprefix, &prdi->DestAddress);
#else
    RTM_IPV4_GET_ADDR_AND_LEN(dst.s_addr, dstprefix, &prdi->DestAddress);
#endif

    /*
     * Determine the nature of the change; whether a route has
     * been added, changed or deleted for the given situation.
     * We look only at the unicast routing view.
     */
    for (i = 0; i < prdi->NumberOfViews; i++) {
    	if (prdi->ViewInfo[i].ViewId == RTM_VIEW_ID_UCAST) {
#ifdef IPV6_DLL
	    /*
	     * XXX: Don't filter IPv6 routes [yet].
	     */
#else	/* IPv4 */
	    /*
	     * Ignore routes to the all-ones broadcast destination.
	     */
	    if ((dst.s_addr == INADDR_BROADCAST && dstprefix == 32)) {
		TRACE0(NETWORK, "ignoring all-ones broadcast");
		break;
	    }
#ifdef notyet
	    /*
	     * XXX: Ignore multicast routes (for now).
	     */
	    if (IN4_IS_ADDR_MULTICAST(dst.s_addr)) {
		TRACE0(NETWORK, "ignoring multicast route");
		break;
	    }
#endif /* notyet */
#endif /* IPV6_DLL */
	    if (prdi->ViewInfo[i].NumRoutes == 0) {
		TRACE0(NETWORK, "route deleted");
 		type = RTM_DELETE;
	    } else if (prdi->ViewInfo[i].NumRoutes == 1) {
		TRACE0(NETWORK, "route added");
 		type = RTM_ADD;
	    } else {
		/*
		 * XXX: The route has multiple next-hops. We do not know
		 * which next-hop we should send to the FEA, so do not
		 * process such changes for now.
		 */
		TRACE1(NETWORK, "route change, dest %d nexthops, no msg",
			prdi->ViewInfo[i].NumRoutes);
		type = 0;
	    }
	    break;  /* stop when unicast route view is dealt with. */
	}
    }
    /*
     * Craft a routing socket message based on the changes.
     * We only allocate memory here if we require it.
     */
    if (type != 0) {
	sockunion_t *sa;
	struct rt_msghdr *rtm;
#ifdef IPV6_DLL
	struct in6_addr nh;
#else
	struct in_addr nh;
#endif
	int maxmsgsize;

	maxmsgsize = sizeof(struct rt_msghdr) + (sizeof(sockunion_t) * 3);
	rtm = malloc(maxmsgsize);
	ZeroMemory(rtm, maxmsgsize);

	sa = (sockunion_t *)(rtm + 1);

 	rtm->rtm_msglen = maxmsgsize - sizeof(*sa);
 	rtm->rtm_version = RTM_VERSION;
 	rtm->rtm_type = type;
 	rtm->rtm_addrs = RTA_DST | RTA_NETMASK;

	/* Destination */
#ifdef IPV6_DLL
	sa->sin6.sin6_family = AF_INET6;
	sa->sin6.sin6_addr = dst;
#else
	sa->sin.sin_family = AF_INET;
	sa->sin.sin_addr = dst;
#endif

	/*
	 * Route additions require that we also report the next-hop.
	 * Perform the necessary RTMv2 incantations to look up the
	 * next-hop from the destination reported as changed.
	 * XXX: Better error checking here considered desirable.
	 */
	if (type == RTM_ADD) {
	    PRTM_ROUTE_INFO prri;
	    RTM_NEXTHOP_INFO nhi;

	    rtm->rtm_msglen += sizeof(*sa);
	    rtm->rtm_addrs |= RTA_GATEWAY;

	    /* XXX weird heap malloc. */
	    MALLOC(&prri,
RTM_SIZE_OF_ROUTE_INFO(g_ce.rrpRtmProfile.MaxNextHopsInRoute), &result);

	    result = RtmGetRouteInfo(reh, prdi->ViewInfo[i].Route, prri, NULL);
	    if (result != NO_ERROR) {
		TRACE1(NETWORK, "RtmGetRouteInfo() returns %d", result);
	    }

	    result = RtmGetNextHopInfo(reh, prri->NextHopsList.NextHops[0],
				       &nhi);
	    if (result != NO_ERROR) {
		TRACE1(ANY, "Error %u getting next hop", result);
	    }

	    /* Gateway */
#ifdef IPV6_DLL
	    RTM_IPV6_GET_ADDR_AND_LEN(nhip.s6_addr, nhprefix,
				      &nhi.NextHopAddress);
	    ++sa;
	    sa->sin6.sin6_family = AF_INET6;
	    sa->sin6.sin6_addr = nhip;
#else
	    RTM_IPV4_GET_ADDR_AND_LEN(nhip.s_addr, nhprefix,
				      &nhi.NextHopAddress);
	    ++sa;
	    sa->sin.sin_family = AF_INET;
	    sa->sin.sin_addr = nhip;
#endif /* IPV6_DLL */

	    /*
	     * Free the next-hop info structures.
	     */
	    (void)RtmReleaseNextHopInfo(reh, &nhi);
	    (void)RtmReleaseRouteInfo(reh, prri);
	    FREE(prri);
	}

	/* Netmask; comes after gateway in the RTM_ADD case. */
	++sa;
#ifdef IPV6_DLL
	/* XXX: may not be right */
	sa->sin6.sin6_family = AF_INET;
	sa->sin6.sin6_addr.s6_addr = RTM_IPV6_MASK_FROM_LEN(dstprefix);
#else
	sa->sin.sin_family = AF_INET;
	sa->sin.sin_addr.s_addr = RTM_IPV4_MASK_FROM_LEN(dstprefix);
#endif

	broadcast_pipe_message(rtm, rtm->rtm_msglen);
	free(rtm);
    }

    return NO_ERROR;    
}

/*
 * Send a message to all connected listeners.
 *
 * XXX: The write blocks the current thread. Because RRAS threads
 * never enter alertable wait state, we can't use WriteFileEx().
 * We must either block or do the additional accounting for overlapped
 * WriteFile().
 * This has a very important consequence: our thread blocks until the
 * client thread reads its data or the pipe is disconnected.
 */
void
broadcast_pipe_message(void *msg, int msgsize)
{
    pipe_instance_t *pp;
    int i;
    int result;
    int nbytes;

    for (i = 0; i < PIPE_INSTANCES; i++) {
	pp = g_ce.pipes[i];
	if (pp != NULL && pp->state == PIPE_STATE_CONNECTED) {
	    result = WriteFile(pp->pipe, msg, msgsize, &nbytes, NULL);
	    if (result == 0) {
		result = GetLastError();
		TRACE1(NETWORK, "broadcast: write error %d", result);
		if (result == ERROR_PIPE_NOT_CONNECTED ||
		    result == ERROR_NO_DATA ||
		    result == ERROR_BROKEN_PIPE) {
		    TRACE1(NETWORK,
"broadcast: pipe %p disconnected; reconnecting.", pp->pipe);
		    /*
		     * We may be called by a reader thread. To avoid
		     * introducing loops, we schedule the listen
		     * operation on another thread.
		     */
		    ResetEvent(pp->revent);
		    QueueUserWorkItem(
(LPTHREAD_START_ROUTINE)pipe_relisten_cb, (PVOID)pp, WT_EXECUTEINIOTHREAD);
		}
	    }
	}
    }
}


DWORD
ProcessRouteChange (VOID)
{
    DWORD           dwErr           = NO_ERROR;
    RTM_DEST_INFO   rdiDestination;             /* 1 view registered for change */
    BOOL            bDone           = FALSE;
    UINT            uiNumDests;
    
    if (!ENTER_XORPRTM_API()) { return ERROR_CAN_NOT_COMPLETE; }

    /* loop dequeueing messages until RTM says there are no more left */
    while (!bDone)
    {
        /* retrieve route changes */
        uiNumDests = 1;
        dwErr = RtmGetChangedDests(
            g_ce.hRtmHandle,                    /* my RTMv2 handle  */
            g_ce.hRtmNotificationHandle,        /* my notification handle  */
            &uiNumDests,                        /*   # dest info's required */
                                                /* g # dest info's supplied */
            &rdiDestination);                   /* g buffer for dest info's */

        switch (dwErr)
        {
            case ERROR_NO_MORE_ITEMS:
                bDone = TRUE;
                dwErr = NO_ERROR;
                if (uiNumDests < 1)
                    break;
                /* else continue below to process the last destination */

/* XXX: Does not specify what the change(s) are, just that they */
/* occurred, on *this destination*. maybe we should figure */
/* this out? */

            case NO_ERROR:
                rtm_send_dest_change(g_ce.hRtmHandle, &rdiDestination);
                
                /* release the destination info */
                if (RtmReleaseChangedDests(
                    g_ce.hRtmHandle,            /* my RTMv2 handle  */
                    g_ce.hRtmNotificationHandle,/* my notif handle  */
                    uiNumDests,                 /* 1 */
                    &rdiDestination             /* released dest info */
                    ) != NO_ERROR)
                    TRACE0(NETWORK, "Error releasing changed dests");

                break;

            default:
                bDone = TRUE;
                TRACE1(NETWORK, "Error %u RtmGetChangedDests", dwErr);
                break;
        }
    } /* while  */
    
    LEAVE_XORPRTM_API();

    return dwErr;
}


/*
 * Where we get called by RTMv2 when things happen to the routing table.
 */
DWORD
APIENTRY
RTM_CallbackEvent (
     RTM_ENTITY_HANDLE   hRtmHandle, /* registration handle */
     RTM_EVENT_TYPE      retEvent,
     PVOID               pvContext1,
     PVOID               pvContext2)
{
    DWORD dwErr = NO_ERROR;

    TRACE1(ENTER, "Entering RTM_CallbackEvent: %u", retEvent);

    do                          /* breakout loop */
    {
        UNREFERENCED_PARAMETER(hRtmHandle);
        UNREFERENCED_PARAMETER(pvContext1);
        UNREFERENCED_PARAMETER(pvContext2);

        /* only route change notifications are processed */
        if (retEvent != RTM_CHANGE_NOTIFICATION)
        {
            dwErr = ERROR_NOT_SUPPORTED;
            break;
        }

        dwErr = ProcessRouteChange();
    } while (FALSE);

    TRACE0(LEAVE, "Leaving  RTM_CallbackEvent");

    return dwErr;
}


/*
 * Create a new instance of a pipe and return a pointer to
 * its instance structure.
 */
pipe_instance_t *
pipe_new(void)
{
    pipe_instance_t *npp;
    int failed;
    DWORD result;

    TRACE0(ENTER, "Entering pipe_new");

    npp = malloc(sizeof(*npp));
    if (npp == NULL)
	return NULL;
    ZeroMemory(npp, sizeof(*npp));

    failed = 1;

    /* XXX buffer management */
    npp->rsize = PIPE_READBUF_SIZE; 
    npp->state = PIPE_STATE_INIT;

    InitializeCriticalSection(&npp->rcs);

    /*
     * Create the event object used to signal connection completion.
     */
    npp->cevent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (npp->cevent == NULL) {
	result = GetLastError();
        TRACE1(CONFIGURATION, "Error %u creating event", result);
	goto fail;
    }
    npp->cov.hEvent = npp->cevent;

    /*
     * Create the event object used to signal read completion.
     */
    npp->revent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (npp->revent == NULL) {
	result = GetLastError();
        TRACE1(CONFIGURATION, "Error %u creating event", result);
	goto fail;
    }
    npp->rov.hEvent = npp->revent;

    /*
     * Create the instance of the named pipe itself.
     */
    npp->pipe = CreateNamedPipeA(XORPRTM_PIPENAME,
				 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
				 PIPE_TYPE_MESSAGE, PIPE_INSTANCES, 0, 0,
				 XORPRTM_PIPETIMEOUT, NULL);
    if (npp->pipe == NULL) {
	result = GetLastError();
	TRACE1(CONFIGURATION, "Error %u creating named pipe", result);
	goto fail;
    }

    failed = 0;
fail:
    if (failed) {
	pipe_destroy(npp);
	npp = NULL;
    }
    TRACE1(ENTER, "Leaving pipe_new %p", npp);
    return (npp);
}

/*
 * XXX: This must be called from primary thread, or lock held if not!
 */
int
pipe_listen(pipe_instance_t *pp)
{
    int retval;
    DWORD result;

    retval = -1;

    TRACE1(ENTER, "Entering pipe_listen %p", pp);

    if (pp == NULL || pp->state != PIPE_STATE_INIT)
	return (retval);

    /*
     * Register a pool thread to wait for pipe connection.
     * Clear event state to avoid spurious signals.
     */
    ResetEvent(pp->cevent);
    result = RegisterWaitForSingleObject(&(pp->cwait), pp->cevent,
					 pipe_connect_cb, pp, INFINITE,
					 WT_EXECUTEINIOTHREAD |
					 WT_EXECUTEONLYONCE);
    if (result == 0) {
	result = GetLastError();
	TRACE1(CONFIGURATION, "Error %u RegisterWaitForSingleObject()", result);
	goto fail;
    }

    /*
     * Register a pool thread to wait for data to be received on the pipe.
     * We don't cause this to be activated until we post a read request
     * from within the connection callback.
     * XXX: We want the read callback to be called whenever the
     * object is signalled, not just once.
     */
    ResetEvent(pp->revent);
    result = RegisterWaitForSingleObject(&(pp->rwait), pp->revent,
					 pipe_read_cb, pp, INFINITE,
					 WT_EXECUTEINIOTHREAD |
					 WT_EXECUTEONLYONCE);
    if (result == 0) {
	result = GetLastError();
	TRACE1(CONFIGURATION, "Error %u RegisterWaitForSingleObject()", result);
	goto fail;
    }

    /*
     * Post the connection request. If it returns non-zero, then the
     * connection attempt is pending and the thread will be signalled
     * when complete. If it returns zero, then there's a problem. 
     * ERROR_NO_DATA means the client disconnected, but we didn't
     * call DisconnectNamedPipe().
     * ConnectNamedPipe() does not reset the event object associated
     * with the OVERLAPPED parameter.
     */
    result = ConnectNamedPipe(pp->pipe, &pp->cov);
    if (result == 0) {
	result = GetLastError();
	if (result == ERROR_PIPE_LISTENING) {
	    TRACE0(NETWORK, "Error: listening; Reconnecting named pipe");
	    result = ConnectNamedPipe(pp->pipe, &pp->cov);
	}
	if (result == ERROR_PIPE_CONNECTED) {
	    TRACE0(NETWORK, "Error: named pipe already connected");
	    goto fail;
	}
	if (result == ERROR_NO_DATA) {
	    TRACE0(NETWORK, "Error: previous session not cleaned up");
	    goto fail;
	}
    }

    pp->state = PIPE_STATE_LISTEN;

    retval = 0;
fail:
    if (retval == -1) {
	if (pp->cwait != NULL) {
	    UnregisterWaitEx(pp->cwait, pp->cevent);
	    ResetEvent(pp->cevent);
	    pp->cwait = NULL;
	}
	if (pp->rwait != NULL) {
	    UnregisterWaitEx(pp->rwait, pp->revent);
	    ResetEvent(pp->revent);
	    pp->rwait = NULL;
	}
    }
    TRACE1(ENTER, "Leaving pipe_listen", pp);
    return (retval);
}

/*
 * Disconnect, but do not close, a pipe handle; and deregister
 * any pending waiter threads from its event handles.
 *
 * XXX: This must be called from primary thread, or lock held if not!
 */
void
pipe_disconnect(pipe_instance_t *pp)
{

    TRACE0(ENTER, "Entering pipe_disconnect");

    if (pp == NULL)
	return;
    /*
     * Cancel pending I/O before deregistering the callback,
     * and disconnect the pipe, to avoid race conditions.
     * We also reset the event(s) to avoid being signalled for
     * things which haven't actually happened yet.
     *
     * XXX: To avoid races during shutdown, we may have to
     * NULL out the second argument to UnregisterWaitEx().
     * We can't, however, do that from a service thread.
     */
    if (pp->cwait != NULL) {
        UnregisterWaitEx(pp->cwait, pp->cevent);
	ResetEvent(pp->cevent);
	pp->cwait = NULL;
    }
    if (pp->rwait != NULL) {
        UnregisterWaitEx(pp->rwait, pp->revent);
	ResetEvent(pp->revent);
	pp->rwait = NULL;
    }

    if (pp->pipe != NULL) {
        CancelIo(pp->pipe);
	if (pp->state == PIPE_STATE_CONNECTED ||
	    pp->state == PIPE_STATE_LISTEN) {
	    DisconnectNamedPipe(pp->pipe);
	}
    }

    pp->state = PIPE_STATE_INIT;

    TRACE0(ENTER, "Leaving pipe_disconnect");
}

void
pipe_destroy(pipe_instance_t *pp)
{

    TRACE0(ENTER, "Leaving pipe_destroy");

    if (pp == NULL)
	return;

    pipe_disconnect(pp);

    if (pp->revent != NULL) {
	CloseHandle(pp->revent);
	pp->rov.hEvent = pp->revent = NULL;
    }
    if (pp->cevent != NULL) {
	CloseHandle(pp->cevent);
	pp->cov.hEvent = pp->cevent = NULL;
    }
    if (pp->pipe != NULL) {
	CloseHandle(pp->pipe);
	pp->pipe = NULL;
    }

    DeleteCriticalSection(&pp->rcs);

    free(pp);

    TRACE0(ENTER, "Leaving pipe_destroy");
}

void CALLBACK
pipe_connect_cb(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    pipe_instance_t *pp;
    DWORD result;
    DWORD nbytes;

    pp = (pipe_instance_t *)lpParameter;
    EnterCriticalSection(&pp->rcs);
    TRACE1(ENTER, "Entering pipe_connect_cb %p", lpParameter);

    /* XXX CHECK STATE */

    if (pp->state != PIPE_STATE_LISTEN) {
	TRACE0(NETWORK, "WARNING: pipe state is not LISTEN");
    }

    /*
     * If you forgot to reset the event object, the following call
     * will block until the connection is actually made. We wish to
     * run as lockless as possible, so do not block the service thread.
     */
    /*
    result = GetOverlappedResult(pp->pipe, &pp->cov, &nbytes, TRUE);
    */

    pp->state = PIPE_STATE_CONNECTED;

    /*
     * Post an overlapped read request to capture a message from
     * the client.
     */
    result = ReadFile(pp->pipe, pp->rbuf, pp->rsize, NULL, &pp->rov);
    if (result == 0) {
	result = GetLastError();
	if (result != ERROR_IO_PENDING) {
	    TRACE1(ANY, "WARNING: pipe_connect_cb read returned %d", result);
	}
    }

    /* XXX: We need to be able to deal with errors immediately to
     * avoid races. */
    TRACE0(ENTER, "Leaving pipe_connect_cb");
    LeaveCriticalSection(&pp->rcs);
}

/*
 * Callback which invokes pipe_listen().
 *
 * When we are in pipe_read_cb(), we may try to call pipe_listen()
 * (after tearing down an old connection). This can cause an infinite
 * loop as they execute in the same helper thread, and pipe_listen()
 * will try to reschedule pipe_read_cb().
 * Therefore, use QueueUserWorkItem() to make sure that pipe_listen()
 * is invoked after a context switch.
 */
void WINAPI
pipe_relisten_cb(void *ctx)
{
    pipe_instance_t *pp;

    pp = (pipe_instance_t *)ctx;
    EnterCriticalSection(&pp->rcs);
    TRACE1(ENTER, "Entering pipe_relisten_cb %p", ctx);

    pipe_disconnect(pp);
    pipe_listen(pp);

    TRACE0(ENTER, "Leaving pipe_relisten_cb");
    LeaveCriticalSection(&pp->rcs);
}

void WINAPI
pipe_reread_cb(void *ctx)
{
    pipe_instance_t *pp;
    DWORD result;
    int failed;

    pp = (pipe_instance_t *)ctx;
    EnterCriticalSection(&pp->rcs);
    TRACE1(ENTER, "Entering pipe_reread_cb %p", ctx);

    failed = 0;

    if (pp->state != PIPE_STATE_CONNECTED) {
	TRACE0(NETWORK, "WARNING: not PIPE_STATE_CONNECTED");
    }

    /*
     * Tear down and wire up read thread callback again.
     * This is probably inefficient.
     */
    UnregisterWaitEx(pp->rwait, pp->revent);
    ResetEvent(pp->revent); /* XXX ReadFile() should do this for us? */
    pp->rwait = NULL;
    /*
     * Post a new read request. Deal with fatal errors.
     */
    result = ReadFile(pp->pipe, pp->rbuf, pp->rsize, NULL, &pp->rov);
    if (result == 0) {
	result = GetLastError();
	if (result != ERROR_IO_PENDING) {
	    TRACE1(ANY, "WARNING: pipe_reread_cb read returned %d", result);
	}
	if (result == ERROR_BROKEN_PIPE) {
	    failed = 1;
	    goto fail;
	}
    }
    /*
     * Now, and only now, do we kick off the read thread, in order
     * to avoid being preempted if the client disconnects.
     */
    result = RegisterWaitForSingleObject(&(pp->rwait), pp->revent,
					 pipe_read_cb, pp, INFINITE,
					 WT_EXECUTEINIOTHREAD |
					 WT_EXECUTEONLYONCE);
    if (result == 0) {
	result = GetLastError();
	TRACE1(CONFIGURATION, "Error %u RegisterWaitForSingleObject()", result);
	failed = 1;
    }

fail:
    /*
     * If a fatal error occurred, disconnect the pipe client, and
     * listen for a new connection on this instance.
     */
    if (failed) {
	ResetEvent(pp->revent);
	QueueUserWorkItem(
(LPTHREAD_START_ROUTINE)pipe_relisten_cb, (PVOID)pp, WT_EXECUTEINIOTHREAD);
    }
out:
    TRACE0(ENTER, "Leaving pipe_reread_cb");
    LeaveCriticalSection(&pp->rcs);
}

void CALLBACK
pipe_read_cb(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    struct rt_msghdr *rtm;
    pipe_instance_t *pp;
    DWORD result;
    DWORD nbytes;

    pp = (pipe_instance_t *)lpParameter;
    EnterCriticalSection(&pp->rcs);
    TRACE1(ENTER, "Entering pipe_read_cb %p", lpParameter);

    if (pp->state != PIPE_STATE_CONNECTED) {
	TRACE0(NETWORK, "WARNING: not PIPE_STATE_CONNECTED, bailing.");
	/*
	 * XXX: Is something racy, or is it just me?
	 * Try to avoid deadlocking by returning if we
	 * got called when we weren't connected.
	 */
	goto out;
    }

    result = GetOverlappedResult(pp->pipe, &pp->rov, &nbytes, TRUE);
    if (result == 0) {
	result = GetLastError();
	TRACE1(NETWORK, "WARNING: pipe_read_cb read returned %d", result);
	if (result == ERROR_BROKEN_PIPE) {
	    /*
	     * We must queue the new listen on a separate thread to
	     * avoid infinite recursion.
	     */
	    TRACE0(NETWORK, "Posting listen again.");
	    ResetEvent(pp->revent);
	    QueueUserWorkItem(
(LPTHREAD_START_ROUTINE)pipe_relisten_cb, (PVOID)pp, WT_EXECUTEINIOTHREAD);
	    goto out;
	}
    }

    TRACE1(NETWORK, "Read %d bytes from named pipe.", nbytes);

    /*
     * Perform sanity checks on input message.
     * XXX: We should use a more appropriate errno value.
     * We use -1 as ENOBUFS, etc are not part of the namespace.
     */
    rtm = (struct rt_msghdr *)&pp->rbuf[0];
    if (rtm->rtm_version != RTM_VERSION) {
	TRACE1(NETWORK, "Invalid rtm_version %d, dropping.", rtm->rtm_version);
	goto drop;
    }
    /*
     * Sanity check size.
     */
    if (rtm->rtm_msglen > nbytes ||
	nbytes < sizeof(struct rt_msghdr)) {
	TRACE1(NETWORK, "Invalid rtm_msglen %d, dropping.", rtm->rtm_msglen);
	rtm->rtm_errno = -1;
	goto drop;
    }
    if (rtm->rtm_pid == 0) {
	TRACE1(NETWORK, "Invalid rtm_pid %d, dropping.", rtm->rtm_pid);
	rtm->rtm_errno = -1;
	goto bounce;
    }

    switch (rtm->rtm_type) {
    case RTM_ADD:
	result = rtm_add_route(rtm, nbytes);
	if (result == 0) {
	    TRACE0(NETWORK, "route added successfully");
	} else {
	    TRACE0(NETWORK, "failed to add route");
	}
	rtm->rtm_errno = result;
	break;

    case RTM_DELETE:
	result = rtm_delete_route(rtm, nbytes);
	if (result == 0) {
	    TRACE0(NETWORK, "route deleted successfully");
	} else {
	    TRACE0(NETWORK, "failed to delete route");
	}
	rtm->rtm_errno = result;
	break;

    default:
	TRACE1(NETWORK, "Invalid rtm_type %d, dropping.", rtm->rtm_type);
	rtm->rtm_errno = -1;
	break;
    }

bounce:
    /*
     * There is currently no analogue of the BSD SO_LOOPBACK option.
     * XXX: Normally processes will hear their own messages echoed across
     * the routing socket emulation pipe. Because the broadcast technique
     * uses blocking NT I/O, processes must read back their own message
     * after issuing it.
     */
    broadcast_pipe_message(pp->rbuf, nbytes);
drop:
    TRACE0(NETWORK, "Posting read again.");
    ResetEvent(pp->revent);
    QueueUserWorkItem(
(LPTHREAD_START_ROUTINE)pipe_reread_cb, (PVOID)pp, WT_EXECUTEINIOTHREAD);

out:
    TRACE0(ENTER, "Leaving pipe_read_cb");
    LeaveCriticalSection(&pp->rcs);
}


static
VOID
FreeEventEntry (
    PQUEUE_ENTRY        pqeEntry)
{
    EE_Destroy(CONTAINING_RECORD(pqeEntry, EVENT_ENTRY, qeEventQueueLink));
}
               


DWORD
EE_Create (
    ROUTING_PROTOCOL_EVENTS rpeEvent,
    MESSAGE                 mMessage,
    PEVENT_ENTRY            *ppeeEventEntry)
{
    DWORD               dwErr = NO_ERROR;
    PEVENT_ENTRY        peeEntry; /* scratch */

    /* validate parameters */
    if (!ppeeEventEntry)
        return ERROR_INVALID_PARAMETER;

    *ppeeEventEntry = NULL;

    /* allocate the interface entry structure */
    MALLOC(&peeEntry, sizeof(EVENT_ENTRY), &dwErr);
    if (dwErr != NO_ERROR)
        return dwErr;

    /* initialize various fields */
    InitializeQueueHead(&(peeEntry->qeEventQueueLink));
    
    peeEntry->rpeEvent = rpeEvent;
    peeEntry->mMessage = mMessage;

    *ppeeEventEntry = peeEntry;
    return dwErr;
}



DWORD
EE_Destroy (
    PEVENT_ENTRY            peeEventEntry)
{
    if (!peeEventEntry)
        return NO_ERROR;

    FREE(peeEventEntry);
    
    return NO_ERROR;
}


DWORD
EnqueueEvent(
    ROUTING_PROTOCOL_EVENTS rpeEvent,
    MESSAGE                 mMessage)
{
    DWORD           dwErr = NO_ERROR;
    PEVENT_ENTRY    peeEntry = NULL;

    dwErr = EE_Create(rpeEvent, mMessage, &peeEntry); 
    /* destroyed in EE_DequeueEvent */
    
    if (dwErr == NO_ERROR)
    {
        ACQUIRE_QUEUE_LOCK(&(g_ce.lqEventQueue));
        
        Enqueue(&(g_ce.lqEventQueue.head), &(peeEntry->qeEventQueueLink));

        RELEASE_QUEUE_LOCK(&(g_ce.lqEventQueue));
    }
    
    return dwErr;
}

DWORD
DequeueEvent(
    ROUTING_PROTOCOL_EVENTS  *prpeEvent,
    MESSAGE                  *pmMessage)
{
    DWORD           dwErr   = NO_ERROR;
    PQUEUE_ENTRY    pqe     = NULL;
    PEVENT_ENTRY    pee     = NULL;

    ACQUIRE_QUEUE_LOCK(&(g_ce.lqEventQueue));

    do {
        if (IsQueueEmpty(&(g_ce.lqEventQueue.head))) {
            dwErr = ERROR_NO_MORE_ITEMS;
            TRACE0(CONFIGURATION, "No events in the queue.");
            break;
        }

        pqe = Dequeue(&(g_ce.lqEventQueue.head));
        pee = CONTAINING_RECORD(pqe, EVENT_ENTRY, qeEventQueueLink);
        *(prpeEvent) = pee->rpeEvent;
        *(pmMessage) = pee->mMessage;

        /* created in EE_EnqueueEvent */
        EE_Destroy(pee);
        pee = NULL;
    } while (FALSE);

    RELEASE_QUEUE_LOCK(&(g_ce.lqEventQueue));
        
    return dwErr;
}   

DWORD
CE_Create (
    PCONFIGURATION_ENTRY    pce)
{
    DWORD dwErr = NO_ERROR;

    /* initialize to default values */
    ZeroMemory(pce, sizeof(CONFIGURATION_ENTRY));
    pce->dwTraceID = INVALID_TRACEID;
    
    do {
        /* initialize the read-write lock */
        CREATE_READ_WRITE_LOCK(&(pce->rwlLock));
        if (!READ_WRITE_LOCK_CREATED(&(pce->rwlLock))) {
            dwErr = GetLastError();
            
            TRACE1(CONFIGURATION, "Error %u creating read-write-lock", dwErr);

            break;
        }

        /* initialize the global heap */
        pce->hGlobalHeap = HeapCreate(0, 0, 0);
        if (pce->hGlobalHeap == NULL) {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u creating global heap", dwErr);

            break;
        }

	/*
         * Initialize the count of threads that are active in subsystem.
         * Create the semaphore released by each thread when it is done;
         * required for clean stop to the protocol.
	 */
        pce->ulActivityCount = 0;
        pce->hActivitySemaphore = CreateSemaphore(NULL, 0, 0xfffffff, NULL);
        if (pce->hActivitySemaphore == NULL) {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u creating semaphore", dwErr);
            break;
        }

        /* Logging & Tracing Information */
        pce->dwTraceID  = TraceRegister(XORPRTM_TRACENAME);

        /* Event Queue */
        INITIALIZE_LOCKED_QUEUE(&(pce->lqEventQueue));
        if (!LOCKED_QUEUE_INITIALIZED(&(pce->lqEventQueue))) {
            dwErr = GetLastError();
            TRACE1(CONFIGURATION, "Error %u initializing locked queue", dwErr);
            break;
        }

        /* Protocol State */
        pce->iscStatus = XORPRTM_STATUS_STOPPED;
        
    } while (FALSE);

    if (dwErr != NO_ERROR) {
        /* something went wrong, so cleanup. */
        TRACE0(CONFIGURATION, "Failed to create configuration entry");
        CE_Destroy(pce);
    }
    
    return dwErr;
}

DWORD
CE_Destroy (
    PCONFIGURATION_ENTRY    pce)
{
    /* Event Queue */
    if (LOCKED_QUEUE_INITIALIZED(&(pce->lqEventQueue)))
        DELETE_LOCKED_QUEUE((&(pce->lqEventQueue)), FreeEventEntry);
    
    /* Logging & Tracing Information */
    if (pce->dwTraceID != INVALID_TRACEID) {
        TraceDeregister(pce->dwTraceID);
        pce->dwTraceID = INVALID_TRACEID;
    }

    /* destroy the semaphore released by each thread when it is done */
    if (pce->hActivitySemaphore != NULL) {
        CloseHandle(pce->hActivitySemaphore);
        pce->hActivitySemaphore = NULL;
    }

    if (pce->hGlobalHeap != NULL) {
        HeapDestroy(pce->hGlobalHeap);
        pce->hGlobalHeap = NULL;
    }

    /* delete the read-write lock */
    if (READ_WRITE_LOCK_CREATED(&(pce->rwlLock)))
        DELETE_READ_WRITE_LOCK(&(pce->rwlLock));

    return NO_ERROR;
}

DWORD
CE_Initialize (
    PCONFIGURATION_ENTRY    pce,
    HANDLE                  hMgrNotificationEvent,
    PSUPPORT_FUNCTIONS      psfSupportFunctions,
    PXORPRTM_GLOBAL_CONFIG pigc)
{
    DWORD   dwErr               = NO_ERROR;
    pipe_instance_t *pp;
    int i, pipefail;

    do {
        pce->ulActivityCount    = 0;

	pce->hMprConfig = NULL;
	dwErr = MprConfigServerConnect(NULL, &pce->hMprConfig);
	if (dwErr != NO_ERROR) {
            TRACE0(CONFIGURATION, "could not obtain mpr config handle");
	}

        /* Router Manager Information */
        pce->hMgrNotificationEvent   = hMgrNotificationEvent;
        if (psfSupportFunctions)
            pce->sfSupportFunctions      = *psfSupportFunctions;

	pipefail = 0;
	for (i = 0; i < PIPE_INSTANCES; i++) {
	    pp = pipe_new();
	    if (pp == NULL) {
		pipefail = 1;
		break;
	    } else {
		pipe_listen(pp);
		pce->pipes[i] = pp;
	    }
	}

	if (pipefail) {
            TRACE0(CONFIGURATION, "failed to allocate all pipes");
	    break;
	}
	TRACE0(ANY, "Listening on pipes ok.");
        
        pce->reiRtmEntity.RtmInstanceId = 0;
#ifdef IPV6_DLL
        pce->reiRtmEntity.AddressFamily = AF_INET6;
#else
        pce->reiRtmEntity.AddressFamily = AF_INET;
#endif
        pce->reiRtmEntity.EntityId.EntityProtocolId = PROTO_IP_XORPRTM;
        pce->reiRtmEntity.EntityId.EntityInstanceId = 0;

        dwErr = RtmRegisterEntity(
            &pce->reiRtmEntity,
            NULL,
            RTM_CallbackEvent,
            TRUE,
            &pce->rrpRtmProfile,
            &pce->hRtmHandle);
        if (dwErr != NO_ERROR) {
            TRACE1(CONFIGURATION, "Error %u registering with RTM", dwErr);
            break;
        }
	TRACE0(ANY, "registered entity ok.");

        dwErr = RtmRegisterForChangeNotification(
            pce->hRtmHandle,
            RTM_VIEW_MASK_UCAST,
            RTM_CHANGE_TYPE_ALL,
            NULL,
            &pce->hRtmNotificationHandle);
        if (dwErr != NO_ERROR) {
            TRACE1(CONFIGURATION,
                   "Error %u registering for change with RTM", dwErr);
            break;
        }
	TRACE0(ANY, "registered rtm changes ok.");

        pce->iscStatus = XORPRTM_STATUS_RUNNING;
    } while (FALSE);

    if (dwErr != NO_ERROR) {
	TRACE0(ANY, "init failed, cleaning up.");
        CE_Cleanup(pce);
    } else {
	TRACE0(ANY, "Leaving init ok ");
    }

    return dwErr;
}

DWORD
CE_Cleanup(PCONFIGURATION_ENTRY pce)
{
    DWORD dwErr = NO_ERROR;
    int i;

    if (pce->hRtmNotificationHandle) {
        dwErr = RtmDeregisterFromChangeNotification(
            pce->hRtmHandle,
            pce->hRtmNotificationHandle);
        if (dwErr != NO_ERROR)
            TRACE1(CONFIGURATION,
                   "Error %u deregistering for change from RTM", dwErr);
    }
    pce->hRtmNotificationHandle = NULL;

    if (pce->hRtmHandle) {
        dwErr = RtmDeregisterEntity(pce->hRtmHandle);

        if (dwErr != NO_ERROR)
            TRACE1(CONFIGURATION,
                   "Error %u deregistering from RTM", dwErr);
    }
    pce->hRtmHandle             = NULL;

    for (i = 0; i < PIPE_INSTANCES; i++) {
	if (pce->pipes[i]) {
	    pipe_destroy(pce->pipes[i]);
	    pce->pipes[i] = NULL;
	}
    }

    if (pce->hMprConfig != NULL) {
	MprConfigServerDisconnect(pce->hMprConfig);
    }
    pce->hMprConfig = NULL;

    pce->iscStatus = XORPRTM_STATUS_STOPPED;

    return NO_ERROR;
}

VOID
CM_WorkerFinishStopProtocol (
    PVOID   pvContext)
{
    DWORD           dwErr = NO_ERROR;
    MESSAGE         mMessage;
    
    ULONG           ulThreadCount = 0;
    
    ulThreadCount = (ULONG)pvContext;

    TRACE1(ENTER, "Entering WorkerFinishStopProtocol: active threads %u",
           ulThreadCount);
    
    /* NOTE: since this is called while the router is stopping, there is no */
    /* need for it to use ENTER_XORPRTM_WORKER()/LEAVE_XORPRTM_WORKER() */

    /* waits for all threads to stop */
    while (ulThreadCount-- > 0)
        WaitForSingleObject(g_ce.hActivitySemaphore, INFINITE);

    
   /* acquire the lock and release it, just to be sure that all threads */
   /* have quit their calls to LeaveSampleWorker() */

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    /* NOTE: there is no need to acquire g_ce.rwlLock for the call to */
    /* CE_Cleanup since there are no threads competing for access to the */
    /* fields being cleaned up.  new competing threads aren't created till */
    /* CE_Cleanup sets the protocol state to XORPRTM_STATUS_STOPPED, which */
    /* is the last thing it does. */

    CE_Cleanup(&g_ce);

    /* inform router manager that we are done */
    ZeroMemory(&mMessage, sizeof(MESSAGE));
    if (EnqueueEvent(ROUTER_STOPPED, mMessage) == NO_ERROR)
        SetEvent(g_ce.hMgrNotificationEvent);

    TRACE0(LEAVE, "Leaving  WorkerFinishStopProtocol");
}

/* APIFUNCTIONS */

DWORD
CM_StartProtocol (
    HANDLE                  hMgrNotificationEvent,
    PSUPPORT_FUNCTIONS      psfSupportFunctions,
    PVOID                   pvGlobalInfo)
{
    DWORD dwErr = NO_ERROR;
    
    /*
     * NOTE: since this is called when the protocol is stopped, there
     * is no need for it to use ENTER_XORPRTM_API()/LEAVE_XORPRTM_API().
     */
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do {
        if (g_ce.iscStatus != XORPRTM_STATUS_STOPPED) {
            TRACE1(CONFIGURATION, "Error: %s already installed",
		   XORPRTM_LOGNAME);
            dwErr = ERROR_CAN_NOT_COMPLETE;
            
            break;
        }
        dwErr = CE_Initialize(&g_ce,
                              hMgrNotificationEvent,
                              psfSupportFunctions,
                              (PXORPRTM_GLOBAL_CONFIG) pvGlobalInfo);
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    if (dwErr == NO_ERROR) {
        TRACE1(CONFIGURATION, "%s has successfully started", XORPRTM_LOGNAME);
    } else {
        TRACE2(CONFIGURATION, "Error: %s failed to start (%d)",
	       XORPRTM_LOGNAME, dwErr);
    }

    return dwErr;
}


DWORD
CM_StopProtocol ()
{
    DWORD dwErr         = NO_ERROR;
    BOOL  bSuccess      = FALSE;
    ULONG ulThreadCount = 0;
    
    /* XXX: no need to use ENTER_XORPRTM_API()/LEAVE_XORPRTM_API() */
    
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do {
        /* cannot stop if already stopped */
        if (g_ce.iscStatus != XORPRTM_STATUS_RUNNING)
        {
            TRACE0(CONFIGURATION, "Error ip sample already stopped");
            dwErr = ERROR_CAN_NOT_COMPLETE;
        
            break;
        }

	/*
	 * Set XORPRTM's status to STOPPING; this prevents any more work
	 * items from being queued, and it prevents the ones already
	 * queued from executing.
	 */
        g_ce.iscStatus = XORPRTM_STATUS_STOPPING;
        
	/*
         * find out how many threads are either queued or active in XORPRTM;
         * we will have to wait for this many threads to exit before we
         * clean up XORPRTM's resources.
	 */
        ulThreadCount = g_ce.ulActivityCount;
        TRACE2(CONFIGURATION, "%u threads are active in %s", ulThreadCount,
	       XORPRTM_LOGNAME);
    } while (FALSE);

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    if (dwErr == NO_ERROR) {
        bSuccess = QueueUserWorkItem(
            (LPTHREAD_START_ROUTINE)CM_WorkerFinishStopProtocol,
            (PVOID) ulThreadCount,
            0); /* no flags */
        dwErr = (bSuccess) ? ERROR_PROTOCOL_STOP_PENDING : GetLastError();
    }

    return dwErr;
}

DWORD
CM_GetGlobalInfo (
    PVOID 	        pvGlobalInfo,
    PULONG              pulBufferSize,
    PULONG	        pulStructureVersion,
    PULONG              pulStructureSize,
    PULONG              pulStructureCount)
{
    DWORD                   dwErr = NO_ERROR;
    PXORPRTM_GLOBAL_CONFIG pigc;
    ULONG                   ulSize = sizeof(XORPRTM_GLOBAL_CONFIG);

    do
    {
        if((*pulBufferSize < ulSize) || (pvGlobalInfo == NULL))
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            TRACE1(CONFIGURATION,
                   "CM_GetGlobalInfo: *ulBufferSize %u",
                   *pulBufferSize);

            *pulBufferSize = ulSize;

            break;
        }

        *pulBufferSize = ulSize;

        if (pulStructureVersion)    *pulStructureVersion    = 1;
        if (pulStructureSize)       *pulStructureSize       = ulSize;
        if (pulStructureCount)      *pulStructureCount      = 1;
        
        pigc = (PXORPRTM_GLOBAL_CONFIG) pvGlobalInfo;

    } while (FALSE);

    return dwErr;
}

/*
 * Called when the Router Manager tells us there's an event
 * in our event queue.
 * NOTE: this can be called after the protocol is stopped, as in when
 * the ip router manager is retrieving the ROUTER_STOPPED message, so
 * we do not call ENTER_XORPRTM_API()/LEAVE_XORPRTM_API().
 */
DWORD
CM_GetEventMessage (
    ROUTING_PROTOCOL_EVENTS *prpeEvent,
    MESSAGE                 *pmMessage)
{
    DWORD           dwErr       = NO_ERROR;

    dwErr = DequeueEvent(prpeEvent, pmMessage);
    
    return dwErr;
}

BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID pvImpLoad)
{
	BOOL bError = TRUE;

	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstance);
		bError = (CE_Create(&g_ce) == NO_ERROR) ? TRUE : FALSE;
		break;

	case DLL_PROCESS_DETACH:
		CE_Destroy(&g_ce);
		break;

	default:
		break;
	}

	return bError;
}

/*
 * Add a route to RTMv2 based on a routing socket message.
 * XXX: We should report errors in more detail, e.g. if the
 * route could not be added because it already existed, etc.
 */
int
rtm_add_route(struct rt_msghdr *rtm, int msgsize)
{
    static const proper_msgsize = (sizeof(struct rt_msghdr) +
				  (sizeof(sockunion_t) * 3));
    sockunion_t *sa;
#ifdef IPV6_DLL
    struct in6_addr in6_dest;
    struct in6_addr in6_mask;
    struct in6_addr in6_nexthop;
#else
    struct in_addr in_dest;
    struct in_addr in_mask;
    struct in_addr in_nexthop;
    MIB_IPFORWARDROW ro;
#endif
    int retval;
    int prefix;
    DWORD result;
    RTM_NET_ADDRESS dest;
    RTM_NET_ADDRESS nexthop;
    RTM_NEXTHOP_HANDLE nhh;
    RTM_NEXTHOP_INFO nhi;
    RTM_ROUTE_HANDLE nrh;
    RTM_ROUTE_INFO ri;
    RTM_ROUTE_CHANGE_FLAGS changeFlags;

    /*
     * Sanity check message size, fields etc.
     */
    if (!rtm)
	return -1;
    if (msgsize < proper_msgsize || (rtm->rtm_msglen < proper_msgsize))
	return -1;
    if (rtm->rtm_type != RTM_ADD)
	return -1;
    if ((rtm->rtm_addrs & (RTA_DST|RTA_GATEWAY|RTA_NETMASK)) !=
	(RTA_DST|RTA_GATEWAY|RTA_NETMASK))
	return -1;

    nhh = NULL;
    nrh = NULL;

    /*
     * Extract destination, netmask and next-hop from routing
     * socket message.
     */
#ifdef IPV6_DLL
    sa = (sockunion_t *)(rtm + 1);
    in6_dest = sa->sin6.sin6_addr;
    if (sa->sa.sa_family != AF_INET6)
	return -1;
    ++sa;
    in6_nexthop = sa->sin6.sin6_addr;
    if (sa->sa.sa_family != AF_INET6)
	return -1;
    ++sa;
    in6_mask = sa->sin6.sin6_addr;
    if (sa->sa.sa_family != AF_INET6)
	return -1;
#else
    sa = (sockunion_t *)(rtm + 1);
    if (sa->sa.sa_family != AF_INET)
	return -1;
    in_dest = sa->sin.sin_addr;
    ++sa;
    if (sa->sa.sa_family != AF_INET)
	return -1;
    in_nexthop = sa->sin.sin_addr;
    ++sa;
    if (sa->sa.sa_family != AF_INET)
	return -1;
    in_mask = sa->sin.sin_addr;
#endif

#ifndef IPV6_DLL
    /*
     * Look up the next-hop in the system routing table via
     * IP Helper. If there is no directly connected route we
     * can use to reach the next-hop, then we reject this attempt
     * to add a route, as we need to know the interface index
     * of this route in order to add the new route.
     * XXX This is not good for multihop.
     * XXX IPv6!
     */
    result = GetBestRoute(in_nexthop.s_addr, INADDR_ANY, &ro);
    if (result != NO_ERROR) {
	TRACE1(NETWORK, "error: GetBestRoute() returned %d", result);
	return -1;
    }
#endif

    /*
     * Convert netmask to a prefix length.
     * Convert destination to an RTM_NET_ADDRESS.
     * Convert next-hop to an RTM_NET_ADDRESS.
     * XXX: IPv6 path needs 'get length from mask' macro.
     * XXX: IPv6 path needs interface index.
     */
#ifdef IPV6_DLL
    RTM_IPV6_LEN_FROM_MASK(prefix, in6_mask.s_addr);
    RTM_IPV6_MAKE_NET_ADDRESS(&dest, in6_dest.s_addr, prefix);
    RTM_IPV6_MAKE_NET_ADDRESS(&nexthop, in6_nexthop.s_addr, 128);
#else
    RTM_IPV4_LEN_FROM_MASK(prefix, in_mask.s_addr);
    RTM_IPV4_MAKE_NET_ADDRESS(&dest, in_dest.s_addr, prefix);
    RTM_IPV4_MAKE_NET_ADDRESS(&nexthop, in_nexthop.s_addr, 32);
    /*
     * Fill out the next-hop info structure.
     * Create the next-hop in the RTMv2 table.
     */
    ZeroMemory(&nhi, sizeof(nhi));
    nhi.InterfaceIndex = ro.dwForwardIfIndex;
    nhi.NextHopAddress = nexthop;
#endif /* IPV6_DLL */

    result = RtmAddNextHop(g_ce.hRtmHandle, &nhi, &nhh, &changeFlags);
    if (result != NO_ERROR) {
	TRACE1(NETWORK, "error %u adding nexthop", result);
	retval = -1;
	goto out;
    }

    /*
     * Fill out the RTM_ROUTE_INFO structure.
     * Attempt to add the route.
     */
    ZeroMemory(&ri, sizeof(ri));
    ri.PrefInfo.Metric = XORPRTM_RI_METRIC;
    ri.PrefInfo.Preference = XORPRTM_RI_PREF;
    ri.BelongsToViews = RTM_VIEW_MASK_UCAST;
    ri.NextHopsList.NumNextHops = 1;
    ri.NextHopsList.NextHops[0] = nhh;
    changeFlags = 0;

    result = RtmAddRouteToDest(g_ce.hRtmHandle, &nrh, &dest, &ri, INFINITE,
			       NULL, 0, NULL, &changeFlags);
    if (result != NO_ERROR) {
	TRACE1(NETWORK, "error %u adding route", result);
	retval = -1;
	goto out;
    }

    retval = 0;

out:
    if (nrh != NULL)
	RtmReleaseRoutes(g_ce.hRtmHandle, 1, &nrh);
    if (nhh != NULL)
	RtmReleaseNextHops(g_ce.hRtmHandle, 1, &nhh);

    return (retval);
}

/*
 * Delete a route from RTMv2 based on a routing socket message.
 * XXX: We should report errors in more detail, e.g. if the
 * route could not be added because it already existed, etc.
 */
int
rtm_delete_route(struct rt_msghdr *rtm, int msgsize)
{
    static const min_msgsize = (sizeof(struct rt_msghdr) +
			       (sizeof(sockunion_t) * 2));
    sockunion_t *sa;
    struct in_addr in_dest;
    struct in_addr in_mask;
    int found;
    int i;
    int prefix;
    int retval;
    DWORD result;
    RTM_DEST_INFO di;
    RTM_NET_ADDRESS dest;
    RTM_ROUTE_CHANGE_FLAGS changeflags;

    /*
     * Sanity check message size, fields etc.
     */
    if (!rtm)
	return -1;
    if (msgsize < min_msgsize || (rtm->rtm_msglen < min_msgsize))
	return -1;
    if (rtm->rtm_type != RTM_DELETE)
	return -1;
    if ((rtm->rtm_addrs & (RTA_DST|RTA_NETMASK)) != (RTA_DST|RTA_NETMASK))
	return -1;
    /*
     * Extract destination, netmask and next-hop from routing
     * socket message.
     * XXX: bsd's delete order is: <DST,GATEWAY,NETMASK>
     * XXX: we don't check to see if gateway is present and
     * if so we do not handle it correctly.
     */
    sa = (sockunion_t *)(rtm + 1);
    in_dest = sa->sin.sin_addr;
    ++sa;
    in_mask = sa->sin.sin_addr;

    /*
     * Convert netmask to a prefix length.
     * Convert destination to an RTM_NET_ADDRESS.
     */
    RTM_IPV4_LEN_FROM_MASK(prefix, in_mask.s_addr);
    RTM_IPV4_MAKE_NET_ADDRESS(&dest, in_dest.s_addr, prefix);

    /*
     * Look up the route to be deleted in RTMv2, from those
     * which belong to our protocol, in the unicast view.
     */
    ZeroMemory(&di, sizeof(di));
    di.DestAddress = dest;
    result = RtmGetExactMatchDestination(g_ce.hRtmHandle, &dest, 
					 RTM_THIS_PROTOCOL,
					 RTM_VIEW_MASK_UCAST, &di);
    if (result != NO_ERROR) {
	TRACE1(NETWORK, "error %u looking up route to delete", result);
	retval = -1;
	goto out;
    }
    i = 0;
    found = 0;
    for (i = 0; i < di.NumberOfViews; i++) {
    	if (di.ViewInfo[i].ViewId == RTM_VIEW_ID_UCAST) {
	    /*
	     * Return a match only if the unicast view for our protocol
	     * contains a single next-hop route to the destination.
	     */
	    if (di.ViewInfo[i].NumRoutes == 1)
		found = 1;
	    break;
	}
    }
    if (!found) {
	TRACE0(NETWORK, "route not found in table");
	retval = -1;
	goto out;
    }

    result = RtmDeleteRouteToDest(g_ce.hRtmHandle, di.ViewInfo[i].Route,
				  &changeflags);
    if (result != NO_ERROR) {
	TRACE1(NETWORK, "error %u deleting route", result);
	retval = -1;
	goto out;
    }

    retval = 0;

out:
    return (retval);
}

int
rtm_ifannounce(LPWSTR ifname, DWORD ifindex, int what)
{
    WCHAR fnameW[IFNAMSIZ];
    struct if_announcemsghdr *ifa;
    int result;
    int retval;

    TRACE3(ENTER, "Entering rtm_ifannounce %S %d %d", ifname, ifindex, what);

    ifa = NULL;
    retval = -1;

    if ((what != IFAN_ARRIVAL) && (what != IFAN_DEPARTURE)) {
	goto out;
    }

    ifa = malloc(sizeof(*ifa));
    if (ifa == NULL) {
	goto out;
    }
    ifa->ifan_name[0] = '\0';

    /*
     * If this is a new interface, then look up the FriendlyName from
     * the unicode GUID name; convert Unicode to ASCII afterwards.
     * If the caller didn't supply this, the error is fatal to this function.
     * If we can't find it, the error is non-fatal to this function.
     *
     * XXX: The very fact that we don't provide the interface name here
     * is a limitation to do with how the notifications work in the
     * Microsoft stack. It only tells us the interface index when
     * the interface goes away. XORP currently depends on both for
     * interface deletion. We could look it up from the transport,
     * but it's more work to deliver redundant information.
     */
    if (what == IFAN_ARRIVAL) {
	if (ifname == NULL)
	    goto out;

	result = MprConfigGetFriendlyName(g_ce.hMprConfig, ifname, fnameW,
					  sizeof(fnameW));
	if (result != NO_ERROR) {
	    TRACE1(NETWORK, "can't find friendlyname for ifname %S", ifname);
	} else {
	    wcstombs(ifa->ifan_name, fnameW, IFNAMSIZ);
	}
    }

    /*
     * Fill our the rest of the interface announcement and send it to
     * all connected clients.
     */
    ifa->ifan_msglen = sizeof(*ifa);
    ifa->ifan_version = RTM_VERSION;	/* XXX should set to 0 or ignore */
    ifa->ifan_type = RTM_IFANNOUNCE;
    ifa->ifan_index = ifindex;
    ifa->ifan_what = what;

    broadcast_pipe_message(ifa, sizeof(*ifa));

    retval = 0;

out:
    if (ifa != NULL)
	free(ifa);

    TRACE0(ENTER, "Leaving rtm_ifannounce");

    return (retval);
}

int
rtm_ifinfo(DWORD ifindex, int up)
{
    struct if_msghdr *ifm;
    int result;
    int retval;

    TRACE2(ENTER, "Entering rtm_ifinfo %d %d", ifindex, up);

    ifm = NULL;
    retval = -1;

    ifm = malloc(sizeof(*ifm));
    if (ifm == NULL) {
	goto out;
    }

    /*
     * Fill our the rest of the interface announcement and send it to
     * all connected clients.
     */
    ifm->ifm_msglen = sizeof(*ifm);
    ifm->ifm_version = RTM_VERSION;
    ifm->ifm_type = RTM_IFANNOUNCE;
    ifm->ifm_addrs = 0;
    ifm->ifm_flags = 0;
    ifm->ifm_index = ifindex;
    ifm->ifm_data.ifi_link_state = (up ? LINK_STATE_UP : LINK_STATE_DOWN);

    broadcast_pipe_message(ifm, sizeof(*ifm));

    retval = 0;

out:
    if (ifm != NULL)
	free(ifm);

    TRACE0(ENTER, "Leaving rtm_ifinfo");

    return (retval);
}

/*
 * Send one RTM_NEWADDR message for each IPv4 address we've found
 * in the binding message. We ignore pbind->RemoteAddress for now.
 *
 * XXX: This does not work like BSD's notifications; again, if we
 * wish to send changes, like routes, we need to maintain state,
 * as the RTMv2 APIs send the entire state of the interface's
 * address list each time.
 *
 * For this reason it's probably better to use IP Helper as
 * the means of interface information discovery (with
 * GetAdaptersAddresses()).
 */
int
rtm_newaddr(DWORD ifindex,
#ifdef IPV6_DLL
	    PIPV6_ADAPTER_BINDING_INFO pbind
#else
	    PIP_ADAPTER_BINDING_INFO pbind
#endif
)
{
    static const msgsize =
sizeof(struct ifa_msghdr) + (sizeof(sockunion_t) * 2);
    struct ifa_msghdr *ifam;
    sockunion_t *sa;
    sockunion_t *sa2;
    int i;
    int result;
    int retval;

    TRACE2(ENTER, "Entering rtm_newaddr %d %p", ifindex, pbind);

    retval = -1;
    ifam = NULL;

    if (pbind == NULL)
	goto out;
    if (pbind->AddressCount == 0)
	goto out;

    ifam = malloc(sizeof(*ifam));
    if (ifam == NULL)
	goto out;

    sa = (sockunion_t *)(ifam + 1);
    sa2 = (sa + 1);

    for (i = 0; i < pbind->AddressCount; i++) {
	ifam->ifam_msglen = msgsize;
	ifam->ifam_version = RTM_VERSION;
	ifam->ifam_type = RTM_NEWADDR;
	ifam->ifam_addrs = RTA_DST | RTA_NETMASK;
	ifam->ifam_flags = 0;
	ifam->ifam_index = ifindex;
	ifam->ifam_metric = 0;
#ifdef IPV6_DLL
	sa->sin6.sin6_family = AF_INET6;
	sa->sin6.sin6_addr.s_addr = pbind->Address[i].Address;
	sa2->sin6.sin6_family = AF_INET6;
	sa2->sin6.sin6_addr.s_addr = pbind->Address[i].Mask;
#else
	sa->sin.sin_family = AF_INET;
	sa->sin.sin_addr.s_addr = pbind->Address[i].Address;
	sa2->sin.sin_family = AF_INET;
	sa2->sin.sin_addr.s_addr = pbind->Address[i].Mask;
#endif

	broadcast_pipe_message(ifam, msgsize);
    }

    retval = 0;

out:
    if (ifam != NULL)
	free(ifam);

    TRACE0(ENTER, "Leaving rtm_newaddr");

    return (retval);
}
