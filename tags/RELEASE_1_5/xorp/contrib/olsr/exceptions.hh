// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/contrib/olsr/exceptions.hh,v 1.1 2008/04/24 15:19:51 bms Exp $

#ifndef __OLSR_EXCEPTIONS_HH__
#define __OLSR_EXCEPTIONS_HH__

/*
 * @short The exception thrown when an operation on an external
 *        route fails.
 */
class BadExternalRoute : public XorpReasonedException {
public:
    BadExternalRoute(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadExternalRoute", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on an OLSR
 *        interface fails.
 */
class BadFace : public XorpReasonedException {
public:
    BadFace(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadFace", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a link code fails.
 *
 * Usually this means the fields provided were invalid.
 */
class BadLinkCode : public XorpReasonedException {
public:
    BadLinkCode(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadLinkCode", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a logical link fails.
 */
class BadLogicalLink : public XorpReasonedException {
public:
    BadLogicalLink(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadLogicalLink", file, line, init_why) {}
};

/**
 * @short The exception thrown when no suitable link to a one-hop
 *        neighbor exists.
 */
class BadLinkCoverage : public XorpReasonedException {
public:
    BadLinkCoverage(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadLinkCoverage", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a MID entry fails.
 */
class BadMidEntry : public XorpReasonedException {
public:
    BadMidEntry(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadMidEntry", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a one-hop
 *        neighbor fails.
 */
class BadNeighbor : public XorpReasonedException {
public:
    BadNeighbor(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadNeighbor", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a topology control
 *        entry fails.
 */
class BadTopologyEntry : public XorpReasonedException {
public:
    BadTopologyEntry(const char* file, size_t line,
		      const string& init_why = "")
     : XorpReasonedException("OlsrBadTopologyEntry", file, line, init_why) {}
};

/**
 * @short The exception thrown when no suitable link to a two-hop
 *        neighbor exists.
 *
 * In particular it may be thrown during MPR calculation, if an
 * inconsistency is detected in the set of MPRs covering a two-hop neighbor
 * which was calculated to be reachable.
 */
class BadTwoHopCoverage : public XorpReasonedException {
public:
    BadTwoHopCoverage(const char* file, size_t line,
			  const string& init_why = "")
     : XorpReasonedException("OlsrBadTwoHopCoverage", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a link in the
 *        two-hop neighborhood fails.
 */
class BadTwoHopLink : public XorpReasonedException {
public:
    BadTwoHopLink(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadTwoHopLink", file, line, init_why) {}
};

/**
 * @short The exception thrown when an operation on a two-hop
 *        neighbor fails.
 */
class BadTwoHopNode : public XorpReasonedException {
public:
    BadTwoHopNode(const char* file, size_t line, const string& init_why = "")
     : XorpReasonedException("OlsrBadTwoHopNode", file, line, init_why) {}
};

/**
 * @short The exception thrown when the decoding or encoding of
 *        a Message fails.
 */
class InvalidMessage : public XorpReasonedException {
public:
    InvalidMessage(const char* file, size_t line, const string& init_why = "")
	: XorpReasonedException("OlsrInvalidMessage", file, line, init_why) {}
};

/**
 * @short The exception thrown when the decoding or encoding of
 * a LinkTuple inside a HelloMessage fails.
 */
class InvalidLinkTuple : public XorpReasonedException {
public:
    InvalidLinkTuple(const char* file, size_t line,
		     const string& init_why = "")
	: XorpReasonedException("OlsrInvalidLinkTuple", file, line, init_why)
    {}
};

#endif // __OLSR_EXCEPTIONS_HH__
