// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $Id$

#ifndef __FINDER_CONNECTION_HH__
#define __FINDER_CONNECTION_HH__

#include <string>
#include <list>

#include "libxorp/exceptions.hh"
#include "libxorp/selector.hh"

class FinderServer;

#define FINDER_TCP_DEFAULT_PORT     19999

// FinderMessageType's - these map into a table in finder-ipc.cc, edit
// with caution.

enum FinderMessageType {
    HELLO       = 0,
    BYE         = 1,
    REGISTER    = 2,
    UNREGISTER  = 3,
    NOTIFY      = 4,
    LOCATE      = 5,
    ERROR       = 6
};

class FinderMessage {
public:
    FinderMessage() : _type(HELLO), _seqno(-1), _ackno(-1), _argc(0) {}

    bool        add_arg(const char*);
    const char* get_arg (uint32_t n) const;

    const FinderMessageType& type() const { return _type; }
    uint16_t	seqno() const { return (uint16_t)_seqno; }
    uint16_t	ackno() const { assert(is_ack()); return (uint16_t)_ackno; }
    bool        is_ack() const { return (_ackno >= 0); }
    bool        operator==(const FinderMessage& m) const;

    string str() const;

private:
    FinderMessageType   _type;
    int32_t             _seqno;
    int32_t             _ackno;
    string	        _argv[2];
    uint32_t            _argc;

    friend class FinderIPCService;
};


// ----------------------------------------------------------------------------
// FinderIPCService provides message handling interface

class FinderIPCService {
public:
    FinderIPCService();
    virtual ~FinderIPCService() {}

    // Compositional helpers
    void prepare_message(FinderMessage& m, FinderMessageType type,
			 const char* arg0 = NULL, const char* arg1 = NULL);
    void prepare_ack(const FinderMessage& m, FinderMessage& ack, 
		     const char *arg0 = NULL, const char* arg1 = NULL);
    void prepare_error(const FinderMessage& m, FinderMessage& err,
		       const char* reason);

    // I/O functions
    bool read_message(FinderMessage& m);
    bool write_message(FinderMessage& m);

    // Authentication functions
    const char* set_auth_key(const char*);
    const char* auth_key();

    // Underlying I/O related functions that subclasses need to implement
    virtual bool        alive() const = 0;
    virtual uint32_t    bytes_buffered() const = 0;
    virtual int         read(char *buf, uint32_t buf_bytes) = 0;
    virtual int         write(const char *buf, uint32_t buf_bytes) = 0;

protected:
    void        free_auth_key();
    uint32_t    read_line(char *buf, uint32_t buf_bytes);
    bool        fetch_and_verify_message(char *buf, uint32_t buf_bytes);
    bool        hmac_str(const char *buf, uint32_t buf_bytes, 
                         char *dst, uint32_t dst_bytes);
    bool        parse_message(const char *buf, uint32_t buf_bytes, 
                              FinderMessage &m);

    string      _hmac_key;
    int32_t     _last_sent;
    int32_t     _last_recv;
};

class FinderTestIPCService : public FinderIPCService {
public:
    FinderTestIPCService();
    virtual ~FinderTestIPCService();
    bool        alive() const;
    uint32_t    bytes_buffered() const;
    int         read(char* buf, uint32_t buf_bytes);
    int         write(const char* buf, uint32_t buf_bytes);
private:
    list<char*> _blks;
    list<char*>::iterator _rbuf, _wbuf;
    uint32_t    _roff, _woff;
    static      uint32_t _blksz;
};

class FinderTCPIPCService : public FinderIPCService {
public:
    FinderTCPIPCService(int fd) : _fd(fd), _io_failed(false) {}
    virtual ~FinderTCPIPCService();

    virtual bool        alive() const;
    virtual uint32_t    bytes_buffered() const;
    virtual int         read(char *buf, uint32_t buf_bytes);
    virtual int         write(const char *buf, uint32_t buf_bytes);
    int			descriptor() { return _fd; }
private:
    int			_fd;
    mutable bool	_io_failed;
};

// ----------------------------------------------------------------------------
// Finder TCP client and server factories

// Hook type when FinderTCPServerIPCFactory accepts a connection and
// creates a FinderTCPIPCService for it.

typedef void (*ServiceCreationHook)(FinderTCPIPCService* created, void* thunk);

class FinderTCPServerIPCFactory {
public:
    FinderTCPServerIPCFactory(SelectorList& selector_list,
			      ServiceCreationHook hook, void* thunk,
			      int port = FINDER_TCP_DEFAULT_PORT);
    virtual ~FinderTCPServerIPCFactory();

    // Exceptions 
    struct FactoryError : public XorpReasonedException {
	FactoryError(const char* file, size_t line, const string& why) 
	    : XorpReasonedException("FactoryError", file, line, why) {}
    };

    // Not implemented
    FinderTCPServerIPCFactory operator=(const FinderTCPServerIPCFactory&);
    FinderTCPServerIPCFactory(const FinderTCPServerIPCFactory&);
protected:
    FinderTCPIPCService* FinderTCPServerIPCFactory::create();
    static void connect_hook(int fd, SelectorMask m, void* thunked_factory);

    SelectorList&		_selector_list;
    ServiceCreationHook		_creation_hook;
    void* 			_creation_thunk;

    int _fd;
};

// ----------------------------------------------------------------------------
// FinderClientIPCFactory

class FinderTCPClientIPCFactory {
public:
    static FinderTCPIPCService* create(const char* addr = "localhost", 
				       int port = FINDER_TCP_DEFAULT_PORT);
};

#endif // __FINDER_CONNECTION_HH__
