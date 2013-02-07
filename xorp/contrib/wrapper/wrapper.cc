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

#include "wrapper.hh"
#include "wrapperpolicy.hh"
#include "io.hh"

#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"


Wrapper::Wrapper(EventLoop& eventloop, IO* io)
    : _eventloop(eventloop), _io(io),_status(INIT),
      _reason("Waiting for IO"), _process_status(PROC_STARTUP),_main_addr("1.1.1.1"),
      server_ready(false),conn_ready(false),clientApp(""),clientPara("")
{
    port = 34567;
    _admin_distance = 202;
    _running_cmd = -1;
    r_buf_left = 0;
    pthread_mutex_init(&mutex,NULL);
    init_opend();
}

bool Wrapper::socketselect(int fd, int delay)
{
    fd_set read_flags;
    struct timeval waitd;

    waitd.tv_sec = 0;
    waitd.tv_usec = delay;
    FD_ZERO(&read_flags); // Zero the flags ready for using
    FD_SET(fd, &read_flags);

    int err=select(fd+1, &read_flags,NULL,(fd_set*)0,&waitd);
    if(err < 0) return false;
    return FD_ISSET(fd, &read_flags);
}

//======================================================================
// send data to client
// data can be udp recv data (called by xorp callback thread)
// or ack of client command (called by wrapper thread)
bool Wrapper::sendData(wrapperData_t * data)
{
    if (!conn_ready) return true;
    int tlen = data->data_len + sizeof(wrapperData_t) + sizeof(uint32_t);
    uint32_t nlen = htonl(data->data_len + sizeof(wrapperData_t));
    int lockret = pthread_mutex_lock(&mutex);
    if (lockret==0) {
        memcpy(_buffer, &nlen, sizeof(nlen));
        memcpy(_buffer + sizeof(nlen), data, sizeof(*data));
        if (data->data_len > 0)
            memcpy(_buffer + sizeof(nlen) + sizeof(*data), data->data, data->data_len);
        try {
            if (send(conn_sock, _buffer, tlen, 0) != tlen) {
                lockret = pthread_mutex_unlock(&mutex);
                conn_ready = false;
                close_opend();
                _io->reset();
                fprintf(stderr,"Connection to wrapper client is lost!!\n");
                runClient(string(""),string(""));
                return false;
            }
        } catch (...) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        lockret = pthread_mutex_unlock(&mutex);
    } else {
        fprintf(stderr,"mutex lock fail %d\n",lockret);
    }
    return true;
}
//-----------------------------------------------------------
// receive command from client (called by wrapper thread)
// blocked when there is no command
bool Wrapper::recvData(wrapperData_t * data)
{
    uint32_t *nlen = NULL;
    int received = 0;
    int exp = 4;
    int bytes = 0;
    int len;
    wrapperData_t * tp;
    while (received < exp) {
        if (r_buf_left<=0) {
            while (!socketselect(conn_sock,45000)) {
                try {
                    _eventloop.run();
                } catch(...) {
                    xorp_catch_standard_exceptions();
                }
            }
            if ((bytes = recv(conn_sock, r_buffer+received, MAXBUFFER-1-received, 0)) < 1) {
                return false;
            }
        } else {
            bytes = r_buf_left;
            r_buf_left = 0;
        }

        received += bytes;
        if (nlen==NULL && received>=(int)(sizeof(uint32_t))) {
            nlen = (uint32_t *)r_buffer;
            len = ntohl(*nlen);
            exp = len + sizeof(uint32_t);
        }
    }

    tp = (wrapperData_t * )(r_buffer+sizeof(uint32_t));
    data->code = tp->code;
    data->data_len = tp->data_len;
    if (tp->data_len>0)
        memcpy(data->data,(void *)(r_buffer+sizeof(uint32_t)+sizeof(wrapperData_t)),tp->data_len);

    r_buf_left = received-exp;
    if (received>exp) {
        for (int i=0; i<r_buf_left; i++) r_buffer[i] = r_buffer[i+exp];
    }


    return true;
}

