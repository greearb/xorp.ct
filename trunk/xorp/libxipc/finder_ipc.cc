// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$Id$"

#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif /* HAVE_SYS_FILIO_H */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "finder_module.h"
#include "libxorp/debug.h"
#include "hmac_md5.h"
#include "sockutil.hh"
#include "finder_ipc.hh"
#include "finder_server.hh"

// Missing proto-type on solaris 8 local boxen...
extern "C" {
    char *strtok_r(char *, const char *, char **);
}

// ----------------------------------------------------------------------------
// Constants

static const int FINDER_LINES_PER_MESSAGE = 3;

static const uint32_t FINDER_HEADER_LINE  = 0;
static const uint32_t FINDER_COMMAND_LINE = 1;
static const uint32_t FINDER_HMAC_LINE    = 2; // This must be last line

static const char   FINDER_PROTOCOL[] = "Finder/1.0";
static const size_t FINDER_PROTOCOL_LENGTH = sizeof(FINDER_PROTOCOL) - 1;

static const char   FINDER_CRLF[] = "\r\n";
static const size_t FINDER_CRLF_LENGTH = sizeof(FINDER_CRLF) - 1;

static const char   FINDER_HMAC[] = "HMAC/MD5";
static const char   FINDER_HMAC_LENGTH = sizeof(FINDER_HMAC) - 1;

static const char   FINDER_MD5_DEFAULT_KEY[] = "Say it ain't so, Mr Smith";

// ----------------------------------------------------------------------------
// Finder Message Type resolution

struct finder_msg_desc {
    const char* name;
    uint32_t    argc;
};

static const struct finder_msg_desc finder_msg[] =
{
    { "HELLO",      0 },
    { "BYE",        0 },
    { "REGISTER",   2 },
    { "UNREGISTER", 1 },
    { "NOTIFY",     2 },
    { "LOCATE",     1 },
    { "ERROR",      2 }
};

static int finder_command_token(const char* command, FinderMessageType* t)
{
    for (size_t i = 0; i < sizeof(finder_msg) / sizeof(finder_msg[0]); i++) {
        size_t l = strlen(finder_msg[i].name);
        if (strncasecmp(command, finder_msg[i].name, l) == 0) {
            *t = (FinderMessageType)i;
            return (int)l;
        }
    }
    return 0;
}

static inline const uint32_t&
finder_command_arg_count(const FinderMessageType &t)
{
    return finder_msg[t].argc;
}

static inline const char*
finder_command_name(const FinderMessageType &t)
{
    return finder_msg[t].name;
}

// ----------------------------------------------------------------------------
// FinderMessage method implementation

bool
FinderMessage::operator==(const FinderMessage& m) const
{
    if ((_type != m._type) ||
        (_seqno != m._seqno) ||
        (_ackno != m._ackno) ||
        (_argc != m._argc)) {
        return false;
    }
    for (uint32_t i = 0; i < _argc; i++) {
        if (_argv[i] != m._argv[i]) {
            return false;
        }
    }
    return true;
}

bool
FinderMessage::add_arg(const char* arg)
{
    size_t arg_len;

    arg_len = strlen(arg);
    if (_argc > (sizeof(_argv)/sizeof(_argv[0]))) {
        debug_msg("no space for field\n");
        return false;
    }
    _argv[_argc] = arg;
    _argc++;
    return true;
}

const char*
FinderMessage::get_arg(uint32_t fieldno) const
{
    if (fieldno > _argc)
        return NULL;
    return _argv[fieldno].c_str();
}

string
FinderMessage::str() const
{
    char   tmp[64];
    string r;

    // E_NO_SSTREAM_AVAILABLE
    r = finder_command_name(_type);

    snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), " SeqNo %d Ack ", _seqno);
    r += tmp;
    if (_ackno >= 0) {
	snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), "%d\n", _ackno);
	r += tmp;
    } else {
	r += "-\n";
    }

    r += string("Args: ");
    for (uint32_t i = 0; i < _argc; i++) {
	r += string("\"") + string(_argv[i]) + string("\" ");
    }
    r += string("\n---\n");

    return r;
}

