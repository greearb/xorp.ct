/*
 *   OSPFD routing simulation controller
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
#include "machtype.h"
#include "sim.h"
#include "simctl.h"
#include <time.h>
#include "mtrace.h"

/* Forward references
 */

bool get_prefix(char *prefix, InAddr &net, InMask &mask);

// Tk/Tcl callbacks
void tick(ClientData);

int StartRouter(ClientData, Tcl_Interp *, int, char *argv[]);
int ToggleRouter(ClientData, Tcl_Interp *, int, char *argv[]);
int RestartRouter(ClientData, Tcl_Interp *, int, char *argv[]);
int HitlessRestart(ClientData, Tcl_Interp *, int, char *argv[]);
int StartPing(ClientData, Tcl_Interp *, int, char *argv[]);
int StopPing(ClientData, Tcl_Interp *, int, char *argv[]);
int StartTraceroute(ClientData, Tcl_Interp *, int, char *argv[]);
int StartMtrace(ClientData, Tcl_Interp *, int, char *argv[]);
int PrefixMatch(ClientData, Tcl_Interp *, int, char *argv[]);
int AddMapping(ClientData, Tcl_Interp *, int, char *argv[]);
int AddNetMember(ClientData, Tcl_Interp *interp, int, char *argv[]);
int TimeStop(ClientData, Tcl_Interp *, int, char *argv[]);
int TimeResume(ClientData, Tcl_Interp *, int, char *argv[]);
int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[]);
int SendArea(ClientData, Tcl_Interp *, int, char *argv[]);
int SendInterface(ClientData, Tcl_Interp *, int, char *argv[]);
int SendVL(ClientData, Tcl_Interp *, int, char *argv[]);
int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[]);
int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[]);
int SendHost(ClientData, Tcl_Interp *, int, char *argv[]);
int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[]);
int SendGroup(ClientData, Tcl_Interp *, int, char *argv[]);
int LeaveGroup(ClientData, Tcl_Interp *, int, char *argv[]);

// Global variables
char *sim_tcl_src = "/ospf_sim.tcl";
char *cfgfile = 0;
SimCtl *sim;

/* Process messages received from simulated nodes.
 */

bool SimCtl::process_replies()

{
    int n_fd;
    fd_set fdset;
    int err;
    SimNode *node;
    timeval timeout;
    bool active = false;

    FD_ZERO(&fdset);
    // Add listen socket
    n_fd = server_fd;
    FD_SET(server_fd, &fdset);
    // Add one connection per simulated router
    for (int i = 0; i <= maxfd; i++) {
        if (!(node = nodes[i]))
	    continue;
	n_fd = MAX(n_fd, node->fd);
	FD_SET(node->fd, &fdset);
    }
    // Poll for I/O
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    err = select(n_fd+1, &fdset, 0, 0, &timeout);
    // Handle errors in select
    if (err == -1 && errno != EINTR) {
	perror("select failed");
	exit(1);
    }
    // Handle new connections
    if (FD_ISSET(server_fd, &fdset)) {
        active = true;
	incoming_call();
    }
    // Handle replies from simulated routers
    for (int i = 0; i <= maxfd; i++) {
        if (!(node = nodes[i]))
	    continue;
	if (FD_ISSET(node->fd, &fdset)) {
	    active = true;
	    simnode_handler_read(node->fd);
	}
    }
    return(active);
}

/* Send data to simulated nodes.
 */

bool SimCtl::send_data()

{
    int n_fd;
    fd_set wrset;
    int err;
    SimNode *node;
    timeval timeout;
    bool active=false;

    FD_ZERO(&wrset);
    n_fd = -1;
    // Add one connection per simulated router
    // If we have data pending to that node
    for (int i = 0; i <= maxfd; i++) {
        if (!(node = nodes[i]))
	    continue;
	if (node->pktdata.xmt_pending()) {
	    n_fd = MAX(n_fd, node->fd);
	    FD_SET(node->fd, &wrset);
	}
    }
    // No data to send?
    if (n_fd == -1)
        return(active);
    // Poll for I/O
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    err = select(n_fd+1, 0, &wrset, 0, &timeout);
    // Handle errors in select
    if (err == -1 && errno != EINTR) {
	perror("select failed");
	exit(1);
    }
    // Write pending data if there is space
    for (int i = 0; i <= maxfd; i++) {
        if (!(node = nodes[i]))
	    continue;
	if ((node == nodes[i]) && FD_ISSET(node->fd, &wrset)) {
	    active = true;
	    if (!node->pktdata.sendpkt())
	        delete_router(node);
	}
    }
    return(active);
}

/* Controlling process for the OSPF simulator.
 * After initialization, simply send timer ticks
 * to each simulated OSPF router and wait for their
 * replies. Their replies indicate whether they have
 * synchronized databases. Also can receiving logging
 * messages from the simulated routers, which are
 * written to a file.
 */

int main(int argc, char *argv[])

{
    Tcl_Interp *interp; // Interpretation of config commands

    if (argc > 2) {
	printf("Syntax: ospf_sim [config_filename]\n");
	exit(1);
    }

    if (argc == 2)
	cfgfile = argv[1];
    interp = Tcl_CreateInterp();
    Tcl_AppInit(interp);
    // Main loop, never exits
    Tk_MainLoop();
    exit(0);
}

