/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/* Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/xlog.c,v 1.4 2003/05/16 17:28:16 hodson Exp $"


/*
 * Message logging utility.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

#include "libxorp_module.h"
#include "config.h"
#include "xlog.h"

/*
 * Exported variables
 */

/*
 * Local constants definitions
 */
#define MAX_XLOG_OUTPUTS	10

/*
 * Local structures, typedefs and macros
 */
#ifdef UNUSED
#undef UNUSED
#endif
#define UNUSED(x) (x) = (x)

/*
 * Local variables
 */
static int		init_flag = 0;
static int		start_flag = 0;
static pid_t		pid = 0;
static char		*preamble_string = NULL;
static char		*process_name_string = NULL;
static FILE		*xlog_outputs_file[MAX_XLOG_OUTPUTS];
static xlog_output_func_t xlog_outputs_func[MAX_XLOG_OUTPUTS];
static void		*xlog_outputs_obj[MAX_XLOG_OUTPUTS];
static size_t		xlog_output_file_count = 0;
static size_t		xlog_output_func_count = 0;
static FILE		*fp_default = NULL;
static int		xlog_level_enabled[XLOG_LEVEL_MAX];
static xlog_verbose_t	xlog_verbose_level[XLOG_LEVEL_MAX];
/*
 * XXX: the log level names below has to be consistent with the XLOG_LEVEL_*
 * values.
 */
static const char	*xlog_level_names[XLOG_LEVEL_MAX] = {
				"FATAL",
				"ERROR",
				"WARNING",
				"INFO",
				"TRACE"
			};


/* State and functions for systems without varargs preprocessors */
static const char	*the_module = NULL;
static const char	*the_file = NULL;
static int		the_line = 0;
static const char	*the_func = NULL;

/*
 * Local functions prototypes
 */

static int	xlog_init_lock(void);
static int	xlog_init_unlock(void);
static int	xlog_exit_lock(void);
static int	xlog_exit_unlock(void);
static int	xlog_write_lock(void);
static int	xlog_write_unlock(void);
static void	xlog_record_va(xlog_level_t log_level, const char* module_name,
			       const char *where, const char* format,
			       va_list ap);
static int	xlog_write(FILE* fp, const char* fmt, ...);
static int	xlog_write_va(FILE* fp, const char* fmt, va_list ap);
static int	xlog_flush(FILE* fp);
static const char* xlog_localtime2string_short(void);


/*
 * ****************************************************************************
 * Control functions to init/exit/start/stop/etc the log utility.
 * ****************************************************************************
 */

/**
 * xlog_init:
 * @argv0 The path to the executable.
 * @preamble_message A string that will become part of the preamble string.
 * 
 * Initialize the log utility.
 * As part of the initialization, the preamble string will be set to
 * <@module_name><@preamble_message>
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_init(const char *argv0, const char *preamble_message)
{
    const char* process_name;
    size_t i;

    if (init_flag)
	return (-1);

    /* Get the init lock */
    if (xlog_init_lock() < 0) {
	fprintf(stderr, "Error obtaining xlog_init_lock()\n");
	exit (1);
    }
    
    pid = getpid();
    if (process_name_string != NULL) {
	free(process_name_string);
	process_name_string = NULL;
    }

    process_name = strrchr(argv0, '/');
    if (process_name != NULL)
	process_name++;		/* Skip the last '/' */
    if (process_name == NULL)
	process_name = argv0;

    if (process_name != NULL)
	process_name_string = strdup(process_name);
    
    /* Set the preamble string */
    xlog_set_preamble(preamble_message);
    
    /* Enable all log messages by default, and set default verbose level */
    for (i = 0; i < XLOG_LEVEL_MAX; i++) {
	xlog_enable(i);
	xlog_verbose_level[i] = XLOG_VERBOSE_LOW;		/* Default */
    }
    xlog_verbose_level[XLOG_LEVEL_FATAL] = XLOG_VERBOSE_HIGH;	/* XXX */
    
    init_flag = 1;
    
    /* Release the init lock */
    xlog_init_unlock();
    
    return (0);
}

/**
 * xlog_exit:
 * @void: 
 * 
 * Gracefully exit logging.
 * 
 * Return value: 0 on success, othewise -1.
 **/
