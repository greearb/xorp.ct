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

/* OspfSysCalls class encapsulating the Linux behaviors
 * common to both the Linux ospfd routing daemon and the
 * Linux routing simulator.
 */

const int OSPFD_MON_PORT = 12767;

class OSInstance : public OspfSysCalls {
    uns16 ospfd_mon_port;
    int listenfd; // Listen for monitoring connection
    AVLtree monfds; // Current monitoring connections
  public:
    void monitor_response(struct MonMsg *, uns16, int, int);

    OSInstance(uns16 mon_port);
    void mon_fd_set(int &, fd_set *, fd_set *);
    void process_mon_io(fd_set *, fd_set *);
    void process_monitor_request(class TcpConn *);
    void accept_monitor_connection();
    void close_monitor_connection(class TcpConn *);
    void close_monitor_connections();
    void monitor_listen();
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