/* Initialize the simulation.
 * First read the TCL commands, and register
 * those that are written in C++. Then read the configuration
 * file (if any), and start separate processes for each
 * simulated router. When the router connects
 * to the controller, send it its complete configuration.
 */

int Tcl_AppInit(Tcl_Interp *interp)

{
//  rlimit rlim;
    int fd;
    struct sockaddr_in addr;
    socklen size;
    int namlen;
    char *filename;

    // Allow core files
//  rlim.rlim_max = RLIM_INFINITY;
//  (void) setrlimit(RLIMIT_CORE, &rlim);
    // Create simulation controller
    sim = new SimCtl(interp);
    // Complete interpreter initialization
    if (Tcl_Init(interp) != TCL_OK) {
        printf("Error in Tcl_Init(): %s\n", interp->result);
	exit(1);
    }
    if (Tk_Init(interp) != TCL_OK) {
        printf("Error in Tk_Init(): %s\n", interp->result);
	exit(1);
    }
    // Install C-language TCl commands
    Tcl_CreateCommand(interp, "startrtr", StartRouter, 0, 0);
    Tcl_CreateCommand(interp, "togglertr", ToggleRouter, 0, 0);
    Tcl_CreateCommand(interp, "rstrtr", RestartRouter, 0, 0);
    Tcl_CreateCommand(interp, "hitlessrtr", HitlessRestart, 0, 0);
    Tcl_CreateCommand(interp, "start_ping", StartPing, 0, 0);
    Tcl_CreateCommand(interp, "stop_ping", StopPing, 0, 0);
    Tcl_CreateCommand(interp, "start_traceroute", StartTraceroute, 0, 0);
    Tcl_CreateCommand(interp, "start_mtrace", StartMtrace, 0, 0);
    Tcl_CreateCommand(interp, "prefix_match", PrefixMatch, 0, 0);
    Tcl_CreateCommand(interp, "add_mapping", AddMapping, 0, 0);
    Tcl_CreateCommand(interp, "add_net_membership", AddNetMember, 0, 0);
    Tcl_CreateCommand(interp, "time_stop", TimeStop, 0, 0);
    Tcl_CreateCommand(interp, "time_resume", TimeResume, 0, 0);
    Tcl_CreateCommand(interp, "sendgen", SendGeneral, 0, 0);
    Tcl_CreateCommand(interp, "sendarea", SendArea, 0, 0);
    Tcl_CreateCommand(interp, "sendifc", SendInterface, 0, 0);
    Tcl_CreateCommand(interp, "sendvl", SendVL, 0, 0);
    Tcl_CreateCommand(interp, "sendnbr", SendNeighbor, 0, 0);
    Tcl_CreateCommand(interp, "sendagg", SendAggregate, 0, 0);
    Tcl_CreateCommand(interp, "sendhost", SendHost, 0, 0);
    Tcl_CreateCommand(interp, "sendextrt", SendExtRt, 0, 0);
    Tcl_CreateCommand(interp, "sendgrp", SendGroup, 0, 0);
    Tcl_CreateCommand(interp, "leavegrp", LeaveGroup, 0, 0);
    // Read additional TCL commands
    namlen = strlen(INSTALL_DIR) + strlen(sim_tcl_src);
    filename = new char[namlen+1];
    strcpy(filename, INSTALL_DIR);
    strcat(filename, sim_tcl_src);
    if (Tcl_EvalFile(interp, filename) != TCL_OK) {
	printf("Error in %s, line %d\r\n", filename, interp->errorLine);
	exit(1);
    }
    delete [] filename;
    // Create server socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    sim->server_fd = fd;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	perror("bind");
	exit (1);
    }
    size = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *) &addr, &size) < 0) {
	perror("getsockname");
	exit (1);
    }
    sim->assigned_port = ntohs(addr.sin_port);

    // Read config file
    if (cfgfile) {
	if (Tcl_EvalFile(interp, cfgfile) != TCL_OK) {
	    printf("Error in %s, line %d", cfgfile, interp->errorLine);
	    exit(1);
	}
	// Tell TCL the config file name
	if (Tcl_VarEval(sim->interp, "ConfigFile ", cfgfile, 0) != TCL_OK)
            printf("ConfigFile: %s\n", sim->interp->result);
    }
    // Use the sample.cfg configuration
    else {
        extern char *sample_cfg;
	int size;
	char *cmd;
	size = strlen(sample_cfg);
	cmd = new char[size+1];
	strcpy(cmd, sample_cfg);
	if (Tcl_Eval(sim->interp, cmd) != TCL_OK) {
            printf("sample.cfg: %s\n", sim->interp->result);
	    exit(1);
	}
    }

    // Now we're up and running
    sim->running = true;
    // Listen for simulated nodes that are initializing
    listen(fd, 150);
    // Start timer ticks
    Tk_CreateTimerHandler(1000/TICKS_PER_SECOND, tick, 0);
    return(TCL_OK);
}

/* Accept an incoming connection from a simulated router.
 * We don't add a simulated node yet, because we have to get
 * the first message to find out the router ID of the other
 * end.
 */

void SimCtl::incoming_call()

{
    int fd;
    struct sockaddr_in addr;
    socklen len;
    SimNode *temp;

    len = sizeof(addr);
    if (!(fd = accept(server_fd, (struct sockaddr *) &addr, &len))) {
	perror("incoming_call:accept");
	return;
    }

    // Allocate placeholder SimNode
    temp = new SimNode(0, fd);
}

