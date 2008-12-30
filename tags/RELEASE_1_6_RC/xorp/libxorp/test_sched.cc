// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libxorp/test_sched.cc,v 1.5 2008/11/08 00:41:03 abittau Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/asyncio.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

// XXX we can't depend on libcomm because make check will compile and perform
// testing together, so when we're testing libxorp, libcomm won't be built yet.
namespace Libcomm {

#include "libcomm/comm_user.c"
#include "libcomm/comm_sock.c"

} // namespace Libcomm

// 
// Test the scheduling of file descriptor handling, tasks and timers, checking
// for fariness, starvation and deadlocks.
//

#define TEST_ASSERT(x)						\
    if (!(x))							\
	xorp_throw(TestException, "Test assertion failed")

namespace {

EventLoop _eventloop;
int	  _debug;
bool	  _run_fail = false;

class TestException : public XorpReasonedException {
public:
    TestException(const char* file, size_t line, const string& why = "")
	: XorpReasonedException("TestException", file, line, why) {}
};

class Socket {
public:
    Socket(const xsock_t& s);
    ~Socket();

    void     write_sync(unsigned len);
    void     write(unsigned len);
    void     write_cb(AsyncFileWriter::Event ev, const uint8_t* buf,
		      size_t len, size_t done);
    void     read(unsigned len);
    void     read_cb(AsyncFileReader::Event ev, const uint8_t* buf,
		     size_t len, size_t done);
    bool     did_read();
    bool     did_write();
    unsigned readb();
    unsigned writeb();
    void     set_priority(int priority);

private:
    typedef map<uint8_t*, unsigned> BUFFERS;	// data -> len

    void     create_ops(int priority);
    uint8_t* create_buffer(unsigned len);
    void     io_cb(AsyncFileOperator::Event ev, const uint8_t* buf, size_t len,
		   size_t done);

    xsock_t	    _s;
    AsyncFileReader *_reader;
    AsyncFileWriter *_writer;
    BUFFERS	    _buffers;
    unsigned	    _readb;
    unsigned	    _writeb;
    bool	    _did_read;
    bool	    _did_write;
};

class SocketPair {
public:
    SocketPair();
    ~SocketPair();

