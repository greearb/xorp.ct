// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/click_elements/ipv4address.hh,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $

#ifndef __FEA_CLICK_ELEMENTS_IPV4ADDRESS_HH__
#define __FEA_CLICK_ELEMENTS_IPV4ADDRESS_HH__

#include <click/string.hh>

// #define	TEST

class IPV4address {
public:
    IPV4address();
    IPV4address(unsigned int);
    ~IPV4address();
    void set(int);
    bool matches_prefix(IPV4address dst, IPV4address mask) const;
    bool longest_mask(IPV4address mask) const;
    IPV4address operator&(IPV4address);
    IPV4address operator&=(IPV4address);
    bool operator==(IPV4address);
    static IPV4address mask(int);
    int IPV4address::mask_length();
    String string(int base = 10) const;
    String prt(int i, int base = 10) const;
    operator String() const	{ return string(); }
    unsigned char * data() const { return (unsigned char *)(&_addr);};

#ifdef	TEST
    void test();
#endif
private:
    uint32_t _addr;	// Network byte order
};

#ifdef	TEST
inline
void
IPV4address::test()
{
    static int once = 0;

    if(0 != once++)
	return;

     for(int i = 0; i < 32; i++)
  	click_chatter("IPV4address::test %d %s", i, mask(i).string().cc());

    for(int i = 0; i < 255; i++)
	click_chatter("IPV4address::test %d %s", i, prt(i, 16).cc());
}
#endif

inline
IPV4address::IPV4address() : _addr(0)
{
#ifdef	TEST
    test();
#endif
}

inline
IPV4address::IPV4address(unsigned int a) : _addr(a)
{
}

inline
IPV4address::~IPV4address()
{
}

inline
void
IPV4address::set(int val)
{
    _addr = val;
}

inline
bool
IPV4address::matches_prefix(IPV4address dst, IPV4address mask) const
{
    return (this->_addr & mask._addr) == dst._addr;
}

inline
bool
IPV4address::longest_mask(IPV4address mask) const
{
    return mask._addr == (this->_addr & mask._addr);
}

inline
IPV4address 
IPV4address::operator&(IPV4address a)
{
    return IPV4address(_addr & a._addr);
}

inline
IPV4address 
IPV4address::operator&=(IPV4address a)
{
    this->_addr &= a._addr;

    return *this;
}

inline
bool
IPV4address::operator==(IPV4address a)
{
    return this->_addr == a._addr;
}

inline
IPV4address 
IPV4address::mask(int n)
{
    IPV4address a = 0;
    uint32_t addr = 0;
    
    uint32_t mask = (1 << 31);

    for(int i = 0; i < n; i++) {
	addr |= mask;
	mask >>= 1;
    }
    
    unsigned char *ptr = a.data();

    for(int i = 0; i < 4; i++) {
	ptr[i] = (addr >> (8 * (3 - i))) & 0xff;
    }

    return a;
}

inline
int
IPV4address::mask_length()
{
    for(int i = 0; i <= 32; i++)
	if(mask(i) == _addr)
	    return i;

    return -1;
}

static
String
prt(int val, int base)
{
    if(0 == val)
	return "";
    else
	return ::prt(val / base, base) + 
	    String("0123456789abcdef"[val % base]);
}

inline
String
IPV4address::prt(int val, int base) const
{
    if(0 == val)
	return "0";
    else
	return ::prt(val, base);
}

inline
String
IPV4address::string(int base) const
{
    String s;
    unsigned char *ptr = data();
    for(int i = 0; i < 4; i++) {
	int val = ptr[i];
  	s += prt(val, base);
	if(i < 3)
	    s += String(".");
    }
    
    return s;
}
#endif // __FEA_CLICK_ELEMENTS_IPV4ADDRESS_HH__