/* Tick processing. If all nodes have responded, increase
 * the current time and send out a new round of ticks.
 */

void tick(ClientData)

{
    bool all_responded=true;
    AVLsearch *iter;
    SimNode *node;
    int max_sync = 0;
    NodeStats *max_dbstats=0;

    // Process all I/O from simulated nodes
    while (sim->process_replies())
        ;

    // Check to see that all have responded
    iter = new AVLsearch(&sim->simnodes);
    while ((node = (SimNode *)iter->next())) {
        if (!node->got_tick) {
	    all_responded = false;
	    break;
	}
    }
    delete iter;
    // Recolor map according to latest database
    // statistics received
    iter = new AVLsearch(&sim->simnodes);
    while ((node = (SimNode *)iter->next())) {
        if (node->dbstats && node->dbstats->refct >= max_sync) {
	    max_sync = node->dbstats->refct;
	    max_dbstats = node->dbstats;
	}
    }
    delete iter;
    iter = new AVLsearch(&sim->simnodes);
    while ((node = (SimNode *)iter->next())) {
        in_addr addr;
	int old_color=node->color;
	char *color;
        if (!node->dbstats || node->dbstats->refct == 1)
	    node->color = SimNode::WHITE;
	else if (node->dbstats == max_dbstats)
	    node->color = SimNode::GREEN;
	else
	    node->color = SimNode::ORANGE;
	if (old_color == node->color)
	    continue;
	switch (node->color) {
	  default:
	  case SimNode::WHITE:
	    color = " white";
	    break;
	  case SimNode::GREEN:
	    color = " green";
	    break;
	  case SimNode::ORANGE:
	    color = " orange";
	    break;
	}
	addr.s_addr = hton32(node->id());
	if (Tcl_VarEval(sim->interp, "color_router ", inet_ntoa(addr),
			color, 0) != TCL_OK)
	    printf("color_router: %s\n", sim->interp->result);
    }
    delete iter;
    // If all responded, and we're not frozen
    // Send out new timer ticks
    if (all_responded && !sim->frozen) {
        char display_buffer[20];
        sim->n_ticks++;
	sprintf(display_buffer, "%d", sim->n_ticks);
	// Update displayed time
	if (Tcl_VarEval(sim->interp, "show_time ",display_buffer, 0) != TCL_OK)
	    printf("show_time: %s\n", sim->interp->result);
	iter = new AVLsearch(&sim->simnodes);
	while ((node = (SimNode *)iter->next())) {
	    TickBody tm;
	    node->got_tick = false;
	    tm.tick = sim->n_ticks;
	    node->pktdata.queue_xpkt(&tm, SIM_TICK, 0, sizeof(tm));
	}
	delete iter;
    }

    // Send all pending data to simulated routers
    while (sim->send_data())
        ;

    // Regardless, schedule next tick() invocation
    Tk_CreateTimerHandler(1000/TICKS_PER_SECOND, tick, 0);
}

/* I/0 activity on the connection to a simulated router.
 * Process any reads, and send pending data if there is
 * room.
 */

void SimCtl::simnode_handler_read(int fd)

{
    SimNode *node;
    void *msg;
    uns16 type;
    uns16 subtype;
    int nbytes;

    if (!(node = sim->nodes[fd]))
        return;
    nbytes = node->pktdata.receive(&msg, type, subtype);
    if (nbytes < 0) {
	sim->delete_router(node);
	return;
    }
    if (type != 0) {
	SimHello *hello;
	DBStats *dbstats;
	in_addr addr;
	char *msgbuf;
	char *text_msg;
	switch(type) {
	    char tcl_command[80];
	  case SIM_HELLO:
	    hello = (SimHello *)msg;
	    // Start the node, reassigning simnode class
	    restart_node(node, hello->rtrid, fd, ntoh16(hello->myport));
	    break;
	  case SIM_TICK_RESPONSE:
            NodeStats *statentry;
	    dbstats = (DBStats *)msg;
	    node->got_tick = true;
	    statentry = (NodeStats *) stats.find((byte *)dbstats,
						 sizeof(DBStats));
	    if (statentry && statentry == node->dbstats)
	        break;
	    if (node->dbstats && --(node->dbstats->refct) == 0) {
	        stats.remove(node->dbstats);
	        delete node->dbstats;
	    }
	    if (!statentry) {
	        statentry = new NodeStats(dbstats);
		stats.add(statentry);
	    }
	    node->dbstats = statentry;
	    statentry->refct++;
	    break;
	  case SIM_LOGMSG:
	    msgbuf = (char *)msg;
	    msgbuf[nbytes-1] = '\0';
	    addr.s_addr = hton32(node->id());
	    printf("%d:%03d (%s) OSPF.%03d: %s\n",
		    sim->elapsed_seconds(), sim->elapsed_milliseconds(),
		    inet_ntoa(addr), subtype, msgbuf);
	    fflush(stdout);
	    break;
	  case SIM_ECHO_REPLY:
	    EchoReplyMsg *em;
	    em = (EchoReplyMsg *)msg;
	    addr.s_addr = hton32(em->src);
	    sprintf(tcl_command, "ping_reply %d %s %d %d %d",
		    subtype, inet_ntoa(addr), em->icmp_seq, em->ttl,
		    em->msd);
	    if (Tcl_VarEval(sim->interp, tcl_command, 0) != TCL_OK)
	        printf("ping_reply: %s\n", sim->interp->result);
	    break;
	  case SIM_ICMP_ERROR:
	    IcmpErrMsg *errmsg;
	    errmsg = (IcmpErrMsg *)msg;
	    addr.s_addr = hton32(errmsg->src);
	    sprintf(tcl_command, "icmp_error %d %s %d %d %d",
		    subtype, inet_ntoa(addr), errmsg->type, errmsg->code,
		    errmsg->msd);
	    if (Tcl_VarEval(sim->interp, tcl_command, 0) != TCL_OK)
	        printf("icmp_error: %s\n", sim->interp->result);
	    break;
	  case SIM_TRACEROUTE_TTL:
	    TrTtlMsg *ttlm;
	    ttlm = (TrTtlMsg *)msg;
	    sprintf(tcl_command, "traceroute_ttl %d %d",
		    subtype, ttlm->ttl);
	    if (Tcl_VarEval(sim->interp, tcl_command, 0) != TCL_OK)
	        printf("traceroute_ttl: %s\n", sim->interp->result);
	    break;
	  case SIM_TRACEROUTE_TMO:
	    sprintf(tcl_command, "print_session %d \"* \"", subtype);
	    if (Tcl_VarEval(sim->interp, tcl_command, 0) != TCL_OK)
	        printf("print_session: %s\n", sim->interp->result);
	    break;
	  case SIM_PRINT_SESSION:
	    text_msg=(char *)msg;
	    sprintf(tcl_command, "print_session %d \"%s\"", subtype,text_msg);
	    if (Tcl_VarEval(sim->interp, tcl_command, 0) != TCL_OK)
	        printf("print_session: %s\n", sim->interp->result);
	    break;
	  default:
	    addr.s_addr = node->id();
	    printf("Bad type %d from %s", type, inet_ntoa(addr));
	    break;
	}
    }
}

