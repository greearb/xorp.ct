/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
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

/* Definitions for the generic Patricia tree data
 * structure.
 */

/* Patricia bit check operation. Returns true if bit is set,
 * false otherwise. We're doing exact match, including length,
 * so we append the length as last four bytes of the string,
 * and then zero fill after that.
 */

inline bool bit_check(const byte *str, int len, int bit)

{
    int32 bitlen;

    bitlen = len << 3;
    if (bit < bitlen)
	return((str[bit>>3] & (0x80 >> (bit & 0x07))) != 0);
    else if (bit < bitlen + 32)
	return((bitlen & (1 << (bit - bitlen))) != 0);
    else
	return(false);
}

/* Element within a Patricia tree.
 */

const uns32 MaxBit = 0xffffffffL;

class PatEntry {
    PatEntry *zeroptr;
    PatEntry *oneptr;
    uns32 chkbit;
  public:
    byte *key;
    int	keylen;

    inline bool	bit_check(int bit);
    friend class PatTree;
};

inline bool PatEntry::bit_check(int bit)
{
    return(::bit_check(key, keylen, bit));
}

/* The Patricia tree root.
 */

class PatTree {
    PatEntry *root;
    int	size;
  public:
    PatTree();
    void init();
    void add(PatEntry *);
    PatEntry *find(const byte *key, int keylen);
    PatEntry *find(const char *key);
    void remove(PatEntry *);
    void clear();
    void clear_subtree(PatEntry *);
};
