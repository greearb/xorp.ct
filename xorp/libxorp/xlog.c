/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2012 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */



/*
 * Message logging utility.
 */

#include "libxorp_module.h"

#include "libxorp/xorp.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

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
int		xlog_level_enabled[XLOG_LEVEL_MAX];
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
				"TRACE",
				/* XXX: temp, to be removed; see Bugzilla entry 795 */
				"RTRMGR_ONLY_NO_PREAMBLE",
			};


/*
 * Local functions prototypes
 */

static void	xlog_record_va(xlog_level_t log_level, const char* module_name,
			       const char *where, const char* format,
			       va_list ap);
static int	xlog_write(FILE* fp, const char* fmt, ...);
static int	xlog_write_va(FILE* fp, const char* fmt, va_list ap);
static int	xlog_flush(FILE* fp);

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
    xlog_level_t level;

    if (init_flag)
	return (-1);

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
    for (level = XLOG_LEVEL_MIN; level < XLOG_LEVEL_MAX; level++) {
	xlog_enable(level);
	xlog_verbose_level[level] = XLOG_VERBOSE_LOW;		/* Default */
    }
    xlog_verbose_level[XLOG_LEVEL_FATAL] = XLOG_VERBOSE_HIGH;	/* XXX */
    /* XXX: temp, to be removed; see Bugzilla entry 795 */
    xlog_verbose_level[XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE] =
	XLOG_VERBOSE_RTRMGR_ONLY_NO_PREAMBLE;

    init_flag = 1;

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
    xlog_level_t level;

    if (! init_flag)
	return (-1);

    if (start_flag)
	xlog_stop();

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

    for (level = XLOG_LEVEL_MIN; level < XLOG_LEVEL_MAX; level++) {
	xlog_disable(level);
	xlog_verbose_level[level] = XLOG_VERBOSE_LOW;		/* Default */
    }
    xlog_verbose_level[XLOG_LEVEL_FATAL] = XLOG_VERBOSE_HIGH;	/* XXX */
    /* XXX: temp, to be removed; see Bugzilla entry 795 */
    xlog_verbose_level[XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE] =
	XLOG_VERBOSE_RTRMGR_ONLY_NO_PREAMBLE;

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

int
xlog_is_running(void)
{
    return start_flag;
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
	/* XXX: temp, to be removed; see Bugzilla entry 795 */
	if (i == XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE)
	    continue;
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

	/* XXX: temp, to be removed; see Bugzilla entry 795 */
    if (log_level == XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE)
	return;

    if (XLOG_VERBOSE_HIGH < verbose_level)
	verbose_level = XLOG_VERBOSE_HIGH;

    xlog_verbose_level[log_level] = verbose_level;
}