/* (Re)start a router.
 */

void SimCtl::restart_node(SimNode *node, InAddr id, int fd, uns16 home_port)

{
    in_addr addr;
    TickBody tm;
    SimNode *newnode;

    newnode = new SimNode(id, fd);
    sim->simnodes.add(newnode);
    newnode->home_port = home_port;
    addr.s_addr = hton32(newnode->id());
    newnode->color = SimNode::WHITE;
    if (Tcl_VarEval(sim->interp, "color_router ", inet_ntoa(addr),
		    " white", 0) != TCL_OK)
        printf("color_router: %s\n", sim->interp->result);
    // If the router has already been running, tell it to restart
    if (node->id() != 0)
        newnode->pktdata.queue_xpkt(NULL, SIM_RESTART, 0, 0);
    // Initialize its idea of time
    tm.tick = sim->n_ticks;
    newnode->pktdata.queue_xpkt(&tm, SIM_FIRST_TICK, 0, sizeof(tm));
    // Send address to port maps
    send_addrmap(newnode);
    send_addrmap_increment(0, newnode);
    // Download node's configuration
    addr.s_addr = hton32(newnode->id());
    if (Tcl_VarEval(sim->interp,"sendcfg ", inet_ntoa(addr),0) != TCL_OK)
        printf("sendcfg: %s\n", sim->interp->result);

    // Delete previous router
    // Also frees message space
    delete node;
}

/* Construct a node statistics entry.
 */

NodeStats::NodeStats(DBStats *d)

{
    dbstats = *d;
    key = (byte *) &dbstats;
    keylen = sizeof(dbstats);
    refct = 0;
}

/* Send the current address to port map to a particular
 * router.
 */

void SimCtl::send_addrmap(SimNode *node)

{
    byte *msg;
    AVLsearch iter(&ifmaps);
    int size;
    IfMap *map;
    AddrMap *addrmap;

    if (!(size = (ifmaps.size() * sizeof(*addrmap))))
	return;
    msg = new byte[size];
    addrmap = (AddrMap *) msg;
    for (size = 0; (map = (IfMap *) iter.next()); ) {
	SimNode *home;
	if (!(home = (SimNode *) simnodes.find(map->owner, 0)))
	    continue;
	addrmap->addr = map->index1();
	addrmap->port = home->home_port;
	addrmap->home = home->id();
	addrmap++;
	size += sizeof(*addrmap);
    }
    node->pktdata.queue_xpkt(msg, SIM_ADDRMAP, 0, size);
    delete [] msg;
}

/* Send a new router's interfaces to all existing routers.
 */

void SimCtl::send_addrmap_increment(IfMap *newmap, SimNode *newnode)

{
    byte *msg;
    AVLsearch iter(&ifmaps);
    int size;
    IfMap *map;
    AddrMap *addrmap;
    AVLsearch niter(&sim->simnodes);
    SimNode *node;
    SimNode *home;

    if (!running)
        return;
    if (newmap)
	size = sizeof(*addrmap);
    else
	size = ifmaps.size() * sizeof(*addrmap);
    if (size == 0)
	return;

    msg = new byte[size];
    addrmap = (AddrMap *) msg;
    size = 0;

    if (newmap) {
	home = (SimNode *) simnodes.find(newmap->owner, 0);
	if (home) {
	    addrmap->addr = newmap->index1();
	    addrmap->port = home->home_port;
	    addrmap->home = home->id();
	    addrmap++;
	    size += sizeof(*addrmap);
	}
    }
    else {
	while ((map = (IfMap *) iter.next())) {
	    home = (SimNode *) simnodes.find(map->owner, 0);
	    if (home != newnode)
		continue;
	    addrmap->addr = map->index1();
	    addrmap->port = home->home_port;
	    addrmap->home = home->id();
	    addrmap++;
	    size += sizeof(*addrmap);
	}
    }

    while ((node = (SimNode *)niter.next())) {
        if (!newmap && node == newnode)
	    continue;
	node->pktdata.queue_xpkt(msg, SIM_ADDRMAP, 0, size);
    }

    delete [] msg;
}