// ----------------------------------------------------------------------------
// FinderIPCService methods

FinderIPCService::FinderIPCService()
{
    _hmac_key = FINDER_MD5_DEFAULT_KEY;
    _last_sent = random() & 0xffff;
    _last_recv = -1;
}

void
FinderIPCService::prepare_message(FinderMessage& m,
                                  FinderMessageType type,
				  const char* arg0, const char* arg1) {
    _last_sent = (_last_sent + 1) & 0xffff;
    assert(_last_sent >= 0);
    m._seqno = _last_sent;
    m._ackno = -1;
    m._type = type;
    m._argc = 0;

    if (arg0) m.add_arg(arg0);
    if (arg1) m.add_arg(arg1);
}

void
FinderIPCService::prepare_ack(const FinderMessage& m, FinderMessage& ack,
			      const char* arg0, const char* arg1) {
    prepare_message(ack, m.type(), arg0, arg1);
    ack._ackno = m._seqno;
}

void
FinderIPCService::prepare_error(const FinderMessage& m, FinderMessage& err,
				const char* reason) {
    char seqno[6];
    snprintf(seqno, sizeof(seqno) / sizeof(seqno[0]), "%5d", m._seqno);
    prepare_message(err, ERROR, seqno, reason);
}

const char*
FinderIPCService::set_auth_key(const char* auth_key)
{
    if (auth_key != NULL) {
        _hmac_key = auth_key;
    }
    return _hmac_key.c_str();
}

const char*
FinderIPCService::auth_key()
{
    return _hmac_key.c_str();
}

bool
FinderIPCService::hmac_str(const char *buf, uint32_t buf_bytes,
                           char *dst, uint32_t dst_bytes) {
    uint8_t digest[16];

    // HMAC header requires:
    // strlen(HMAC header) + tab + '0x' + 16 * 2 chars
    const uint32_t hmac_str_size = strlen(FINDER_HMAC) + 1 + 2 + 32;

    if (dst_bytes < hmac_str_size + 1) {
        debug_msg("hmac destination string too short\n");
        return false;
    }
    hmac_md5((const uint8_t*)buf, (int)buf_bytes,
             (const uint8_t*)_hmac_key.c_str(), (int)strlen(_hmac_key.c_str()),
             digest);

    snprintf(dst, dst_bytes, "%s\t0x", FINDER_HMAC);
    size_t offset = strlen(dst);
    for (size_t i = 0; i < 16; i++) {
        snprintf(dst + offset + 2 * i, dst_bytes - (offset + 2 * i),
		 "%02x", digest[i]);
    }

    assert(strlen(dst) == hmac_str_size);
    return true;
}

bool
FinderIPCService::write_message(FinderMessage& m)
{
    string      out;
    char        tmp[128];

    if (alive() == false) {
        debug_msg("Service uninitialized or finished.\n");
        return false;
    }

    if ((finder_command_arg_count(m._type) != m._argc) &&
	(m._ackno >= 0 && m._argc > 0)) {
        debug_msg("Invalid argument count\n");
        return false;
    }

    // Header Line
    snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), "%s\t%d",
	     FINDER_PROTOCOL, m._seqno);
    out = tmp;
    if (m._ackno >= 0) {
        snprintf(tmp, sizeof(tmp) / sizeof(tmp[0]), "\t%d", m._ackno);
        out += tmp;
    }
    out += FINDER_CRLF;

    // Command Line
    out += finder_msg[m._type].name;
    for (uint32_t i = 0; i < m._argc; i++) {
        out += "\t";
        out += m._argv[i];
    }
    out += FINDER_CRLF;

    // Message hash
    hmac_str(out.c_str(), (uint32_t)out.size(),
             (char*)tmp, (uint32_t)sizeof(tmp));
    out += tmp;
    out += FINDER_CRLF;

    return write(out.c_str(), out.size()) == (int)out.size();
}

