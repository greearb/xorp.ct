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

/* Classes representing a Priority queue. This data structure is
 * particular efficient for adding items to a list and then deleting
 * the item with the smallest cost. We use it for the Dijkstra
 * algorithm, and also to implement the timer queue.
 */

/* Representation of an individual element on a
 * priority queue.
 */

class PriQElt {
    PriQElt *left;
    PriQElt *right;
    PriQElt *parent;
    uns32 dist;
  protected:
    uns32 cost0;
    uns16 cost1;
    byte tie1;
    uns32 tie2;
  public:
    friend class PriQ;
    friend class OSPF;
    friend class SpfArea;
    inline PriQElt();
    inline bool costs_less(PriQElt *);
/*    friend int main(int argc, char *argv[]); */
};

// Constructor
inline PriQElt::PriQElt()
{
    left = 0;
    right = 0;
    parent = 0;
    dist = 0;
    cost0 = 0;
    cost1 = 0;
    tie1 = 0;
    tie2 = 0;
}

// Cost comparison function
inline bool PriQElt::costs_less(PriQElt *oqe)
{
    if (cost0 < oqe->cost0)
	return(true);
    else if (cost0 > oqe->cost0)
	return(false);
    else if (cost1 < oqe->cost1)
	return(true);
    else if (cost1 > oqe->cost1)
	return(false);
    else if (tie1 > oqe->tie1)
	return(true);
    else if (tie1 < oqe->tie1)
	return(false);
    else if (tie2 > oqe->tie2)
	return(true);
    else
	return(false);
}

/* Representation of the Priority queue itself
 */

class PriQ {
    PriQElt *root;
    void priq_adjust(PriQElt *balance_pt, int deleting);
    void priq_merge(PriQ & otherq);
  public:
    inline PriQ();
    inline PriQElt *priq_gethead();
    PriQElt *priq_rmhead();
    void priq_add(PriQElt *item);
    void priq_delete(PriQElt *item);
};

// Inline functions
inline PriQ::PriQ() : root(0)
{
}
inline PriQElt *PriQ::priq_gethead()
{
    return(root);
}
