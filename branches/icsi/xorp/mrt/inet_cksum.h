/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001,2002 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/mrt/inet_cksum.h,v 1.2 2002/12/09 18:29:21 hodson Exp $
 */


#ifndef __MRT_CRC_H__
#define __MRT_CRC_H__


/*
 * Header file for Internet Protocol family headers checksum computation.
 */


/*
 * Constants definitions
 */

/*
 * Structures, typedefs and macros
 */
#define INET_CKSUM(addr, len)	inet_cksum((uint16_t *)(addr), (u_int)(len))
#define INET_CKSUM_ADD(sum1, sum2) inet_cksum_add((sum1), (sum2))

/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS
extern int	inet_cksum(uint16_t *addr, u_int len);
extern int	inet_cksum_add(uint16_t sum1, uint16_t sum2);
__END_DECLS

#endif /* __MRT_CRC_H__ */
