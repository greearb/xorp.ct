/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
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

/* OspfSysCalls class encapsulating the Xorp behaviors
 * common to both the Xorp ospfd routing daemon and the
 * Xorp routing simulator.
 */

/* The name here is a (slight misnomer) OSInstance is really the
 * monitor interface i/o controller.  Deriving from OspfSysCalls is
 * not in practice useful, could just as well have as has-a object in
 * the real platform specific class, ie LinuxOspfd, XorpOspfd.  
 *
 * We've tweaked this code from the Linux version to include the
 * Xorp Eventloop and use it's selectorlist to detect i/o readiness.
 */

#include "ospf_module.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

const int OSPFD_MON_PORT = 12767;

class OSInstance : public OspfSysCalls {
  public:
    void monitor_response(struct MonMsg *, uns16, int, int);

    OSInstance(EventLoop& e, uns16 mon_port);
    void mon_fd_set(int &, fd_set *, fd_set *);
    void process_monitor_request(class TcpConn *);
    void accept_monitor_connection();
    void close_monitor_connection(class TcpConn *);
    void close_monitor_connections();
    void monitor_listen();
    inline int get_listenfd() const { return listenfd; }

 private:
    void accept_cb(int fd, SelectorMask);
    void tcp_data_cb(int fd, SelectorMask, struct TcpConn*);

 private:
    uns16 ospfd_mon_port;
    int listenfd; // Listen for monitoring connection
    AVLtree monfds; // Current monitoring connections

 protected:
    EventLoop& _event_loop;
};

class TcpConn : public AVLitem {
    TcpPkt monpkt; // Packet processing for monitor connection
  public:
    inline int monfd();	// Monitoring connection
    TcpConn(int fd);
    friend class OSInstance;
};

inline int TcpConn::monfd()
{
    return(index1());
}
