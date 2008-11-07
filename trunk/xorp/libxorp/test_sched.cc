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

#ident "$XORP$"

#include "libxorp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libcomm/comm_api.h"
#include "libxorp/asyncio.hh"

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

class TestException : public XorpReasonedException {
public:
    TestException(const char* file, size_t line, const string& why = "")
	: XorpReasonedException("TestException", file, line, why) {}
};

class SocketPair {
public:
    SocketPair();
    ~SocketPair();

    void     write(void *buf, unsigned len);
    void     write_cb(AsyncFileWriter::Event ev, const uint8_t* buf,
		      size_t len, size_t done);
    void     read(unsigned len);
    void     read_cb(AsyncFileReader::Event ev, const uint8_t* buf,
		     size_t len, size_t done);
    bool     have_new();
    unsigned readb();

private:
    typedef map<uint8_t*, unsigned> BUFFERS;	// data -> len

    xsock_t	    _s[2];
    AsyncFileReader *_reader;
    AsyncFileWriter *_writer;
    BUFFERS	    _buffers;
    unsigned	    _readb;
    bool	    _have_new;
};

SocketPair::SocketPair()
    : _reader(NULL),
      _writer(NULL),
      _readb(0),
      _have_new(false)
{
    if (comm_sock_pair(AF_UNIX, SOCK_STREAM, 0, _s) != XORP_OK)
	xorp_throw(TestException, "comm_sock_pair()");

    for (unsigned i = 0; i < 2; i++) {
	if (comm_sock_set_blocking(_s[i], 0) != XORP_OK)
	    xorp_throw(TestException, "comm_sock_set_blocking()");
    }

    _reader = new AsyncFileReader(_eventloop, _s[0]);
    _writer = new AsyncFileWriter(_eventloop, _s[1]);
}

SocketPair::~SocketPair()
{
    delete _reader;
    delete _writer;

    for (unsigned i = 0; i < 2; i++) {
	if (comm_sock_close(_s[i]) != XORP_OK) {
	    xorp_throw(TestException, "comm_sock_close()");
	}
    }

    for (BUFFERS::iterator i = _buffers.begin(); i != _buffers.end(); ++i)
	delete [] i->first;
}

void
SocketPair::write(void *buf, unsigned len)
{
    XLOG_ASSERT(!_writer->running());

    _writer->add_buffer(static_cast<const uint8_t*>(buf), len,
			callback(this, &SocketPair::write_cb));
    _writer->start();

    // XXX need a watchdog in case we deadlock (i.e., TX queue fills).
    while (_writer->running())
	_eventloop.run();
}

void
SocketPair::write_cb(AsyncFileWriter::Event ev, const uint8_t* buf,
		     size_t len, size_t done)
{
    UNUSED(buf);

    XLOG_ASSERT(ev == AsyncFileOperator::DATA);
    XLOG_ASSERT(len == done);
}

void
SocketPair::read(unsigned len)
{
    uint8_t* buf = new uint8_t[len];

    XLOG_ASSERT(buf);
    XLOG_ASSERT(_buffers.find(buf) == _buffers.end());

    _buffers[buf] = len;

    _reader->add_buffer(buf, len, callback(this, &SocketPair::read_cb));

    if (!_reader->running())
	_reader->start();
}

void
SocketPair::read_cb(AsyncFileReader::Event ev, const uint8_t* buf,
		    size_t len, size_t done)
{
    XLOG_ASSERT(ev == AsyncFileOperator::DATA);

    BUFFERS::iterator i = _buffers.find(const_cast<uint8_t*>(buf));
    XLOG_ASSERT(i != _buffers.end());
    XLOG_ASSERT(i->second == len);
    XLOG_ASSERT(len == done);

    delete i->first;
    _buffers.erase(i);

    _readb    += done;
    _have_new  = true;
}

bool
SocketPair::have_new()
{
    bool x = _have_new;

    _have_new = false;

    return x;
}

unsigned
SocketPair::readb()
{
    return _readb;
}

void
wait_for_read(SocketPair* s, const unsigned num)
{
    while (1) {
	for (unsigned i = 0; i < num; i++) {
	    if (s[i].have_new())
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
test_fd_2recv_starve(void)
{
    const unsigned num = 2;
    SocketPair s[num];
    unsigned char crap[1024];
    unsigned times = 10;

    xprintf("Running %s\n", __FUNCTION__);

    // give both sockets stuff to read
    for (unsigned i = 0; i < num; i++)
	s[i].write(crap, sizeof(crap));

    // read stuff from both sockets
    for (unsigned i = 0; i < times; i++) {
	for (unsigned i = 0; i < num; i++)
	    s[i].read(1);

	wait_for_read(s, num);
    }

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

void
do_tests(void)
{
    test_fd_2recv_starve();
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

    while ((ch = getopt(argc, argv, "hv")) != -1) {
	switch (ch) {
	case 'h':
	    printf("Usage: %s <opts>\n"
		   "-h\thelp\n"
		   "-v\tverbose\n"
		   , argv[0]);
	    exit(1);

	case 'v':
	    _debug++;
	    break;
	}
    }

    if (comm_init() != XORP_OK)
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

    comm_exit();

    xlog_stop();
    xlog_exit();

    exit(rc);
}