int
xlog_exit(void)
{
    int i;
    
    if (! init_flag)
	return (-1);
    
    if (start_flag)
	xlog_stop();
    
    /* Get the exit lock */
    if (xlog_exit_lock() < 0) {
	return (-1);
    }
    
    /* Reset local variables */
    init_flag = 0;
    pid = 0;
    if (process_name_string != NULL) {
	free (process_name_string);
	process_name_string = NULL;
    }
    if (preamble_string != NULL) {
	free (preamble_string);
	preamble_string = NULL;
    }
    
    for (i = 0; i < MAX_XLOG_OUTPUTS; i++) {
	xlog_outputs_file[i] = NULL;
	xlog_outputs_func[i] = NULL;
	xlog_outputs_obj[i] = NULL;
    }
    xlog_output_file_count = 0;
    xlog_output_func_count = 0;
    fp_default = 0;
    
    for (i = 0; i < XLOG_LEVEL_MAX; i++) {
	xlog_disable(i);
	xlog_verbose_level[i] = XLOG_VERBOSE_LOW;		/* Default */
    }
    xlog_verbose_level[XLOG_LEVEL_FATAL] = XLOG_VERBOSE_HIGH;	/* XXX */
    
    /* Release the exit lock */
    xlog_exit_unlock();
    
    return (0);
}

/**
 * xlog_start:
 * @void: 
 * 
 * Start logging.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_start(void)
{
    if (! init_flag)
	return (-1);
    
    if (start_flag)
	return (-1);
    
    start_flag = 1;
    
    return (0);
}

/**
 * xlog_stop:
 * @void: 
 * 
 * Stop logging.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_stop(void)
{
    if (! start_flag)
	return (-1);
    
    start_flag = 0;
    
    return (0);
}

/**
 * xlog_enable:
 * @log_level: The message type (e.g., %XLOG_LEVEL_WARNING) to enable
 * the logging for.
 * 
 * Enable logging for messages of type &log_level_t.
 * By default, all message types are enabled.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_enable(xlog_level_t log_level)
{
    if (XLOG_LEVEL_MAX <= log_level)
	return (-1);
    
    xlog_level_enabled[log_level] = 1;
    
    return (0);
}

/**
 * xlog_disable:
 * @log_level: The message type (e.g., %XLOG_LEVEL_WARNING) to disable
 * the logging for.
 * 
 * Enable logging for messages of type &log_level_t.
 * XXX: %XLOG_LEVEL_FATAL cannot be disabled.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_disable(xlog_level_t log_level)
{
    if (XLOG_LEVEL_MAX <= log_level)
	return (-1);
    
    /* XXX: XLOG_LEVEL_FATAL can never be disabled */
    if (log_level == XLOG_LEVEL_FATAL)
	return (-1);
    
    xlog_level_enabled[log_level] = 0;
    
    return (0);
}

/**
 * xlog_set_preamble:
 * @text: The preamble string, or NULL if no preamble.
 * 
 * Set the preamble string for the log entries.
 **/
void
xlog_set_preamble(const char* text)
{
    /* Free the memory for the old preamble */
    if (preamble_string != NULL) {
	free(preamble_string);
	preamble_string = NULL;
    }
    /* Duplicate the new preamble string */
    if (text != NULL) {
	preamble_string = strdup(text);
    }
}

/**
 * xlog_process_name:
 * 
 * Get the process name as set by xlog_init.
 * Return value: pointer to process name on success, NULL otherwise.
 **/
const char*
xlog_process_name()
{
    return process_name_string;
}

/**
 * xlog_set_verbose:
 * @verbose_level: The level of verbosity #xlog_verbose_t
 * (higher is more verbose).
 * 
 * Set the level of verbosity (#xlog_verbose_t) for the log entries.
 * Applies for all type of messages except for %XLOG_LEVEL_FATAL
 * which always is set to the most verbose level.
 **/
void
xlog_set_verbose(xlog_verbose_t verbose_level)
{
    int i;
    
    if (XLOG_VERBOSE_HIGH < verbose_level)
	verbose_level = XLOG_VERBOSE_HIGH;
    
    for (i = 0; i < XLOG_LEVEL_MAX; i++) {
	if (i == XLOG_LEVEL_FATAL)
	    continue;		/* XXX: XLOG_LEVEL_FATAL cannot be changed */
	xlog_verbose_level[i] = verbose_level;
    }
}

