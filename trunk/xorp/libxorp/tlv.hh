// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/devnotes/template.hh,v 1.5 2005/03/25 02:52:59 pavlin Exp $

#ifndef __LIBXORP_TLV_HH__
#define __LIBXORP_TLV_HH__

/**
 * <Type,Length,Value>
 * Read and Write TLV records.
 */
class Tlv {
 public:
    Tlv() : _fp(0)
	{}

    bool open(string& fname, bool read) {
	if ("-" == fname) {
	    _fp = read ? stdin : stdout;
	} else {
	    _fp = fopen(fname.c_str(), read ? "r" : "w");
	}
	if (0 == _fp)
	    return false;

	return true;
    }

    bool read(uint32_t& type, vector<uint8_t>& data) {
	uint32_t n;
	if (1 != fread(&n, sizeof(n), 1, _fp))
	    return false;
	
	type = ntohl(n);

	if (1 != fread(&n, sizeof(n), 1, _fp))
	    return false;

	uint32_t len = ntohl(n);

	data.resize(len);
	if (len != fread(&data[0], 1, len, _fp))
	    return false;

	return true;
    }

    bool write(uint32_t type, vector<uint8_t>& data) {
	uint32_t n = htonl(type);
	if (1 != fwrite(&n, sizeof(n), 1, _fp))
	    return false;
	
	uint32_t len = data.size();
	n = ntohl(len);

	if (1 != fwrite(&n, sizeof(n), 1, _fp))
	    return false;

	if (len != fwrite(&data[0], 1, len, _fp))
	    return false;

	return true;
    }

    bool close() {
	if (0 == _fp) {
	    return false;
	}
	return fclose(_fp);
    }

    bool get32(vector<uint8_t>& data, uint32_t offset, uint32_t& u32) {

	if (data.size() < offset + sizeof(u32))
	    return false;

	u32 = data[offset + 0];
	u32 <<= 8;
	u32 |= data[offset + 1];
	u32 <<= 8;
	u32 |= data[offset + 2];
	u32 <<= 8;
	u32 |= data[offset + 3];
	
	return true;
    }

    bool put32(vector<uint8_t>& data, uint32_t offset, uint32_t u32) {

	if (data.size() < offset + sizeof(u32))
	    return false;

	data[offset + 0] = (u32 >> 24) & 0xff;
	data[offset + 1] = (u32 >> 16) & 0xff;
	data[offset + 2] = (u32 >> 8) & 0xff;
	data[offset + 3] = u32 & 0xff;

	return true;
    }

 private:
    FILE *_fp;
};

#endif // __LIBXORP_TLV_HH__