uint32_t
FinderIPCService::read_line(char *buf, uint32_t buf_bytes)
{
    uint32_t done;

    // We'd really like to be using line buffered i/o
    if (buf_bytes < FINDER_CRLF_LENGTH) {
        return 0;
    }

    done = read(buf, FINDER_CRLF_LENGTH);
    if (done != FINDER_CRLF_LENGTH) {
        debug_msg("read error: %s\n", strerror(errno));
        return 0;
    }

    while (memcmp(FINDER_CRLF,
                 buf + done - FINDER_CRLF_LENGTH,
                 FINDER_CRLF_LENGTH) != 0 && done < buf_bytes) {
        if (read(&buf[done], 1) != 1) {
            debug_msg("read error: %s\n", strerror(errno));
            return 0;
        }
        done++;
    }

    return done;
}

bool
FinderIPCService::fetch_and_verify_message(char *buf,
                                           uint32_t buf_bytes) {
    const char *verify[3];
    char        hmac[128];
    uint32_t    avail, got, start[FINDER_LINES_PER_MESSAGE + 1];

    verify[FINDER_HEADER_LINE]  = FINDER_PROTOCOL;
    verify[FINDER_COMMAND_LINE] = NULL;
    verify[FINDER_HMAC_LINE]    = FINDER_HMAC;

    for (int i = start[0] = 0; i < FINDER_LINES_PER_MESSAGE; i++) {
        avail = bytes_buffered();
        if (avail == 0) {
            return false;
        }

        got = read_line(buf + start[i],
                        (uint32_t)min(avail, buf_bytes - start[i]));
        if (got == 0) {
            return false;
        }

        // Quick sanity check - bail quickly to help dealing with crap
        if (verify[i]) {
            size_t vlen = strlen(verify[i]);
            if (got < vlen) {
                debug_msg("Read too short - got %u, expected %u\n",
                          got, (uint32_t)vlen);
                return false;
            }
            if (strncasecmp(verify[i], buf + start[i], vlen)) {
                char tmp[vlen + 1];
                strncpy(tmp, buf + start[i], vlen);
                debug_msg("Verification check failed \"%s\" != \"%s\"\n",
                          tmp, verify[i]);
                return false;
            }
        }
        start[i + 1] = start[i] + got;
        avail -= got;
    }

    // Check hmac digest by computing digest of everything up to start
    // of digest...
    hmac_str(buf, start[FINDER_HMAC_LINE], hmac, sizeof(hmac));
    if (strncasecmp(hmac, buf + start[FINDER_HMAC_LINE], FINDER_HMAC_LENGTH)) {
        debug_msg("HMAC failed \n\t\"%s\" != \n\t\"%s\"\n",
                  hmac,
                  buf + start[FINDER_HMAC_LINE]);
        return false;
    }

    return true;
}