/**
 * xlog_level_set_verbose:
 * @log_level: The message type #xlog_level_t to set the verbosity of.
 * @verbose_level: The level of verbosity #xlog_verbose_t
 * (higher is more verbose).
 * 
 * Set the level of verbosity (#xlog_verbose_t) for the log entries
 * of messages of type #xlog_level_t.
 * Note: %XLOG_LEVEL_FATAL verbosity cannot be changed, and is
 * always set to the most verbose level.
 **/
void
xlog_level_set_verbose(xlog_level_t log_level, xlog_verbose_t verbose_level)
{
    if (XLOG_LEVEL_MAX <= log_level)
	return;
    
    if (log_level == XLOG_LEVEL_FATAL)
	return;			/* XXX: XLOG_LEVEL_FATAL cannot be changed */
    
    if (XLOG_VERBOSE_HIGH < verbose_level)
	verbose_level = XLOG_VERBOSE_HIGH;
    
    xlog_verbose_level[log_level] = verbose_level;
}

/*
 * Macro function to generate xlog_info, xlog_warning, etc...
 */
#define xlog_fn(fn, log_level)					\
void									\
xlog_##fn (const char *module_name, const char *where, const char *fmt, ...) \
{									\
    va_list ap;								\
    va_start(ap, fmt);							\
    xlog_record_va(log_level, module_name, where, fmt, ap);		\
    va_end(ap);								\
}

/*
 * Macro function to generate a log function that aborts. E.g.: xlog_fatal
 * XXX: if signal SIGABRT is caught and the signal handler does not return,
 * the program will NOT terminate.
 */
#define xlog_fn_abort(fn, log_level)					\
void									\
xlog_##fn (const char *module_name, const char *where, const char *fmt, ...) \
{									\
    va_list ap;								\
    va_start(ap, fmt);							\
    xlog_record_va(log_level, module_name, where, fmt, ap);		\
    va_end(ap);								\
    abort();								\
}

/*
 * Generate the log functions
 */
xlog_fn_abort(fatal, 	XLOG_LEVEL_FATAL)
xlog_fn(error, 		XLOG_LEVEL_ERROR)
xlog_fn(warning,	XLOG_LEVEL_WARNING)
xlog_fn(info, 		XLOG_LEVEL_INFO)


/*
 * ****************************************************************************
 * Lock/unlock functions
 *
 * TODO: not implemented yet. Add whatever locks may be needed.
 * ****************************************************************************
 */

/**
 * xlog_init_lock:
 * @void: 
 * 
 * Obtain a lock needed during initialization of the log utility.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_init_lock(void)
{
    return (0);
}

/**
 * xlog_init_unlock:
 * @void: 
 * 
 * Release the lock used during initialization of the log utility.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_init_unlock(void)
{
    return (0);
}

/**
 * xlog_exit_lock:
 * @void: 
 * 
 * Obtain a lock needed during exit from the log utility.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_exit_lock(void)
{
    return (0);
}

/**
 * xlog_exit_unlock:
 * @void: 
 * 
 * Release the lock used during exit from the log utility.
 * 
 * Return value: 
 **/
static int
xlog_exit_unlock(void)
{
    return (0);
}

/**
 * xlog_write_lock:
 * @void: 
 * 
 * Obtain a lock needed during writing of a log message.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_write_lock(void)
{
    return (0);
}

/**
 * xlog_write_unlock:
 * @void: 
 * 
 * Release the lock used during writing of a log message.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_write_unlock(void)
{
    return (0);
}

/*
 * ****************************************************************************
 * Print functions
 * ****************************************************************************
 */

/**
 * xlog_record_va:
 * @log_level: The log level of the message to write.
 * @module_name: The name of the module this message applies to.
 * @where: Where the log was generated (e.g., the file name, line number, etc).
 * @format: The printf() format of the message to write.
 * Note that a trailing newline is added if none is present.
 * @ap: The vararg list of arguments.
 * 
 * Write a log message.
 **/
