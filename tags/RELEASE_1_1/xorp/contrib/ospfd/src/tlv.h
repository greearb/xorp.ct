/*
 *   OSPFD routing daemon
 *   Copyright (C) 2001 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* This file contains the class used to build and parse
 * TLVs within the body of OSPF LSAs.
 */

class TLVbuf {
    byte *buf;
    int blen;
    TLV *current;

    int tlen(int vlen);
    TLV *reserve(int vlen);
  public:
    // Parse routines
    TLVbuf(byte *body, int blen);
    bool next_tlv(int & type);
    bool get_int(int32 & val);
    bool get_short(uns16 & val);
    bool get_byte(byte & val);
    char *get_string(int & len);
    // Build routines
    TLVbuf();
    void reset();
    void put_int(int type, int32 val);
    void put_short(int type, uns16 val);
    void put_byte(int type, byte val);
    void put_string(int type, char *str, int len);
    inline byte *start();
    inline int length();
};

inline byte *TLVbuf::start()
{
    return(buf);
}
inline int TLVbuf::length()
{
    return(((byte *) current) - buf);
}

