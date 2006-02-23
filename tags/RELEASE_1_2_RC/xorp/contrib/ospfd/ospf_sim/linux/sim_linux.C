/*
 *   OSPFD routing simulation controller
 *   Linux-specific portion
 *   Copyright (C) 1999 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <unistd.h>
#include <tcl.h>
#include <tk.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "../src/ospfinc.h"
#include "../src/monitor.h"
#include "../src/system.h"
#include "tcppkt.h"
#include "linux.h"
#include "sim.h"
#include "simctl.h"
#include <time.h>

// External references
extern SimCtl *sim;

/* Start a simulated router.
 * Router uses two sockets, one for monitoring and
 * one to receive OSPF packets.
 * The former we allocate so that they are in well known
 * places, the latter the router allocates.
 */

int StartRouter(ClientData, Tcl_Interp *, int, char *argv[])

{
    char server_port[8];

    sprintf(server_port, "%d", (int) sim->assigned_port);
    // Child is now a simulated router
    if (fork() == 0) {
        execlp("ospfd_sim", "ospfd_sim", argv[1], server_port, 0);
	// Should never get this far
	perror("execl ospfd_sim failed");
	exit(1);
    }

    return(TCL_OK);
}
