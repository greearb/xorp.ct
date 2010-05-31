/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/* vim:set sts=4 ts=8: */
/*
 * Copyright (c) 2001
 * YOID Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  $XORP: xorp/libcomm/comm_private.h,v 1.5 2006/10/12 01:24:45 pavlin Exp $
 */

#ifndef __LIBCOMM_COMM_PRIVATE_H__
#define __LIBCOMM_COMM_PRIVATE_H__


/*
 * COMM socket library private include header.
 */

/*
 * Constants definitions
 */

/*
 * Structures, typedefs and macros
 */

/*
 * Global variables
 */

extern int _comm_serrno;

/*
 * Global functions prototypes
 */
__BEGIN_DECLS

/**
 * Log an error if an IPv6 specific method is called when IPv6 is not
 * present.
 *
 * An error message is output via XLOG_ERROR().
 * Set an appropriate error code relevant to the underlying system.
 * Note: This is currently done with knowledge of how the error code is
 * stored internally, which is a design bug (we're not thread friendly).
 * This function is variadic so it can be used to remove unused variable
 * warnings in non-IPv6 code as well as log the error.
 *
 * @param method C-style string denoting the ipv6 function called.
 */
void comm_sock_no_ipv6(const char* method, ...);

/**
 * Fetch and record the last socket layer error code from the underlying
 * system.
 *
 * Note: Currently not thread-safe. Internal use only.
 * This is done using a function to facilitate using explicit
 * Thread Local Storage (TLS) at a later time.
 */
void _comm_set_serrno(void);

__END_DECLS


#endif /* __LIBCOMM_COMM_PRIVATE_H__ */