/* Create a simulated router.
 * Give it the benefit of the doubt on the first tick.
 */

SimNode::SimNode(uns32 id, int file) : AVLitem(id, 0), pktdata(file)

{
    fd = file;
    got_tick = true;
    home_port = 0;
    awaiting_htl_restart = false;
    dbstats = 0;
    sim->nodes[fd] = this;
    sim->maxfd = MAX(file, sim->maxfd);
    color = RED;
}

/* Delete a simulated router.
 */

void SimCtl::delete_router(SimNode *node)

{
    if (node->id()) {
        in_addr addr;
        close(node->fd);
	node->home_port = 0;
	send_addrmap_increment(0, node);
	addr.s_addr = hton32(node->id());
	if (Tcl_VarEval(interp, "color_router ", inet_ntoa(addr),
			" red", 0) != TCL_OK)
	    printf("color_router: %s\n", interp->result);
	if (node->dbstats && --(node->dbstats->refct) == 0) {
	    stats.remove(node->dbstats);
	    delete node->dbstats;
	}
	simnodes.remove(node);
    }
    sim->nodes[node->fd] = 0;
    delete node;
}

/* Toggle the operation state of a simulated router.
 */

int ToggleRouter(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        Tcl_VarEval(interp, "startrtr ", argv[1], 0);
    else
        node->pktdata.queue_xpkt(NULL, SIM_SHUTDOWN, 0, 0);

    return(TCL_OK);
}

/* Restart a simulated router.
 * If it is not running, simply start it.
 */

int RestartRouter(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        Tcl_VarEval(interp, "startrtr ", argv[1], 0);
    else {
	sim->simnodes.remove(node);
	sim->restart_node(node, node->id(), node->fd, node->home_port);
    }

    return(TCL_OK);
}

/* Perform a hitless restart of a simulated router.
 * If it is not running, simply start it.
 */

int HitlessRestart(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;
    HitlessRestartMsg m;

    m.period = 100;
    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        Tcl_VarEval(interp, "startrtr ", argv[1], 0);
    else if (node->awaiting_htl_restart) {
	// Complete hitless restart
        in_addr addr;
        node->awaiting_htl_restart =  false;
        node->pktdata.queue_xpkt(&m, SIM_RESTART_HITLESS, 0, sizeof(m));
	// Download node's configuration
	addr.s_addr = hton32(node->id());
	if (Tcl_VarEval(sim->interp,"sendcfg ", inet_ntoa(addr),0) != TCL_OK)
	    printf("sendcfg: %s\n", sim->interp->result);
    }
    else {
        // Prepare for hitless restart
        node->awaiting_htl_restart = true;
        node->pktdata.queue_xpkt(&m, SIM_RESTART_HITLESS, 0, sizeof(m));
    }
    return(TCL_OK);
}

/* Start a ping session in the specified router.
 */

int StartPing(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;
    PingStartMsg m;
    uns16 s_id;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.src = ntoh32(inet_addr(argv[2]));
    m.dest = ntoh32(inet_addr(argv[3]));
    m.ttl = atoi(argv[4]);
    s_id = atoi(argv[5]);
    node->pktdata.queue_xpkt(&m, SIM_START_PING, s_id, sizeof(m));

    return(TCL_OK);
}

/* Stop a ping session in the specified router.
 */

int StopPing(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;
    uns16 s_id;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    s_id = atoi(argv[2]);
    node->pktdata.queue_xpkt(0, SIM_STOP_PING, s_id, 0);

    return(TCL_OK);
}

/* Start a traceroute session in the specified router.
 */

int StartTraceroute(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;
    PingStartMsg m;
    uns16 s_id;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.dest = ntoh32(inet_addr(argv[3]));
    m.ttl = atoi(argv[4]);
    s_id = atoi(argv[5]);
    node->pktdata.queue_xpkt(&m, SIM_START_TR, s_id, sizeof(m));

    return(TCL_OK);
}

/* Start a multicast traceroute session in the specified router.
 */

int StartMtrace(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr id;
    SimNode *node;
    MTraceHdr m;
    uns16 s_id;
    InAddr net;
    InMask mask;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.src = ntoh32(inet_addr(argv[2]));
    get_prefix(argv[3], net, mask);
    m.dest = net;
    m.group = ntoh32(inet_addr(argv[4]));
    s_id = atoi(argv[5]);
    m.ttl_qid = atoi(argv[6]); // phyint of destination net
    node->pktdata.queue_xpkt(&m, SIM_START_MTRACE, s_id, sizeof(m));

    return(TCL_OK);
}

/* Determine whether an address falls under a particular
 * prefix.
 */

int PrefixMatch(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr net;
    InAddr mask;
    InAddr addr;

    Tcl_SetResult(interp, "1", TCL_STATIC);
    if (get_prefix(argv[1], net, mask)) {
        addr = ntoh32(inet_addr(argv[2]));
	if ((addr & mask) == net)
	    Tcl_SetResult(interp, "0", TCL_STATIC);
    }
    return(TCL_OK);
}

