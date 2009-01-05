// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/contrib/olsr/exceptions.hh,v 1.3 2008/10/02 21:56:34 bms Exp $

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