    void	write_sync(unsigned len);
    void	read(unsigned len);
    bool	did_read();
    unsigned	readb();
    Socket&	s0();
    Socket&	s1();
    void	set_priority(int priority);

private:
    Socket* _s[2];
};

Socket::Socket(const xsock_t& s)
    : _s(s),
      _reader(NULL),
      _writer(NULL),
      _readb(0),
      _writeb(0),
      _did_read(false),
      _did_write(false)
{
    if (Libcomm::comm_sock_set_blocking(_s, 0) != XORP_OK)
	    xorp_throw(TestException, "comm_sock_set_blocking()");

    create_ops(XorpTask::PRIORITY_DEFAULT);
}

Socket::~Socket()
{
    delete _reader;
    delete _writer;

    if (Libcomm::comm_sock_close(_s) != XORP_OK)
	xorp_throw(TestException, "comm_sock_close()");

    for (BUFFERS::iterator i = _buffers.begin(); i != _buffers.end(); ++i)
	delete [] i->first;
}

void
Socket::set_priority(int priority)
{
    create_ops(priority);
}

void
Socket::create_ops(int priority)
{
    delete _reader;
    delete _writer;

    if (_reader) {
	XLOG_ASSERT(_writer);
	XLOG_ASSERT(!_reader->running());
	XLOG_ASSERT(!_writer->running());
    }

    _reader = new AsyncFileReader(_eventloop, _s, priority);
    _writer = new AsyncFileWriter(_eventloop, _s, priority);
}

void
Socket::write_sync(unsigned len)
{
    write(len);

    // XXX need a watchdog in case we deadlock (i.e., TX queue fills).
    while (_writer->running())
	_eventloop.run();
}

void
Socket::write(unsigned len)
{
    uint8_t* buf = create_buffer(len);

    _writer->add_buffer(buf, len, callback(this, &Socket::write_cb));

    if (!_writer->running())
	_writer->start();
}

uint8_t*
Socket::create_buffer(unsigned len)
{
    uint8_t* buf = new uint8_t[len];

    XLOG_ASSERT(buf);
    XLOG_ASSERT(_buffers.find(buf) == _buffers.end());

    _buffers[buf] = len;

    return buf;
}

void
Socket::write_cb(AsyncFileWriter::Event ev, const uint8_t* buf,
		 size_t len, size_t done)
{
    io_cb(ev, buf, len, done);

    _writeb    += done;
    _did_write  = true;
}

void
Socket::read(unsigned len)
{
    uint8_t* buf = create_buffer(len);

    _reader->add_buffer(buf, len, callback(this, &Socket::read_cb));

    if (!_reader->running())
	_reader->start();
}

void
Socket::read_cb(AsyncFileReader::Event ev, const uint8_t* buf,
		size_t len, size_t done)
{
    io_cb(ev, buf, len, done);

    _readb    += done;
    _did_read  = true;
}

void
Socket::io_cb(AsyncFileReader::Event ev, const uint8_t* buf,
	      size_t len, size_t done)
{
    XLOG_ASSERT(ev == AsyncFileOperator::DATA);

    BUFFERS::iterator i = _buffers.find(const_cast<uint8_t*>(buf));
    XLOG_ASSERT(i != _buffers.end());
    XLOG_ASSERT(i->second == len);
    XLOG_ASSERT(len == done);

    delete i->first;
    _buffers.erase(i);
}

bool
Socket::did_read()
{
    bool x = _did_read;

    _did_read = false;

    return x;
}

bool
Socket::did_write()
{
    bool x = _did_write;

    _did_write = false;

    return x;
}

unsigned
Socket::readb()
{
    return _readb;
}

unsigned
Socket::writeb()
{
    return _writeb;
}

SocketPair::SocketPair()
{
    xsock_t s[2];

    memset(_s, 0, sizeof(_s));

    if (Libcomm::comm_sock_pair(AF_UNIX, SOCK_STREAM, 0, s) != XORP_OK)
	xorp_throw(TestException, "comm_sock_pair()");

    for (unsigned i = 0; i < 2; i++)
	_s[i] = new Socket(s[i]);
}

SocketPair::~SocketPair()
{
    for (unsigned i = 0; i < 2; i++)
	delete _s[i];
}

unsigned
SocketPair::readb()
{
    return s0().readb();
}

Socket&
SocketPair::s0()
{
    return *_s[0];
}

Socket&
SocketPair::s1()
{
    return *_s[1];
}

bool
SocketPair::did_read()
{
    return s0().did_read();
}

void
SocketPair::write_sync(unsigned len)
{
    return s1().write_sync(len);
}

void
SocketPair::read(unsigned len)
{
    return s0().read(len);
}

void
SocketPair::set_priority(int priority)
{
    for (unsigned i = 0; i < 2; i++)
	_s[i]->set_priority(priority);
}

void
wait_for_read(SocketPair* s, const unsigned num)
{
    while (1) {
	for (unsigned i = 0; i < num; i++) {
	    if (s[i].did_read())
		return;
	}

	_eventloop.run();
    }
}

void
xprintf(const char *fmt, ...)
{
    va_list ap;

    if (!_debug)
	return;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void
feed(SocketPair* s, unsigned num, unsigned len = 1024)
{
    for (unsigned i = 0; i < num; i++)
	s[i].write_sync(len);
}

void
eat(SocketPair* s, unsigned num, unsigned len = 1)
{
    for (unsigned i = 0; i < num; i++)
	s[i].read(len);
}

void
eat_and_wait(SocketPair* s, unsigned num, unsigned len = 1)
{
    eat(s, num, len);
    wait_for_read(s, num);
}

// Two readers with same priority - see if one starves.
void
test_fd_2read_starve(void)
{
    const unsigned num = 2;
    SocketPair s[num];
    unsigned times = 10;

    xprintf("Running %s\n", __FUNCTION__);

    // give both sockets stuff to read
    feed(s, num);

    // read stuff from both sockets
    for (unsigned i = 0; i < times; i++)
	eat_and_wait(s, 2);

    // check what happened
    for (unsigned i = 0; i < num; i++) {
	unsigned x = s[i].readb();

	xprintf("Socket %u read %u\n", i, x);
	TEST_ASSERT(x != 0);
    }

#if 0
    // could check for fairness while we're at it
    XLOG_ASSERT((times & 1) == 0);
    TEST_ASSERT(s[0].readb() == s[1].readb());
#endif
}

// Reading & writing on the same FD - see if one operation starves.
void
test_fd_read_write_starve()
{
    SocketPair p;
    unsigned times = 10;

    xprintf("Running %s\n", __FUNCTION__);

    // give it enough stuff to read
    feed(&p, 1);

    // read & write on socket.
    Socket& s = p.s0();
    for (unsigned i = 0; i < times; i++) {
	s.read(1);
	s.write(1);

	while (!s.did_read() && !s.did_write())
	    _eventloop.run();
    }

    // check results
    xprintf("Read %u Wrote %u\n", s.readb(), s.writeb());
    TEST_ASSERT(s.readb()  != 0);
    TEST_ASSERT(s.writeb() != 0);

#if 0
    // check fairness
    XLOG_ASSERT((times & 1) == 0);
    TEST_ASSERT(s.readb() == s.writeb());
#endif
}

// There are two low priority readers, and both should get serviced round robin.
// However, we introduce a high priority reader "every other round" and check
// whether the low priority readers get starved.
void
test_fd_1high_2low_starve(void)
{
    const unsigned num = 2;
    SocketPair s[num], high;
    unsigned times = 10;

    xprintf("Running %s\n", __FUNCTION__);

    high.set_priority(XorpTask::PRIORITY_HIGH);

    // give the sockets something to eat
    feed(s, num);
    feed(&high, 1);

    // constantly have the low priority guys read, and introduce the high
    // priority guy after each low priority event.
    for (unsigned i = 0; i < times; i++) {
	// read
	eat_and_wait(s, num);

	// OK our low priority dudes read something.  Lets get the high priority
	// guy into the game to see if scheduling state gets "reset".
	eat(&high, 1);
	while (!high.did_read())
	    _eventloop.run();

	for (unsigned j = 0; j < num; j++) {
	    XLOG_ASSERT(!s[0].did_read());
	}
    }

    // see if any of the low priority guys got starved
    for (unsigned i = 0; i < num; i++) {
	unsigned x = s[i].readb();

	xprintf("Socket %u read %u\n", i, x);
	TEST_ASSERT(x != 0);
    }
}

struct Test {
    void	(*_run)(void);
    bool	_fails;
    const char*	_desc;
};

struct Test _tests[] = {
    { test_fd_2read_starve, false, "Two readers, same priority, check starve" },
    { test_fd_read_write_starve, false, "RW same FD & priority, check starve" },
    { test_fd_1high_2low_starve, true, "High pri. starving some low pri." },
};

void
do_tests(void)
{
    for (unsigned i = 0; i < sizeof(_tests) / sizeof(*_tests); ++i) {
	struct Test& t = _tests[i];

	if (!_run_fail && t._fails) {
	    xprintf("Skipping test #%u [%s]\n", i + 1, t._desc);
	    continue;
	}

	xprintf("Running test #%u%s [%s]\n",
	        i + 1,
		t._fails ? " (known to fail)" : "",
		t._desc);

	t._run();
    }
}

} // anonymous namespace

int
main(int argc, char *argv[])
{
    int rc = 0;
    int ch;

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    while ((ch = getopt(argc, argv, "hvf")) != -1) {
	switch (ch) {
	case 'h':
	    printf("Usage: %s <opts>\n"
		   "-h\thelp\n"
		   "-v\tverbose\n"
		   "-f\trun tests known to fail\n"
		   , argv[0]);
	    exit(1);

	case 'f':
	    _run_fail = true;
	    break;

	case 'v':
	    _debug++;
	    break;
	}
    }

    if (Libcomm::comm_init() != XORP_OK)
	XLOG_FATAL("comm_init()");

    try {
	do_tests();
    } catch(const TestException& e) {
	cout << "TestException: " << e.str() << endl;
	rc = 1;
    }

    if (rc == 0)
	xprintf("All good\n");
    else
	xprintf("=(\n");

    Libcomm::comm_exit();

    xlog_stop();
    xlog_exit();

    exit(rc);
}
