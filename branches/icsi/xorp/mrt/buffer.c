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

#ident "$XORP: xorp/mrt/buffer.c,v 1.4 2002/12/09 18:29:21 hodson Exp $"


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
