/* vim:set sts=4 ts=8: */

#ifndef _MIBMANAGER_H_
#define _MIBMANAGER_H_

typedef enum { GET_EXACT, GET_FIRST, GET_NEXT } MODE;

DWORD
WINAPI
MM_MibSet (PXORPRTM_MIB_SET_INPUT_DATA    pimsid);

DWORD
WINAPI
MM_MibGet (PXORPRTM_MIB_GET_INPUT_DATA    pimgid,
    PXORPRTM_MIB_GET_OUTPUT_DATA   pimgod,
    PULONG	                        pulOutputSize,
    MODE                            mMode);

#endif