void
_xlog_with_level(int log_level, const char *module_name, int line, const char *file,
		 const char *function, const char *fmt, ...)
{
    va_list ap;
    static char where_buf[8000]; // we are single threaded, global buffer is good enough.
    snprintf(where_buf, sizeof(where_buf), "%s:%d %s",
	     file, line, (function) ? function : "(unknown_func)");
    va_start(ap, fmt);
    xlog_record_va(log_level, module_name, where_buf, fmt, ap);
    va_end(ap);
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
#ifdef SIGPIPE
    sig_t	sigpipe_handler;
#endif

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

#ifdef SIGPIPE
    sigpipe_handler = signal(SIGPIPE, SIG_IGN);
#endif

    preamble_lead = (preamble_string) ? preamble_string : "";
    process_name_lead = (process_name_string) ? process_name_string : "";

    /* Special case for the pre-amble level */
    if (log_level == XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE) {
	x_asprintf(&buf_preamble_ptr, "");
    }
    else {
	/*
	 * Prepare the preamble string to write.
	 * XXX: we need to prepare it once, otherwise the time may be
	 * different when we write to more than one outputs.
	 */
	switch (xlog_verbose_level[log_level]) {
	case XLOG_VERBOSE_LOW:	/* The minimum log information */
	    x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s ] ",
		       xlog_localtime2string(),
		       xlog_level_names[log_level],
		       process_name_lead,
		       module_name);
	    break;

	case XLOG_VERBOSE_MEDIUM:	/* Add preamble string if non-NULL */
	    x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s %s ] ",
		       xlog_localtime2string(),
		       preamble_lead,
		       xlog_level_names[log_level],
		       process_name_lead,
		       module_name);
	    break;

	case XLOG_VERBOSE_HIGH:	/* Most verbose */
	default:
	    x_asprintf(&buf_preamble_ptr, "[ %s %s %s %s:%d %s %s ] ",
		       xlog_localtime2string(),
		       preamble_lead,
		       xlog_level_names[log_level],
		       process_name_lead,
		       (int)pid,
		       module_name,
		       where);
	    break;
	}
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
	if (func(obj, log_level, buf_output_ptr) < 0) {
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
#ifdef SIGPIPE
    signal(SIGPIPE, sigpipe_handler);
#endif
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
#ifdef HOST_OS_WINDOWS
    FILE *fp = NULL;
    HANDLE hstd = INVALID_HANDLE_VALUE;

    /*
     * XXX: We may need to duplicate these handles;
     * fclose() on _open_osfhandle() derived handles results
     * in the underlying handle being closed.
     */
    hstd = GetStdHandle(STD_ERROR_HANDLE);
    fp = _fdopen(_open_osfhandle((long)hstd, _O_TEXT), "w");
    if (fp != NULL)
	return (xlog_add_output(fp));

    fp = fopen("CONOUT$", "w");
    if (fp != NULL)
	return (xlog_add_output(fp));

    hstd = GetStdHandle(STD_OUTPUT_HANDLE),
    fp = _fdopen(_open_osfhandle((long)hstd, _O_TEXT), "w");
    if (fp != NULL)
	return (xlog_add_output(fp));

#else /* !HOST_OS_WINDOWS */

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
#endif /* HOST_OS_WINDOWS */

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
 * XXX: This function uses a BSD-specific function, gettimeofday().
 **/
const char *
xlog_localtime2string(void)
{
#ifdef HOST_OS_WINDOWS
    static char buf[64];
    SYSTEMTIME st;
    DWORD usecs;

    GetSystemTime(&st);
    usecs = st.wMilliseconds * 1000;
    snprintf(buf, sizeof(buf), "%u/%u/%u %u:%u:%u.%lu",
	     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
	     usecs);
    return (buf);
#else /* !HOST_OS_WINDOWS */
    static char ret_buf[64];
    struct timeval tv;
    time_t clock;
    struct tm *tm;

    gettimeofday(&tv, NULL);
    clock = tv.tv_sec;
    tm = localtime(&clock);
    time_t sofar = strftime(ret_buf, sizeof(ret_buf), "%Y/%m/%d %H:%M:%S", tm);
    if (sofar == 0) {
	snprintf(ret_buf, sizeof(ret_buf), "strftime ERROR");
	return (ret_buf);
    }

#ifdef XORP_LOG_PRINT_USECS
    snprintf(ret_buf + sofar, sizeof(ret_buf) - sofar, ".%lu", (unsigned long)tv.tv_usec);
#endif

    return (ret_buf);
#endif /* HOST_OS_WINDOWS */
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
#ifdef HAVE_VA_COPY
    va_list temp;
#endif

    for (i = 0; i < 3; i++) {
	/*
	 * XXX: two iterations should be sufficient to compute the
	 * buffer size and allocate it, but anyway we try one more time
	 */
	buf_ptr = malloc(buf_size);
	if (buf_ptr == NULL)
	    break;		/* Cannot allocate memory */
	buf_ptr[0] = '\0';
	/* XXX
	 * va_copy does not exist in older compilers like gcc
	 * 2.95. On some architectures like the i386 using a va_list
	 * argument multiple times doesn't seem to cause a problem. On
	 * the AMD 64 it is required.
	 */
#ifdef HAVE_VA_COPY
 	va_copy(temp, ap);
	ret_size = vsnprintf(buf_ptr, buf_size, format, temp);
#else
	ret_size = vsnprintf(buf_ptr, buf_size, format, ap);
#endif
	if (ret_size < 0)
	    break;		/* Cannot format the string */
	if ((size_t)ret_size < buf_size) {
	    *ret = buf_ptr;
	    return (ret_size);
	}
	/* The allocated buffer was too small. Try again. */
	free(buf_ptr);
	buf_ptr = NULL;
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

/*
 * ****************************************************************************
 * Syslog interface functions, conforming to X/Open.
 * ****************************************************************************
 */

#if defined(HAVE_SYSLOG_H) && defined(HAVE_SYSLOG)
typedef struct _code {
	const char	*c_name;
	int32_t		c_val;
} SYSLOG_CODE;

static SYSLOG_CODE prioritynames[] = {
	{ "alert",	LOG_ALERT,	},
	{ "crit",	LOG_CRIT,	},
	{ "debug",	LOG_DEBUG,	},
	{ "emerg",	LOG_EMERG,	},
	{ "err",	LOG_ERR,	},
	{ "info",	LOG_INFO,	},
	{ "notice",	LOG_NOTICE,	},
	{ "warning",	LOG_WARNING,	},
	{ NULL,		-1,		}
};

static SYSLOG_CODE facilitynames[] = {
	{ "auth",	LOG_AUTH,	},
	{ "cron", 	LOG_CRON,	},
	{ "daemon",	LOG_DAEMON,	},
	{ "kern",	LOG_KERN,	},
	{ "lpr",	LOG_LPR,	},
	{ "mail",	LOG_MAIL,	},
	{ "news",	LOG_NEWS,	},
	{ "user",	LOG_USER,	},
	{ "uucp",	LOG_UUCP,	},
	{ "local0",	LOG_LOCAL0,	},
	{ "local1",	LOG_LOCAL1,	},
	{ "local2",	LOG_LOCAL2,	},
	{ "local3",	LOG_LOCAL3,	},
	{ "local4",	LOG_LOCAL4,	},
	{ "local5",	LOG_LOCAL5,	},
	{ "local6",	LOG_LOCAL6,	},
	{ "local7",	LOG_LOCAL7,	},
	{ NULL,		-1,		}
};

static int32_t
xlog_level_to_syslog_priority(xlog_level_t xloglevel)
{

    switch (xloglevel) {
    case XLOG_LEVEL_FATAL:
	return (LOG_CRIT);
	break;
    case XLOG_LEVEL_ERROR:
	return (LOG_ERR);
	break;
    case XLOG_LEVEL_WARNING:
	return (LOG_WARNING);
	break;
    case XLOG_LEVEL_INFO:
	return (LOG_INFO);
	break;
    case XLOG_LEVEL_RTRMGR_ONLY_NO_PREAMBLE:
	/* XXX: temp, to be removed; see Bugzilla entry 795 */
	return (LOG_INFO);
	break;
    default:
	XLOG_UNREACHABLE();
    }
    return (-1);
}

static int
xlog_syslog_output_func(void *obj, xlog_level_t level, const char *msg)
{
    int32_t facility = (intptr_t)obj;
    int32_t priority = xlog_level_to_syslog_priority(level);

    syslog(facility|priority, "%s", msg);

    return (0);
}

/*
 * Parse <facility.priority>.
 */
static int
xlog_parse_syslog_spec(const char *syslogspec, int *facility, int *priority)
{
    int retval;
    int8_t i;
    int32_t xfacility, xpriority;
    char *facname, *priname, *tmpspec;
    SYSLOG_CODE* sc;

    retval = -1;
    facname = priname = tmpspec = NULL;

    tmpspec = strdup(syslogspec);
    if (tmpspec == NULL)
	return (retval);

    priname = strchr(tmpspec, '.');
    if (priname != NULL)
	*priname = '\0';

    facname = tmpspec;
    xfacility = -1;
    for (i = 0, sc = &facilitynames[0]; sc->c_val != -1;  ++sc, ++i) {
	if (0 == strcasecmp(sc->c_name, facname)) {
	    xfacility = i;
	    break;
	}
    }
    if (xfacility == -1)
	goto out;

    *facility = xfacility;

    if (priname != NULL && (*(++priname) != '\0')) {
	    xpriority = -1;
	    for (i = 0, sc = &prioritynames[0]; sc->c_val != -1; ++sc, ++i) {
		if (0 == strcasecmp(sc->c_name, priname)) {
		    xpriority = i;
		    break;
		}
	    }
	    if (xpriority == -1) {
		goto out;
	    }
	    *priority = xpriority;
    } else
	*priority = LOG_WARNING;

    retval = 0;
out:
    free(tmpspec);
    return (retval);
}

int
xlog_add_syslog_output(const char *syslogspec)
{
    int32_t facility = -1;
    int32_t priority = -1;

    if (-1 == xlog_parse_syslog_spec(syslogspec, &facility, &priority))
	return (-1);

    openlog("xorp", LOG_PID | LOG_NDELAY | LOG_CONS, facility);
    xlog_add_output_func(xlog_syslog_output_func, (void *)(intptr_t)facility);

    return (0);
}

#else /* !(HAVE_SYSLOG_H && HAVE_SYSLOG) */
int
xlog_add_syslog_output(const char *syslogspec)
{
    return (-1);
    UNUSED(syslogspec);
}
#endif /* HAVE_SYSLOG_H && HAVE_SYSLOG */