static void
xlog_record_va(xlog_level_t log_level, const char *module_name,
	       const char *where, const char *format, va_list ap)
{
    const char	*preamble_lead;
    const char	*process_name_lead;
    char	*buf_payload_ptr = NULL, *buf_preamble_ptr = NULL;
    char	*buf_output_ptr = NULL;
    int		buf_output_size;
    size_t	i;
    sig_t	sigpipe_handler;
    
    if (! start_flag) {
	if (! init_flag)
	    fprintf(stderr,
		    "Logging must be initialized first by xlog_init()\n");
	if (! start_flag)
	    fprintf(stderr,
		    "Logging must be started first by xlog_start()\n");
	
	abort();
    }
    
    if ((xlog_output_file_count == 0) && (xlog_output_func_count == 0))
	return;
    
    if (XLOG_LEVEL_MAX <= log_level)
	return;			/* Invalid log level */
    
    if (! xlog_level_enabled[log_level])
	return;			/* The log level is disabled */
    
    xlog_write_lock();
    sigpipe_handler = signal(SIGPIPE, SIG_IGN);
    
    preamble_lead = (preamble_string) ? preamble_string : "";
    process_name_lead = (process_name_string) ? process_name_string : "";
    
    /*
     * Prepare the preamble string to write.
     * XXX: we need to prepare it once, otherwise the time may be
     * different when we write to more than one outputs.
     */
    switch (xlog_verbose_level[log_level]) {
    case XLOG_VERBOSE_LOW:	/* The minimum log information */
	x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s ] ",
		   xlog_localtime2string_short(),
		   xlog_level_names[log_level],
		   process_name_lead,
		   module_name);
	break;
	
    case XLOG_VERBOSE_MEDIUM:	/* Add preamble string if non-NULL */
	x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s %s ] ",
		   xlog_localtime2string_short(),
		   preamble_lead,
		   xlog_level_names[log_level],
		   process_name_lead,
		   module_name);
	break;
	
    case XLOG_VERBOSE_HIGH:	/* Most verbose */
    default:
	x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s:%d %s %s ] ",
		   xlog_localtime2string_short(),
		   preamble_lead,
		   xlog_level_names[log_level],
		   process_name_lead,
		   (int)pid,
		   module_name,
		   where);
	break;
    }
    
    /*
     * Prepare the payload string to write
     */
    x_vasprintf(&buf_payload_ptr, format, ap);
    
    if ((buf_preamble_ptr == NULL)
	&& ((buf_payload_ptr == NULL) || buf_payload_ptr[0] == '\0'))
	goto cleanup_label;
    
    /*
     * Prepare the output string to write.
     * XXX: here we explicitly add the  '\n' at the end of the
     * output string, because the XLOG message format implies it.
     */
    buf_output_size = x_asprintf(&buf_output_ptr, "%s%s\n",
				 buf_preamble_ptr, buf_payload_ptr);
    if ((buf_output_ptr == NULL)
	|| (buf_output_ptr[0] == '\0')
	|| (buf_output_size < 0)) {
	goto cleanup_label;
    }
    
    /* Remove our '\n' from the end if the payload itself already has one */
    if (buf_output_size >= 2) {
	char n1, n2;
	n1 = buf_output_ptr[buf_output_size - 2];
	n2 = buf_output_ptr[buf_output_size - 1];
	if ((n1 == '\n') && (n2 == '\n'))
	    buf_output_ptr[buf_output_size - 1] = '\0';
    }
    
    /*
     * Write to the file descriptors
     */
    for (i = 0; i < xlog_output_file_count; ) {
	FILE *fp = xlog_outputs_file[i];
	if (xlog_write(fp, "%s", buf_output_ptr) || xlog_flush(fp)) {
	    xlog_remove_output(fp);
	    continue;
	}
	i++;
    }
    
    /*
     * Write to the functions
     */
    for (i = 0; i < xlog_output_func_count; ) {
	xlog_output_func_t func = xlog_outputs_func[i];
	void *obj = xlog_outputs_obj[i];
	if (func(obj, buf_output_ptr) < 0) {
	    xlog_remove_output_func(func, obj);
	    continue;
	}
	i++;
    }
    
 cleanup_label:
    /*
     * Cleanup
     */
    if (buf_preamble_ptr)
	free(buf_preamble_ptr);
    if (buf_payload_ptr)
	free(buf_payload_ptr);
    if (buf_output_ptr)
	free(buf_output_ptr);
    
    signal(SIGPIPE, sigpipe_handler);
    xlog_write_unlock();
}

