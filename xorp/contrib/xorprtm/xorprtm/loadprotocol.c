/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/contrib/win32/xorprtm/loadprotocol.c,v 1.7 2008/10/02 21:56:40 bms Exp $"

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

/*
 * Modified version of LoadProtocol for integration into XORP.
 * Restarts RRAS service after making registry changes.
 * Loads the DLL into RRAS using MPR API calls.
 */

#define UNICODE
#include <winsock2.h>
#include <windows.h>
#include <winbase.h>
#include <mprapi.h>
#include <rtinfo.h>
#include <routprot.h>

#include "xorprtm.h"

#include "bsdroute.h"

HRESULT
add_protocol_to_rras(int family)
{
    static const SHORT XORPRTM_BLOCK_SIZE = 0x0004;
    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwErrT = ERROR_SUCCESS;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE   hMprConfig = NULL;
    LPBYTE      pByte = NULL;
    LPVOID        pHeader = NULL;
    LPVOID        pNewHeader = NULL;
    DWORD       dwSize = 0;
    HANDLE      hTransport = NULL;
    LPCWSTR pswzServerName = NULL;
    int     pid;
    XORPRTM_GLOBAL_CONFIG   igc;

    memset(&igc, 0, sizeof(igc));

#ifdef IPV6_DLL
    if (family == AF_INET) {
 pid = PID_IP;
    } else {
 pid = PID_IPV6;
    }
#else
    pid = PID_IP;
#endif

    /* Connect to the server */
    /* ---------------------------------------------------------------- */
    dwErr = MprAdminServerConnect((LPWSTR) pswzServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS)
    {
        /* Ok, get the infobase from the server */
        /* ------------------------------------------------------------ */
        dwErr = MprAdminTransportGetInfo(hMprServer,
                                         pid,
                                         &pByte,
                                         &dwSize,
                                         NULL,
                                         NULL);

        if (dwErr == ERROR_SUCCESS)
        {
            /* Call MprInfoDuplicate to create a duplicate of */
            /* the infoblock */
            /* -------------------------------------------------------- */
            MprInfoDuplicate(pByte, &pHeader);
            MprAdminBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    /* We also have to open the hMprConfig, but we can ignore the error */
    /* ---------------------------------------------------------------- */
    dwErrT = MprConfigServerConnect((LPWSTR) pswzServerName, &hMprConfig);
    if (dwErrT == ERROR_SUCCESS)
    {
        dwErrT = MprConfigTransportGetHandle(hMprConfig, pid, &hTransport);
    }

    if (dwErr != ERROR_SUCCESS)
    {
        /* Ok, try to use the MprConfig calls. */
        /* ------------------------------------------------------------ */
        MprConfigTransportGetInfo(hMprConfig,
                                  hTransport,
                                  &pByte,
                                  &dwSize,
                                  NULL,
                                  NULL,
                                  NULL);

        /* Call MprInfoDuplicate to create a duplicate of */
        /* the infoblock */
        /* ------------------------------------------------------------ */
        MprInfoDuplicate(pByte, &pHeader);
        MprConfigBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    /* Call MprInfoBlockRemove to remove the old protocol block */
    MprInfoBlockRemove(pHeader, PROTO_IP_XORPRTM, &pNewHeader);

    /* Did we remove the block? */
    if (pNewHeader != NULL)
    {
        /* The block was found and removed, so use the new header. */
        MprInfoDelete(pHeader);
        pHeader = pNewHeader;
        pNewHeader = NULL;
    }

    /* Add protocol to the infoblock here!     */
    MprInfoBlockAdd(pHeader,
                    PROTO_IP_XORPRTM,
                    XORPRTM_BLOCK_SIZE,
                    1,
                    (LPBYTE)&igc,
                    &pNewHeader);
    MprInfoDelete(pHeader);
    pHeader = NULL;


    if (hMprServer)
    {

        MprAdminTransportSetInfo(hMprServer,
                                 pid,
                                 (BYTE*)pNewHeader,
                                 MprInfoBlockQuerySize(pNewHeader),
                                 NULL,
                                 0);
    }

    if (hMprConfig && hTransport)
    {

        MprConfigTransportSetInfo(hMprConfig,
                                  hTransport,
                                  (BYTE*)pNewHeader,
                                  MprInfoBlockQuerySize(pNewHeader),
                                  NULL,
                                  0,
                                  NULL);
    }

    if (pHeader)
        MprInfoDelete(pHeader);

    if (pNewHeader)
        MprInfoDelete(pNewHeader);

    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;
}

int
restart_rras()
{
    SERVICE_STATUS ss;
    SC_HANDLE h_scm;
    SC_HANDLE h_rras;
    DWORD result;
    int is_running, tries, fatal;

    h_scm = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (h_scm == NULL) {
 return (-1);
    }

    h_rras = OpenService(h_scm, RRAS_SERVICE_NAME, GENERIC_READ);
    if (h_rras == NULL) {
     result = GetLastError();
 /*printf("OpenService() failed: %d", result); */
 CloseServiceHandle(h_scm);
 return (-1);
    }

    fatal = 0;

    /*printf("Stoping service \"%s\" ", RRAS_SERVICE_NAME); */

    for (tries = 30; tries > 0; tries++) {
 /* Check if the service is running, stopping, or stopped. */
 result = ControlService(h_rras, SERVICE_CONTROL_INTERROGATE, &ss);
 if (result == NO_ERROR) {
     /* Stopped; carry on */
     if (ss.dwCurrentState == SERVICE_STOPPED)
  break;
     /* Stopping; poll until it's done */
     if (ss.dwCurrentState == SERVICE_STOP_PENDING) {
  Sleep(1000);
  continue;
     }
 } else if (result == ERROR_SERVICE_NOT_ACTIVE) {
     break;
 } else {
     fatal = 1;
     break;
 }

 result = ControlService(h_rras, SERVICE_CONTROL_STOP, &ss);
 if (result == ERROR_SERVICE_NOT_ACTIVE) {
     break;
 } else if (result != NO_ERROR) {
     fatal = 1;
     break;
 }
    }

    /* XXX: We should really check to see if it started OK. */
    result = StartService(h_rras, 0, NULL);

    /* ... should finish doing this ... */

    CloseServiceHandle(h_rras);
    CloseServiceHandle(h_scm);

    return (0);
}

/*
 * Registry stuff. This is messy.
 */

#define HKLM_XORPRTM4_NAME \
"SOFTWARE\\Microsoft\\Router\\CurrentVersion\\RouterManagers\\Ip\\XORPRTM4"

#define HKLM_XORPRTM6_NAME \
"SOFTWARE\\Microsoft\\Router\\CurrentVersion\\RouterManagers\\Ipv6\\XORPRTM6"

#define HKLM_XORPRTM4_TRACING_NAME \
"SOFTWARE\\Microsoft\\Tracing\\XORPRTM4"

#define HKLM_XORPRTM6_TRACING_NAME \
"SOFTWARE\\Microsoft\\Tracing\\XORPRTM6"

static CHAR DLL_CLSID_IPV4[] = "{C2FE450A-D6C2-11D0-A37B-00C04FC9DA04}";
static CHAR DLL_CLSID_IPV6[] = "{C2FE451A-D6C2-11D0-A37B-00C04FC9DA04}";
static CHAR DLL_CONFIG_DLL[] = "nonexistent.dll";
static CHAR DLL_NAME_IPV4[] = "xorprtm4.dll";
static CHAR DLL_NAME_IPV6[] = "xorprtm6.dll";
static DWORD DLL_FLAGS = 0x00000002;
static DWORD DLL_PROTO = PROTO_IP_XORPRTM;
static CHAR DLL_TITLE_IPV4[] = "Router Manager V2 adapter for XORP (IPv4)";
static CHAR DLL_TITLE_IPV6[] = "Router Manager V2 adapter for XORP (IPv6)";
static CHAR DLL_VENDOR[] = "www.xorp.org";
#if 1
static CHAR TRACING_DIR[] = "%windir%\\Tracing";
#endif

void
add_protocol_to_registry(int family)
{
    DWORD result;
    DWORD foo;
    HKEY hKey;

    result = RegCreateKeyExA(
  HKEY_LOCAL_MACHINE,
  family == AF_INET ? HKLM_XORPRTM4_NAME : HKLM_XORPRTM6_NAME,
  0,
  NULL,
  REG_OPTION_NON_VOLATILE,
  KEY_ALL_ACCESS,
  NULL,
  &hKey,
  NULL);

    RegSetValueExA(hKey, "ConfigDll", 0, REG_SZ, DLL_CONFIG_DLL, sizeof(DLL_CONFIG_DLL));
    if (family == AF_INET) {
     RegSetValueExA(hKey, "ConfigClsId", 0, REG_SZ, DLL_CLSID_IPV4, sizeof(DLL_CLSID_IPV4));
     RegSetValueExA(hKey, "Title", 0, REG_SZ, DLL_TITLE_IPV4, sizeof(DLL_TITLE_IPV4));
     RegSetValueExA(hKey, "DllName", 0, REG_SZ, DLL_NAME_IPV4, sizeof(DLL_NAME_IPV4));
    } else {
     RegSetValueExA(hKey, "ConfigClsId", 0, REG_SZ, DLL_CLSID_IPV6, sizeof(DLL_CLSID_IPV6));
     RegSetValueExA(hKey, "Title", 0, REG_SZ, DLL_TITLE_IPV6, sizeof(DLL_TITLE_IPV6));
     RegSetValueExA(hKey, "DllName", 0, REG_SZ, DLL_NAME_IPV6, sizeof(DLL_NAME_IPV6));
    }
    RegSetValueExA(hKey, "Flags", 0, REG_DWORD, (BYTE*)&DLL_FLAGS, sizeof(DLL_FLAGS));
    RegSetValueExA(hKey, "ProtocolId", 0, REG_DWORD, (BYTE*)&DLL_PROTO, sizeof(DLL_PROTO));
    RegSetValueExA(hKey, "VendorName", 0, REG_SZ, DLL_VENDOR, sizeof(DLL_VENDOR));
    RegCloseKey(hKey);

#if 1
 /* XXX: Enable console tracing for debugging. */

    result = RegCreateKeyExA(
  HKEY_LOCAL_MACHINE,
  family == AF_INET ? HKLM_XORPRTM4_TRACING_NAME : HKLM_XORPRTM6_TRACING_NAME,
  0,
  NULL,
  REG_OPTION_NON_VOLATILE,
  KEY_ALL_ACCESS,
  NULL,
  &hKey,
  NULL);

    foo = 1;
    RegSetValueExA(hKey, "EnableConsoleTracing", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    RegSetValueExA(hKey, "EnableFileTracing", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    foo = 0xFFFF0000;
    RegSetValueExA(hKey, "ConsoleTracingMask", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    RegSetValueExA(hKey, "FileTracingMask", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));
    foo = 0x00100000;
    RegSetValueExA(hKey, "MaxFileSize", 0, REG_DWORD, (BYTE*)&foo, sizeof(foo));

    RegSetValueExA(hKey, "FileDirectory", 0, REG_EXPAND_SZ, TRACING_DIR, sizeof(TRACING_DIR));

    RegCloseKey(hKey);
#endif
}

int
main(int argc, char *argv[])
{
    add_protocol_to_registry(AF_INET);
#ifdef IPV6_DLL
    add_protocol_to_registry(AF_INET6);
#endif

    restart_rras();

    add_protocol_to_rras(AF_INET);
#ifdef IPV6_DLL
    add_protocol_to_rras(AF_INET6);
#endif
}
