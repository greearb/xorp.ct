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

/* Declaration and implementation of the stack class.
 * This is used by the AVL trees deletion function.
 */

class Stack {
    enum {StkSz = 32};
    void *stack[StkSz];
    void **sp;
    void **marker;

public:
    Stack() : sp(stack) {};
    inline void push(void *ptr);
    inline void *pop();
    inline void mark();
    inline void replace_mark(void *ptr);
    inline int is_empty();
    inline void *truncate_to_mark();
    inline void reset();
};

// Inline functions
inline void Stack::push(void *ptr)
{
    *sp++ = ptr;
}
inline void *Stack::pop()
{
    if (sp > stack)
	return(*(--sp));
    else return(0);
}
inline void Stack::mark()
{
    marker = sp;
}
inline void Stack::replace_mark(void *ptr)
{
    *marker = ptr;
}
inline int Stack::is_empty()
{
    return(sp == stack);
}
inline void *Stack::truncate_to_mark()
{
    sp = marker;
    return(*sp);
}
inline void Stack::reset()
{
    sp = stack;
}
