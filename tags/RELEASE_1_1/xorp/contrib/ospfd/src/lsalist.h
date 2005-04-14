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

/* Definitions for LSA lists. Consists of the following:
 *
 *	LsaList:	Class for list head.
 *	LsaListElement:	Class describing individual elements.
 *	LsaListIterator:Class allowing list traversal
 *
 */

/*
 * Dynamically constructed lists are made
 * out of the following two word structure: the first field for the
 * singly-linked list, and the second for a pointer to the LSA (car/cdrs).
 *
 * Whenever an LSA is on a list, its reference count
 * is incremented. LSAs are not actually freed until their reference
 * count goes to 0.
 */

class LsaListElement {
    LsaListElement *next; 	// Next in list
    LSA	*lsap;			// Pointer to LSA
    // For customized memory mgmt
    enum {
	BlkSize = 256
	};		// Allocate BlkSize elts at a time
    static LsaListElement *freelist;// Free list of elements
    static int n_allocated;		// # list elements allocated
    static int n_free;		// # list elements free

    void * operator new(size_t size);
    void operator delete(void *ptr, size_t);
    inline LsaListElement(LSA *);
    inline ~LsaListElement();

    friend class LsaList;
    friend class LsaListIterator;

public:
    inline int n_free_elements();
    inline int n_alloc_elements();
};

inline LsaListElement::LsaListElement(LSA *adv) : next(0), lsap(adv)
{
    adv->ref();
}
inline LsaListElement::~LsaListElement()
{
    lsap->deref();
}
inline int LsaListElement::n_free_elements()
{
    return(n_free);
}
inline int LsaListElement::n_alloc_elements()
{
    return(n_allocated);
}

/* The list header, which consists of a head pointer, tail pointer
 * and a count of the current list size.
 */

class LsaList {
    LsaListElement *head; 	// Head of list
    LsaListElement *tail; 	// tail of list
    int	size;			// # elements on list

public:
    inline LsaList();
    inline void addEntry(LSA *);
    inline LSA *FirstEntry();
    int	remove(LSA *);
    void clear();
    int	garbage_collect();
    void append(LsaList *);
    inline bool is_empty();
    inline int count();

    friend class LsaListIterator;
};

inline LsaList::LsaList() : head(0), tail(0), size(0)
{
}
inline LSA *LsaList::FirstEntry()
{
    return(head ? head->lsap : 0);
}
inline bool LsaList::is_empty()
{
    return(head == 0);
}
inline int LsaList::count()
{
    return(size);
}

// LsaList::addEntry(). Add element to list tail.

inline void LsaList::addEntry(LSA *lsap)

{
    LsaListElement *ep;

    ep = new LsaListElement(lsap);
    if (!head) {
	head = ep;
	tail = ep;
    }
    else {
	tail->next = ep;
	tail = ep;
    }
    size++;
}

/* The iterator class, used when walking down an LsaList.
 * Primitives provided to walk down the list, and to delete the
 * "current" list element (which may or may not be the list's
 * head.
 */

class LsaListIterator {
    LsaList *list;	// List being operated upon
    LsaListElement *prev; // Address of previous pointer
    LsaListElement *current;// Current list element

public:
    inline LsaListIterator(LsaList *);
    inline LSA *get_next();	// Return next LSA on list
    inline void remove_current(); // remove current element from list
    inline LSA	*search(int lstype, lsid_t id, rtid_t org);
};

inline LsaListIterator::LsaListIterator(LsaList *xlist)
{
    list = xlist;
    prev = 0;
    current = 0;
}

// LsaListIterator::get_next(). Return next LSA on list

inline LSA *LsaListIterator::get_next()

{
    prev = current;
    current = (current ? current->next : list->head);

    return(current ? current->lsap : 0);
}

/* LsaListIterator::remove_current(). Remove LSA that is currently
 * being accessed by the iterator.
 * remove_current() must not be called twice in a row
 * without an intervening call to get_next().
 */

inline void LsaListIterator::remove_current()

{
    if (current) {
	if (list->tail == current)
	    list->tail = prev;
	list->size--;
	if (!prev)
	    list->head = current->next;
	else
	    prev->next = current->next;
	delete current;
	current = prev;
    }
}

// LsaListIterator::search(). Find particular LSA on list, regardless
// of particular instance

inline LSA *LsaListIterator::search(int lstype, lsid_t id, rtid_t org)

{
    LSA	*lsap;

    while ((lsap = get_next())) {
	if (lsap->lsa_type != lstype)
	    continue;
	if (lsap->ls_id() != id)
	    continue;
	if (lsap->adv_rtr() == org)
	    return(lsap);
    }
    
    return(0);
}
