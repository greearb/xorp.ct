/*
 * Copyright (c) 2001-2003 International Computer Science Institute
 * See LICENSE file for licensing, conditions, and warranties on use.
 *
 * DO NOT EDIT THIS FILE - IT IS PROGRAMMATICALLY GENERATED
 *
 * Generated by 'tgt-gen'.
 */

#ident "$XORP: xorp/xrl/targets/socket_server_base.cc,v 1.1 2003/12/17 00:04:03 hodson Exp $"


#include "socket_server_base.hh"


XrlSocketServerTargetBase::XrlSocketServerTargetBase(XrlCmdMap* cmds)
    : _cmds(cmds)
{
    if (_cmds)
	add_handlers();
}

XrlSocketServerTargetBase::~XrlSocketServerTargetBase()
{
    if (_cmds)
	remove_handlers();
}

bool
XrlSocketServerTargetBase::set_command_map(XrlCmdMap* cmds)
{
    if (_cmds == 0 && cmds) {
        _cmds = cmds;
        add_handlers();
        return true;
    }
    if (_cmds && cmds == 0) {
	remove_handlers();
        _cmds = cmds;
        return true;
    }
    return false;
}

const XrlCmdError
XrlSocketServerTargetBase::handle_common_0_1_get_target_name(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != 0) handling common/0.1/get_target_name",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string name;
    try {
	XrlCmdError e = common_0_1_get_target_name(
	    name);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for common/0.1/get_target_name failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("name", name);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_common_0_1_get_version(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != 0) handling common/0.1/get_version",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string version;
    try {
	XrlCmdError e = common_0_1_get_version(
	    version);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for common/0.1/get_version failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("version", version);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_common_0_1_get_status(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != 0) handling common/0.1/get_status",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    uint32_t status;
    string reason;
    try {
	XrlCmdError e = common_0_1_get_status(
	    status,
	    reason);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for common/0.1/get_status failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("status", status);
	pxa_outputs->add("reason", reason);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_common_0_1_shutdown(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 0) {
	XLOG_ERROR("Wrong number of arguments (%u != 0) handling common/0.1/shutdown",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = common_0_1_shutdown();
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for common/0.1/shutdown failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_finder_event_observer_0_1_xrl_target_birth(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling finder_event_observer/0.1/xrl_target_birth",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = finder_event_observer_0_1_xrl_target_birth(
	    xa_inputs.get_string("target_class"),
	    xa_inputs.get_string("target_instance"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for finder_event_observer/0.1/xrl_target_birth failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_finder_event_observer_0_1_xrl_target_death(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling finder_event_observer/0.1/xrl_target_death",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = finder_event_observer_0_1_xrl_target_death(
	    xa_inputs.get_string("target_class"),
	    xa_inputs.get_string("target_instance"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for finder_event_observer/0.1/xrl_target_death failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_tcp_open_and_bind(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket4/0.1/tcp_open_and_bind",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket4_0_1_tcp_open_and_bind(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv4("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/tcp_open_and_bind failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_udp_open_and_bind(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket4/0.1/udp_open_and_bind",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket4_0_1_udp_open_and_bind(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv4("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/udp_open_and_bind failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_udp_open_bind_join(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 6) {
	XLOG_ERROR("Wrong number of arguments (%u != 6) handling socket4/0.1/udp_open_bind_join",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket4_0_1_udp_open_bind_join(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv4("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv4("mcast_addr"),
	    xa_inputs.get_uint32("ttl"),
	    xa_inputs.get_bool("reuse"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/udp_open_bind_join failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_tcp_open_bind_connect(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket4/0.1/tcp_open_bind_connect",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket4_0_1_tcp_open_bind_connect(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv4("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv4("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/tcp_open_bind_connect failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_udp_open_bind_connect(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket4/0.1/udp_open_bind_connect",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket4_0_1_udp_open_bind_connect(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv4("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv4("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/udp_open_bind_connect failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_close(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != 1) handling socket4/0.1/close",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_close(
	    xa_inputs.get_string("sockid"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/close failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_tcp_listen(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket4/0.1/tcp_listen",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_tcp_listen(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_uint32("backlog"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/tcp_listen failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_send(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket4/0.1/send",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_send(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_binary("data"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/send failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_send_with_flags(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket4/0.1/send_with_flags",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_send_with_flags(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_binary("data"),
	    xa_inputs.get_bool("out_of_band"),
	    xa_inputs.get_bool("end_of_record"),
	    xa_inputs.get_bool("end_of_file"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/send_with_flags failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_send_to(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 4) {
	XLOG_ERROR("Wrong number of arguments (%u != 4) handling socket4/0.1/send_to",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_send_to(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_ipv4("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    xa_inputs.get_binary("data"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/send_to failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_send_to_with_flags(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 7) {
	XLOG_ERROR("Wrong number of arguments (%u != 7) handling socket4/0.1/send_to_with_flags",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_send_to_with_flags(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_ipv4("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    xa_inputs.get_binary("data"),
	    xa_inputs.get_bool("out_of_band"),
	    xa_inputs.get_bool("end_of_record"),
	    xa_inputs.get_bool("end_of_file"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/send_to_with_flags failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_set_socket_option(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket4/0.1/set_socket_option",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket4_0_1_set_socket_option(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_string("optname"),
	    xa_inputs.get_uint32("optval"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/set_socket_option failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket4_0_1_get_socket_option(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket4/0.1/get_socket_option",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    uint32_t optval;
    try {
	XrlCmdError e = socket4_0_1_get_socket_option(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_string("optname"),
	    optval);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket4/0.1/get_socket_option failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("optval", optval);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_tcp_open_and_bind(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket6/0.1/tcp_open_and_bind",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket6_0_1_tcp_open_and_bind(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv6("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/tcp_open_and_bind failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_udp_open_and_bind(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket6/0.1/udp_open_and_bind",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket6_0_1_udp_open_and_bind(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv6("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/udp_open_and_bind failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_udp_open_bind_join(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 6) {
	XLOG_ERROR("Wrong number of arguments (%u != 6) handling socket6/0.1/udp_open_bind_join",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket6_0_1_udp_open_bind_join(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv6("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv6("mcast_addr"),
	    xa_inputs.get_uint32("ttl"),
	    xa_inputs.get_bool("reuse"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/udp_open_bind_join failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_tcp_open_bind_connect(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket6/0.1/tcp_open_bind_connect",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket6_0_1_tcp_open_bind_connect(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv6("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv6("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/tcp_open_bind_connect failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_udp_open_bind_connect(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket6/0.1/udp_open_bind_connect",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    string sockid;
    try {
	XrlCmdError e = socket6_0_1_udp_open_bind_connect(
	    xa_inputs.get_string("creator"),
	    xa_inputs.get_ipv6("local_addr"),
	    xa_inputs.get_uint32("local_port"),
	    xa_inputs.get_ipv6("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    sockid);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/udp_open_bind_connect failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("sockid", sockid);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_close(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 1) {
	XLOG_ERROR("Wrong number of arguments (%u != 1) handling socket6/0.1/close",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_close(
	    xa_inputs.get_string("sockid"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/close failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_tcp_listen(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket6/0.1/tcp_listen",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_tcp_listen(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_uint32("backlog"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/tcp_listen failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_send(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket6/0.1/send",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_send(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_binary("data"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/send failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_send_with_flags(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 5) {
	XLOG_ERROR("Wrong number of arguments (%u != 5) handling socket6/0.1/send_with_flags",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_send_with_flags(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_binary("data"),
	    xa_inputs.get_bool("out_of_band"),
	    xa_inputs.get_bool("end_of_record"),
	    xa_inputs.get_bool("end_of_file"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/send_with_flags failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_send_to(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 4) {
	XLOG_ERROR("Wrong number of arguments (%u != 4) handling socket6/0.1/send_to",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_send_to(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_ipv6("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    xa_inputs.get_binary("data"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/send_to failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_send_to_with_flags(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 7) {
	XLOG_ERROR("Wrong number of arguments (%u != 7) handling socket6/0.1/send_to_with_flags",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_send_to_with_flags(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_ipv6("remote_addr"),
	    xa_inputs.get_uint32("remote_port"),
	    xa_inputs.get_binary("data"),
	    xa_inputs.get_bool("out_of_band"),
	    xa_inputs.get_bool("end_of_record"),
	    xa_inputs.get_bool("end_of_file"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/send_to_with_flags failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_set_socket_option(const XrlArgs& xa_inputs, XrlArgs* /* pxa_outputs */)
{
    if (xa_inputs.size() != 3) {
	XLOG_ERROR("Wrong number of arguments (%u != 3) handling socket6/0.1/set_socket_option",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    try {
	XrlCmdError e = socket6_0_1_set_socket_option(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_string("optname"),
	    xa_inputs.get_uint32("optval"));
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/set_socket_option failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }
    return XrlCmdError::OKAY();
}

const XrlCmdError
XrlSocketServerTargetBase::handle_socket6_0_1_get_socket_option(const XrlArgs& xa_inputs, XrlArgs* pxa_outputs)
{
    if (xa_inputs.size() != 2) {
	XLOG_ERROR("Wrong number of arguments (%u != 2) handling socket6/0.1/get_socket_option",
            (uint32_t)xa_inputs.size());
	return XrlCmdError::BAD_ARGS();
    }

    if (pxa_outputs == 0) {
	XLOG_FATAL("Return list empty");
	return XrlCmdError::BAD_ARGS();
    }

    /* Return value declarations */
    uint32_t optval;
    try {
	XrlCmdError e = socket6_0_1_get_socket_option(
	    xa_inputs.get_string("sockid"),
	    xa_inputs.get_string("optname"),
	    optval);
	if (e != XrlCmdError::OKAY()) {
	    XLOG_WARNING("Handling method for socket6/0.1/get_socket_option failed: %s",
            		 e.str().c_str());
	    return e;
        }
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	XLOG_ERROR("Argument not found");
	return XrlCmdError::BAD_ARGS();
    }

    /* Marshall return values */
    try {
	pxa_outputs->add("optval", optval);
    } catch (const XrlArgs::XrlAtomFound& ) {
	XLOG_FATAL("Duplicate atom name"); /* XXX Should never happen */
    }
    return XrlCmdError::OKAY();
}

