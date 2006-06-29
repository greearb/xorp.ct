/* vim:set sts=4 ts=8: */


#include "pchsample.h"
#pragma hdrstop

BOOL
EnterSubsystemAPI()
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    if (g_ce.iscStatus == XORPRTM_STATUS_RUNNING) {
        /* subsystem is running, so continue */
        g_ce.ulActivityCount++;
        bEntered = TRUE;
    }
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}

BOOL
EnterSubsystemWorker()
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do {
        /* subsystem is running, so the function may continue */
        if (g_ce.iscStatus == XORPRTM_STATUS_RUNNING) {
            bEntered = TRUE;
            break;
        }

        /* subsystem is not running, but it was, so the function must stop */
        if (g_ce.iscStatus == XORPRTM_STATUS_STOPPING) {
            g_ce.ulActivityCount--;
            ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);
            break;
        }

        /* subsystem probably never started. quit. */
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}

VOID
LeaveSubsystemWorker()
{
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    g_ce.ulActivityCount--;
    if (g_ce.iscStatus == XORPRTM_STATUS_STOPPING) {
        ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);
    }

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));
}