/**
 * xlog_write:
 * @fp: The file descriptor to write to.
 * @fmt: The printf() style format of the message to write.
 * @: 
 * 
 * Format a message for writing and call a function to write it to @fp.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_write(FILE* fp, const char* fmt, ...)
{
    va_list ap;
    int retval;
    
    va_start(ap, fmt);
    retval = xlog_write_va(fp, fmt, ap);
    va_end(ap);
    
    return (retval);
}

/**
 * xlog_write_va:
 * @fp: The file descriptor to write to.
 * @fmt: The printf() style format of the message to write.
 * @ap: The vararg list of arguments.
 * 
 * Write a message to @fp.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_write_va(FILE* fp, const char* fmt, va_list ap)
{
    assert(fp != NULL);
    if (vfprintf(fp, fmt, ap) < 0) {
	return (-1);
    }
    
    return (0);
}

/**
 * xlog_flush:
 * @fp: The file descriptor to flush.
 * 
 * Flushes all buffered data for output stream @fp.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
static int
xlog_flush(FILE* fp)
{
    if (! fflush(fp))
	return (0);
    else
	return (-1);
}

/**
 * _xcond_trace_msg_long:
 * @module_name: The name of the module this message applies to.
 * @file: The file name where XLOG_TRACE() has occured.
 * @line: The line number where XLOG_TRACE() has occured.
 * @func: The function name where XLOG_TRACE() has occured.
 * @flag: The boolean flag that defines whether the messages should be printed.
 * @fmt: The printf() style format.
 * @: 
 * 
 * The entry function for XLOG_TRACE(). Used if the compiler supports
 * variable arguments for macros.
 * This function should not be called directly.
 **/
void
_xcond_trace_msg_long(const char	*module_name,
		      const char	*file,
		      int		line, 
		      const char	*func,
		      int		flag,
		      const char	*fmt, ...)
{
    if (flag) {
	va_list ap;
	char xlog_where_buf[8000];
	
	/* TODO: use _xcond_trace_entry and _xcond_trace_msg_short instead */
	sprintf(xlog_where_buf, "+%d %s %s", line, file,
		(func) ? func : "(unknown_func)");
	va_start(ap, fmt);
	xlog_record_va(XLOG_LEVEL_TRACE, module_name, xlog_where_buf, fmt, ap);
	va_end(ap);
    }
}

/**
 * _xcond_trace_entry:
 * @module_name: The name of the module this message applies to.
 * @file: The file name where XLOG_TRACE() has occured.
 * @line: The line number where XLOG_TRACE() has occured.
 * @func: The function name where XLOG_TRACE() has occured.
 * 
 * A helper function that sets the file name, line number and function name.
 * Needed if the compiler does not support variable arguments for macros.
 * This function should not be called directly.
 **/
void
_xcond_trace_entry(const char *module_name, const char *file,
		   int line, const char* func)
{
    the_module = module_name;
    the_file = file;	
    the_line = line;
    the_func = func;
}

/**
 * _xcond_trace_msg_short:
 * @flag: The boolean flag that defines whether the messages should be printed.
 * @fmt: The printf() style format.
 * @: 
 * 
 * The entry function for XLOG_TRACE(). Used if the compiler does not support
 * variable arguments for macros.
 * This function should not be called directly.
 **/
void 
_xcond_trace_msg_short(int flag, const char* fmt, ...)
{
    if (flag) {
	va_list ap;
	char xlog_where_buf[8000];

	sprintf(xlog_where_buf, "+%d %s %s", the_line, the_file,
		(the_func) ? the_func : "(unknown_func)");
	va_start(ap, fmt);
	xlog_record_va(XLOG_LEVEL_TRACE, the_module, xlog_where_buf, fmt, ap);
	va_end(ap);
    }
}

/*
 * ****************************************************************************
 * Output stream functions
 * ****************************************************************************
 */

