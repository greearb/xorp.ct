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

/* Classes supporting iteration through various lists.
 * Good candidates for implementation via templates.
 */

/* The Interface iterator class, used when walking down a list of interfaces.
 * Primitives provided to walk down the list, and to delete the
 * "current" list element (which may or may not be the list's
 * head.
 */

class IfcIterator {
    SpfIfc *next;	// Next list element
    class SpfArea *area;
    const class OSPF *inst;

public:
    inline IfcIterator(const class OSPF *);
    inline IfcIterator(SpfArea *);
    inline void reset();
    inline SpfIfc *get_next();
};

inline IfcIterator::IfcIterator(const class OSPF *spf)
{
    next = spf->ifcs;
    area = 0;
    inst = spf;
}
inline IfcIterator::IfcIterator(SpfArea *ap)
{
    next = ap->a_ifcs;
    area = ap;
    inst = 0;
}
inline void IfcIterator::reset()
{
    if (area)
	next = area->a_ifcs;
    else
	next = inst->ifcs;
}
inline SpfIfc *IfcIterator::get_next()
{
    SpfIfc *retval;
    retval = next;
    if (next)
	next = (area ? next->anext : next->next);
    return(retval);
}

/* The Neighbor iterator class. Similar to the above interface
 * iterator, only it's always per-interface.
 */

class NbrIterator {
    SpfNbr *next;	// Next list element
    SpfIfc *ifc;	// Containing interface

public:
    inline NbrIterator(SpfIfc *);
    inline void reset();
    inline SpfNbr *get_next();
};

inline NbrIterator::NbrIterator(SpfIfc *ip)
{
    next = ip->if_nlst;
    ifc = ip;
}
inline void NbrIterator::reset()
{
    next = ifc->if_nlst;
}
inline SpfNbr *NbrIterator::get_next()
{
    SpfNbr *retval;
    retval = next;
    if (next)
	next = next->next;
    return(retval);
}

/* The Area iterator class.
 */

class AreaIterator {
    SpfArea *next;	// Next list element
    OSPF *inst;		// OSPF instance

public:
    inline AreaIterator(OSPF *);
    inline SpfArea *get_next();
};

inline AreaIterator::AreaIterator(OSPF *instance)
{
    next = instance->areas;
    inst = instance;
}
inline SpfArea *AreaIterator::get_next()
{
    SpfArea *retval;
    retval = next;
    if (next)
	next = next->next;
    return(retval);
}

/* Iterate through all MD5 keys belonging to an interface.
 */

class KeyIterator {
    CryptK *next;	// Next list element
    SpfIfc *ifc;	// Containing interface

public:
    inline KeyIterator(SpfIfc *);
    inline CryptK *get_next();
};

inline KeyIterator::KeyIterator(SpfIfc *ip)
{
    next = ip->if_keys;
    ifc = ip;
}
inline CryptK *KeyIterator::get_next()
{
    CryptK *retval;
    retval = next;
    if (next)
	next = next->link;
    return(retval);
}