/* Add mapping between IP address and owning router
 */

int AddMapping(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    InAddr addr;
    InAddr mask;
    InAddr rtr;

    if (get_prefix(argv[1], addr, mask)) {
        if (mask != 0xffffffff)
	    return(TCL_OK);
    } else
        addr = ntoh32(inet_addr(argv[1]));
    rtr = ntoh32(inet_addr(argv[2]));
    sim->store_mapping(addr, rtr);
    return(TCL_OK);
}

/* Add mapping between network and attached router
 */

int AddNetMember(ClientData, Tcl_Interp *interp, int, char *argv[])

{
    uns16 port;
    InAddr rtr;

    port = atoi(argv[1]);
    rtr = ntoh32(inet_addr(argv[2]));
    sim->store_mapping(port, rtr);
    return(TCL_OK);
}

/* Add mapping between IP address and owning router
 */

void SimCtl::store_mapping(uns32 port_or_addr, uns32 rtr)

{
    IfMap *map;

    if (port_or_addr != 0) {
	map = new IfMap(port_or_addr, rtr);
	ifmaps.add(map);
	send_addrmap_increment(map, 0);
    }
}

/* Stop the simulated time.
 */

int TimeStop(ClientData, Tcl_Interp *, int, char *[])

{
    sim->frozen = true;
    return(TCL_OK);
}

/* Resum the simulated time.
 */

int TimeResume(ClientData, Tcl_Interp *, int, char *[])

{
    sim->frozen = false;
    return(TCL_OK);
}

/* Download the global configuration values into the ospfd
 * software. If try to change Router ID, refuse reconfig.
 * If first time, create OSPF protocol instance.
 */

int SendGeneral(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgGen m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.lsdb_limit = 0;
    m.mospf_enabled = atoi(argv[3]);
    m.inter_area_mc = 1;
    m.ovfl_int = 300;
    m.new_flood_rate = 1000;
    m.max_rxmt_window = 8;
    m.max_dds = 2;
    m.host_mode = atoi(argv[2]);
    m.log_priority = 2;
    m.refresh_rate = 6000;
    m.PPAdjLimit = atoi(argv[4]);
    m.random_refresh = atoi(argv[5]);
    node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Gen, len);

    return(TCL_OK);
}

/* Download configuration of a single area
 */

int SendArea(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgArea m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.area_id = ntoh32(inet_addr(argv[2]));
    m.stub = atoi(argv[3]);
    m.dflt_cost = atoi(argv[4]);
    m.import_summs = atoi(argv[5]);
    node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Area, len);

    return(TCL_OK);
}

/* Download an interface's configuration.
 * Interface can by identified by its address, name, or
 * for point-to-point addresses, the other end of the link.
 */

int SendInterface(ClientData, Tcl_Interp *, int, char *argv[])

{
    CfgIfc m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);
    int port;
    InAddr net;
    InAddr mask;
    int command;
    bool enabled;
    bool run_ospf;

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    // Figure out interface type
    if (strcmp(argv[3], "broadcast") == 0)
	m.IfType = IFT_BROADCAST;
    else if (strcmp(argv[3], "pp") == 0)
	m.IfType = IFT_PP;
    else if (strcmp(argv[3], "nbma") == 0)
	m.IfType = IFT_NBMA;
    else if (strcmp(argv[3], "ptmp") == 0)
	m.IfType = IFT_P2MP;
    else
        return(TCL_ERROR);

    port = atoi(argv[2]);
    m.address = ntoh32(inet_addr(argv[5]));
    m.phyint = port;
    get_prefix(argv[8], net, mask);
    m.mask = mask;
    m.mtu = (m.IfType == IFT_BROADCAST ? 1500 : 2048);
    m.IfIndex = atoi(argv[7]);
    m.area_id = ntoh32(inet_addr(argv[4]));
    m.dr_pri = atoi(argv[13]);
    m.xmt_dly = 1;
    m.rxmt_int = 5;
    m.hello_int = 10;
    m.if_cost = atoi(argv[6]);
    m.dead_int = 40;
    m.poll_int = 60;
    m.auth_type = 0;
    memset(m.auth_key, 0, 8);
    m.mc_fwd = 1;
    m.demand = atoi(argv[9]);
    m.passive = atoi(argv[11]);
    m.igmp = ((m.IfType == IFT_BROADCAST) ? 1 : 0);
    enabled = (atoi(argv[10]) != 0);
    run_ospf = (atoi(argv[12]) != 0);
    command = (enabled && run_ospf) ? SIM_CONFIG : SIM_CONFIG_DEL;
    node->pktdata.queue_xpkt(&m, command, CfgType_Ifc, len);

    if (m.IfType == IFT_BROADCAST || m.IfType == IFT_NBMA) {
        CfgExRt rtm;
	rtm.net = net;
	rtm.mask = mask;
	rtm.type2 = 0;
	rtm.mc = 0;
	rtm.direct = 1;
	rtm.noadv = !run_ospf;
	rtm.cost = 1;
	rtm.gw = 0;
	rtm.phyint = 0;
	rtm.tag = 0;
	command = (enabled && !run_ospf) ? SIM_CONFIG : SIM_CONFIG_DEL;
	node->pktdata.queue_xpkt(&rtm, command, CfgType_Route, len);
    }

    return(TCL_OK);
}

