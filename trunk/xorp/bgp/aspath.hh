// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/bgp/aspath.hh,v 1.34 2002/12/09 18:28:40 hodson Exp $

#ifndef __BGP_ASPATH_HH__
#define __BGP_ASPATH_HH__

#include "config.h"
#include <sys/types.h>
#include <inttypes.h>
#include <string>
#include <list>

#include "libxorp/debug.h"
#include "libxorp/asnum.hh"
#include "libxorp/exceptions.hh"

//AS Path Values
enum ASPathSegType {
    AS_SET = 1,
    AS_SEQUENCE = 2
};

class AsSegment {
public:
    AsSegment();
    AsSegment(const AsSegment& asseg);
    virtual ~AsSegment();
    const char * get_data() const;
    void set_size(size_t s);
    virtual size_t get_as_path_length() const = 0;
    size_t get_as_size() const { return _num_as_entries; }
    size_t get_as_byte_size() const { return _num_bytes_in_aspath; }
    void add_as(const AsNum& n);
    void prepend_as(const AsNum& n);
    bool contains(const AsNum& as_num) const;
    const AsNum& first_asnum() const;
    virtual void decode() = 0;
    virtual void encode() const = 0;
    void encode(ASPathSegType type) const;
    virtual ASPathSegType type() const = 0;
    virtual string str() const = 0;
    bool operator==(const AsSegment& him) const;
    bool operator<(const AsSegment& him) const;
protected:
    size_t _num_as_entries; // number of as numbers in the as path
    mutable size_t _num_bytes_in_aspath; // number of bytes in the as path
    mutable const uint8_t* _data;
    list <AsNum> _aslist;
private:
};

class AsSet : public AsSegment {
public:
    AsSet();
    AsSet(const uint8_t* d, size_t l);
    AsSet(const AsSet& asset);
    ~AsSet();
    void decode();
    void encode() const;
    ASPathSegType type() const { return AS_SET; }
    // Note: the contribution of an AS Set to AS Path Length is 1,
    // irrespective of the number of entries in the AS Set
    size_t get_as_path_length() const { return 1; }
    string str() const;
protected:
private:
};

class AsSequence : public AsSegment {
public:
    AsSequence();
    AsSequence(const uint8_t* d, size_t l);
    AsSequence(const AsSequence& asseq);
    ~AsSequence();
    void decode();
    void encode() const;
    ASPathSegType type() const { return AS_SEQUENCE; }
    size_t get_as_path_length() const { return _num_as_entries; }
    string str() const;
protected:
private:
};

class AsPath {
public:
    AsPath();
    AsPath(const char *as_path) throw(InvalidString);
    AsPath(const AsPath &as_path);
    ~AsPath();
    void add_segment(const AsSegment& s);
    size_t get_path_length() const { return _as_path_length; }
    bool contains(const AsNum& as_num) const;
    const AsNum& first_asnum() const;
    string str() const;
    const AsSegment* get_segment(size_t n) const;
    size_t get_num_segments() const { return _num_segments; }
    void encode() const;
    const char* get_data() const;
    uint16_t get_byte_size() const { return _byte_length; }
    void add_AS_in_sequence(const AsNum &asn);
    bool operator==(const AsPath& him) const;
    bool operator<(const AsPath& him) const;
protected:
private:
    void increase_as_path_length(size_t n);
    void decrease_as_path_length(size_t n);

    list <AsSegment*> _segments;
    size_t _num_segments;
    size_t _as_path_length;
    mutable const uint8_t* _data;
    mutable uint16_t _byte_length;
};

#endif // __BGP_ASPATH_HH__