/**
 * xlog_add_output:
 * @fp: The file descriptor to add to the set of output streams.
 * 
 * Add a file descriptor to the set of output streams.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int 
xlog_add_output(FILE* fp)
{
    size_t i;
    
    for (i = 0; i < xlog_output_file_count; i++) {
	if (xlog_outputs_file[i] == fp) {
	    return (0);
	}
    }
    if (i < MAX_XLOG_OUTPUTS) {
	xlog_outputs_file[i] = fp;
	xlog_output_file_count++;
	return (0);
    }
    
    return (-1);
}

/**
 * xlog_remove_output:
 * @fp: The file descriptor to remove from the set of output streams.
 * 
 * Remove a file descriptor from the set of output streams.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_remove_output(FILE* fp)
{
    size_t i, j;
    
    for (i = 0; i < xlog_output_file_count; i++) {
	if (xlog_outputs_file[i] == fp) {
	    for(j = i + 1; j < xlog_output_file_count; j++) {
		xlog_outputs_file[j - 1] = xlog_outputs_file[j];
	    }
	    xlog_output_file_count--;
	    return (0);
	}
    }
    
    return (-1);
}

/**
 * xlog_add_output_func:
 * @func: The function to add to the set of output streams.
 * @obj: The object to supply @func with when called.
 * 
 * Add a processing function and an object to the set of output streams.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int 
xlog_add_output_func(xlog_output_func_t func, void *obj)
{
    size_t i;
    
    for (i = 0; i < xlog_output_func_count; i++) {
	if ((xlog_outputs_func[i] == func)
	    && (xlog_outputs_obj[i] == obj)) {
	    return (0);
	}
    }
    if (i < MAX_XLOG_OUTPUTS) {
	xlog_outputs_func[i] = func;
	xlog_outputs_obj[i] = obj;
	xlog_output_func_count++;
	return (0);
    }
    
    return (-1);
}

/**
 * xlog_remove_output_func:
 * @func: The function to remove from the set of output streams.
 * @obj: The object that @func was supplied with.
 * 
 * Remove a processing function and an object from the set of output streams.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_remove_output_func(xlog_output_func_t func, void *obj)
{
    size_t i, j;
    
    for (i = 0; i < xlog_output_func_count; i++) {
	if ((xlog_outputs_func[i] == func)
	    && (xlog_outputs_obj[i] == obj)) {
	    for(j = i + 1; j < xlog_output_func_count; j++) {
		xlog_outputs_func[j - 1] = xlog_outputs_func[j];
		xlog_outputs_obj[j - 1] = xlog_outputs_obj[j];
	    }
	    xlog_output_func_count--;
	    return (0);
	}
    }
    
    return (-1);
}

/**
 * xlog_add_default_output:
 * @void: 
 * 
 * Add the default output stream to the set of output streams.
 * XXX: right now the default is '/dev/stderr', but it should eventually be:
 * `/dev/console' if the process has sufficient permissions,
 * and `/dev/stderr' otherwise.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_add_default_output(void)
{
    const char* defaults[] = {	/* The default outputs (in preference order) */
	"/dev/stderr",		/* XXX: temporary this is the default */
	"/dev/console",
	"/dev/stdout"
    };
    size_t ndefaults = sizeof(defaults) / sizeof(defaults[0]);
    size_t i;
    
    /*
     * Attempt to open default output stream, console first in case
     * we are root, then stderr.
     */
    for (i = 0; fp_default == NULL && i < ndefaults; i++) {
	if ((fp_default = fopen(defaults[i], "w")) != NULL) {
	    return (xlog_add_output(fp_default));
	}
    }
    return -1;
}

/**
 * xlog_remove_default_output:
 * @void: 
 * 
 * Remove the default output stream from the set of output streams.
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
xlog_remove_default_output(void)
{
    int r = 0;
    
    if (fp_default != NULL) {
	r = xlog_remove_output(fp_default);
	fclose (fp_default);
	fp_default = NULL;
    }
    
    return (r);
}

/**
 * xlog_localtime2string_short:
 * @void: 
 * 
 * Compute the current local time and return it as a string in the format:
 *   Year/Month/Day Hour:Minute:Second
 *   Example: 2002/02/05 20:22:09
 *   Note that the returned string uses statically allocated memory,
 *   and does not need to be de-allocated.
 * 
 * Return value: A statically allocated string with the local time using
 * the format described above.
 **/
static const char*
xlog_localtime2string_short(void)
{
    static char	  buf[64];
    static size_t buf_bytes = sizeof(buf) / sizeof(buf[0]);
    time_t	now = time(NULL);
    struct tm	*timeptr = localtime(&now);
    
    strftime(buf, buf_bytes, "%Y/%m/%d %H:%M:%S", timeptr);
    
    return (buf);
}

