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

#include "ospfinc.h"

/* Constructor for a ConfigItem. Enqueue onto the
 * doubly linked list of configuration data within the
 * main ospf class.
 */

ConfigItem::ConfigItem()

{
    updated = true;
    prev = 0;
    next = cfglist;
    cfglist = this;
    if (next)
	next->prev = this;
}

/* Destructor for a ConfigItem. Remove from the
 * doubly linked list of configuration items.
 */

ConfigItem::~ConfigItem()

{
    if (next)
	next->prev = prev;
    if (prev)
	prev->next = next;
    else
	cfglist = next;
}

/* A reconfiguration sequence has started.
 * Go through and mark all the configuration items
 * so that we can tell which don't get updated
 * (those we will either delete or return to their
 * default values).
 */

void OSPF::cfgStart()

{
    ConfigItem *item;

    for (item = cfglist; item; item = item->next)
	item->updated = false;
}

/* A reconfiguration sequence has ended.
 * Those configuration items that have not been
 * updated are either deleted or returned to their
 * default values.
 * We have to be a little careful traversing the list
 * of configured items because the removal of one
 * item by ConfigItem::clear_config() may cause
 * other items to get deleted further down the list.
 */

void OSPF::cfgDone()

{
    ConfigItem **prev;
    ConfigItem *item;

    for (prev = &cfglist; (item = *prev); ) {
	if (!item->updated)
	    item->clear_config();
	if (item == *prev)
	    prev = &item->next;
    }
}