bool
FinderIPCService::parse_message(const char*    msg,
                                uint32_t       msg_bytes,
                                FinderMessage& m) {
    char *buf, *header, *command, *tok, *tok_last;

    // work on a copy so we can dump original in case of error
    buf = strdup(msg);

    // isolate header and command lines
    header = buf;
    command = strstr(header, FINDER_CRLF);
    if (command == NULL || msg_bytes < FINDER_CRLF_LENGTH) {
        debug_msg("Invalid message: too small or not using CRLF\n");
        goto parse_fail;
    }
    memset(command, 0, FINDER_CRLF_LENGTH);
    command += FINDER_CRLF_LENGTH;

    // null terminate command line - ignore hmac as dealt with elsewhere
    tok = strstr(command, FINDER_CRLF);
    if (tok == NULL) {
        debug_msg("Invalid message: no hmac line\n");
        goto parse_fail;
    }
    memset(tok, 0, FINDER_CRLF_LENGTH);

    // Header format is <protocol> <seqno> [<ackno>]
    tok = strtok_r(header, "\t", &tok_last);
    if (tok == NULL ||
        strncasecmp(tok, FINDER_PROTOCOL, FINDER_PROTOCOL_LENGTH)) {
        debug_msg("Incompatible protocol version");
        goto parse_fail;
    }

    tok = strtok_r(NULL, "\t", &tok_last);
    if (tok) {
        m._seqno = (int32_t)atoi(tok);
        if (m._seqno < 0 || m._seqno > 0xffff) {
            debug_msg("Invalid sequence number\n");
            goto parse_fail;
        }
    } else {
        debug_msg("No sequence number\n");
        goto parse_fail;
    }

    tok = strtok_r(NULL, "\t", &tok_last);
    if (tok) {
        m._ackno = (int32_t)atoi(tok);
        if (m._ackno < 0 || m._ackno > 0xffff) {
            debug_msg("Invalid ackno number\n");
            goto parse_fail;
        }
    }

    // Command format is <command> <value1> [<value2>]
    tok = strtok_r(command, "\t", &tok_last);
    if (finder_command_token(tok, &m._type) == 0) {
        debug_msg("Unrecognized command: %s\n", command);
        goto parse_fail;
    }

    if (m._ackno < 0) {
	for (m._argc = 0;
	    m._argc != finder_command_arg_count(m._type);
	    m._argc++) {
	    tok = strtok_r(NULL, "\t", &tok_last);
	    if (tok == NULL) {
		debug_msg("Argument (%d) missing\n", m._argc);
		goto parse_fail;
	    }
	    m._argv[m._argc] = tok;
	}
	tok = strtok_r(NULL, "\t", &tok_last);
	if (tok != NULL) {
	    debug_msg("At least 1 unexpected argument\n");
	    goto parse_fail;
	}
    }
    // Check seqno. This relatively late on it, but we have read
    // complete info in buffer and don't leave any stuff that the next
    // read would need to flush.
    if (_last_recv >= 0 &&
        m._seqno != ((_last_recv + 1) & 0xffff)) {
        debug_msg("Out of sequence packet discard (%d != %d)\n",
                  m._seqno, (_last_recv + 1) & 0xffff);
        goto parse_fail;
    }
    _last_recv = m._seqno;

    free(buf);
    return true;

 parse_fail:
    debug_msg("Message:\n%s", msg);
    free(buf);

    return false;
}