/**
 * xlog_localtime2string:
 * @void: 
 * 
 * Compute the current local time and return it as a string in the format:
 *   Year/Month/Day Hour:Minute:Second.Microsecond
 *   Example: 2002/02/05 20:22:09.808632
 *   Note that the returned string uses statically allocated memory,
 *   and does not need to be de-allocated.
 * 
 * Return value: A statically allocated string with the local time using
 * the format described above.
 **/
const char *
xlog_localtime2string(void)
{
    /* XXX: The code below may stop working after year 999999999 */
    char buf[sizeof("999999999/99/99 99/99/99.999999999 ")];
    static char ret_buf[sizeof(buf)];
    struct timeval tv;
    time_t clock;
    struct tm *tm;
    
    gettimeofday(&tv, NULL);
    clock = tv.tv_sec;
    tm = localtime(&clock);
    if (strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", tm) == 0) {
	sprintf(ret_buf, "strftime ERROR");
	return (ret_buf);
    }
    
    sprintf(ret_buf, "%s.%lu", buf, (unsigned long)tv.tv_usec);
    
    return (ret_buf);
}

/**
 * x_vasprintf:
 * @ret: A pointer to the string pointer to store the result.
 * @format: The printf(3)-style format.
 * @ap: The variable arguments for @format.
 * 
 * A local implementation of vasprintf(3).
 * Note that vasprintf(3) is not POSIX, so we don't attempt to
 * check whether it is available and use the system implementation
 * instead of our own below.
 * Part of the reasoning for not using it is technical:
 * on Linux vasprintf(3) is declared #ifdef __USE_GNU in <stdio.h>,
 * and it is not healthy to define __USE_GNU just to make the code
 * compile on Linux.
 * 
 * Return value: (From FreeBSD vasprintf(3) manual page).
 * The number of characters printed (not including the
 * trailing '\0' used to end output to strings). Also, set @*ret to be
 * a pointer to a buffer sufficiently large to hold the formatted string.
 * This pointer should be passed to free(3) to release the allocated
 * storage when it is no longer needed. If sufficient space cannot
 * be allocated, will return -1 and set @ret to be a NULL pointer.
 *
 * TODO: this function and x_asprintf() below probably should
 * be renamed to vasprintf() and asprintf() and go somewhere else,
 * something like "compat.c" .
 **/
int
x_vasprintf(char **ret, const char *format, va_list ap)
{
    size_t i, buf_size = 1024 + 1;
    char *buf_ptr = NULL;
    int ret_size;
    
    for (i = 0; i < 3; i++) {
	/*
	 * XXX: two iterations should be sufficient to compute the
	 * buffer size and allocate it, but anyway we try one more time
	 */
	buf_ptr = malloc(buf_size);
	if (buf_ptr == NULL)
	    break;		/* Cannot allocate memory */
	buf_ptr[0] = '\0';
	ret_size = vsnprintf(buf_ptr, buf_size, format, ap);
	if (ret_size < 0)
	    break;		/* Cannot format the string */
	if ((size_t)ret_size < buf_size) {
	    *ret = buf_ptr;
	    return (ret_size);
	}
	/* The allocated buffer was too small. Try again. */
	free(buf_ptr);
	buf_size = ret_size + 1; /* XXX: include space for trailing '\0' */
    }
    
    /*
     * Error: cannot format the string or cannot allocate enough memory
     */
    if (buf_ptr != NULL)
	free(buf_ptr);
    *ret = NULL;
    
    return (-1);
}


/**
 * x_asprintf:
 * @ret: A pointer to the string pointer to store the result.
 * @format: The printf(3)-style format.
 * @...: The variable arguments for @format.
 * 
 * A local implementation of asprintf(3) if it is missing.
 * If vasprintf(3) is available, it is called instead.
 * 
 * Return value: (From FreeBSD asprintf(3) manual page).
 * The number of characters printed (not including the
 * trailing '\0' used to end output to strings). Also, set @*ret to be
 * a pointer to a buffer sufficiently large to hold the formatted string.
 * This pointer should be passed to free(3) to release the allocated
 * storage when it is no longer needed. If sufficient space cannot
 * be allocated, will return -1 and set @ret to be a NULL pointer.
 **/
int
x_asprintf(char **ret, const char *format, ...)
{
    va_list ap;
    int ret_size;
    
    va_start(ap, format);
    ret_size = x_vasprintf(ret, format, ap);
    va_end(ap);
    
    return (ret_size);
}