int SendVL(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgVL m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.nbr_id = ntoh32(inet_addr(argv[3]));
    m.transit_area = ntoh32(inet_addr(argv[2]));
    m.xmt_dly = 1;
    m.rxmt_int = 5;
    m.hello_int = 10;
    m.dead_int = 60;
    m.auth_type = 0;
    memset(m.auth_key, 0, 8);
    node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_VL, len);

    return(TCL_OK);
}

int SendNeighbor(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgNbr m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.nbr_addr = ntoh32(inet_addr(argv[2]));
    m.dr_eligible = atoi(argv[3]);
    node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Nbr, len);

    return(TCL_OK);
}

int SendAggregate(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgRnge m;
    InAddr net;
    InAddr mask;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    if (get_prefix(argv[3], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.area_id = ntoh32(inet_addr(argv[2]));
	m.no_adv = atoi(argv[4]);
	node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Range, len);
    }

    return(TCL_OK);
}

int SendHost(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgHost m;
    InAddr net;
    InAddr mask;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    if (get_prefix(argv[2], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.area_id = ntoh32(inet_addr(argv[3]));
	m.cost = 0;
	node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Host, len);
    }

    return(TCL_OK);
}

int SendExtRt(ClientData, Tcl_Interp *, int, char *argv[])
{
    CfgExRt m;
    InAddr net;
    InMask mask;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    if (get_prefix(argv[2], net, mask)) {
	m.net = net;
	m.mask = mask;
	m.type2 = (atoi(argv[4]) == 2);
	m.mc = 0;
	m.direct = 0;
	m.noadv = (atoi(argv[6]) != 0);
	m.cost = atoi(argv[5]);
	m.gw = ntoh32(inet_addr(argv[3]));
	m.phyint = (m.gw != 0) ? -1 : 0;
	m.tag = 0;
	node->pktdata.queue_xpkt(&m, SIM_CONFIG, CfgType_Route, len);
    }
    return(TCL_OK);
}

/* Inform the simulated router of a group member on one of
 * its attached interfaces.
 */

int SendGroup(ClientData, Tcl_Interp *, int, char *argv[])
{
    GroupMsg m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.phyint = atoi(argv[3]);
    m.group = ntoh32(inet_addr(argv[2]));
    node->pktdata.queue_xpkt(&m, SIM_ADD_MEMBER, 0, len);
    return(TCL_OK);
}

/* Similarly, a group member has left on one of the
 * attached interfaces.
 */

int LeaveGroup(ClientData, Tcl_Interp *, int, char *argv[])
{
    GroupMsg m;
    InAddr id;
    SimNode *node;
    int len = sizeof(m);

    id = ntoh32(inet_addr(argv[1]));
    if (!(node = (SimNode *) sim->simnodes.find(id, 0)))
        return(TCL_OK);

    m.phyint = atoi(argv[3]);
    m.group = ntoh32(inet_addr(argv[2]));
    node->pktdata.queue_xpkt(&m, SIM_DEL_MEMBER, 0, len);
    return(TCL_OK);
}

/* Utility to parse prefixes. Returns false if the
 * prefix is malformed.
 */

InMask masks[33] = {
    0x00000000L, 0x80000000L, 0xc0000000L, 0xe0000000L,
    0xf0000000L, 0xf8000000L, 0xfc000000L, 0xfe000000L,
    0xff000000L, 0xff800000L, 0xffc00000L, 0xffe00000L,
    0xfff00000L, 0xfff80000L, 0xfffc0000L, 0xfffe0000L,
    0xffff0000L, 0xffff8000L, 0xffffc000L, 0xffffe000L,
    0xfffff000L, 0xfffff800L, 0xfffffc00L, 0xfffffe00L,
    0xffffff00L, 0xffffff80L, 0xffffffc0L, 0xffffffe0L,
    0xfffffff0L, 0x80000000L, 0xfffffffcL, 0xfffffffeL,
    0xffffffffL
};

bool get_prefix(char *prefix, InAddr &net, InMask &mask)

{
    char *string;
    char temp[20];
    char *netstr;
    int len;

    strncpy(temp, prefix, sizeof(temp));
    string = temp;
    if (!(netstr = strsep(&string, "/")) || string == 0)
	return(false);
    net = ntoh32(inet_addr(netstr));
    len = atoi(string);
    if (len < 0 || len > 32)
	return(false);
    mask = masks[len];
    return(true);
}

// The sample.cfg configuration file

