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

/* List of OSPF include files.
 */

#include <stdio.h>
#include "machdep.h"
#include "stack.h"
#include "priq.h"
#include "timer.h"
#include "ip.h"
#include "spftype.h"
#include "arch.h"
#include "avl.h"
#include "lshdr.h"
#include "spfparam.h"
#include "tlv.h"
#include "config.h"
#include "pat.h"
#include "rte.h"
#include "lsa.h"
#include "lsalist.h"
#include "spfpkt.h"
#include "spfutil.h"
#include "spfarea.h"
#include "spfifc.h"
#include "spfnbr.h"
#include "spflog.h"
#include "mcache.h"
#include "ospf.h"
#include "dbage.h"
#include "iterator.h"
#include "globals.h"