//======================================================================
// creat server socket and listen for client command
// return true when xrl call made (need run eventloop.run())
// return false when socket server error (quit the wrapper)
bool Wrapper::wait_for_cmd()
{
    wrapperData_t wdata;

    while (_status!=QUITTING) {
        if (!server_ready) {
            // create server socket
            /*  Create the listening socket  */

            if ( (listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
                fprintf(stderr, "Wrapper: Error creating listening socket.\n");
                return false;
            }
            /*  Set all bytes in socket address structure to
                zero, and fill in the relevant data members   */
            memset(&servaddr, 0, sizeof(servaddr));
            servaddr.sin_family      = AF_INET;
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port        = htons(port);
            /*  Bind our socket addresss to the
            listening socket, and call listen()  */
            if ( bind(listen_sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
                fprintf(stderr, "Wrapper: Error calling bind()\n");
                return false;
            }
            if ( listen(listen_sock, LISTENQLEN) < 0 ) {
                fprintf(stderr, "Wrapper: Error calling listen()\n");
                return false;
            }

            server_ready = true;
        }

        if (!conn_ready) {
            _status = WAITTING;
            /*  Wait for a connection, then accept() it  */
            while (!socketselect(listen_sock,45000)) {
                try {
                    _eventloop.run();
                } catch(...) {
                    xorp_catch_standard_exceptions();
                }
            }
            if ( (conn_sock = accept4(listen_sock, NULL, NULL,SOCK_NONBLOCK) ) < 0 ) {
                fprintf(stderr, "ECHOSERV: Error calling accept()\n");
                return false;
            }
            close_opend();
            conn_ready = true;
        }

        _status = RUNNING;
        wdata.data = _data_buffer;
        if (recvData(&wdata)) {
            if (process_cmd(&wdata))
                return true;
        } else {
            conn_ready = false;
            close_opend();
            _io->reset();
            runClient(string(""),string(""));
        }
    }
    return false;
}

//-----------------------------------------------------------
void Wrapper::quiting()
{
    fprintf(stderr,"xorp wrapper4 is quitting...\n");
    _status = QUITTING;
    close(listen_sock);
    shutdown();
}

ProcessStatus
Wrapper::status(string& reason)
{
    if (PROC_STARTUP == _process_status) {
        _process_status = PROC_READY;
        _reason = "Running";
    }

    reason = _reason;
    return _process_status;
}

void
Wrapper::shutdown()
{
    _io->wshutdown();
    _reason = "shutting down";
    _process_status = PROC_SHUTDOWN;
}

void
Wrapper::restart()
{
    _io->reset();
    _reason = "restart";
    _process_status = PROC_STARTUP;
}


//======================================================================
// main entry of wrapper
// return false to kill the wrapper
// return true to run eventloop.run()
bool Wrapper::running()
{
    if (_running_cmd!=-1) {
        // calling XRL
        if (_xrl_done) {
            // XRL callback done
            process_cmd(_cmd);
            _running_cmd = -1;
            // wait next cmd
        } else {
            // run eventloop.run() again
            return true;
        }
    }
    // wait cmd from ldwrapper
    return wait_for_cmd();
}

//======================================================================

void Wrapper::xrl_error(char *data, uint32_t data_len)
{
    wrapperData_t back;
    back.code = LDWRAPPER_ERROR;
    back.data = data;
    back.data_len = data_len;
    sendData(&back);
}
void Wrapper::xrl_OK(uint32_t code, char *data, uint32_t data_len)
{
    wrapperData_t back;
    back.code = code;
    back.data = data;
    back.data_len = data_len;
    sendData(&back);
}

void Wrapper::set_callback_result(const XrlError &e, const void *data, uint32_t len)
{
    _xrl_error = e;
    _xrl_data_len = len;
    if (e != XrlError::OKAY()) {
        fprintf(stderr, "ERROR: \t%s\n", e.str().c_str());
        _xrl_done = true;
        return;
    }

    if (len>0) {
        memcpy((void *)_xrl_data,data,len);
        _xrl_data_len = len;
    } else {
        _xrl_data[0] = '\0';
    }
    _xrl_done = true;
    return;
}

void Wrapper::copy_cmd(wrapperData_t * cmd)
{
    _cmd = (wrapperData_t *)(&_cmd_buffer);
    _cmd->code = cmd->code;
    _cmd->data_len = cmd->data_len;
    _cmd->data = (void *)(_cmd_buffer+sizeof(wrapperData_t));
    memcpy((void *)(_cmd_buffer+sizeof(wrapperData_t)),(void *)(cmd->data),cmd->data_len);
}

void Wrapper::udp_recv(xrl_recv_udp_t * udpdata)
{
    wrapperData_t back;
    char  buf[MAXBUFFER];
    back.code = LDWRAPPER_UDP_RECV;
    back.data = buf;
    back.data_len = sizeof(xrl_recv_udp_t)+udpdata->data_len;

    memcpy(buf, udpdata, sizeof(xrl_recv_udp_t));
    memcpy(buf + sizeof(xrl_recv_udp_t), udpdata->data, udpdata->data_len);

//    fprintf(stderr,"udp recv: len %u\n", back.data_len);
    sendData(&back);
}

void Wrapper::policy(xrl_policy_t *policy)
{
    wrapperData_t back;
    xrl_policy_t routeP;
    char  buf[MAXBUFFER];
    back.code = policy->cmd;
    back.data = buf;
    back.data_len = sizeof(xrl_policy_t);

    memcpy((void *)buf,(void *)policy, sizeof(xrl_policy_t));
    if (back.code == LDWRAPPER_PUSH_ROUTE)
        sendData(&back);
    else {
        memcpy((void *)&routeP,(void *)policy, sizeof(xrl_policy_t));
        redist_route.push(routeP);
    }
}


//======================================================================
void Wrapper::init_opend()
{
    int i;
    for (i = 0; i < MAX_OPEN_SOCK; i ++) {
        opened_sock[i]._used = false;
    }
}

void Wrapper::add_opend(char * xrl_sock_id)
{
    int i;
    std::string sock(xrl_sock_id);
    del_opend(xrl_sock_id);
    for (i = 0; i < MAX_OPEN_SOCK; i ++) {
        if (!opened_sock[i]._used) {// found
            opened_sock[i].sock = sock;
            opened_sock[i]._used = true;
            break;
        }
    }
}

void Wrapper::del_opend(char * xrl_sock_id)
{
    int i;
    std::string sock(xrl_sock_id);
    for (i = 0; i < MAX_OPEN_SOCK; i ++) {
        if (opened_sock[i]._used && sock == opened_sock[i].sock) {// found
            opened_sock[i]._used = false;
            break;
        }
    }
}

void Wrapper:: close_opend()
{
    bool b = false;
    for (int i = 0; i < MAX_OPEN_SOCK; i ++) {
        if (opened_sock[i]._used) {// found
            _io->forceclose(opened_sock[i].sock);
            opened_sock[i]._used = false;
            b = true;
        }
    }
    if (b) socketselect(0,369000);
}

//======================================================================
// return true to run eventloop.run() when _running_cmd == -1
bool Wrapper::process_cmd(wrapperData_t *wdata)
{
    xrl_socket_id_t *sockid;
    if (_running_cmd != -1) {
        int len = 0;
        if (_xrl_error != XrlError::OKAY()) {
            len = _xrl_error.str().length()+1;
            memcpy(_xrl_buf,_xrl_error.str().c_str(),len);
        }
        // process result and send ack back to ldwrapper
        switch (_running_cmd) {
        case LDWRAPPER_ADD_ROUTE:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_ADD_ROUTE_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_DEL_ROUTE:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_DEL_ROUTE_OK,NULL,0);
            } else xrl_error(_xrl_buf,len+1);
            break;
        case LDWRAPPER_CLOSE:
            if (_xrl_error == XrlError::OKAY()) {
                sockid = (xrl_socket_id_t *)(_cmd->data);
                (sockid->xorp_sock_name)[sockid->len] = '\0';
                del_opend(sockid->xorp_sock_name);
                xrl_OK(LDWRAPPER_CLOSE_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_OPEN:
            if (_xrl_error == XrlError::OKAY()) {
                memcpy(_xrl_buf,_xrl_data,_xrl_data_len);
                add_opend(_xrl_data);
                xrl_OK(LDWRAPPER_UDP_OPEN_OK,_xrl_buf,_xrl_data_len);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_OPENBIND:
            if (_xrl_error == XrlError::OKAY()) {
                if (_xrl_data_len>0) {
                    memcpy(_xrl_buf,_xrl_data,_xrl_data_len);
                    add_opend(_xrl_data);
                    xrl_OK(LDWRAPPER_UDP_OPENBIND_OK,_xrl_buf,_xrl_data_len);
                } else {
                    xrl_OK(LDWRAPPER_UDP_OPENBIND_OK,NULL,0);
                }
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_ENABLE_RECV:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_UDP_ENABLE_RECV_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_SEND_MSG:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_UDP_SEND_MSG_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_SET_OPTION:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_UDP_SET_OPTION_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        case LDWRAPPER_UDP_SET_OPTION_TO:
            if (_xrl_error == XrlError::OKAY()) {
                xrl_OK(LDWRAPPER_UDP_SET_OPTION_TO_OK,NULL,0);
            } else xrl_error(_xrl_buf,len);
            break;
        default:
            if (_xrl_error != XrlError::OKAY()) {
                fprintf(stderr, "ERROR: \t%s\n", _xrl_error.str().c_str());
                xrl_error(_xrl_buf,len);
            }
        }

        return false;
    }

    copy_cmd(wdata);
    _xrl_done = false;
    switch (wdata->code) {
    case LDWRAPPER_NOOP:
        sendData(wdata);
        break;
    case LDWRAPPER_ADD_ROUTE:
        _running_cmd = LDWRAPPER_ADD_ROUTE;		// need return true
        add_del_route(_running_cmd,_cmd);
        return true;
    case LDWRAPPER_DEL_ROUTE:
        _running_cmd = LDWRAPPER_DEL_ROUTE;		// need return true
        add_del_route(_running_cmd,_cmd);
        return true;
    case LDWRAPPER_CLOSE:
        _running_cmd = wdata->code;
        udp_close(_cmd);
        return true;
    case LDWRAPPER_UDP_OPEN:
        _running_cmd = wdata->code;
        udp_open(_cmd);
        return true;
    case LDWRAPPER_UDP_OPENBIND:
        _running_cmd = wdata->code;
        udp_open_bind(_cmd);
        return true;
    case LDWRAPPER_UDP_ENABLE_RECV:
        _running_cmd = wdata->code;
        udp_enable_recv(_cmd);
        return true;
    case LDWRAPPER_UDP_SEND_MSG:
        _running_cmd = wdata->code;
        send_udp(_cmd);
        return true;
    case LDWRAPPER_UDP_SET_OPTION:
        _running_cmd = wdata->code;
        socket_option(_cmd);
        return true;
    case LDWRAPPER_UDP_SET_OPTION_TO:
        _running_cmd = wdata->code;
        socket_option_to(_cmd);
        return true;

    case LDWRAPPER_GET_POLICY_ROUTE:
        if (redist_route.empty()) {
            xrl_OK(LDWRAPPER_GET_POLICY_ROUTE_OK,NULL,0);
        } else {
            xrl_policy_t routeP = redist_route.front();
            redist_route.pop();
            xrl_OK(LDWRAPPER_GET_POLICY_ROUTE_OK,(char *)&routeP,sizeof(xrl_policy_t));
        }
        break;
    default:
        fprintf(stderr, "FIXME: Receive unknown command: %d data_len: %d\n",wdata->code,wdata->data_len);
        wdata->code = LDWRAPPER_ERROR;
        sendData(wdata);
    }

    return false;
}
//-----------------------------------------------------------

//-----------------------------------------------------------

//-----------------------------------------------------------
bool Wrapper::send_udp(wrapperData_t * data)
{
    xrl_send_udp_t * sendudp;
    sendudp = (xrl_send_udp_t *)(data->data);
    sendudp->data = ((uint8_t *)(data->data))+sizeof(xrl_send_udp_t);

    (sendudp->sock_id).xorp_sock_name[(sendudp->sock_id).len] = '\0';

    vector<uint8_t> payload;
    IPv4         target_addr(sendudp->target_addr);
    uint16_t        target_port = sendudp->target_port;
    string    xrlsock((sendudp->sock_id).xorp_sock_name);

    payload.resize(sendudp->data_len);
    memcpy((char*)&(*payload.begin()), (char *)(sendudp->data),sendudp->data_len);
    _io->send_send_udp(xrlsock,target_addr,target_port,payload,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::socket_option_to(wrapperData_t * data)
{
    xrl_socket_option_t * sockoption;
    sockoption = (xrl_socket_option_t *)(data->data);

    (sockoption->sock_id).xorp_sock_name[(sockoption->sock_id).len] = '\0';
    string    xrlsock((sockoption->sock_id).xorp_sock_name);
    uint32_t    level = sockoption->level;
    string   optname(sockoption->opt_name);
    string   devname(sockoption->dev_name);
    _io->send_socket_option_to(xrlsock,level,optname,devname,sockoption->length,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::socket_option(wrapperData_t * data)
{
    xrl_socket_option_t * sockoption;
    sockoption = (xrl_socket_option_t *)(data->data);

    (sockoption->sock_id).xorp_sock_name[(sockoption->sock_id).len] = '\0';
    string    xrlsock((sockoption->sock_id).xorp_sock_name);
    uint32_t    level = sockoption->level;
    string   optname(sockoption->opt_name);
    _io->send_socket_option(xrlsock,level,optname,sockoption->opt_value,sockoption->length,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::udp_enable_recv(wrapperData_t * data)
{
    xrl_socket_id_t *sockid;
    sockid = (xrl_socket_id_t *)(data->data);
    (sockid->xorp_sock_name)[sockid->len] = '\0';
    string xrlsock(sockid->xorp_sock_name);
    _io->send_enable_recv(xrlsock,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::udp_open_bind(wrapperData_t * data)
{
    xrl_socket_t *openbind;
    openbind = (xrl_socket_t *)(data->data);
    (openbind->sock_id).xorp_sock_name[(openbind->sock_id).len] = '\0';
    string    xrlsock((openbind->sock_id).xorp_sock_name);
    IPv4 local_address(openbind->local_addr);
    uint32_t local_port = openbind->local_port;
    if (xrlsock.compare("N/A")!=0)
        _io->send_bind_udp(xrlsock, local_address,local_port,&Wrapper::set_callback_result);
    else
        _io->send_open_bind_udp(local_address,local_port,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::udp_open(wrapperData_t * data)
{
    udp_open_t * udpopen;
    udpopen = (udp_open_t *)(data->data);
    _io->send_open_udp(udpopen->domain,udpopen->type,udpopen->protocol,&Wrapper::set_callback_result);
    return true;
}
//-----------------------------------------------------------
bool Wrapper::udp_close(wrapperData_t * data)
{
    xrl_socket_id_t *sockid;
    sockid = (xrl_socket_id_t *)(data->data);
    (sockid->xorp_sock_name)[sockid->len] = '\0';
    string xrlsock(sockid->xorp_sock_name);
    _io->send_close(xrlsock,&Wrapper::set_callback_result);
    return true;
}

//-----------------------------------------------------------
bool Wrapper::add_del_route(int cmd, wrapperData_t * data)
{
    add_route_t *add;
    del_route_t *del;
    PolicyTags policytags;
    bool accepted;

    if (cmd==LDWRAPPER_ADD_ROUTE) {
        add = (add_route_t *)(data->data);
        IPv4Net net(add->dst);
        IPv4 nexthop(add->next_hop);
        accepted = policy_filtering(net, nexthop, add->metric, _main_addr, IPv4::ZERO(), 0, policytags);
        if (accepted)
            _io->send_add_route(add,policytags,&Wrapper::set_callback_result);
    } else {
        del = (del_route_t *)(data->data);
        _io->send_del_route(del,&Wrapper::set_callback_result);
    }
    return true;
}


//======================================================================
void Wrapper::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter, conf);
}

void Wrapper::reset_filter(const uint32_t& filter)
{
    _policy_filters.reset(filter);
}

bool Wrapper::policy_filtering(IPv4Net& net, IPv4& nexthop,
                               uint32_t& metric, IPv4 originator,
                               IPv4 main_addr,uint32_t type,
                               PolicyTags& policytags)
{
    try {
        // Import filtering.
        WrapperVarRW varrw(net, nexthop, metric, originator, main_addr, type,
                           policytags);

        bool accepted = false;

        // fprintf(stderr,"[WRAPPER] Running filter: %s on route: %s\n",
        //           filter::filter2str(filter::IMPORT), cstring(net));

        accepted = _policy_filters.run_filter(filter::IMPORT, varrw);

        if (!accepted)
            return accepted;

        // Export source-match filtering.
        WrapperVarRW varrw2(net, nexthop, metric, originator, main_addr, type,
                            policytags);

        //fprintf(stderr,"[WRAPPER] Running filter: %s on route: %s\n",
        //          filter::filter2str(filter::EXPORT_SOURCEMATCH),
        //          cstring(net));

        _policy_filters.run_filter(filter::EXPORT_SOURCEMATCH, varrw2);

        return accepted;

    } catch(const PolicyException& e) {
        XLOG_WARNING("PolicyException: %s", cstring(e));
        return false;
    }

    return true;
}


void Wrapper::set_admin_dist(const uint32_t admin)
{
    _admin_distance = admin;
    _io->set_admin_dist(admin);
}

uint32_t Wrapper::get_admin_dist()
{
    return _admin_distance;
}


void Wrapper::set_main_addr(const IPv4& addr)
{
    _main_addr = addr;
}

IPv4 Wrapper::get_main_addr()
{
    return _main_addr;
}

void Wrapper::runClient(string cmd, string para)
{
    RunShellCommand *run_cmd;
    RunShellCommand::ExecId exec_id(getuid (),getgid ());

    if (clientApp.length()==0) {
        clientApp = cmd;
        clientPara = para;
    }
    fprintf(stderr,"Wrapper4 run >>%s %s<<\n",clientApp.c_str(),clientPara.c_str());

    run_cmd = new RunShellCommand(
        _eventloop,
        clientApp,
        clientPara,
        callback(this, &Wrapper::runcmdCB),
        callback(this, &Wrapper::runcmdCB),
        callback(this, &Wrapper::runcmdCBdone));
    run_cmd->set_exec_id(exec_id);
    if (run_cmd->execute() != XORP_OK) {
        delete run_cmd;
    }
}


void Wrapper::runcmdCB(RunShellCommand* run_command,
                       const string& output)
{
    UNUSED(run_command);
    UNUSED(output);
}

void Wrapper::runcmdCBdone(RunShellCommand* run_command,
                           bool success,
                           const string& error_msg)
{
    if (! success)
        fprintf(stderr,"Wrapper4 cannot start client: %s\n",error_msg.c_str());

    if (run_command != NULL) {
        delete run_command;
    }
}