char *sample_cfg = "\
router 10.0.0.1 210 209 1\n\
router 10.0.0.2 248 122 1\n\
router 10.0.0.3 313 120 1\n\
router 10.0.0.4 349 209 1\n\
router 10.1.1.8 124.0 116.0 1\n\
router 10.0.0.9 125.0 267.0 1\n\
router 10.0.0.10 246.0 40.0 1\n\
router 10.0.0.11 313.0 40.0 1\n\
router 10.0.0.12 399.0 96.0 1\n\
router 10.0.0.13 519.0 94.0 1\n\
router 10.0.0.5 453.0 166.0 1\n\
router 10.0.0.6 453.0 246.0 1\n\
router 10.0.0.7 387.0 290.0 1\n\
router 10.0.0.14 528.0 297.0 1\n\
nbma 10.1.3.0/24 0.0.0.2 125.0 191.0 0\n\
broadcast 10.1.2.0/24 0.0.0.2 45.0 119.0 0\n\
broadcast 10.1.1.0/24 0.0.0.2 43.0 266.0 0\n\
broadcast 10.2.2.0/24 0.0.0.1 380.0 20.0 0\n\
broadcast 10.2.1.0/24 0.0.0.1 178.0 21.0 0\n\
broadcast 10.4.1.0/24 0.0.0.4 460.0 42.0 0\n\
broadcast 10.4.2.0/24 0.0.0.5 459.0 330.0 0\n\
broadcast 10.3.7.0/24 0.0.0.3 537.0 165.0 0\n\
broadcast 10.8.2.0/24 0.0.0.3 529.0 244.0 0\n\
pplink 10.0.0.1 0.0.0.0 1 10.0.0.2 0.0.0.0 1 0.0.0.0 0\n\
pplink 10.0.0.1 0.0.0.0 2 10.0.0.3 0.0.0.0 2 0.0.0.0 0\n\
pplink 10.0.0.1 0.0.0.0 1 10.0.0.4 0.0.0.0 1 0.0.0.0 0\n\
interface 10.0.0.1 10.1.3.1 3 0 1\n\
neighbor 10.0.0.1 10.1.3.8 0\n\
neighbor 10.0.0.1 10.1.3.9 0\n\
loopback 10.0.0.1 1.0.0.1/32 0.0.0.0\n\
pplink 10.0.0.2 0.0.0.0 2 10.0.0.4 0.0.0.0 2 0.0.0.0 0\n\
loopback 10.0.0.2 1.0.0.2/32 0.0.0.0\n\
pplink 10.0.0.3 0.0.0.0 1 10.0.0.4 0.0.0.0 1 0.0.0.0 0\n\
loopback 10.0.0.3 1.0.0.3/32 0.0.0.0\n\
pplink 10.0.0.4 192.168.4.1 3 10.0.0.5 192.168.5.1 3 0.0.0.3 0\n\
pplink 10.0.0.4 192.168.4.2 3 10.0.0.6 192.168.6.1 3 0.0.0.3 0\n\
loopback 10.0.0.4 1.0.0.4/32 0.0.0.0\n\
interface 10.1.1.8 10.1.3.8 3 0 1\n\
drpri 10.1.1.8 10.1.3.8 0\n\
interface 10.1.1.8 10.1.2.8 1 0 1\n\
interface 10.0.0.9 10.1.3.9 3 0 1\n\
drpri 10.0.0.9 10.1.3.9 0\n\
interface 10.0.0.9 10.1.1.9 1 0 1\n\
interface 10.0.0.10 10.2.1.10 1 0 1\n\
pplink 10.0.0.10 0.0.0.0 3 10.0.0.2 0.0.0.0 3 0.0.0.1 0\n\
pplink 10.0.0.10 0.0.0.0 3 10.0.0.11 0.0.0.0 3 0.0.0.1 1\n\
interface 10.0.0.11 10.2.2.11 1 0 1\n\
pplink 10.0.0.11 0.0.0.0 3 10.0.0.3 0.0.0.0 3 0.0.0.1 1\n\
interface 10.0.0.12 10.4.1.12 1 0 1\n\
pplink 10.0.0.12 0.0.0.0 3 10.0.0.5 0.0.0.0 3 0.0.0.4 0\n\
interface 10.0.0.13 10.4.1.13 1 0 1\n\
pplink 10.0.0.13 0.0.0.0 3 10.0.0.5 0.0.0.0 3 0.0.0.4 0\n\
interface 10.0.0.5 10.3.7.5 1 0 1\n\
pplink 10.0.0.5 0.0.0.0 3 10.0.0.6 0.0.0.0 3 0.0.0.3 0\n\
pplink 10.0.0.6 0.0.0.0 3 10.0.0.7 0.0.0.0 3 0.0.0.5 0\n\
pplink 10.0.0.6 0.0.0.0 3 10.0.0.14 0.0.0.0 3 0.0.0.5 0\n\
interface 10.0.0.6 10.8.2.6 1 0 1\n\
interface 10.0.0.7 10.4.2.7 1 0 1\n\
interface 10.0.0.14 10.4.2.6 1 0 1\n\
extrt 10.0.0.2 1.0.0.0/8 0.0.0.0 2 1 0\n\
extrt 10.0.0.2 2.0.0.0/8 0.0.0.0 2 1 0\n\
extrt 10.0.0.2 4.0.0.0/8 11.0.0.254 2 1 0\n\
extrt 10.0.0.2 3.0.0.0/8 0.0.0.0 1 10 0\n\
extrt 10.0.0.12 5.0.0.0/8 0.0.0.0 1 10 0\n\
extrt 10.0.0.12 6.0.0.0/8 0.0.0.0 1 10 0\n\
extrt 10.1.1.8 0.0.0.0/0 10.1.2.254 1 1 0\n\
extrt 10.1.1.8 3.0.0.0/8 0.0.0.0 2 1 0\n\
vlink 10.0.0.4 10.0.0.5 0.0.0.3\n\
vlink 10.0.0.4 10.0.0.6 0.0.0.3\n\
aggr 10.0.0.1 0.0.0.2 10.1.0.0/16 0\n\
aggr 10.0.0.2 0.0.0.1 10.2.0.0/16 0\n\
aggr 10.0.0.3 0.0.0.1 10.2.0.0/16 0\n";
