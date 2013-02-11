// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
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
// ===========================================================================

#ifndef __WRAPPER_HH__
#define __WRAPPER_HH__

#include "wrapper_types.hh"

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#define LISTENQLEN   (1024)   /*  Backlog for listen()   */

#define MAXBUFFER	65535

#include "policy/backend/policytags.hh"
#include "libxorp/profile.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxipc/xrl_error.hh"
#include "policy/backend/policy_filters.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/run_command.hh"


class IO;

enum wrapperStatus {
    QUITTING,
    RUNNING,
    WAITTING,
    INIT
};

typedef struct tcp_udp_opend_st {
    bool        _used;
    std::string      sock;

} tcp_udp_opend_t;
#define MAX_OPEN_SOCK	1024

class Wrapper {
public:
    Wrapper(EventLoop& eventloop, IO* io);

    bool socketselect(int fd, int delay);
    bool sendData(wrapperData_t * data);
    bool recvData(wrapperData_t * data);

    bool add_del_route(int cmd, wrapperData_t * data);
    bool send_udp(wrapperData_t * data);
    bool socket_option_to(wrapperData_t * data);
    bool socket_option(wrapperData_t * data);
    bool udp_enable_recv(wrapperData_t * data);
    bool udp_open_bind(wrapperData_t * data);
    bool udp_open(wrapperData_t * data);
    bool udp_close(wrapperData_t * data);

    void udp_recv(xrl_recv_udp_t * udpdata);
    void policy(xrl_policy_t *policy);

    ProcessStatus status(string& reason);
    void quiting();
    void restart();
    void shutdown();
    bool wait_for_cmd();
    bool running();
    bool process_cmd(wrapperData_t *wdata);
    void set_callback_result(const XrlError &e, const void *data, uint32_t len);

    void configure_filter(const uint32_t& filter, const string& conf);
    void reset_filter(const uint32_t& filter);
    bool policy_filtering(IPv4Net& net, IPv4& nexthop,
                          uint32_t& metric, IPv4 originator,
                          IPv4 main_addr,uint32_t type,
                          PolicyTags& policytags);

    void set_admin_dist(const uint32_t admin);
    uint32_t get_admin_dist();
    void set_main_addr(const IPv4& addr);
    IPv4 get_main_addr();
    void runClient(string cmd, string para);
    void runcmdCB(RunShellCommand* run_command,
                  const string& output);
    void runcmdCBdone(RunShellCommand* run_command,
                      bool success,
                      const string& error_msg);

private:

    void copy_cmd(wrapperData_t * cmd);
    void xrl_error(char *data, uint32_t data_len);
    void xrl_OK(uint32_t code, char *data, uint32_t data_len);

    void init_opend();
    void add_opend(char * xrl_sock_id);
    void del_opend(char * xrl_sock_id);
    void close_opend();
    tcp_udp_opend_t opened_sock[MAX_OPEN_SOCK];

    EventLoop&          _eventloop;
    IO*			_io;
    wrapperStatus	_status;
    char     		_data_buffer[MAXBUFFER];

    string              _reason;
    ProcessStatus       _process_status;

    PolicyFilters       _policy_filters;
    IPv4		_main_addr;
    uint32_t		_admin_distance;

    bool 		_xrl_done;
    XrlError 		_xrl_error;
    char 		_xrl_data[MAXBUFFER];
    uint32_t		_xrl_data_len;
    char		_xrl_buf[MAXBUFFER];

    int 		_running_cmd;
    wrapperData_t * _cmd;
    char       		_cmd_buffer[MAXBUFFER];

    bool	server_ready;
    bool	conn_ready;

    int         listen_sock;                /*  listening socket          */
    int         conn_sock;                  /*  connection socket         */
    short int   port;                       /*  port number               */
    struct      sockaddr_in servaddr;       /*  socket address structure  */

    wrapperData_t  w_data;
    char                _buffer[MAXBUFFER];
    char                r_buffer[MAXBUFFER];
    int 		r_buf_left;
    pthread_mutex_t     mutex;

    std::queue<xrl_policy_t> redist_route;

    string	clientApp;
    string	clientPara;

};

#endif  //__WRAPPER_HH__