bool
FinderIPCService::read_message(FinderMessage &m)
{
    char        buf[1024];
    uint32_t    buf_bytes = sizeof(buf);

    if (fetch_and_verify_message(buf, buf_bytes) &&
        parse_message(buf, buf_bytes, m)) {
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
// FinderTestIPCService

uint32_t FinderTestIPCService::_blksz = 4096;

FinderTestIPCService::FinderTestIPCService()
{
    _blks.push_back(new char[_blksz]);
    _rbuf = _wbuf = _blks.begin();
    _roff = _woff = 0;
}

FinderTestIPCService::~FinderTestIPCService()
{
    while (_blks.empty() == false) {
        delete [] *_rbuf;
        _blks.erase(_rbuf);
        _rbuf ++;
    }
}

bool
FinderTestIPCService::alive() const
{
    return true;
}

uint32_t
FinderTestIPCService::bytes_buffered() const
{
    return (_blks.size() - 1) * _blksz + _woff - _roff;
}

int
FinderTestIPCService::read(char *buf, uint32_t buf_bytes)
{
    uint32_t done;

    buf_bytes = min(buf_bytes, bytes_buffered());
    if (buf_bytes == 0) {
	return 0;
    }

    done = 0;
    while (done < buf_bytes) {
	uint32_t l = min(_blksz - _roff, buf_bytes - done);
	memcpy(buf + done, *_rbuf + _roff, l);
	done += l;
	_roff += l;
	if (_roff == _blksz) {
	    delete [] *_rbuf;
	    _blks.erase(_rbuf);
	    _rbuf = _blks.begin();
	    _roff = 0;
	}
    }
    return (int)done;
}

int
FinderTestIPCService::write(const char* buf, uint32_t buf_bytes)
{
    uint32_t done;

    done = 0;
    while (done < buf_bytes) {
        uint32_t l = min(_blksz - _woff, buf_bytes - done);
        memcpy(*_wbuf + _woff, buf + done, l);
        done += l;
        _woff += l;
        if (_woff == _blksz) {
            _blks.push_back(new char[_blksz]);
            _wbuf++;
            _woff = 0;
        }
    }
    return done;
}

// ----------------------------------------------------------------------------
// FinderTCPIPCService

FinderTCPIPCService::~FinderTCPIPCService()
{
    close_socket(_fd);
}

bool
FinderTCPIPCService::alive() const
{
    return (_io_failed == false);
}

uint32_t
FinderTCPIPCService::bytes_buffered() const
{
    int avail;

    if (ioctl(_fd, FIONREAD, &avail) < 0) {
        debug_msg("FIONREAD failed: %s\n", strerror(errno));
        avail = 0;
	_io_failed = true;
    }
    return (uint32_t)avail;
}

int
FinderTCPIPCService::read(char* buf, uint32_t buf_bytes)
{
    int l = (int)min(bytes_buffered(), buf_bytes);
    int done = ::read(_fd, buf, l);

    if (done < 0) {
        debug_msg("read failed: %s\n", strerror(errno));
	_io_failed = true;
        done = 0;
    }
    return done;
}

int
FinderTCPIPCService::write(const char* buf, uint32_t buf_bytes)
{
    errno = 0;
    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);
    int done = ::write(_fd, buf, buf_bytes);
    signal(SIGPIPE, old_sigpipe);

    if (errno != 0 || done != (int)buf_bytes) {
        debug_msg("write failed: %s\n", strerror(errno));
	_io_failed = true;
	// write() may report correct number of bytes though failing
	// with SIGPIPE
        errno = done = 0;
    }
    return done;
}

// ----------------------------------------------------------------------------
// FinderTCPServerIPCFactory

FinderTCPServerIPCFactory::FinderTCPServerIPCFactory(SelectorList& slctr,
						     ServiceCreationHook hook,
						     void* thunk,
						     int port)
    : _selector_list(slctr), _creation_hook(hook), _creation_thunk(thunk) {

    _fd = create_listening_ip_socket(TCP, port);
    if (_fd < 0) {
	xorp_throw(FactoryError, strerror(errno));
    }

    _selector_list.add_selector(_fd, SEL_RD, connect_hook,
				 reinterpret_cast<void*>(this));
}

FinderTCPServerIPCFactory::~FinderTCPServerIPCFactory()
{
    if (_fd > 0) {
	_selector_list.remove_selector(_fd);
        close_socket(_fd);
    }
}

FinderTCPIPCService*
FinderTCPServerIPCFactory::create()
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);

    int r = accept(_fd, (sockaddr*)&sin, &slen);
    if (r > 0) {
        return new FinderTCPIPCService(r);
    } else {
        debug_msg("accept failed: %s\n",
                  strerror(errno));
    }
    return NULL;
}

void
FinderTCPServerIPCFactory::connect_hook(int fd, SelectorMask m,
					void* thunked_factory) {
    FinderTCPServerIPCFactory *f =
	reinterpret_cast<FinderTCPServerIPCFactory*>(thunked_factory);

    assert(fd == f->_fd);
    assert(m == SEL_RD);

    FinderTCPIPCService* service = f->create();
    if (service && f->_creation_hook) {
	f->_creation_hook(service, f->_creation_thunk);
    }
}

// ----------------------------------------------------------------------------
// FinderTCPClientIPCFactory

FinderTCPIPCService*
FinderTCPClientIPCFactory::create(const char* addr, int port)
{

    int fd = create_connected_ip_socket(TCP, addr, port);
    if (fd < 0)
	return NULL;

    return new FinderTCPIPCService(fd);
}
