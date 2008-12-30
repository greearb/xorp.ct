/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/mrt/buffer.c,v 1.8 2008/07/23 05:11:05 pavlin Exp $"


/*
 * Buffer management.
 */


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "buffer.h"


/*
 * Exported variables
 */

/*
 * Local constants definitions
 */

/*
 * Local structures, typedefs and macros
 */

/*
 * Local variables
 */

/*
 * Local functions prototypes
 */


/**
 * buffer_malloc_utils_:
 * @buffer_size: The desired maximum size of the data stream.
 * 
 * Allocate and initialize a #buffer_t structure.
 * 
 * Return value: The allocated #buffer_t structure.
 **/
buffer_t *
buffer_malloc_utils_(size_t buffer_size)
{
    buffer_t *buffer;
    
    buffer = malloc(sizeof(*buffer));
    buffer->_data = malloc(buffer_size);
    buffer->_buffer_size = buffer_size;
    BUFFER_RESET(buffer);
    
    return (buffer);
}

/**
 * buffer_free_utils_:
 * @buffer: The #buffer_t structure to free.
 * 
 * Free a #buffer_t structure and all associated memory with it (including
 * the data stream).
 **/
void
buffer_free_utils_(buffer_t *buffer)
{
    if (buffer->_data != NULL)
	free(buffer->_data);
    
    free(buffer);
}

/**
 * buffer_reset_utils_:
 * @buffer: The #buffer_t structure to reset.
 * 
 * Reset a #buffer_t structure (e.g., it will not contain any data).
 * 
 * Return value: The reset #buffer_t structure.
 **/
buffer_t *
buffer_reset_utils_(buffer_t *buffer)
{
    buffer->_data_head	= buffer->_data;
    buffer->_data_tail	= buffer->_data;
    memset(buffer->_data, 0, buffer->_buffer_size);
    
    return (buffer);
}
