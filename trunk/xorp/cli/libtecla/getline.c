/*
 * Copyright (c) 2000, 2001 by Martin C. Shepherd.
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

/*
 * Standard headers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <setjmp.h>

/*
 * UNIX headers.
 */
#include <sys/ioctl.h>
#ifdef HAVE_SELECT
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

/*
 * The source of terminal control string and size information.
 * Note that if no terminal information database is available,
 * ANSI VT100 control sequences are used.
 */
#if defined(USE_TERMINFO)
#include <curses.h>
#include <term.h>
#elif defined(USE_TERMCAP)
#if defined(HAVE_TERMCAP_H)
#include <termcap.h>
#else
extern int tgetent(char *, char *);
extern int tputs(char *, int, int (*)(char));
extern int tgetnum(char *);
extern char *tgetstr(char *, char **);
#endif
#endif

/*
 * Under Solaris default Curses the output function that tputs takes is
 * declared to have a char argument. On all other systems and on Solaris
 * X/Open Curses (Issue 4, Version 2) it expects an int argument (using
 * c89 or options -I /usr/xpg4/include -L /usr/xpg4/lib -R /usr/xpg4/lib
 * selects XPG4v2 Curses on Solaris 2.6 and later).
 */
#if defined USE_TERMINFO || defined USE_TERMCAP
#if defined __sun && defined __SVR4 && !defined _XOPEN_CURSES
typedef char TputsType;
#else
typedef int TputsType;
#endif
static int gl_tputs_putchar(TputsType c);
#endif

/*
 * POSIX headers.
 */
#include <unistd.h>
#include <termios.h>

/*
 * Does the system provide the signal and ioctl query facility used
 * to inform the process of terminal window size changes?
 */
#if defined(SIGWINCH) && defined(TIOCGWINSZ)
#define USE_SIGWINCH 1
#endif

/*
 * Provide typedefs for standard POSIX structures.
 */
typedef struct sigaction SigAction;
typedef struct termios Termios;

/*
 * Local headers.
 */
#include "pathutil.h"
#include "libtecla.h"
#include "keytab.h"
#include "history.h"
#include "freelist.h"
#include "stringrp.h"
#include "getline.h"

/*
 * Enumerate the available editing styles.
 */
typedef enum {
  GL_EMACS_MODE,   /* Emacs style editing */
  GL_VI_MODE,      /* Vi style editing */
  GL_NO_EDITOR     /* Fall back to the basic OS-provided editing */
} GlEditor;

/*
 * In vi mode, the following datatype is used to implement the
 * undo command. It records a copy of the input line from before
 * the command-mode action which edited the input line.
 */
typedef struct {
  char *line;        /* A historical copy of the input line */
  int buff_curpos;   /* The historical location of the cursor in */
                     /*  line[] when the line was modified. */
  int ntotal;        /* The number of characters in line[] */
  int saved;         /* True once a line has been saved after the */
                     /*  last call to gl_interpret_char(). */
} ViUndo;

/*
 * In vi mode, the following datatype is used to record information
 * needed by the vi-repeat-change command.
 */
typedef struct {
  KtKeyFn *fn;               /* The last action function that made a */
                             /*  change to the line. */
  int count;                 /* The repeat count that was passed to the */
                             /*  above command. */  
  int input_curpos;          /* Whenever vi command mode is entered, the */
                             /*  the position at which it was first left */
                             /*  is recorded here. */
  int command_curpos;        /* Whenever vi command mode is entered, the */
                             /*  the location of the cursor is recorded */
                             /*  here. */
  char input_char;           /* Commands that call gl_read_character() */
                             /*  record the character here, so that it can */
                             /*  used on repeating the function. */
  int saved;                 /* True if a function has been saved since the */
                             /*  last call to gl_interpret_char(). */
  int active;                /* True while a function is being repeated. */
} ViRepeat;

/*
 * The following datatype is used to encapsulate information specific
 * to vi mode.
 */
typedef struct {
  ViUndo undo;               /* Information needed to implement the vi */
                             /*  undo command. */
  ViRepeat repeat;           /* Information needed to implement the vi */
                             /*  repeat command. */
  int command;               /* True in vi command-mode */
  int find_forward;          /* True if the last character search was in the */
                             /*  forward direction. */
  int find_onto;             /* True if the last character search left the */
                             /*  on top of the located character, as opposed */
                             /*  to just before or after it. */
  char find_char;            /* The last character sought, or '\0' if no */
                             /*  searches have been performed yet. */
} ViMode;

#ifdef HAVE_SELECT
/*
 * Define a type for recording a file-descriptor callback and its associated
 * data.
 */
typedef struct {
  GlFdEventFn *fn;   /* The callback function */
  void *data;        /* Anonymous data to pass to the callback function */
} GlFdHandler;

/*
 * A list of nodes of the following type is used to record file-activity
 * event handlers, but only on systems that have the select() system call.
 */
typedef struct GlFdNode GlFdNode;
struct GlFdNode {
  GlFdNode *next;    /* The next in the list of nodes */
  int fd;            /* The file descriptor being watched */
  GlFdHandler rd;    /* The callback to call when fd is readable */
  GlFdHandler wr;    /* The callback to call when fd is writable */
  GlFdHandler ur;    /* The callback to call when fd has urgent data */
};

/*
 * Set the number of the above structures to allocate every time that
 * the freelist of GlFdNode's becomes exhausted.
 */
#define GLFD_FREELIST_BLOCKING 10

/*
 * Listen for and handle file-descriptor events.
 */
static int gl_event_handler(GetLine *gl);

static int gl_call_fd_handler(GetLine *gl, GlFdHandler *gfh, int fd,
			      GlFdEvent event);
#endif

/*
 * Each signal that gl_get_line() traps is described by a list node
 * of the following type.
 */
typedef struct GlSignalNode GlSignalNode;
struct GlSignalNode {
  GlSignalNode *next;  /* The next signal in the list */
  int signo;           /* The number of the signal */
  sigset_t proc_mask;  /* A process mask which only includes signo */
  SigAction original;  /* The signal disposition of the calling program */
                       /*  for this signal. */
  unsigned flags;      /* A bitwise union of GlSignalFlags enumerators */
  GlAfterSignal after; /* What to do after the signal has been handled */
  int errno_value;     /* What to set errno to */
};

/*
 * Set the number of the above structures to allocate every time that
 * the freelist of GlSignalNode's becomes exhausted.
 */
#define GLS_FREELIST_BLOCKING 30

/*
 * Set the largest key-sequence that can be handled.
 */
#define GL_KEY_MAX 64

/*
 * Define the contents of the GetLine object.
 * Note that the typedef for this object can be found in libtecla.h.
 */
struct GetLine {
  GlHistory *glh;            /* The line-history buffer */
  WordCompletion *cpl;       /* String completion resource object */
  CplMatchFn(*cpl_fn);       /* The tab completion callback function */
  void *cpl_data;            /* Callback data to pass to cpl_fn() */
  ExpandFile *ef;            /* ~user/, $envvar and wildcard expansion */
                             /*  resource object. */
  StringGroup *capmem;       /* Memory for recording terminal capability */
                             /*  strings. */
  int input_fd;              /* The file descriptor to read on */
  int output_fd;             /* The file descriptor to write to */
  FILE *input_fp;            /* A stream wrapper around input_fd */
  FILE *output_fp;           /* A stream wrapper around output_fd */
  FILE *file_fp;             /* When input is being temporarily taken from */
                             /*  a file, this is its file-pointer. Otherwise */
                             /*  it is NULL. */
  char *term;                /* The terminal type specified on the last call */
                             /*  to gl_change_terminal(). */
  int is_term;               /* True if stdin is a terminal */
  int is_net;                /* True if the in/out is a network connection */
  int net_may_block;         /* True if we may block if reading from the net */
  int net_read_attempt;      /* True if attempt to read from the net */
  char keyseq[GL_KEY_MAX+1]; /* A special key sequence being read */
  int  nkey;                 /* The number of characters in the key sequence */
  int user_event_value;      /* The user event value code */
  size_t linelen;            /* The max number of characters per line */
  char *line;                /* A line-input buffer of allocated size */
                             /*  linelen+2. The extra 2 characters are */
                             /*  reserved for "\n\0". */
  char *cutbuf;              /* A cut-buffer of the same size as line[] */
  const char *prompt;        /* The current prompt string */
  int prompt_len;            /* The length of the prompt string */
  int prompt_changed;        /* True after a callback changes the prompt */
  int prompt_style;          /* How the prompt string is displayed */
  FreeList *sig_mem;         /* Memory for nodes of the signal list */
  GlSignalNode *sigs;        /* The head of the list of signals */
  sigset_t old_signal_set;   /* The signal set on entry to gl_get_line() */
  sigset_t new_signal_set;   /* The set of signals that we are trapping */
  Termios oldattr;           /* Saved terminal attributes. */
  KeyTab *bindings;          /* A table of key-bindings */
  int ntotal;                /* The number of characters in gl->line[] */
  int buff_curpos;           /* The cursor position within gl->line[] */
  int term_curpos;           /* The cursor position on the terminal */
  int buff_mark;             /* A marker location in the buffer */
  int insert_curpos;         /* The cursor position at start of insert */
  int insert;                /* True in insert mode */ 
  int number;                /* If >= 0, a numeric argument is being read */
  int endline;               /* True to tell gl_get_input_line() to return */
                             /*  the current contents of gl->line[] */
  KtKeyFn *current_fn;       /* The action function that is currently being */
                             /*  invoked. */
  int current_count;         /* The repeat count passed to current_fn() */
  GlhLineID preload_id;      /* When not zero, this should be the ID of a */
                             /*  line in the history buffer for potential */
                             /*  recall. */
  int preload_history;       /* If true, preload the above history line when */
                             /*  gl_get_input_line() is next called. */
  long keyseq_count;         /* The number of key sequences entered by the */
                             /*  the user since new_GetLine() was called. */
  long last_search;          /* The value of oper_count during the last */
                             /*  history search operation. */
  GlEditor editor;           /* The style of editing, (eg. vi or emacs) */
  int silence_bell;          /* True if gl_ring_bell() should do nothing. */
  ViMode vi;                 /* Parameters used when editing in vi mode */
  const char *left;          /* The string that moves the cursor 1 character */
                             /*  left. */
  const char *right;         /* The string that moves the cursor 1 character */
                             /*  right. */
  const char *up;            /* The string that moves the cursor 1 character */
                             /*  up. */
  const char *down;          /* The string that moves the cursor 1 character */
                             /*  down. */
  const char *home;          /* The string that moves the cursor home */
  const char *bol;           /* Move cursor to beginning of line */
  const char *clear_eol;     /* The string that clears from the cursor to */
                             /*  the end of the line. */
  const char *clear_eod;     /* The string that clears from the cursor to */
                             /*  the end of the display. */
  const char *u_arrow;       /* The string returned by the up-arrow key */
  const char *d_arrow;       /* The string returned by the down-arrow key */
  const char *l_arrow;       /* The string returned by the left-arrow key */
  const char *r_arrow;       /* The string returned by the right-arrow key */
  const char *sound_bell;    /* The string needed to ring the terminal bell */
  const char *bold;          /* Switch to the bold font */
  const char *underline;     /* Underline subsequent characters */
  const char *standout;      /* Turn on standout mode */
  const char *dim;           /* Switch to a dim font */
  const char *reverse;       /* Turn on reverse video */
  const char *blink;         /* Switch to a blinking font */
  const char *text_attr_off; /* Turn off all text attributes */
  int nline;                 /* The height of the terminal in lines */
  int ncolumn;               /* The width of the terminal in columns */
#ifdef USE_TERMCAP
  char *tgetent_buf;         /* The buffer that is used by tgetent() to */
                             /*  store a terminal description. */
  char *tgetstr_buf;         /* The buffer that is used by tgetstr() to */
                             /*  store terminal capabilities. */
#endif
#ifdef USE_TERMINFO
  const char *left_n;        /* The parameter string that moves the cursor */
                             /*  n characters left. */
  const char *right_n;       /* The parameter string that moves the cursor */
                             /*  n characters right. */
#endif
  char *app_file;            /* The pathname of the application-specific */
                             /*  .teclarc configuration file, or NULL. */
  char *user_file;           /* The pathname of the user-specific */
                             /*  .teclarc configuration file, or NULL. */
  int configured;            /* True as soon as any teclarc configuration */
                             /*  file has been read. */
  int echo;                  /* True to display the line as it is being */
                             /*  entered. If 0, only the prompt will be */
                             /*  displayed, and the line will not be */
                             /*  archived in the history list. */
  int last_signal;           /* The last signal that was caught by */
                             /*  the last call to gl_get_line(), or -1 */
                             /*  if no signal has been caught yet. */
#ifdef HAVE_SELECT
  FreeList *fd_node_mem;     /* A freelist of GlFdNode structures */
  GlFdNode *fd_nodes;        /* The list of fd event descriptions */
  fd_set rfds;               /* The set of fds to watch for readability */
  fd_set wfds;               /* The set of fds to watch for writability */
  fd_set ufds;               /* The set of fds to watch for urgent data */
  int max_fd;                /* The maximum file-descriptor being watched */
#endif
};

/*
 * Define the max amount of space needed to store a termcap terminal
 * description. Unfortunately this has to be done by guesswork, so
 * there is the potential for buffer overflows if we guess too small.
 * Fortunately termcap has been replaced by terminfo on most
 * platforms, and with terminfo this isn't an issue. The value that I
 * am using here is the conventional value, as recommended by certain
 * web references.
 */
#ifdef USE_TERMCAP
#define TERMCAP_BUF_SIZE 2048
#endif

/*
 * Set the size of the string segments used to store terminal capability
 * strings.
 */
#define CAPMEM_SEGMENT_SIZE 512

/*
 * If no terminal size information is available, substitute the
 * following vt100 default sizes.
 */
#define GL_DEF_NLINE 24
#define GL_DEF_NCOLUMN 80

/*
 * List the signals that we need to catch. In general these are
 * those that by default terminate or suspend the process, since
 * in such cases we need to restore terminal settings.
 */
static const struct GlDefSignal {
  int signo;            /* The number of the signal */
  unsigned flags;       /* A bitwise union of GlSignalFlags enumerators */
  GlAfterSignal after;  /* What to do after the signal has been delivered */
  int errno_value;      /* What to set errno to */
} gl_signal_list[] = {
  {SIGABRT,   GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
  {SIGINT,    GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
  {SIGTERM,   GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
  {SIGALRM,   GLS_RESTORE_ENV, GLS_CONTINUE, 0},
  {SIGCONT,   GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#if defined(SIGHUP)
#ifdef ENOTTY
  {SIGHUP,    GLS_SUSPEND_INPUT, GLS_ABORT, ENOTTY},
#else
  {SIGHUP,    GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
#endif
#endif
#if defined(SIGPIPE)
#ifdef EPIPE
  {SIGPIPE,   GLS_SUSPEND_INPUT, GLS_ABORT, EPIPE},
#else
  {SIGPIPE,   GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
#endif
#endif
#ifdef SIGPWR
  {SIGPWR,    GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
#ifdef SIGQUIT
  {SIGQUIT,   GLS_SUSPEND_INPUT, GLS_ABORT, EINTR},
#endif
#ifdef SIGTSTP
  {SIGTSTP,   GLS_SUSPEND_INPUT, GLS_CONTINUE, 0},
#endif
#ifdef SIGTTIN
  {SIGTTIN,   GLS_SUSPEND_INPUT, GLS_CONTINUE, 0},
#endif
#ifdef SIGTTOU
  {SIGTTOU,   GLS_SUSPEND_INPUT, GLS_CONTINUE, 0},
#endif
#ifdef SIGUSR1
  {SIGUSR1,   GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
#ifdef SIGUSR2
  {SIGUSR2,   GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
#ifdef SIGVTALRM
  {SIGVTALRM, GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
#ifdef SIGWINCH
  {SIGWINCH,  GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
#ifdef SIGXCPU
  {SIGXCPU,   GLS_RESTORE_ENV, GLS_CONTINUE, 0},
#endif
};

/*
 * Define file-scope variables for use in signal handlers.
 */
static volatile sig_atomic_t gl_pending_signal = -1;
static sigjmp_buf gl_setjmp_buffer;

static void gl_signal_handler(int signo);

static int gl_check_caught_signal(GetLine *gl);

/*
 * Unfortunately both terminfo and termcap require one to use the tputs()
 * function to output terminal control characters, and this function
 * doesn't allow one to specify a file stream. As a result, the following
 * file-scope variable is used to pass the current output file stream.
 * This is bad, but there doesn't seem to be any alternative.
 */
#if defined(USE_TERMINFO) || defined(USE_TERMCAP)
static FILE *tputs_fp = NULL;
#endif

/*
 * Define a tab to be a string of 8 spaces.
 */
#define TAB_WIDTH 8

/*
 * Does the system send us SIGWINCH signals when the terminal size
 * changes?
 */
#ifdef USE_SIGWINCH
static int gl_resize_terminal(GetLine *gl, int redisplay);
#endif

/*
 * Getline calls this to temporarily override certain signal handlers
 * of the calling program.
 */
static int gl_override_signal_handlers(GetLine *gl);

/*
 * Getline calls this to restore the signal handlers of the calling
 * program.
 */
static int gl_restore_signal_handlers(GetLine *gl);

/*
 * Put the terminal into raw input mode, after saving the original
 * terminal attributes in gl->oldattr.
 */
static int gl_raw_terminal_mode(GetLine *gl);

/*
 * Restore the terminal attributes from gl->oldattr.
 */
static int gl_restore_terminal_attributes(GetLine *gl);

/*
 * Read a line from the user in raw mode.
 */
static int gl_get_input_line(GetLine *gl, const char *start_line,
			     int start_pos, int val);

/*
 * Handle the receipt of the potential start of a new key-sequence from
 * the user.
 */
static int gl_interpret_char(GetLine *gl, char c);

/*
 * Bind a single control or meta character to an action.
 */
static int gl_bind_control_char(GetLine *gl, KtBinder binder,
				char c, const char *action);

/*
 * Set up terminal-specific key bindings.
 */
static int gl_bind_terminal_keys(GetLine *gl);

/*
 * Lookup terminal control string and size information.
 */
static int gl_control_strings(GetLine *gl, const char *term);

/*
 * Wrappers around the terminfo and termcap functions that lookup
 * strings in the terminal information databases.
 */
#ifdef USE_TERMINFO
static const char *gl_tigetstr(GetLine *gl, const char *name);
#elif defined(USE_TERMCAP)
static const char *gl_tgetstr(GetLine *gl, const char *name, char **bufptr);
#endif

/*
 * Output a binary string directly to the terminal.
 */
static int gl_output_raw_string(GetLine *gl, const char *string);

/*
 * Output a terminal control sequence.
 */
static int gl_output_control_sequence(GetLine *gl, int nline,
				      const char *string);

/*
 * Output a character or string to the terminal after converting tabs
 * to spaces and control characters to a caret followed by the modified
 * character.
 */
static int gl_output_char(GetLine *gl, char c, char pad);
static int gl_output_string(GetLine *gl, const char *string, char pad);

/*
 * Delete nc characters starting from the one under the cursor.
 * Optionally copy the deleted characters to the cut buffer.
 */
static int gl_delete_chars(GetLine *gl, int nc, int cut);

/*
 * Add a character to the line buffer at the current cursor position,
 * inserting or overwriting according the current mode.
 */
static int gl_add_char_to_line(GetLine *gl, char c);

/*
 * Insert/append a string to the line buffer and terminal at the current
 * cursor position.
 */
static int gl_add_string_to_line(GetLine *gl, const char *s);

/*
 * Read a single character from the terminal.
 */
static int gl_read_character(GetLine *gl, char *c, int val);


/*
 * Move the terminal cursor n positions to the left or right.
 */
static int gl_terminal_move_cursor(GetLine *gl, int n);

/*
 * Move the terminal cursor to a given position.
 */
static int gl_set_term_curpos(GetLine *gl, int term_curpos);

/*
 * Set the position of the cursor both in the line input buffer and on the
 * terminal.
 */
/* static int gl_place_cursor(GetLine *gl, int buff_curpos); */

/*
 * Return the terminal cursor position that corresponds to a given
 * line buffer cursor position.
 */
static int gl_buff_curpos_to_term_curpos(GetLine *gl, int buff_curpos);

/*
 * Return the number of terminal characters needed to display a
 * given raw character.
 */
static int gl_displayed_char_width(GetLine *gl, char c, int term_curpos);

/*
 * Return the number of terminal characters needed to display a
 * given substring.
 */
static int gl_displayed_string_width(GetLine *gl, const char *string, int nc,
				     int term_curpos);
/*
 * Return non-zero if 'c' is to be considered part of a word.
 */
static int gl_is_word_char(int c);

/*
 * Read a tecla configuration file.
 */
static int _gl_read_config_file(GetLine *gl, const char *filename, KtBinder who);

/*
 * Read a tecla configuration string.
 */
static int _gl_read_config_string(GetLine *gl, const char *buffer, KtBinder who);

/*
 * Define the callback function used by _gl_parse_config_line() to
 * read the next character of a configuration stream.
 */
#define GLC_GETC_FN(fn) int (fn)(void *stream)
typedef GLC_GETC_FN(GlcGetcFn);

static GLC_GETC_FN(glc_file_getc);  /* Read from a file */
static GLC_GETC_FN(glc_buff_getc);  /* Read from a string */

/*
 * Parse a single configuration command line.
 */
static int _gl_parse_config_line(GetLine *gl, void *stream, GlcGetcFn *getc_fn,
				 const char *origin, KtBinder who, int *lineno);

/*
 * Bind the actual arrow key bindings to match those of the symbolic
 * arrow-key bindings.
 */
static int _gl_bind_arrow_keys(GetLine *gl);

/*
 * Copy the binding of the specified symbolic arrow-key binding to
 * the terminal specific, and default arrow-key key-sequences. 
 */
static int _gl_rebind_arrow_key(KeyTab *bindings, const char *name,
				const char *term_seq,
				const char *def_seq1,
				const char *def_seq2);

/*
 * After the gl_read_from_file() action has been used to tell gl_get_line()
 * to temporarily read input from a file, gl_revert_input() arranges
 * for input to be reverted to the input stream last registered with
 * gl_change_terminal().
 */
static void gl_revert_input(GetLine *gl);

/*
 * Flush unwritten characters to the terminal.
 */
static int gl_flush_output(GetLine *gl);

/*
 * Change the editor style being emulated.
 */
static int gl_change_editor(GetLine *gl, GlEditor editor);

/*
 * Searching in a given direction, return the index of a given (or
 * read) character in the input line, or the character that precedes
 * it in the specified search direction. Return -1 if not found.
 */
static int gl_find_char(GetLine *gl, int count, int forward, int onto, char c);

/*
 * Return the buffer index of the nth word ending after the cursor.
 */
static int gl_nth_word_end_forward(GetLine *gl, int n);

/*
 * Return the buffer index of the nth word start after the cursor.
 */
static int gl_nth_word_start_forward(GetLine *gl, int n);

/*
 * Return the buffer index of the nth word start before the cursor.
 */
static int gl_nth_word_start_backward(GetLine *gl, int n);

/*
 * When called when vi command mode is enabled, this function saves the
 * current line and cursor position for potential restoration later
 * by the vi undo command.
 */
static void gl_save_for_undo(GetLine *gl);

/*
 * If in vi mode, switch to vi command mode.
 */
static void gl_vi_command_mode(GetLine *gl);

/*
 * In vi mode this is used to delete up to or onto a given or read
 * character in the input line. Also switch to insert mode if requested
 * after the deletion.
 */
static int gl_delete_find(GetLine *gl, int count, char c, int forward,
			  int onto, int change);

/*
 * Copy the characters between the cursor and the count'th instance of
 * a specified (or read) character in the input line, into the cut buffer.
 */
static int gl_copy_find(GetLine *gl, int count, char c, int forward, int onto);

/*
 * Return the line index of the parenthesis that either matches the one under
 * the cursor, or not over a parenthesis character, the index of the next
 * close parenthesis. Return -1 if not found.
 */
static int gl_index_of_matching_paren(GetLine *gl);

/*
 * Replace a malloc'd string (or NULL), with another malloc'd copy of
 * a string (or NULL).
 */
static int gl_record_string(char **sptr, const char *string);

/*
 * Enumerate text display attributes as powers of two, suitable for
 * use in a bit-mask.
 */
typedef enum {
  GL_TXT_STANDOUT=1,   /* Display text highlighted */
  GL_TXT_UNDERLINE=2,  /* Display text underlined */
  GL_TXT_REVERSE=4,    /* Display text with reverse video */
  GL_TXT_BLINK=8,      /* Display blinking text */
  GL_TXT_DIM=16,       /* Display text in a dim font */
  GL_TXT_BOLD=32       /* Display text using a bold font */
} GlTextAttr;

/*
 * Display the prompt regardless of the current visibility mode.
 */
static int gl_display_prompt(GetLine *gl);

/*
 * Return the number of characters used by the prompt on the terminal.
 */
static int gl_displayed_prompt_width(GetLine *gl);

/*
 * Prepare the return the current input line to the caller of gl_get_line().
 */
static int gl_line_ended(GetLine *gl, int newline_char, int archive);

/*
 * Set the maximum length of a line in a user's tecla configuration
 * file (not counting comments).
 */
#define GL_CONF_BUFLEN 100

/*
 * Set the maximum number of arguments supported by individual commands
 * in tecla configuration files.
 */
#define GL_CONF_MAXARG 10

/*
 * Prototype the available action functions.
 */
static KT_KEY_FN(gl_user_interrupt);
static KT_KEY_FN(gl_abort);
static KT_KEY_FN(gl_suspend);
static KT_KEY_FN(gl_stop_output);
static KT_KEY_FN(gl_start_output);
static KT_KEY_FN(gl_literal_next);
static KT_KEY_FN(gl_cursor_left);
static KT_KEY_FN(gl_cursor_right);
static KT_KEY_FN(gl_insert_mode);
static KT_KEY_FN(gl_beginning_of_line);
static KT_KEY_FN(gl_end_of_line);
static KT_KEY_FN(gl_delete_line);
static KT_KEY_FN(gl_kill_line);
static KT_KEY_FN(gl_forward_word);
static KT_KEY_FN(gl_backward_word);
static KT_KEY_FN(gl_forward_delete_char);
static KT_KEY_FN(gl_backward_delete_char);
static KT_KEY_FN(gl_forward_delete_word);
static KT_KEY_FN(gl_backward_delete_word);
static KT_KEY_FN(gl_delete_refind);
static KT_KEY_FN(gl_delete_invert_refind);
static KT_KEY_FN(gl_delete_to_column);
static KT_KEY_FN(gl_delete_to_parenthesis);
static KT_KEY_FN(gl_forward_delete_find);
static KT_KEY_FN(gl_backward_delete_find);
static KT_KEY_FN(gl_forward_delete_to);
static KT_KEY_FN(gl_backward_delete_to);
static KT_KEY_FN(gl_upcase_word);
static KT_KEY_FN(gl_downcase_word);
static KT_KEY_FN(gl_capitalize_word);
static KT_KEY_FN(gl_redisplay);
static KT_KEY_FN(gl_clear_screen);
static KT_KEY_FN(gl_transpose_chars);
static KT_KEY_FN(gl_set_mark);
static KT_KEY_FN(gl_exchange_point_and_mark);
static KT_KEY_FN(gl_kill_region);
static KT_KEY_FN(gl_copy_region_as_kill);
static KT_KEY_FN(gl_yank);
static KT_KEY_FN(gl_up_history);
static KT_KEY_FN(gl_down_history);
static KT_KEY_FN(gl_history_search_backward);
static KT_KEY_FN(gl_history_re_search_backward);
static KT_KEY_FN(gl_history_search_forward);
static KT_KEY_FN(gl_history_re_search_forward);
static KT_KEY_FN(gl_complete_word);
static KT_KEY_FN(gl_expand_filename);
static KT_KEY_FN(gl_del_char_or_list_or_eof);
static KT_KEY_FN(gl_list_or_eof);
static KT_KEY_FN(gl_read_from_file);
static KT_KEY_FN(gl_beginning_of_history);
static KT_KEY_FN(gl_end_of_history);
static KT_KEY_FN(gl_digit_argument);
static KT_KEY_FN(gl_newline);
static KT_KEY_FN(gl_repeat_history);
static KT_KEY_FN(gl_vi_insert);
static KT_KEY_FN(gl_vi_overwrite);
static KT_KEY_FN(gl_change_case);
static KT_KEY_FN(gl_vi_insert_at_bol);
static KT_KEY_FN(gl_vi_append_at_eol);
static KT_KEY_FN(gl_vi_append);
static KT_KEY_FN(gl_list_glob);
static KT_KEY_FN(gl_backward_kill_line);
static KT_KEY_FN(gl_goto_column);
static KT_KEY_FN(gl_forward_to_word);
static KT_KEY_FN(gl_vi_replace_char);
static KT_KEY_FN(gl_vi_change_rest_of_line);
static KT_KEY_FN(gl_vi_change_line);
static KT_KEY_FN(gl_vi_change_to_bol);
static KT_KEY_FN(gl_vi_change_refind);
static KT_KEY_FN(gl_vi_change_invert_refind);
static KT_KEY_FN(gl_vi_change_to_column);
static KT_KEY_FN(gl_vi_change_to_parenthesis);
static KT_KEY_FN(gl_vi_forward_change_word);
static KT_KEY_FN(gl_vi_backward_change_word);
static KT_KEY_FN(gl_vi_forward_change_find);
static KT_KEY_FN(gl_vi_backward_change_find);
static KT_KEY_FN(gl_vi_forward_change_to);
static KT_KEY_FN(gl_vi_backward_change_to);
static KT_KEY_FN(gl_vi_forward_change_char);
static KT_KEY_FN(gl_vi_backward_change_char);
static KT_KEY_FN(gl_forward_copy_char);
static KT_KEY_FN(gl_backward_copy_char);
static KT_KEY_FN(gl_forward_find_char);
static KT_KEY_FN(gl_backward_find_char);
static KT_KEY_FN(gl_forward_to_char);
static KT_KEY_FN(gl_backward_to_char);
static KT_KEY_FN(gl_repeat_find_char);
static KT_KEY_FN(gl_invert_refind_char);
static KT_KEY_FN(gl_append_yank);
static KT_KEY_FN(gl_backward_copy_word);
static KT_KEY_FN(gl_forward_copy_word);
static KT_KEY_FN(gl_copy_to_bol);
static KT_KEY_FN(gl_copy_refind);
static KT_KEY_FN(gl_copy_invert_refind);
static KT_KEY_FN(gl_copy_to_column);
static KT_KEY_FN(gl_copy_to_parenthesis);
static KT_KEY_FN(gl_copy_rest_of_line);
static KT_KEY_FN(gl_copy_line);
static KT_KEY_FN(gl_backward_copy_find);
static KT_KEY_FN(gl_forward_copy_find);
static KT_KEY_FN(gl_backward_copy_to);
static KT_KEY_FN(gl_forward_copy_to);
static KT_KEY_FN(gl_vi_undo);
static KT_KEY_FN(gl_emacs_editing_mode);
static KT_KEY_FN(gl_vi_editing_mode);
static KT_KEY_FN(gl_ring_bell);
static KT_KEY_FN(gl_vi_repeat_change);
static KT_KEY_FN(gl_find_parenthesis);
static KT_KEY_FN(gl_read_init_files);
static KT_KEY_FN(gl_list_history);
static KT_KEY_FN(gl_user_event1);
static KT_KEY_FN(gl_user_event2);
static KT_KEY_FN(gl_user_event3);
static KT_KEY_FN(gl_user_event4);

/*
 * Name the available action functions.
 */
static const struct {const char *name; KT_KEY_FN(*fn);} gl_actions[] = {
  {"user-interrupt",             gl_user_interrupt},
  {"abort",                      gl_abort},
  {"suspend",                    gl_suspend},
  {"stop-output",                gl_stop_output},
  {"start-output",               gl_start_output},
  {"literal-next",               gl_literal_next},
  {"cursor-right",               gl_cursor_right},
  {"cursor-left",                gl_cursor_left},
  {"insert-mode",                gl_insert_mode},
  {"beginning-of-line",          gl_beginning_of_line},
  {"end-of-line",                gl_end_of_line},
  {"delete-line",                gl_delete_line},
  {"kill-line",                  gl_kill_line},
  {"forward-word",               gl_forward_word},
  {"backward-word",              gl_backward_word},
  {"forward-delete-char",        gl_forward_delete_char},
  {"backward-delete-char",       gl_backward_delete_char},
  {"forward-delete-word",        gl_forward_delete_word},
  {"backward-delete-word",       gl_backward_delete_word},
  {"delete-refind",              gl_delete_refind},
  {"delete-invert-refind",       gl_delete_invert_refind},
  {"delete-to-column",           gl_delete_to_column},
  {"delete-to-parenthesis",      gl_delete_to_parenthesis},
  {"forward-delete-find",        gl_forward_delete_find},
  {"backward-delete-find",       gl_backward_delete_find},
  {"forward-delete-to",          gl_forward_delete_to},
  {"backward-delete-to",         gl_backward_delete_to},
  {"upcase-word",                gl_upcase_word},
  {"downcase-word",              gl_downcase_word},
  {"capitalize-word",            gl_capitalize_word},
  {"redisplay",                  gl_redisplay},
  {"clear-screen",               gl_clear_screen},
  {"transpose-chars",            gl_transpose_chars},
  {"set-mark",                   gl_set_mark},
  {"exchange-point-and-mark",    gl_exchange_point_and_mark},
  {"kill-region",                gl_kill_region},
  {"copy-region-as-kill",        gl_copy_region_as_kill},
  {"yank",                       gl_yank},
  {"up-history",                 gl_up_history},
  {"down-history",               gl_down_history},
  {"history-search-backward",    gl_history_search_backward},
  {"history-re-search-backward", gl_history_re_search_backward},
  {"history-search-forward",     gl_history_search_forward},
  {"history-re-search-forward",  gl_history_re_search_forward},
  {"complete-word",              gl_complete_word},
  {"expand-filename",            gl_expand_filename},
  {"del-char-or-list-or-eof",    gl_del_char_or_list_or_eof},
  {"read-from-file",             gl_read_from_file},
  {"beginning-of-history",       gl_beginning_of_history},
  {"end-of-history",             gl_end_of_history},
  {"digit-argument",             gl_digit_argument},
  {"newline",                    gl_newline},
  {"repeat-history",             gl_repeat_history},
  {"vi-insert",                  gl_vi_insert},
  {"vi-overwrite",               gl_vi_overwrite},
  {"vi-insert-at-bol",           gl_vi_insert_at_bol},
  {"vi-append-at-eol",           gl_vi_append_at_eol},
  {"vi-append",                  gl_vi_append},
  {"change-case",                gl_change_case},
  {"list-glob",                  gl_list_glob},
  {"backward-kill-line",         gl_backward_kill_line},
  {"goto-column",                gl_goto_column},
  {"forward-to-word",            gl_forward_to_word},
  {"vi-replace-char",            gl_vi_replace_char},
  {"vi-change-rest-of-line",     gl_vi_change_rest_of_line},
  {"vi-change-line",             gl_vi_change_line},
  {"vi-change-to-bol",           gl_vi_change_to_bol},
  {"vi-change-refind",           gl_vi_change_refind},
  {"vi-change-invert-refind",    gl_vi_change_invert_refind},
  {"vi-change-to-column",        gl_vi_change_to_column},
  {"vi-change-to-parenthesis",   gl_vi_change_to_parenthesis},
  {"forward-copy-char",          gl_forward_copy_char},
  {"backward-copy-char",         gl_backward_copy_char},
  {"forward-find-char",          gl_forward_find_char},
  {"backward-find-char",         gl_backward_find_char},
  {"forward-to-char",            gl_forward_to_char},
  {"backward-to-char",           gl_backward_to_char},
  {"repeat-find-char",           gl_repeat_find_char},
  {"invert-refind-char",         gl_invert_refind_char},
  {"append-yank",                gl_append_yank},
  {"backward-copy-word",         gl_backward_copy_word},
  {"forward-copy-word",          gl_forward_copy_word},
  {"copy-to-bol",                gl_copy_to_bol},
  {"copy-refind",                gl_copy_refind},
  {"copy-invert-refind",         gl_copy_invert_refind},
  {"copy-to-column",             gl_copy_to_column},
  {"copy-to-parenthesis",        gl_copy_to_parenthesis},
  {"copy-rest-of-line",          gl_copy_rest_of_line},
  {"copy-line",                  gl_copy_line},
  {"backward-copy-find",         gl_backward_copy_find},
  {"forward-copy-find",          gl_forward_copy_find},
  {"backward-copy-to",           gl_backward_copy_to},
  {"forward-copy-to",            gl_forward_copy_to},
  {"list-or-eof",                gl_list_or_eof},
  {"vi-undo",                    gl_vi_undo},
  {"vi-backward-change-word",    gl_vi_backward_change_word},
  {"vi-forward-change-word",     gl_vi_forward_change_word},
  {"vi-backward-change-find",    gl_vi_backward_change_find},
  {"vi-forward-change-find",     gl_vi_forward_change_find},
  {"vi-backward-change-to",      gl_vi_backward_change_to},
  {"vi-forward-change-to",       gl_vi_forward_change_to},
  {"vi-backward-change-char",    gl_vi_backward_change_char},
  {"vi-forward-change-char",     gl_vi_forward_change_char},
  {"emacs-mode",                 gl_emacs_editing_mode},
  {"vi-mode",                    gl_vi_editing_mode},
  {"ring-bell",                  gl_ring_bell},
  {"vi-repeat-change",           gl_vi_repeat_change},
  {"find-parenthesis",           gl_find_parenthesis},
  {"read-init-files",            gl_read_init_files},
  {"list-history",               gl_list_history},
  {"user-event1",                gl_user_event1},
  {"user-event2",                gl_user_event2},
  {"user-event3",                gl_user_event3},
  {"user-event4",                gl_user_event4},
};

/*
 * Define the default key-bindings in emacs mode.
 */
static const KtKeyBinding gl_emacs_bindings[] = {
  {"right",        "cursor-right"},
  {"^F",           "cursor-right"},
  {"left",         "cursor-left"},
  {"^B",           "cursor-left"},
  {"M-i",          "insert-mode"},
  {"M-I",          "insert-mode"},
  {"^A",           "beginning-of-line"},
  {"^E",           "end-of-line"},
  {"^U",           "delete-line"},
  {"^K",           "kill-line"},
  {"M-f",          "forward-word"},
  {"M-F",          "forward-word"},
  {"M-b",          "backward-word"},
  {"M-B",          "backward-word"},
  {"^D",           "del-char-or-list-or-eof"},
  {"^H",           "backward-delete-char"},
  {"^?",           "backward-delete-char"},
  {"M-d",          "forward-delete-word"},
  {"M-D",          "forward-delete-word"},
  {"M-^H",         "backward-delete-word"},
  {"M-^?",         "backward-delete-word"},
  {"M-u",          "upcase-word"},
  {"M-U",          "upcase-word"},
  {"M-l",          "downcase-word"},
  {"M-L",          "downcase-word"},
  {"M-c",          "capitalize-word"},
  {"M-C",          "capitalize-word"},
  {"^R",           "redisplay"},
  {"^L",           "clear-screen"},
  {"^T",           "transpose-chars"},
  {"^@",           "set-mark"},
  {"^X^X",         "exchange-point-and-mark"},
  {"^W",           "kill-region"},
  {"M-w",          "copy-region-as-kill"},
  {"M-W",          "copy-region-as-kill"},
  {"^Y",           "yank"},
  {"^P",           "up-history"},
  {"up",           "up-history"},
  {"^N",           "down-history"},
  {"down",         "down-history"},
  {"M-p",          "history-search-backward"},
  {"M-P",          "history-search-backward"},
  {"M-n",          "history-search-forward"},
  {"M-N",          "history-search-forward"},
  {"\t",           "complete-word"},
  {"^X*",          "expand-filename"},
  {"^X^F",         "read-from-file"},
  {"^X^R",         "read-init-files"},
  {"^Xg",          "list-glob"},
  {"^XG",          "list-glob"},
  {"^Xh",          "list-history"},
  {"^XH",          "list-history"},
  {"M-<",          "beginning-of-history"},
  {"M->",          "end-of-history"},
  {"M-0",          "digit-argument"},
  {"M-1",          "digit-argument"},
  {"M-2",          "digit-argument"},
  {"M-3",          "digit-argument"},
  {"M-4",          "digit-argument"},
  {"M-5",          "digit-argument"},
  {"M-6",          "digit-argument"},
  {"M-7",          "digit-argument"},
  {"M-8",          "digit-argument"},
  {"M-9",          "digit-argument"},
  {"\r",           "newline"},
  {"\n",           "newline"},
  {"M-o",          "repeat-history"},
  {"M-C-v",        "vi-mode"},
};

/*
 * Define the default key-bindings in vi mode. Note that in vi-mode
 * meta-key bindings are command-mode bindings. For example M-i first
 * switches to command mode if not already in that mode, then moves
 * the cursor one position right, as in vi.
 */
static const KtKeyBinding gl_vi_bindings[] = {
  {"^D",           "list-or-eof"},
  {"^G",           "list-glob"},
  {"^H",           "backward-delete-char"},
  {"\t",           "complete-word"},
  {"\r",           "newline"},
  {"\n",           "newline"},
  {"^L",           "clear-screen"},
  {"^N",           "down-history"},
  {"^P",           "up-history"},
  {"^R",           "redisplay"},
  {"^U",           "backward-kill-line"},
  {"^W",           "backward-delete-word"},
  {"^X^F",         "read-from-file"},
  {"^X^R",         "read-init-files"},
  {"^X*",          "expand-filename"},
  {"^?",           "backward-delete-char"},
  {"M- ",          "cursor-right"},
  {"M-$",          "end-of-line"},
  {"M-*",          "expand-filename"},
  {"M-+",          "down-history"},
  {"M--",          "up-history"},
  {"M-<",          "beginning-of-history"},
  {"M->",          "end-of-history"},
  {"M-^",          "beginning-of-line"},
  {"M-;",          "repeat-find-char"},
  {"M-,",          "invert-refind-char"},
  {"M-|",          "goto-column"},
  {"M-~",          "change-case"},
  {"M-.",          "vi-repeat-change"},
  {"M-%",          "find-parenthesis"},
  {"M-0",          "digit-argument"},
  {"M-1",          "digit-argument"},
  {"M-2",          "digit-argument"},
  {"M-3",          "digit-argument"},
  {"M-4",          "digit-argument"},
  {"M-5",          "digit-argument"},
  {"M-6",          "digit-argument"},
  {"M-7",          "digit-argument"},
  {"M-8",          "digit-argument"},
  {"M-9",          "digit-argument"},
  {"M-a",          "vi-append"},
  {"M-A",          "vi-append-at-eol"},
  {"M-b",          "backward-word"},
  {"M-B",          "backward-word"},
  {"M-C",          "vi-change-rest-of-line"},
  {"M-cb",         "vi-backward-change-word"},
  {"M-cB",         "vi-backward-change-word"},
  {"M-cc",         "vi-change-line"},
  {"M-ce",         "vi-forward-change-word"},
  {"M-cE",         "vi-forward-change-word"},
  {"M-cw",         "vi-forward-change-word"},
  {"M-cW",         "vi-forward-change-word"},
  {"M-cF",         "vi-backward-change-find"},
  {"M-cf",         "vi-forward-change-find"},
  {"M-cT",         "vi-backward-change-to"},
  {"M-ct",         "vi-forward-change-to"},
  {"M-c;",         "vi-change-refind"},
  {"M-c,",         "vi-change-invert-refind"},
  {"M-ch",         "vi-backward-change-char"},
  {"M-c^H",        "vi-backward-change-char"},
  {"M-c^?",        "vi-backward-change-char"},
  {"M-cl",         "vi-forward-change-char"},
  {"M-c ",         "vi-forward-change-char"},
  {"M-c^",         "vi-change-to-bol"},
  {"M-c0",         "vi-change-to-bol"},
  {"M-c$",         "vi-change-rest-of-line"},
  {"M-c|",         "vi-change-to-column"},
  {"M-c%",         "vi-change-to-parenthesis"},
  {"M-dh",         "backward-delete-char"},
  {"M-d^H",        "backward-delete-char"},
  {"M-d^?",        "backward-delete-char"},
  {"M-dl",         "forward-delete-char"},
  {"M-d ",         "forward-delete-char"},
  {"M-dd",         "delete-line"},
  {"M-db",         "backward-delete-word"},
  {"M-dB",         "backward-delete-word"},
  {"M-de",         "forward-delete-word"},
  {"M-dE",         "forward-delete-word"},
  {"M-dw",         "forward-delete-word"},
  {"M-dW",         "forward-delete-word"},
  {"M-dF",         "backward-delete-find"},
  {"M-df",         "forward-delete-find"},
  {"M-dT",         "backward-delete-to"},
  {"M-dt",         "forward-delete-to"},
  {"M-d;",         "delete-refind"},
  {"M-d,",         "delete-invert-refind"},
  {"M-d^",         "backward-kill-line"},
  {"M-d0",         "backward-kill-line"},
  {"M-d$",         "kill-line"},
  {"M-D",          "kill-line"},
  {"M-d|",         "delete-to-column"},
  {"M-d%",         "delete-to-parenthesis"},
  {"M-e",          "forward-word"},
  {"M-E",          "forward-word"},
  {"M-f",          "forward-find-char"},
  {"M-F",          "backward-find-char"},
  {"M--",          "up-history"},
  {"M-h",          "cursor-left"},
  {"M-H",          "beginning-of-history"},
  {"M-i",          "vi-insert"},
  {"M-I",          "vi-insert-at-bol"},
  {"M-j",          "down-history"},
  {"M-J",          "history-search-forward"},
  {"M-k",          "up-history"},
  {"M-K",          "history-search-backward"},
  {"M-l",          "cursor-right"},
  {"M-L",          "end-of-history"},
  {"M-n",          "history-re-search-forward"},
  {"M-N",          "history-re-search-backward"},
  {"M-p",          "append-yank"},
  {"M-P",          "yank"},
  {"M-r",          "vi-replace-char"},
  {"M-R",          "vi-overwrite"},
  {"M-s",          "vi-forward-change-char"},
  {"M-S",          "vi-change-line"},
  {"M-t",          "forward-to-char"},
  {"M-T",          "backward-to-char"},
  {"M-u",          "vi-undo"},
  {"M-w",          "forward-to-word"},
  {"M-W",          "forward-to-word"},
  {"M-x",          "forward-delete-char"},
  {"M-X",          "backward-delete-char"},
  {"M-yh",         "backward-copy-char"},
  {"M-y^H",        "backward-copy-char"},
  {"M-y^?",        "backward-copy-char"},
  {"M-yl",         "forward-copy-char"},
  {"M-y ",         "forward-copy-char"},
  {"M-ye",         "forward-copy-word"},
  {"M-yE",         "forward-copy-word"},
  {"M-yw",         "forward-copy-word"},
  {"M-yW",         "forward-copy-word"},
  {"M-yb",         "backward-copy-word"},
  {"M-yB",         "backward-copy-word"},
  {"M-yf",         "forward-copy-find"},
  {"M-yF",         "backward-copy-find"},
  {"M-yt",         "forward-copy-to"},
  {"M-yT",         "backward-copy-to"},
  {"M-y;",         "copy-refind"},
  {"M-y,",         "copy-invert-refind"},
  {"M-y^",         "copy-to-bol"},
  {"M-y0",         "copy-to-bol"},
  {"M-y$",         "copy-rest-of-line"},
  {"M-yy",         "copy-line"},
  {"M-Y",          "copy-line"},
  {"M-y|",         "copy-to-column"},
  {"M-y%",         "copy-to-parenthesis"},
  {"M-^E",         "emacs-mode"},
  {"M-^H",         "cursor-left"},
  {"M-^?",         "cursor-left"},
  {"M-^L",         "clear-screen"},
  {"M-^N",         "down-history"},
  {"M-^P",         "up-history"},
  {"M-^R",         "redisplay"},
  {"M-^D",         "list-or-eof"},
  {"M-\r",         "newline"},
  {"M-\t",         "complete-word"},
  {"M-\n",         "newline"},
  {"M-^X^R",       "read-init-files"},
  {"M-^Xh",        "list-history"},
  {"M-^XH",        "list-history"},
  {"down",         "down-history"},
  {"up",           "up-history"},
  {"left",         "cursor-left"},
  {"right",        "cursor-right"},
};

/*.......................................................................
 * Create a new GetLine object.
 *
 * Input:
 *  linelen  size_t    The maximum line length to allow for.
 *  histlen  size_t    The number of bytes to allocate for recording
 *                     a circular buffer of history lines.
 * Output:
 *  return  GetLine *  The new object, or NULL on error.
 */
GetLine *new_GetLine(size_t linelen, size_t histlen)
{
  GetLine *gl;  /* The object to be returned */
  int i;
/*
 * Check the arguments.
 */
  if(linelen < 10) {
    fprintf(stderr, "new_GetLine: Line length too small.\n");
    return NULL;
  };
/*
 * Allocate the container.
 */
  gl = (GetLine *) malloc(sizeof(GetLine));
  if(!gl) {
    fprintf(stderr, "new_GetLine: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be passed
 * to del_GetLine().
 */
  gl->glh = NULL;
  gl->cpl = NULL;
  gl->cpl_fn = cpl_file_completions;
  gl->cpl_data = NULL;
  gl->ef = NULL;
  gl->capmem = NULL;
  gl->term = NULL;
  gl->is_term = 0;
  gl->is_net = 0;
  gl->net_may_block = 0;
  gl->net_read_attempt = 0;
  gl->nkey = 0;
  gl->user_event_value = 0;
  gl->input_fd = -1;
  gl->output_fd = -1;
  gl->input_fp = NULL;
  gl->output_fp = NULL;
  gl->file_fp = NULL;
  gl->linelen = linelen;
  gl->line = NULL;
  gl->cutbuf = NULL;
  gl->linelen = linelen;
  gl->prompt = "";
  gl->prompt_len = 0;
  gl->prompt_changed = 0;
  gl->prompt_style = GL_LITERAL_PROMPT;
  gl->vi.undo.line = NULL;
  gl->vi.undo.buff_curpos = 0;
  gl->vi.undo.ntotal = 0;
  gl->vi.undo.saved = 0;
  gl->vi.repeat.fn = 0;
  gl->vi.repeat.count = 0;
  gl->vi.repeat.input_curpos = 0;
  gl->vi.repeat.command_curpos = 0;
  gl->vi.repeat.input_char = '\0';
  gl->vi.repeat.saved = 0;
  gl->vi.repeat.active = 0;
  gl->sig_mem = NULL;
  gl->sigs = NULL;
  sigemptyset(&gl->old_signal_set);
  sigemptyset(&gl->new_signal_set);
  gl->bindings = NULL;
  gl->ntotal = 0;
  gl->buff_curpos = 0;
  gl->term_curpos = 0;
  gl->buff_mark = 0;
  gl->insert_curpos = 0;
  gl->insert = 1;
  gl->number = -1;
  gl->endline = 0;
  gl->current_fn = 0;
  gl->current_count = 0;
  gl->preload_id = 0;
  gl->preload_history = 0;
  gl->keyseq_count = 0;
  gl->last_search = -1;
  gl->editor = GL_EMACS_MODE;
  gl->silence_bell = 0;
  gl->vi.command = 0;
  gl->vi.find_forward = 0;
  gl->vi.find_onto = 0;
  gl->vi.find_char = '\0';
  gl->left = NULL;
  gl->right = NULL;
  gl->up = NULL;
  gl->down = NULL;
  gl->home = NULL;
  gl->bol = 0;
  gl->clear_eol = NULL;
  gl->clear_eod = NULL;
  gl->u_arrow = NULL;
  gl->d_arrow = NULL;
  gl->l_arrow = NULL;
  gl->r_arrow = NULL;
  gl->sound_bell = NULL;
  gl->bold = NULL;
  gl->underline = NULL;
  gl->standout = NULL;
  gl->dim = NULL;
  gl->reverse = NULL;
  gl->blink = NULL;
  gl->text_attr_off = NULL;
  gl->nline = 0;
  gl->ncolumn = 0;
#ifdef USE_TERMINFO
  gl->left_n = NULL;
  gl->right_n = NULL;
#elif defined(USE_TERMCAP)
  gl->tgetent_buf = NULL;
  gl->tgetstr_buf = NULL;
#endif
  gl->app_file = NULL;
  gl->user_file = NULL;
  gl->configured = 0;
  gl->echo = 1;
  gl->last_signal = -1;
#ifdef HAVE_SELECT
  gl->fd_node_mem = NULL;
  gl->fd_nodes = NULL;
  FD_ZERO(&gl->rfds);
  FD_ZERO(&gl->wfds);
  FD_ZERO(&gl->ufds);
  gl->max_fd = 0;
#endif
/*
 * Allocate the history buffer.
 */
  gl->glh = _new_GlHistory(histlen);
  if(!gl->glh)
    return del_GetLine(gl);
/*
 * Allocate the resource object for file-completion.
 */
  gl->cpl = new_WordCompletion();
  if(!gl->cpl)
    return del_GetLine(gl);
/*
 * Allocate the resource object for file-completion.
 */
  gl->ef = new_ExpandFile();
  if(!gl->ef)
    return del_GetLine(gl);
/*
 * Allocate a string-segment memory allocator for use in storing terminal
 * capablity strings.
 */
  gl->capmem = _new_StringGroup(CAPMEM_SEGMENT_SIZE);
  if(!gl->capmem)
    return del_GetLine(gl);
/*
 * Allocate a line buffer, leaving 2 extra characters for the terminating
 * '\n' and '\0' characters
 */
  gl->line = (char *) malloc(linelen + 2);
  if(!gl->line) {
    fprintf(stderr,
	    "new_GetLine: Insufficient memory to allocate line buffer.\n");
    return del_GetLine(gl);
  };
  gl->line[0] = '\0';
/*
 * Allocate a cut buffer.
 */
  gl->cutbuf = (char *) malloc(linelen + 2);
  if(!gl->cutbuf) {
    fprintf(stderr,
	    "new_GetLine: Insufficient memory to allocate cut buffer.\n");
    return del_GetLine(gl);
  };
  gl->cutbuf[0] = '\0';
/*
 * Allocate a vi undo buffer.
 */
  gl->vi.undo.line = (char *) malloc(linelen + 2);
  if(!gl->vi.undo.line) {
    fprintf(stderr,
	    "new_GetLine: Insufficient memory to allocate undo buffer.\n");
    return del_GetLine(gl);
  };
  gl->vi.undo.line[0] = '\0';
/*
 * Allocate a freelist from which to allocate nodes for the list
 * of signals.
 */
  gl->sig_mem = _new_FreeList("new_GetLine", sizeof(GlSignalNode),
			      GLS_FREELIST_BLOCKING);
  if(!gl->sig_mem)
    return del_GetLine(gl);
/*
 * Install dispositions for the default list of signals that gl_get_line()
 * traps.
 */
  for(i=0; i<sizeof(gl_signal_list)/sizeof(gl_signal_list[0]); i++) {
    const struct GlDefSignal *sig = gl_signal_list + i;
    if(gl_trap_signal(gl, sig->signo, sig->flags, sig->after,
		       sig->errno_value))
      return del_GetLine(gl);
  };
/*
 * Allocate an empty table of key bindings.
 */
  gl->bindings = _new_KeyTab();
  if(!gl->bindings)
    return del_GetLine(gl);
/*
 * Define the available actions that can be bound to key sequences.
 */
  for(i=0; i<sizeof(gl_actions)/sizeof(gl_actions[0]); i++) {
    if(_kt_set_action(gl->bindings, gl_actions[i].name, gl_actions[i].fn))
      return del_GetLine(gl);
  };
/*
 * Set up the default bindings.
 */
  if(gl_change_editor(gl, gl->editor))
    return del_GetLine(gl);
/*
 * Allocate termcap buffers.
 */
#ifdef USE_TERMCAP
  gl->tgetent_buf = (char *) malloc(TERMCAP_BUF_SIZE);
  gl->tgetstr_buf = (char *) malloc(TERMCAP_BUF_SIZE);
  if(!gl->tgetent_buf || !gl->tgetstr_buf) {
    fprintf(stderr, "new_GetLine: Insufficient memory for termcap buffers.\n");
    return del_GetLine(gl);
  };
#endif
/*
 * Set up for I/O assuming stdin and stdout.
 */
  if(gl_change_terminal(gl, stdin, stdout, getenv("TERM")))
    return del_GetLine(gl);
/*
 * Create a freelist for use in allocating GlFdNode list nodes.
 */
#ifdef HAVE_SELECT
  gl->fd_node_mem = _new_FreeList("new_GetLine", sizeof(GlFdNode),
				  GLFD_FREELIST_BLOCKING);
  if(!gl->fd_node_mem)
    return del_GetLine(gl);
#endif
/*
 * We are done for now.
 */
  return gl;
}

/*.......................................................................
 * Delete a GetLine object.
 *
 * Input:
 *  gl     GetLine *  The object to be deleted.
 * Output:
 *  return GetLine *  The deleted object (always NULL).
 */
GetLine *del_GetLine(GetLine *gl)
{
  if(gl) {
    gl->glh = _del_GlHistory(gl->glh);
    gl->cpl = del_WordCompletion(gl->cpl);
    gl->ef = del_ExpandFile(gl->ef);
    gl->capmem = _del_StringGroup(gl->capmem);
    if(gl->line)
      free(gl->line);
    if(gl->cutbuf)
      free(gl->cutbuf);
    if(gl->vi.undo.line)
      free(gl->vi.undo.line);
    gl->sig_mem = _del_FreeList(NULL, gl->sig_mem, 1);
    gl->sigs = NULL;       /* Already freed by freeing sig_mem */
    gl->bindings = _del_KeyTab(gl->bindings);
#ifdef USE_TERMCAP
    if(gl->tgetent_buf)
      free(gl->tgetent_buf);
    if(gl->tgetstr_buf)
      free(gl->tgetstr_buf);
#endif
    if(gl->file_fp)
      fclose(gl->file_fp);
    if(gl->term)
      free(gl->term);
#ifdef HAVE_SELECT
    gl->fd_node_mem = _del_FreeList(NULL, gl->fd_node_mem, 1);
#endif
    free(gl);
  };
  return NULL;
}

/*.......................................................................
 * Bind a control or meta character to an action.
 *
 * Input:
 *  gl         GetLine *  The resource object of this program.
 *  binder    KtBinder    The source of the binding.
 *  c             char    The control or meta character.
 *                        If this is '\0', the call is ignored.
 *  action  const char *  The action name to bind the key to.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int gl_bind_control_char(GetLine *gl, KtBinder binder, char c,
				const char *action)
{
  char keyseq[2];
/*
 * Quietly reject binding to the NUL control character, since this
 * is an ambiguous prefix of all bindings.
 */
  if(c == '\0')
    return 0;
/*
 * Making sure not to bind characters which aren't either control or
 * meta characters.
 */
  if(IS_CTRL_CHAR(c) || IS_META_CHAR(c)) {
    keyseq[0] = c;
    keyseq[1] = '\0';
  } else {
    return 0;
  };
/*
 * Install the binding.
 */
  return _kt_set_keybinding(gl->bindings, binder, keyseq, action);
}

/*.......................................................................
 * Read a line from the user.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 *  prompt      char *  The prompt to prefix the line with.
 *  start_line  char *  The initial contents of the input line, or NULL
 *                      if it should start out empty.
 *  start_pos    int    If start_line isn't NULL, this specifies the
 *                      index of the character over which the cursor
 *                      should initially be positioned within the line.
 *                      If you just want it to follow the last character
 *                      of the line, send -1.
 * Output:
 *  return      char *  An internal buffer containing the input line, or
 *                      NULL at the end of input. If the line fitted in
 *                      the buffer there will be a '\n' newline character
 *                      before the terminating '\0'. If it was truncated
 *                      there will be no newline character, and the remains
 *                      of the line should be retrieved via further calls
 *                      to this function.
 */
char *gl_get_line(GetLine *gl, const char *prompt,
		  const char *start_line, int start_pos)
{
  int waserr = 0;    /* True if an error occurs */
  gl->is_net = 0;    /* Reset the 'is_net' flag */
  gl->net_may_block = 0;
  gl->net_read_attempt = 0;
  gl->user_event_value = 0;
/*
 * Check the arguments.
 */
  if(!gl || !prompt) {
    fprintf(stderr, "gl_get_line: NULL argument(s).\n");
    return NULL;
  };
/*
 * If this is the first call to this function since new_GetLine(),
 * complete any postponed configuration.
 */
  if(!gl->configured) {
    (void) gl_configure_getline(gl, NULL, NULL, TECLA_CONFIG_FILE);
    gl->configured = 1;
  };
/*
 * If input is temporarily being taken from a file, return lines
 * from the file until the file is exhausted, then revert to
 * the normal input stream.
 */
  if(gl->file_fp) {
    if(fgets(gl->line, gl->linelen, gl->file_fp))
      return gl->line;
    gl_revert_input(gl);
  };
/*
 * Is input coming from a non-interactive source?
 */
  if(!gl->is_term)
    return fgets(gl->line, gl->linelen, gl->input_fp);
/*
 * Record the new prompt and its displayed width.
 */
  gl_replace_prompt(gl, prompt);
/*
 * Before installing our signal handler functions, record the fact
 * that there are no pending signals.
 */
  gl_pending_signal = -1;
/*
 * Temporarily override the signal handlers of the calling program,
 * so that we can intercept signals that would leave the terminal
 * in a bad state.
 */
  waserr = gl_override_signal_handlers(gl);
/*
 * After recording the current terminal settings, switch the terminal
 * into raw input mode.
 */
  waserr = waserr || gl_raw_terminal_mode(gl);
/*
 * Attempt to read the line.
 */
  waserr = waserr || gl_get_input_line(gl, start_line, start_pos, -1);
/*
 * Restore terminal settings.
 */
  gl_restore_terminal_attributes(gl);
/*
 * Restore the signal handlers.
 */
  gl_restore_signal_handlers(gl);
/*
 * Having restored the program terminal and signal environment,
 * re-submit any signals that were received.
 */
  if(gl_pending_signal != -1) {
    raise(gl_pending_signal);
    waserr = 1;
  };
/*
 * If gl_get_input_line() aborted input due to the user asking to
 * temporarily read lines from a file, read the first line from
 * this file.
 */
  if(!waserr && gl->file_fp)
    return gl_get_line(gl, prompt, NULL, 0);
/*
 * Return the new input line.
 */
  return waserr ? NULL : gl->line;
}

/*.......................................................................
 * Record of the signal handlers of the calling program, so that they
 * can be restored later.
 *
 * Input:
 *  gl    GetLine *   The resource object of this library.
 * Output:
 *  return    int     0 - OK.
 *                    1 - Error.
 */
static int gl_override_signal_handlers(GetLine *gl)
{
  GlSignalNode *sig;   /* A node in the list of signals to be caught */
/*
 * Set up our signal handler.
 */
  SigAction act;
  act.sa_handler = gl_signal_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
/*
 * Get the process signal mask so that we can see which signals the
 * calling program currently has blocked, and so that we can restore this
 * mask before returning to the calling program.
 */
  if(sigprocmask(SIG_SETMASK, NULL, &gl->old_signal_set) == -1) {
    fprintf(stderr, "gl_get_line(): sigprocmask error: %s\n", strerror(errno));
    return 1;
  };
/*
 * Form a new process signal mask from the list of signals that we have
 * been asked to trap.
 */
  sigemptyset(&gl->new_signal_set);
  for(sig=gl->sigs; sig; sig=sig->next) {
/*
 * Trap this signal? If it is blocked by the calling program and we
 * haven't been told to unblock it, don't arrange to trap this signal.
 */
    if(sig->flags & GLS_UNBLOCK_SIG ||
       !sigismember(&gl->old_signal_set, sig->signo)) {
      if(sigaddset(&gl->new_signal_set, sig->signo) == -1) {
	fprintf(stderr, "gl_get_line(): sigaddset error: %s\n",
		strerror(errno));
	return 1;
      };
    };
  };
/*
 * Before installing our signal handlers, block all of the signals
 * that we are going to be trapping.
 */
  if(sigprocmask(SIG_BLOCK, &gl->new_signal_set, NULL) == -1) {
    fprintf(stderr, "gl_get_line(): sigprocmask error: %s\n", strerror(errno));
    return 1;
  };
/*
 * Override the actions of the signals that we are trapping.
 */
  for(sig=gl->sigs; sig; sig=sig->next) {
    if(sigismember(&gl->new_signal_set, sig->signo) &&
       sigaction(sig->signo, &act, &sig->original)) {
      fprintf(stderr, "gl_get_line(): sigaction error: %s\n", strerror(errno));
      return 1;
    };
  };
/*
 * Just in case a SIGWINCH signal was sent to the process while our
 * SIGWINCH signal handler wasn't in place, check to see if the terminal
 * size needs updating.
 */
#ifdef USE_SIGWINCH
  if (gl->is_term) {
    if(gl_resize_terminal(gl, 0))
      return 1;
  }
#endif
  return 0;
}

/*.......................................................................
 * Restore the signal handlers of the calling program.
 *
 * Input:
 *  gl     GetLine *  The resource object of this library.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int gl_restore_signal_handlers(GetLine *gl)
{
  GlSignalNode *sig;   /* A node in the list of signals to be caught */
/*
 * Restore application signal handlers that were overriden
 * by gl_override_signal_handlers().
 */
  for(sig=gl->sigs; sig; sig=sig->next) {
    if(sigismember(&gl->new_signal_set, sig->signo) &&
       sigaction(sig->signo, &sig->original, NULL)) {
      fprintf(stderr, "gl_get_line(): sigaction error: %s\n", strerror(errno));
      return 1;
    };
  };
/*
 * Restore the original signal mask.
 */
  if(sigprocmask(SIG_SETMASK, &gl->old_signal_set, NULL) == -1) {
    fprintf(stderr, "gl_get_line(): sigprocmask error: %s\n", strerror(errno));
    return 1;
  };
  return 0;
}

/*.......................................................................
 * This signal handler simply records the fact that a given signal was
 * caught in the file-scope gl_pending_signal variable.
 */
static void gl_signal_handler(int signo)
{
  gl_pending_signal = signo;
  siglongjmp(gl_setjmp_buffer, 1);
}

/*.......................................................................
 * Switch the terminal into raw mode after storing the previous terminal
 * settings in gl->attributes.
 *
 * Input:
 *  gl     GetLine *   The resource object of this program.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int gl_raw_terminal_mode(GetLine *gl)
{
  Termios newattr;   /* The new terminal attributes */
/*
 * Record the current terminal attributes.
 */
  if(tcgetattr(gl->input_fd, &gl->oldattr)) {
    fprintf(stderr, "getline(): tcgetattr error: %s\n", strerror(errno));
    return 1;
  };
/*
 * This function shouldn't do anything but record the current terminal
 * attritubes if editing has been disabled.
 */
  if(gl->editor == GL_NO_EDITOR)
    return 0;
/*
 * Modify the existing attributes.
 */
  newattr = gl->oldattr;
/*
 * Turn off local echo, canonical input mode and extended input processing.
 */
  newattr.c_lflag &= ~(ECHO | ICANON | IEXTEN);
/*
 * Don't translate carriage return to newline, turn off input parity
 * checking, don't strip off 8th bit, turn off output flow control.
 */
  newattr.c_iflag &= ~(ICRNL | INPCK | ISTRIP);
/*
 * Clear size bits, turn off parity checking, and allow 8-bit characters.
 */
  newattr.c_cflag &= ~(CSIZE | PARENB);
  newattr.c_cflag |= CS8;
/*
 * Turn off output processing.
 */
  newattr.c_oflag &= ~(OPOST);
/*
 * Request one byte at a time, without waiting.
 */
  newattr.c_cc[VMIN] = 1;
  newattr.c_cc[VTIME] = 0;
/*
 * Install the new terminal modes.
 */
  while(tcsetattr(gl->input_fd, TCSADRAIN, &newattr)) {
    if (errno != EINTR) {
      fprintf(stderr, "getline(): tcsetattr error: %s\n", strerror(errno));
      return 1;
    };
  };
  return 0;
}

/*.......................................................................
 * Restore the terminal attributes recorded in gl->oldattr.
 *
 * Input:
 *  gl     GetLine *   The resource object of this library.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int gl_restore_terminal_attributes(GetLine *gl)
{
  int waserr = 0;
/*
 * Before changing the terminal attributes, make sure that all output
 * has been passed to the terminal.
 */
  if(gl_flush_output(gl))
    waserr = 1;
/*
 * Reset the terminal attributes to the values that they had on
 * entry to gl_get_line().
 */
  while(tcsetattr(gl->input_fd, TCSADRAIN, &gl->oldattr)) {
    if(errno != EINTR) {
      fprintf(stderr, "gl_get_line(): tcsetattr error: %s\n", strerror(errno));
      waserr = 1;
      break;
    };
  };
  return waserr;
}

/*.......................................................................
 * Read a new input line from the user.
 *
 * Input:
 *  gl         GetLine *  The resource object of this library.
 *  start_line    char *  The initial contents of the input line, or NULL
 *                        if it should start out empty.
 *  start_pos      int    If start_line isn't NULL, this specifies the
 *                        index of the character over which the cursor
 *                        should initially be positioned within the line.
 *                        If you just want it to follow the last character
 *                        of the line, send -1.
 *  val          int      If non-negative, the single-character value already
 *                        read from the network.
 * Output:
 *  return    int    0 - OK.
 *                   1 - Error.
 */
static int gl_get_input_line(GetLine *gl, const char *start_line,
			     int start_pos, int val)
{
  char c;       /* The character being read */
  int is_endline = 0;
  int is_redraw = 0;
/*
 * Reset the properties of the line.
 */
  if (gl->endline) {
    gl->ntotal = 0;
    gl->buff_curpos = 0;
    gl->term_curpos = 0;
    gl->insert_curpos = 0;
    gl->number = -1;
    gl->endline = 0;
    gl->vi.command = 0;
    gl->vi.undo.line[0] = '\0';
    gl->vi.undo.ntotal = 0;
    gl->vi.undo.buff_curpos = 0;
    gl->vi.repeat.fn = 0;
    gl->last_signal = -1;
    is_endline = 1;
  } else {
    if (! gl->is_net)
      gl->term_curpos = 0;
  }
/*
 * Reset the history search pointers.
 */
  if (is_endline) {
    if(_glh_cancel_search(gl->glh))
      return 1;
  }
/*
 * Draw the prompt at the start of the line.
 */
  if (is_endline || ! gl->term_curpos) {
    if(gl_display_prompt(gl))
      return 1;
    is_redraw = 1;
  }
/*
 * Present an initial line?
 */
  if(start_line) {
    char *cptr;      /* A pointer into gl->line[] */
/*
 * Load the line into the buffer, and display it.
 */
    if(start_line != gl->line)
      strncpy(gl->line, start_line, gl->linelen);
    gl->line[gl->linelen] = '\0';
    gl->ntotal = strlen(gl->line);
/*
 * Strip off any trailing newline and carriage return characters.
 */
    for(cptr=gl->line + gl->ntotal - 1; cptr >= gl->line &&
	(*cptr=='\n' || *cptr=='\r'); cptr--,gl->ntotal--)
      ;
    if(gl->ntotal < 0)
      gl->ntotal = 0;
    gl->line[gl->ntotal] = '\0';
/*
 * Display the string that remains.
 */
    if (is_redraw) {
      if(gl_output_string(gl, gl->line, '\0'))
        return 1;
    }
/*
 * Where should the cursor be placed within the line?
 */
    if(start_pos < 0 || start_pos > gl->ntotal) {
      if(gl_place_cursor(gl, gl->ntotal))
	return 1;
    } else {
      if(gl_place_cursor(gl, start_pos))
	return 1;
    };
  } else {
    gl->line[0] = '\0';
  };
/*
 * Preload a history line?
 */
  if(gl->preload_history) {
    gl->preload_history = 0;
    if(gl->preload_id) {
      if(_glh_recall_line(gl->glh, gl->preload_id, gl->line, gl->linelen)) {
	gl->ntotal = strlen(gl->line);
	gl->buff_curpos = strlen(gl->line);
      };
      gl->preload_id = 0;
    }; 
    gl_redisplay(gl, 1);
  };
/*
 * Read one character at a time.
 */
  while(gl_read_character(gl, &c, val) == 0) {
/*
 * Increment the count of the number of key sequences entered.
 */
    gl->keyseq_count++;
/*
 * Interpret the character either as the start of a new key-sequence,
 * as a continuation of a repeat count, or as a printable character
 * to be added to the line.
 */
    if(gl_interpret_char(gl, c))
      break;
/*
 * If we just ran an action function which temporarily asked for
 * input to be taken from a file, abort this call.
 */
    if(gl->file_fp)
      return 0;
/*
 * Has the line been completed?
 */
    if(gl->endline)
      return gl_line_ended(gl, isprint((int)(unsigned char) c) ? c : '\n',
			   gl->echo && (c=='\n' || c=='\r'));
    if (gl->is_net)
      return 0;  /* For network connection we read a character at a time */
  };
/*
 * To get here, gl_read_character() must have returned non-zero. See
 * if this was because a signal was caught that requested that the
 * current line be returned.
 */
  if(gl->endline)
    return gl_line_ended(gl, '\n', gl->echo);
  return 1;
}

/*.......................................................................
 * Add a character to the line buffer at the current cursor position,
 * inserting or overwriting according the current mode.
 *
 * Input:
 *  gl   GetLine *   The resource object of this library.
 *  c       char     The character to be added.
 * Output:
 *  return   int     0 - OK.
 *                   1 - Insufficient room.
 */
static int gl_add_char_to_line(GetLine *gl, char c)
{
/*
 * Keep a record of the current cursor position.
 */
  int buff_curpos = gl->buff_curpos;
  int term_curpos = gl->term_curpos;
/*
 * Work out the displayed width of the new character.
 */
  int width = gl_displayed_char_width(gl, c, term_curpos);
/*
 * If we are in insert mode, or at the end of the line,
 * check that we can accomodate a new character in the buffer.
 * If not, simply return, leaving it up to the calling program
 * to check for the absence of a newline character.
 */
  if((gl->insert || buff_curpos >= gl->ntotal) && gl->ntotal >= gl->linelen)
    return 0;
/*
 * Are we adding characters to the line (ie. inserting or appending)?
 */
  if(gl->insert || buff_curpos >= gl->ntotal) {
/*
 * If inserting, make room for the new character.
 */
    if(buff_curpos < gl->ntotal) {
      memmove(gl->line + buff_curpos + 1, gl->line + buff_curpos,
	      gl->ntotal - buff_curpos);
    };
/*
 * Copy the character into the buffer.
 */
    gl->line[buff_curpos] = c;
    gl->buff_curpos++;
/*
 * If the line was extended, update the record of the string length
 * and terminate the extended string.
 */
    gl->ntotal++;
    gl->line[gl->ntotal] = '\0';
/*
 * Redraw the line from the cursor position to the end of the line,
 * and move the cursor to just after the added character.
 */
    if(gl_output_string(gl, gl->line + buff_curpos, '\0') ||
       gl_set_term_curpos(gl, term_curpos + width))
      return 1;
/*
 * Are we overwriting an existing character?
 */
  } else {
/*
 * Get the widths of the character to be overwritten and the character
 * that is going to replace it.
 */
    int old_width = gl_displayed_char_width(gl, gl->line[buff_curpos],
					    term_curpos);
/*
 * Overwrite the character in the buffer.
 */
    gl->line[buff_curpos] = c;
/*
 * If we are replacing with a narrower character, we need to
 * redraw the terminal string to the end of the line, then
 * overwrite the trailing old_width - width characters
 * with spaces.
 */
    if(old_width > width) {
      if(gl_output_string(gl, gl->line + buff_curpos, '\0'))
	return 1;
/*
 * Clear to the end of the terminal.
 */
      if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
	return 1;
/*
 * Move the cursor to the end of the new character.
 */
      if(gl_set_term_curpos(gl, term_curpos + width))
	return 1;
      gl->buff_curpos++;
/*
 * If we are replacing with a wider character, then we will be
 * inserting new characters, and thus extending the line.
 */
    } else if(width > old_width) {
/*
 * Redraw the line from the cursor position to the end of the line,
 * and move the cursor to just after the added character.
 */
      if(gl_output_string(gl, gl->line + buff_curpos, '\0') ||
	 gl_set_term_curpos(gl, term_curpos + width))
	return 1;
      gl->buff_curpos++;
/*
 * The original and replacement characters have the same width,
 * so simply overwrite.
 */
    } else {
/*
 * Copy the character into the buffer.
 */
      gl->line[buff_curpos] = c;
      gl->buff_curpos++;
/*
 * Overwrite the original character.
 */
      if(gl_output_char(gl, c, gl->line[gl->buff_curpos]))
	return 1;
    };
  };
  return 0;
}

/*.......................................................................
 * Insert/append a string to the line buffer and terminal at the current
 * cursor position.
 *
 * Input:
 *  gl   GetLine *   The resource object of this library.
 *  s       char *   The string to be added.
 * Output:
 *  return   int     0 - OK.
 *                   1 - Insufficient room.
 */
static int gl_add_string_to_line(GetLine *gl, const char *s)
{
  int buff_slen;   /* The length of the string being added to line[] */
  int term_slen;   /* The length of the string being written to the terminal */
  int buff_curpos; /* The original value of gl->buff_curpos */
  int term_curpos; /* The original value of gl->term_curpos */
/*
 * Keep a record of the current cursor position.
 */
  buff_curpos = gl->buff_curpos;
  term_curpos = gl->term_curpos;
/*
 * How long is the string to be added?
 */
  buff_slen = strlen(s);
  term_slen = gl_displayed_string_width(gl, s, buff_slen, term_curpos);
/*
 * Check that we can accomodate the string in the buffer.
 * If not, simply return, leaving it up to the calling program
 * to check for the absence of a newline character.
 */
  if(gl->ntotal + buff_slen > gl->linelen)
    return 0;
/*
 * Move the characters that follow the cursor in the buffer by
 * buff_slen characters to the right.
 */
  if(gl->ntotal > gl->buff_curpos) {
    memmove(gl->line + gl->buff_curpos + buff_slen, gl->line + gl->buff_curpos,
	    gl->ntotal - gl->buff_curpos);
  };
/*
 * Copy the string into the buffer.
 */
  memcpy(gl->line + gl->buff_curpos, s, buff_slen);
  gl->ntotal += buff_slen;
  gl->buff_curpos += buff_slen;
/*
 * Maintain the buffer properly terminated.
 */
  gl->line[gl->ntotal] = '\0';
/*
 * Write the modified part of the line to the terminal, then move
 * the terminal cursor to the end of the displayed input string.
 */
  if(gl_output_string(gl, gl->line + buff_curpos, '\0') ||
     gl_set_term_curpos(gl, term_curpos + term_slen))
    return 1;
  return 0;
}

/*.......................................................................
 * Read a single character from the terminal.
 *
 * Input:
 *  gl    GetLine *   The resource object of this library.
 *  val       int     If non-negative, the single-character value already
 *                    read from the network.
 * Output:
 *  return    int     0 - OK.
 *                    1 - Either an I/O error occurred, or a signal was
 *                        caught who's disposition is to abort gl_get_line()
 *                        or to have gl_get_line() return the current line
 *                        as though the user had pressed return. In the
 *                        latter case gl->endline will be non-zero.
 */
static int gl_read_character(GetLine *gl, char *c, int val)
{
/*
 * We must take special care between blocking calls.
 */
  if (gl->is_net) {
    if (gl->net_read_attempt + 1 > 0)
      gl->net_read_attempt++;
    if (gl->net_read_attempt > 1)
      return 1;      /* XXX: don't read from the net more than once in a row */
  }
  if (gl->net_may_block)
    return 1;
/*
 * Before waiting for a new character to be input, flush unwritten
 * characters to the terminal.
 */
  if(gl_flush_output(gl))
    return 1;
/*
 * We may have to repeat the read if window change signals are received.
 */
  for(;;) {
/*
 * If the endline flag becomes set, don't wait for another character.
 */
    if(gl->endline)
      return 1;
/*
 * If we have given the pre-read value, just set it and return.
 */
    if (val >= 0) {
      *c = (char)val;
      return 0;
    }
/*
 * Since the code in this function can block, trap signals.
 */
    if(sigsetjmp(gl_setjmp_buffer, 1)==0) {
/*
 * Unblock the signals that we are trapping.
 */
      if(sigprocmask(SIG_UNBLOCK, &gl->new_signal_set, NULL) == -1) {
	fprintf(stderr, "getline(): sigprocmask error: %s\n", strerror(errno));
	return 1;
      };
/*
 * If select() is available, watch for activity on any file descriptors
 * that the user has registered, and for data available on the terminal
 * file descriptor.
 */
#ifdef HAVE_SELECT
      if(gl_event_handler(gl))
	return 1;
#endif
/*
 * Read one character from the terminal. This could take more
 * than one call if an interrupt that we aren't trapping is
 * received.
 */
      while(read(gl->input_fd, (void *)c, 1) != 1) {
	if(errno != EINTR) {
#ifdef EAGAIN
	  if(!errno)          /* This can happen with SysV O_NDELAY */
	    errno = EAGAIN;
	  if ((errno == EAGAIN) && gl->is_net) {
	      gl->net_may_block = 1;
	      return 1;	     /* No more characters to read from the net */
	  }
#elif defined(EWOULDBLOCK)
	  if ((errno == EWOULDBLOCK) && gl->is_net) {
	      gl->net_may_block = 1;
	      return 1;	     /* No more characters to read from the net */
	  }
#endif
	  return 1;
	};
      };
/*
 * Block all of the signals that we are trapping.
 */
      if(sigprocmask(SIG_BLOCK, &gl->new_signal_set, NULL) == -1) {
	fprintf(stderr, "getline(): sigprocmask error: %s\n", strerror(errno));
	return 1;
      };
      return 0;
    };
/*
 * To get here, one of the signals that we are trapping must have
 * been received. Note that by using sigsetjmp() instead of setjmp()
 * the signal mask that was blocking these signals will have been
 * reinstated, so we can be sure that no more of these signals will
 * be received until we explicitly unblock them again.
 */
    if(gl_check_caught_signal(gl))
      return 1;
  };
}

/*.......................................................................
 * This function is called to handle signals caught between calls to
 * sigsetjmp() and siglongjmp().
 *
 * Input:
 *  gl      GetLine *   The resource object of this library.
 * Output:
 *  return      int     0 - Signal handled internally.
 *                      1 - Signal requires gl_get_line() to abort.
 */
static int gl_check_caught_signal(GetLine *gl)
{
  GlSignalNode *sig;      /* The signal disposition */
  SigAction keep_action;  /* The signal disposition of tecla signal handlers */
/*
 * Was no signal caught?
 */
  if(gl_pending_signal == -1)
    return 0;
/*
 * Record the signal that was caught, so that the user can query it later.
 */
  gl->last_signal = gl_pending_signal;
/*
 * Did we receive a terminal size signal?
 */
#ifdef USE_SIGWINCH
  if(gl_pending_signal == SIGWINCH && gl_resize_terminal(gl, 1))
    return 1;
#endif
/*
 * Lookup the requested disposition of this signal.
 */
  for(sig=gl->sigs; sig && sig->signo != gl_pending_signal; sig=sig->next)
    ;
  if(!sig)
    return 0;
/*
 * Start a fresh line?
 */
  if(sig->flags & GLS_RESTORE_LINE) {
    if(gl_set_term_curpos(gl, gl_buff_curpos_to_term_curpos(gl, gl->ntotal)) ||
       gl_output_raw_string(gl, "\r\n"))
      return 1;
  };
/*
 * Restore terminal settings to how they were before gl_get_line() was
 * called?
 */
  if(sig->flags & GLS_RESTORE_TTY)
    gl_restore_terminal_attributes(gl);
/*
 * Restore signal handlers to how they were before gl_get_line() was
 * called? If this hasn't been requested, only reinstate the signal
 * handler of the signal that we are handling.
 */
  if(sig->flags & GLS_RESTORE_SIG) {
    gl_restore_signal_handlers(gl);
  } else {
    (void) sigaction(sig->signo, &sig->original, &keep_action);
    (void) sigprocmask(SIG_UNBLOCK, &sig->proc_mask, NULL);
  };
/*
 * Forward the signal to the application's signal handler.
 */
  if(!(sig->flags & GLS_DONT_FORWARD))
    raise(gl_pending_signal);
  gl_pending_signal = -1;
/*
 * Reinstate our signal handlers.
 */
  if(sig->flags & GLS_RESTORE_SIG) {
    gl_override_signal_handlers(gl);
  } else {
    (void) sigaction(sig->signo, &keep_action, NULL);
    (void) sigprocmask(SIG_BLOCK, &sig->proc_mask, NULL);
  };
/*
 * Do we need to reinstate our terminal settings?
 */
  if(sig->flags & GLS_RESTORE_TTY)
    gl_raw_terminal_mode(gl);
/*
 * Redraw the line?
 */
  if(sig->flags & GLS_REDRAW_LINE && gl_redisplay(gl, 1))
    return 1;
/*
 * Set errno.
 */
  errno = sig->errno_value;
/*
 * What next?
 */
  switch(sig->after) {
  case GLS_RETURN:
    return gl_newline(gl, 1);
    break;
  case GLS_ABORT:
    return 1;
    break;
  case GLS_CONTINUE:
    return 0;
    break;
  };
  return 0;
}

/*.......................................................................
 * Get pertinent terminal control strings and the initial terminal size.
 *
 * Input:
 *  gl     GetLine *  The resource object of this library.
 *  term      char *  The type of the terminal.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int gl_control_strings(GetLine *gl, const char *term)
{
  int bad_term = 0;   /* True if term is unusable */
/*
 * Discard any existing control strings from a previous terminal.
 */
  gl->left = NULL;
  gl->right = NULL;
  gl->up = NULL;
  gl->down = NULL;
  gl->home = NULL;
  gl->bol = 0;
  gl->clear_eol = NULL;
  gl->clear_eod = NULL;
  gl->u_arrow = NULL;
  gl->d_arrow = NULL;
  gl->l_arrow = NULL;
  gl->r_arrow = NULL;
  gl->sound_bell = NULL;
  gl->bold = NULL;
  gl->underline = NULL;
  gl->standout = NULL;
  gl->dim = NULL;
  gl->reverse = NULL;
  gl->blink = NULL;
  gl->text_attr_off = NULL;
  gl->nline = 0;
  gl->ncolumn = 0;
#ifdef USE_TERMINFO
  gl->left_n = NULL;
  gl->right_n = NULL;
#endif
/*
 * If possible lookup the information in a terminal information
 * database.
 */
#ifdef USE_TERMINFO
  if(!term || setupterm((char *)term, gl->input_fd, NULL) == ERR) {
    bad_term = 1;
  } else {
    _clr_StringGroup(gl->capmem);
    gl->left = gl_tigetstr(gl, "cub1");
    gl->right = gl_tigetstr(gl, "cuf1");
    gl->up = gl_tigetstr(gl, "cuu1");
    gl->down = gl_tigetstr(gl, "cud1");
    gl->home = gl_tigetstr(gl, "home");
    gl->clear_eol = gl_tigetstr(gl, "el");
    gl->clear_eod = gl_tigetstr(gl, "ed");
    gl->u_arrow = gl_tigetstr(gl, "kcuu1");
    gl->d_arrow = gl_tigetstr(gl, "kcud1");
    gl->l_arrow = gl_tigetstr(gl, "kcub1");
    gl->r_arrow = gl_tigetstr(gl, "kcuf1");
    gl->left_n = gl_tigetstr(gl, "cub");
    gl->right_n = gl_tigetstr(gl, "cuf");
    gl->sound_bell = gl_tigetstr(gl, "bel");
    gl->bold = gl_tigetstr(gl, "bold");
    gl->underline = gl_tigetstr(gl, "smul");
    gl->standout = gl_tigetstr(gl, "smso");
    gl->dim = gl_tigetstr(gl, "dim");
    gl->reverse = gl_tigetstr(gl, "rev");
    gl->blink = gl_tigetstr(gl, "blink");
    gl->text_attr_off = gl_tigetstr(gl, "sgr0");
  };
#elif defined(USE_TERMCAP)
  if(!term || tgetent(gl->tgetent_buf, (char *)term) < 0) {
    bad_term = 1;
  } else {
    char *tgetstr_buf_ptr = gl->tgetstr_buf;
    _clr_StringGroup(gl->capmem);
    gl->left = gl_tgetstr(gl, "le", &tgetstr_buf_ptr);
    gl->right = gl_tgetstr(gl, "nd", &tgetstr_buf_ptr);
    gl->up = gl_tgetstr(gl, "up", &tgetstr_buf_ptr);
    gl->down = gl_tgetstr(gl, "do", &tgetstr_buf_ptr);
    gl->home = gl_tgetstr(gl, "ho", &tgetstr_buf_ptr);
    gl->clear_eol = gl_tgetstr(gl, "ce", &tgetstr_buf_ptr);
    gl->clear_eod = gl_tgetstr(gl, "cd", &tgetstr_buf_ptr);
    gl->u_arrow = gl_tgetstr(gl, "ku", &tgetstr_buf_ptr);
    gl->d_arrow = gl_tgetstr(gl, "kd", &tgetstr_buf_ptr);
    gl->l_arrow = gl_tgetstr(gl, "kl", &tgetstr_buf_ptr);
    gl->r_arrow = gl_tgetstr(gl, "kr", &tgetstr_buf_ptr);
    gl->sound_bell = gl_tgetstr(gl, "bl", &tgetstr_buf_ptr);
    gl->bold = gl_tgetstr(gl, "md", &tgetstr_buf_ptr);
    gl->underline = gl_tgetstr(gl, "us", &tgetstr_buf_ptr);
    gl->standout = gl_tgetstr(gl, "so", &tgetstr_buf_ptr);
    gl->dim = gl_tgetstr(gl, "mh", &tgetstr_buf_ptr);
    gl->reverse = gl_tgetstr(gl, "mr", &tgetstr_buf_ptr);
    gl->blink = gl_tgetstr(gl, "mb", &tgetstr_buf_ptr);
    gl->text_attr_off = gl_tgetstr(gl, "me", &tgetstr_buf_ptr);
  };
#endif
/*
 * Report term being unusable.
 */
  if(bad_term) {
    fprintf(stderr, "Bad terminal type: \"%s\". Will assume vt100.\n",
	    term ? term : "(null)");
  };
/*
 * Fill in missing information with ANSI VT100 strings.
 */
  if(!gl->left)
    gl->left = "\b";    /* ^H */
  if(!gl->right)
    gl->right = GL_ESC_STR "[C";
  if(!gl->up)
    gl->up = GL_ESC_STR "[A";
  if(!gl->down)
    gl->down = "\n";
  if(!gl->home)
    gl->home = GL_ESC_STR "[H";
  if(!gl->bol)
    gl->bol = "\r";
  if(!gl->clear_eol)
    gl->clear_eol = GL_ESC_STR "[K";
  if(!gl->clear_eod)
    gl->clear_eod = GL_ESC_STR "[J";
  if(!gl->u_arrow)
    gl->u_arrow = GL_ESC_STR "[A";
  if(!gl->d_arrow)
    gl->d_arrow = GL_ESC_STR "[B";
  if(!gl->l_arrow)
    gl->l_arrow = GL_ESC_STR "[D";
  if(!gl->r_arrow)
    gl->r_arrow = GL_ESC_STR "[C";
  if(!gl->sound_bell)
    gl->sound_bell = "\a";
  if(!gl->bold)
    gl->bold = GL_ESC_STR "[1m";
  if(!gl->underline)
    gl->underline = GL_ESC_STR "[4m";
  if(!gl->standout)
    gl->standout = GL_ESC_STR "[1;7m";
  if(!gl->dim)
    gl->dim = "";  /* Not available */
  if(!gl->reverse)
    gl->reverse = GL_ESC_STR "[7m";
  if(!gl->blink)
    gl->blink = GL_ESC_STR "[5m";
  if(!gl->text_attr_off)
    gl->text_attr_off = GL_ESC_STR "[m";
/*
 * Find out the current terminal size.
 */
  (void) gl_terminal_size(gl, GL_DEF_NCOLUMN, GL_DEF_NLINE);
  return 0;
}

#ifdef USE_TERMINFO
/*.......................................................................
 * This is a private function of gl_control_strings() used to look up
 * a termninal capability string from the terminfo database and make
 * a private copy of it.
 *
 * Input:
 *  gl         GetLine *  The resource object of gl_get_line().
 *  name    const char *  The name of the terminfo string to look up.
 * Output:
 *  return  const char *  The local copy of the capability, or NULL
 *                        if not available.
 */
static const char *gl_tigetstr(GetLine *gl, const char *name)
{
  const char *value = tigetstr((char *)name);
  if(!value || value == (char *) -1)
    return NULL;
  return _sg_store_string(gl->capmem, value, 0);
}
#elif defined(USE_TERMCAP)
/*.......................................................................
 * This is a private function of gl_control_strings() used to look up
 * a termninal capability string from the termcap database and make
 * a private copy of it. Note that some emulations of tgetstr(), such
 * as that used by Solaris, ignores the buffer pointer that is past to
 * it, so we can't assume that a private copy has been made that won't
 * be trashed by another call to gl_control_strings() by another
 * GetLine object. So we make what may be a redundant private copy
 * of the string in gl->capmem.
 *
 * Input:
 *  gl         GetLine *  The resource object of gl_get_line().
 *  name    const char *  The name of the terminfo string to look up.
 * Input/Output:
 *  bufptr        char ** On input *bufptr points to the location in
 *                        gl->tgetstr_buf at which to record the
 *                        capability string. On output *bufptr is
 *                        incremented over the stored string.
 * Output:
 *  return  const char *  The local copy of the capability, or NULL
 *                        on error.
 */
static const char *gl_tgetstr(GetLine *gl, const char *name, char **bufptr)
{
  const char *value = tgetstr((char *)name, bufptr);
  if(!value || value == (char *) -1)
    return NULL;
  return _sg_store_string(gl->capmem, value, 0);
}
#endif

/*.......................................................................
 * This is an action function that implements a user interrupt (eg. ^C).
 */
static KT_KEY_FN(gl_user_interrupt)
{
  raise(SIGINT);
  return 1;
}

/*.......................................................................
 * This is an action function that implements the abort signal.
 */
static KT_KEY_FN(gl_abort)
{
  raise(SIGABRT);
  return 1;
}

/*.......................................................................
 * This is an action function that sends a suspend signal (eg. ^Z) to the
 * the parent process.
 */
static KT_KEY_FN(gl_suspend)
{
  raise(SIGTSTP);
  return 0;
}

/*.......................................................................
 * This is an action function that halts output to the terminal.
 */
static KT_KEY_FN(gl_stop_output)
{
  tcflow(gl->output_fd, TCOOFF);
  return 0;
}

/*.......................................................................
 * This is an action function that resumes halted terminal output.
 */
static KT_KEY_FN(gl_start_output)
{
  tcflow(gl->output_fd, TCOON);
  return 0;
}

/*.......................................................................
 * This is an action function that allows the next character to be accepted
 * without any interpretation as a special character.
 */
static KT_KEY_FN(gl_literal_next)
{
  char c;   /* The character to be added to the line */
  int i;
/*
 * Get the character to be inserted literally.
 */
  if(gl_read_character(gl, &c, -1))
    return 1;
/*
 * Add the character to the line 'count' times.
 */
  for(i=0; i<count; i++)
    gl_add_char_to_line(gl, c);
  return 0;
}

/*.......................................................................
 * Return the number of characters needed to display a given character
 * on the screen. Tab characters require eight spaces, and control
 * characters are represented by a caret followed by the modified
 * character.
 *
 * Input:
 *  gl       GetLine *  The resource object of this library.
 *  c           char    The character to be displayed.
 *  term_curpos  int    The destination terminal location of the character.
 *                      This is needed because the width of tab characters
 *                      depends on where they are, relative to the
 *                      preceding tab stops.
 * Output:
 *  return       int    The number of terminal charaters needed.
 */
static int gl_displayed_char_width(GetLine *gl, char c, int term_curpos)
{
  if(c=='\t')
    return TAB_WIDTH - ((term_curpos % gl->ncolumn) % TAB_WIDTH);
  if(IS_CTRL_CHAR(c))
    return 2;
  if(!isprint((int)(unsigned char) c)) {
    char string[TAB_WIDTH + 4];
    sprintf(string, "\\%o", (int)(unsigned char)c);
    return strlen(string);
  };
  return 1;
}

/*.......................................................................
 * Work out the length of given string of characters on the terminal.
 *
 * Input:
 *  gl       GetLine *  The resource object of this library.
 *  string      char *  The string to be measured.
 *  nc           int    The number of characters to be measured, or -1
 *                      to measure the whole string.
 *  term_curpos  int    The destination terminal location of the character.
 *                      This is needed because the width of tab characters
 *                      depends on where they are, relative to the
 *                      preceding tab stops.
 * Output:
 *  return       int    The number of displayed characters.
 */
static int gl_displayed_string_width(GetLine *gl, const char *string, int nc,
				     int term_curpos)
{
  int slen=0;   /* The displayed number of characters */
  int i;
/*
 * How many characters are to be measured?
 */
  if(nc < 0)
    nc = strlen(string);
/*
 * Add up the length of the displayed string.
 */
  for(i=0; i<nc; i++)
    slen += gl_displayed_char_width(gl, string[i], term_curpos + slen);
  return slen;
}

/*.......................................................................
 * Write a string directly to the terminal.
 *
 * Input:
 *  gl     GetLine *   The resource object of this program.
 *  string    char *   The string to be written.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int gl_output_raw_string(GetLine *gl, const char *string)
{
  if(gl->echo) {
    int ndone = 0;   /* The number of characters written so far */
/*
 * How long is the string to be written?
 */
    int slen = strlen(string);
/*
 * Attempt to write the string to the terminal, restarting the
 * write if a signal is caught.
 */
    while(ndone < slen) {
      int nnew = fwrite(string + ndone, sizeof(char), slen-ndone,
			gl->output_fp);
      if(nnew > 0)
	ndone += nnew;
      else if(errno != EINTR)
	return 1;
    };
  };
  return 0;
}

/*.......................................................................
 * Output a terminal control sequence. When using terminfo,
 * this must be a sequence returned by tgetstr() or tigetstr()
 * respectively.
 *
 * Input:
 *  gl     GetLine *   The resource object of this library.
 *  nline      int     The number of lines affected by the operation,
 *                     or 1 if not relevant.
 *  string    char *   The control sequence to be sent.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int gl_output_control_sequence(GetLine *gl, int nline,
				      const char *string)
{
  if(gl->echo) {
#if defined(USE_TERMINFO) || defined(USE_TERMCAP)
    tputs_fp = gl->output_fp;
    errno = 0;
    tputs((char *)string, nline, gl_tputs_putchar);
    return errno != 0;
#else
    return gl_output_raw_string(gl, string);
#endif
  };
  return 0;
}

#if defined(USE_TERMINFO) || defined(USE_TERMCAP)
/*.......................................................................
 * The following function is used as the output function of tputs when
 * terminfo is being used.
 *
 * Input:
 *  c   TputsType  The character to be output.
 * Output:
 *  return    int  The character that was written or EOF on error.
 */
static int gl_tputs_putchar(TputsType c)
{
  return putc(c, tputs_fp);
}
#endif
  
/*.......................................................................
 * Move the terminal cursor n characters to the left or right.
 *
 * Input:
 *  gl     GetLine *   The resource object of this program.
 *  n          int     number of positions to the right (> 0) or left (< 0).
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int gl_terminal_move_cursor(GetLine *gl, int n)
{
  int cur_row, cur_col; /* The current terminal row and column index of */
                        /*  the cursor wrt the start of the input line. */
  int new_row, new_col; /* The target terminal row and column index of */
                        /*  the cursor wrt the start of the input line. */
/*
 * How far can we move left?
 */
  if(gl->term_curpos + n < 0)
    n = gl->term_curpos;
/*
 * Break down the current and target cursor locations into rows and columns.
 */
  cur_row = gl->term_curpos / gl->ncolumn;
  cur_col = gl->term_curpos % gl->ncolumn;
  new_row = (gl->term_curpos + n) / gl->ncolumn;
  new_col = (gl->term_curpos + n) % gl->ncolumn;
/*
 * Move down to the next line.
 */
  for(; cur_row < new_row; cur_row++) {
    if(gl_output_control_sequence(gl, 1, gl->down))
      return 1;
  };
/*
 * Move up to the previous line.
 */
  for(; cur_row > new_row; cur_row--) {
    if(gl_output_control_sequence(gl, 1, gl->up))
      return 1;
  };
/*
 * Move to the right within the target line?
 */
  if(cur_col < new_col) {
#ifdef USE_TERMINFO
/*
 * Use a parameterized control sequence if it generates less control
 * characters (guess based on ANSI terminal termcap entry).
 */
    if(gl->right_n != NULL && new_col - cur_col > 1) {
      if(gl_output_control_sequence(gl, 1, tparm((char *)gl->right_n,
           (long)(new_col - cur_col), 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l)))
	return 1;
    } else
#endif
    {
      for(; cur_col < new_col; cur_col++) {
        if(gl_output_control_sequence(gl, 1, gl->right))
          return 1;
      };
    };
/*
 * Move to the left within the target line?
 */
  } else if(cur_col > new_col) {
#ifdef USE_TERMINFO
/*
 * Use a parameterized control sequence if it generates less control
 * characters (guess based on ANSI terminal termcap entry).
 */
    if(gl->left_n != NULL && cur_col - new_col > 3) {
      if(gl_output_control_sequence(gl, 1, tparm((char *)gl->left_n,
           (long)(cur_col - new_col), 0l, 0l, 0l, 0l, 0l, 0l, 0l, 0l)))
	return 1;
    } else
#endif
    {
      for(; cur_col > new_col; cur_col--) {
        if(gl_output_control_sequence(gl, 1, gl->left))
          return 1;
      };
    };
  }
/*
 * Update the recorded position of the terminal cursor.
 */
  gl->term_curpos += n;
  return 0;
}

/*.......................................................................
 * Write a character to the terminal after expanding tabs and control
 * characters to their multi-character representations.
 *
 * Input:
 *  gl    GetLine *   The resource object of this program.
 *  c        char     The character to be output.
 *  pad      char     Many terminals have the irritating feature that
 *                    when one writes a character in the last column of
 *                    of the terminal, the cursor isn't wrapped to the
 *                    start of the next line until one more character
 *                    is written. Some terminals don't do this, so
 *                    after such a write, we don't know where the
 *                    terminal is unless we output an extra character.
 *                    This argument specifies the character to write.
 *                    If at the end of the input line send '\0' or a
 *                    space, and a space will be written. Otherwise,
 *                    pass the next character in the input line
 *                    following the one being written.
 * Output:
 *  return    int     0 - OK.
 */ 
static int gl_output_char(GetLine *gl, char c, char pad)
{
  char string[TAB_WIDTH + 4]; /* A work area for composing compound strings */
  int nchar;                  /* The number of terminal characters */
  int i;
/*
 * Check for special characters.
 */
  if(c == '\t') {
/*
 * How many spaces do we need to represent a tab at the current terminal
 * column?
 */
    nchar = gl_displayed_char_width(gl, '\t', gl->term_curpos);
/*
 * Compose the tab string.
 */
    for(i=0; i<nchar; i++)
      string[i] = ' ';
  } else if(IS_CTRL_CHAR(c)) {
    string[0] = '^';
    string[1] = CTRL_TO_CHAR(c);
    nchar = 2;
  } else if(!isprint((int)(unsigned char) c)) {
    sprintf(string, "\\%o", (int)(unsigned char)c);
    nchar = strlen(string);
  } else {
    string[0] = c;
    nchar = 1;
  };
/*
 * Terminate the string.
 */
  string[nchar] = '\0';
/*
 * Write the string to the terminal.
 */
  if(gl_output_raw_string(gl, string))
    return 1;
/*
 * Except for one exception to be described in a moment, the cursor should
 * now have been positioned after the character that was just output.
 */
  gl->term_curpos += nchar;
/*
 * If the new character ended exactly at the end of a line,
 * most terminals won't move the cursor onto the next line until we
 * have written a character on the next line, so append an extra
 * space then move the cursor back.
 */
  if(gl->term_curpos % gl->ncolumn == 0) {
    int term_curpos = gl->term_curpos;
    if(gl_output_char(gl, pad ? pad : ' ', ' ') ||
       gl_set_term_curpos(gl, term_curpos))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Write a string to the terminal after expanding tabs and control
 * characters to their multi-character representations.
 *
 * Input:
 *  gl    GetLine *   The resource object of this program.
 *  string   char *   The string to be output.
 *  pad      char     Many terminals have the irritating feature that
 *                    when one writes a character in the last column of
 *                    of the terminal, the cursor isn't wrapped to the
 *                    start of the next line until one more character
 *                    is written. Some terminals don't do this, so
 *                    after such a write, we don't know where the
 *                    terminal is unless we output an extra character.
 *                    This argument specifies the character to write.
 *                    If at the end of the input line send '\0' or a
 *                    space, and a space will be written. Otherwise,
 *                    pass the next character in the input line
 *                    following the one being written.
 * Output:
 *  return    int     0 - OK.
 */
static int gl_output_string(GetLine *gl, const char *string, char pad)
{
  const char *cptr;   /* A pointer into string[] */
  for(cptr=string; *cptr; cptr++) {
    char nextc = cptr[1];
    if(gl_output_char(gl, *cptr, nextc ? nextc : pad))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Given a character position within gl->line[], work out the
 * corresponding gl->term_curpos position on the terminal.
 *
 * Input:
 *  gl      GetLine *   The resource object of this library.
 *  buff_curpos int     The position within gl->line[].
 *  
 * Output:
 *  return      int     The gl->term_curpos position on the terminal.
 */
static int gl_buff_curpos_to_term_curpos(GetLine *gl, int buff_curpos)
{
  return gl->prompt_len + gl_displayed_string_width(gl, gl->line, buff_curpos,
						    gl->prompt_len);
}

/*.......................................................................
 * Move the terminal cursor position.
 *
 * Input:
 *  gl      GetLine *  The resource object of this library.
 *  term_curpos int    The destination terminal cursor position.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
static int gl_set_term_curpos(GetLine *gl, int term_curpos)
{
  return gl_terminal_move_cursor(gl, term_curpos - gl->term_curpos);
}

/*.......................................................................
 * This is an action function that moves the buffer cursor one character
 * left, and updates the terminal cursor to match.
 */
static KT_KEY_FN(gl_cursor_left)
{
  return gl_place_cursor(gl, gl->buff_curpos - count);
}

/*.......................................................................
 * This is an action function that moves the buffer cursor one character
 * right, and updates the terminal cursor to match.
 */
static KT_KEY_FN(gl_cursor_right)
{
  return gl_place_cursor(gl, gl->buff_curpos + count);
}

/*.......................................................................
 * This is an action function that toggles between overwrite and insert
 * mode.
 */
static KT_KEY_FN(gl_insert_mode)
{
  gl->insert = !gl->insert;
  return 0;
}

/*.......................................................................
 * This is an action function which moves the cursor to the beginning of
 * the line.
 */
static KT_KEY_FN(gl_beginning_of_line)
{
  return gl_place_cursor(gl, 0);
}

/*.......................................................................
 * This is an action function which moves the cursor to the end of
 * the line.
 */
static KT_KEY_FN(gl_end_of_line)
{
  return gl_place_cursor(gl, gl->ntotal);
}

/*.......................................................................
 * This is an action function which deletes the entire contents of the
 * current line.
 */
static KT_KEY_FN(gl_delete_line)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Copy the contents of the line to the cut buffer.
 */
  strcpy(gl->cutbuf, gl->line);
/*
 * Clear the buffer.
 */
  gl->ntotal = 0;
  gl->line[0] = '\0';
/*
 * Move the terminal cursor to just after the prompt.
 */
  if(gl_place_cursor(gl, 0))
    return 1;
/*
 * Clear from the end of the prompt to the end of the terminal.
 */
  if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
  return 0;
}

/*.......................................................................
 * This is an action function which deletes all characters between the
 * current cursor position and the end of the line.
 */
static KT_KEY_FN(gl_kill_line)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Copy the part of the line that is about to be deleted to the cut buffer.
 */
  strcpy(gl->cutbuf, gl->line + gl->buff_curpos);
/*
 * Terminate the buffered line at the current cursor position.
 */
  gl->ntotal = gl->buff_curpos;
  gl->line[gl->ntotal] = '\0';
/*
 * Clear the part of the line that follows the cursor.
 */
  if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
/*
 * Explicitly reset the cursor position to allow vi command mode
 * constraints on its position to be set.
 */
  return gl_place_cursor(gl, gl->buff_curpos);
}

/*.......................................................................
 * This is an action function which deletes all characters between the
 * start of the line and the current cursor position.
 */
static KT_KEY_FN(gl_backward_kill_line)
{
/*
 * How many characters are to be deleted from before the cursor?
 */
  int nc = gl->buff_curpos - gl->insert_curpos;
  if (!nc)
    return 0;
/*
 * Move the cursor to the start of the line, or in vi input mode,
 * the start of the sub-line at which insertion started, and delete
 * up to the old cursor position.
 */
  return gl_place_cursor(gl, gl->insert_curpos) ||
         gl_delete_chars(gl, nc, gl->editor == GL_EMACS_MODE || gl->vi.command);
}

/*.......................................................................
 * This is an action function which moves the cursor forward by a word.
 */
static KT_KEY_FN(gl_forward_word)
{
  return gl_place_cursor(gl, gl_nth_word_end_forward(gl, count) +
			 (gl->editor==GL_EMACS_MODE));
}

/*.......................................................................
 * This is an action function which moves the cursor forward to the start
 * of the next word.
 */
static KT_KEY_FN(gl_forward_to_word)
{
  return gl_place_cursor(gl, gl_nth_word_start_forward(gl, count));
}

/*.......................................................................
 * This is an action function which moves the cursor backward by a word.
 */
static KT_KEY_FN(gl_backward_word)
{
  return gl_place_cursor(gl, gl_nth_word_start_backward(gl, count));
}

/*.......................................................................
 * Delete one or more characters, starting with the one under the cursor.
 *
 * Input:
 *  gl     GetLine *  The resource object of this library.
 *  nc         int    The number of characters to delete.
 *  cut        int    If true, copy the characters to the cut buffer.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int gl_delete_chars(GetLine *gl, int nc, int cut)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * If there are fewer than nc characters following the cursor, limit
 * nc to the number available.
 */
  if(gl->buff_curpos + nc > gl->ntotal)
    nc = gl->ntotal - gl->buff_curpos;
/*
 * Copy the about to be deleted region to the cut buffer.
 */
  if(cut) {
    memcpy(gl->cutbuf, gl->line + gl->buff_curpos, nc);
    gl->cutbuf[nc] = '\0';
  }
/*
 * Nothing to delete?
 */
  if(nc <= 0)
    return 0;
/*
 * In vi overwrite mode, restore any previously overwritten characters
 * from the undo buffer.
 */
  if(gl->editor == GL_VI_MODE && !gl->vi.command && !gl->insert) {
/*
 * How many of the characters being deleted can be restored from the
 * undo buffer?
 */
    int nrestore = gl->buff_curpos + nc <= gl->vi.undo.ntotal ?
      nc : gl->vi.undo.ntotal - gl->buff_curpos;
/*
 * Restore any available characters.
 */
    if(nrestore > 0)
      memcpy(gl->line + gl->buff_curpos, gl->vi.undo.line + gl->buff_curpos,
             nrestore);
/*
 * If their were insufficient characters in the undo buffer, then this
 * implies that we are deleting from the end of the line, so we need
 * to terminate the line either where the undo buffer ran out, or if
 * we are deleting from beyond the end of the undo buffer, at the current
 * cursor position.
 */
    if(nc != nrestore) {
      gl->ntotal = gl->vi.undo.ntotal > gl->buff_curpos ? gl->vi.undo.ntotal :
	gl->buff_curpos;
      gl->line[gl->ntotal] = '\0';
    };
  } else {
/*
 * Copy the remaining part of the line back over the deleted characters.
 */
    memmove(gl->line + gl->buff_curpos, gl->line + gl->buff_curpos + nc,
	    gl->ntotal - gl->buff_curpos - nc + 1);
    gl->ntotal -= nc;
  };
/*
 * Redraw the remaining characters following the cursor.
 */
  if(gl_output_string(gl, gl->line + gl->buff_curpos, '\0'))
    return 1;
/*
 * Clear to the end of the terminal.
 */
  if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
/*
 * Place the cursor at the start of where the deletion was performed.
 */
  return gl_place_cursor(gl, gl->buff_curpos);
}

/*.......................................................................
 * This is an action function which deletes character(s) under the
 * cursor without moving the cursor.
 */
static KT_KEY_FN(gl_forward_delete_char)
{
/*
 * Delete 'count' characters.
 */
  return gl_delete_chars(gl, count, gl->vi.command);
}

/*.......................................................................
 * This is an action function which deletes character(s) under the
 * cursor and moves the cursor back one character.
 */
static KT_KEY_FN(gl_backward_delete_char)
{
/*
 * Restrict the deletion count to the number of characters that
 * precede the insertion point.
 */
  if(count > gl->buff_curpos - gl->insert_curpos)
    count = gl->buff_curpos - gl->insert_curpos;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
  return gl_cursor_left(gl, count) ||
    gl_delete_chars(gl, count, gl->vi.command);
}

/*.......................................................................
 * Starting from the cursor position delete to the specified column.
 */
static KT_KEY_FN(gl_delete_to_column)
{
  if (--count >= gl->buff_curpos)
    return gl_forward_delete_char(gl, count - gl->buff_curpos);
  else
    return gl_backward_delete_char(gl, gl->buff_curpos - count);
}

/*.......................................................................
 * Starting from the cursor position delete characters to a matching
 * parenthesis.
 */
static KT_KEY_FN(gl_delete_to_parenthesis)
{
  int curpos = gl_index_of_matching_paren(gl);
  if(curpos >= 0) {
    gl_save_for_undo(gl);
    if(curpos >= gl->buff_curpos)
      return gl_forward_delete_char(gl, curpos - gl->buff_curpos + 1);
    else
      return gl_backward_delete_char(gl, ++gl->buff_curpos - curpos + 1);
  };
  return 0;
}

/*.......................................................................
 * This is an action function which deletes from the cursor to the end
 * of the word that the cursor is either in or precedes.
 */
static KT_KEY_FN(gl_forward_delete_word)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * In emacs mode delete to the end of the word. In vi mode delete to the
 * start of the net word.
 */
  if(gl->editor == GL_EMACS_MODE) {
    return gl_delete_chars(gl,
		gl_nth_word_end_forward(gl,count) - gl->buff_curpos + 1, 1);
  } else {
    return gl_delete_chars(gl,
		gl_nth_word_start_forward(gl,count) - gl->buff_curpos,
		gl->vi.command);
  };
}

/*.......................................................................
 * This is an action function which deletes the word that precedes the
 * cursor.
 */
static KT_KEY_FN(gl_backward_delete_word)
{
/*
 * Keep a record of the current cursor position.
 */
  int buff_curpos = gl->buff_curpos;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Move back 'count' words.
 */
  if(gl_backward_word(gl, count))
    return 1;
/*
 * Delete from the new cursor position to the original one.
 */
  return gl_delete_chars(gl, buff_curpos - gl->buff_curpos,
  			 gl->editor == GL_EMACS_MODE || gl->vi.command);
}

/*.......................................................................
 * Searching in a given direction, delete to the count'th
 * instance of a specified or queried character, in the input line.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  count        int    The number of times to search.
 *  c           char    The character to be searched for, or '\0' if
 *                      the character should be read from the user.
 *  forward      int    True if searching forward.
 *  onto         int    True if the search should end on top of the
 *                      character, false if the search should stop
 *                      one character before the character in the
 *                      specified search direction.
 *  change       int    If true, this function is being called upon
 *                      to do a vi change command, in which case the
 *                      user will be left in insert mode after the
 *                      deletion.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
static int gl_delete_find(GetLine *gl, int count, char c, int forward,
			  int onto, int change)
{
/*
 * Search for the character, and abort the deletion if not found.
 */
  int pos = gl_find_char(gl, count, forward, onto, c);
  if(pos < 0)
    return 0;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Allow the cursor to be at the end of the line if this is a change
 * command.
 */
  if(change)
    gl->vi.command = 0;
/*
 * Delete the appropriate span of characters.
 */
  if(forward) {
    if(gl_delete_chars(gl, pos - gl->buff_curpos + 1, 1))
      return 1;
  } else {
    int buff_curpos = gl->buff_curpos;
    if(gl_place_cursor(gl, pos) ||
       gl_delete_chars(gl, buff_curpos - gl->buff_curpos, 1))
      return 1;
  };
/*
 * If this is a change operation, switch the insert mode.
 */
  if(change && gl_vi_insert(gl, 0))
    return 1;
  return 0;
}

/*.......................................................................
 * This is an action function which deletes forward from the cursor up to and
 * including a specified character.
 */
static KT_KEY_FN(gl_forward_delete_find)
{
  return gl_delete_find(gl, count, '\0', 1, 1, 0);
}

/*.......................................................................
 * This is an action function which deletes backward from the cursor back to
 * and including a specified character.
 */
static KT_KEY_FN(gl_backward_delete_find)
{
  return gl_delete_find(gl, count, '\0', 0, 1, 0);
}

/*.......................................................................
 * This is an action function which deletes forward from the cursor up to but
 * not including a specified character.
 */
static KT_KEY_FN(gl_forward_delete_to)
{
  return gl_delete_find(gl, count, '\0', 1, 0, 0);
}

/*.......................................................................
 * This is an action function which deletes backward from the cursor back to
 * but not including a specified character.
 */
static KT_KEY_FN(gl_backward_delete_to)
{
  return gl_delete_find(gl, count, '\0', 0, 0, 0);
}

/*.......................................................................
 * This is an action function which deletes to a character specified by a
 * previous search.
 */
static KT_KEY_FN(gl_delete_refind)
{
  return gl_delete_find(gl, count, gl->vi.find_char, gl->vi.find_forward,
			gl->vi.find_onto, 0);
}

/*.......................................................................
 * This is an action function which deletes to a character specified by a
 * previous search, but in the opposite direction.
 */
static KT_KEY_FN(gl_delete_invert_refind)
{
  return gl_delete_find(gl, count, gl->vi.find_char,
			!gl->vi.find_forward, gl->vi.find_onto, 0);
}

/*.......................................................................
 * This is an action function which converts the characters in the word
 * following the cursor to upper case.
 */
static KT_KEY_FN(gl_upcase_word)
{
/*
 * Locate the count'th word ending after the cursor.
 */
  int last = gl_nth_word_end_forward(gl, count);
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Upcase characters from the current cursor position to 'last'.
 */
  while(gl->buff_curpos <= last) {
    char *cptr = gl->line + gl->buff_curpos++;
/*
 * Convert the character to upper case?
 */
    if(islower((int)(unsigned char) *cptr))
      *cptr = toupper((int) *cptr);
/*
 * Write the possibly modified character back. Note that for non-modified
 * characters we want to do this as well, so as to advance the cursor.
 */
    if(gl_output_char(gl, *cptr, cptr[1]))
      return 1;
  };
  return gl_place_cursor(gl, gl->buff_curpos);	/* bounds check */
}

/*.......................................................................
 * This is an action function which converts the characters in the word
 * following the cursor to lower case.
 */
static KT_KEY_FN(gl_downcase_word)
{
/*
 * Locate the count'th word ending after the cursor.
 */
  int last = gl_nth_word_end_forward(gl, count);
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Upcase characters from the current cursor position to 'last'.
 */
  while(gl->buff_curpos <= last) {
    char *cptr = gl->line + gl->buff_curpos++;
/*
 * Convert the character to upper case?
 */
    if(isupper((int)(unsigned char) *cptr))
      *cptr = tolower((int) *cptr);
/*
 * Write the possibly modified character back. Note that for non-modified
 * characters we want to do this as well, so as to advance the cursor.
 */
    if(gl_output_char(gl, *cptr, cptr[1]))
      return 1;
  };
  return gl_place_cursor(gl, gl->buff_curpos);	/* bounds check */
}

/*.......................................................................
 * This is an action function which converts the first character of the
 * following word to upper case, in order to capitalize the word, and
 * leaves the cursor at the end of the word.
 */
static KT_KEY_FN(gl_capitalize_word)
{
  char *cptr;   /* &gl->line[gl->buff_curpos] */
  int first;    /* True for the first letter of the word */
  int i;
/*
 * Keep a record of the current insert mode and the cursor position.
 */
  int insert = gl->insert;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * We want to overwrite the modified word.
 */
  gl->insert = 0;
/*
 * Capitalize 'count' words.
 */
  for(i=0; i<count && gl->buff_curpos < gl->ntotal; i++) {
    int pos = gl->buff_curpos;
/*
 * If we are not already within a word, skip to the start of the word.
 */
    for(cptr = gl->line + pos ; pos<gl->ntotal && !gl_is_word_char((int) *cptr);
	pos++, cptr++)
      ;
/*
 * Move the cursor to the new position.
 */
    if(gl_place_cursor(gl, pos))
      return 1;
/*
 * While searching for the end of the word, change lower case letters
 * to upper case.
 */
    for(first=1; gl->buff_curpos<gl->ntotal && gl_is_word_char((int) *cptr);
	gl->buff_curpos++, cptr++) {
/*
 * Convert the character to upper case?
 */
      if(first) {
	if(islower((int)(unsigned char) *cptr))
	  *cptr = toupper((int) *cptr);
      } else {
	if(isupper((int)(unsigned char) *cptr))
	  *cptr = tolower((int) *cptr);
      };
      first = 0;
/*
 * Write the possibly modified character back. Note that for non-modified
 * characters we want to do this as well, so as to advance the cursor.
 */
      if(gl_output_char(gl, *cptr, cptr[1]))
	return 1;
    };
  };
/*
 * Restore the insertion mode.
 */
  gl->insert = insert;
  return gl_place_cursor(gl, gl->buff_curpos);	/* bounds check */
}

/*.......................................................................
 * This is an action function which redraws the current line.
 */
static KT_KEY_FN(gl_redisplay)
{
/*
 * Keep a record of the current cursor position.
 */
  int buff_curpos = gl->buff_curpos;
/*
 * Move the cursor to the start of the terminal line, and clear from there
 * to the end of the display.
 */
  if(gl_set_term_curpos(gl, 0) ||
     gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
/*
 * Display the current prompt.
 */
  if(gl_display_prompt(gl))
    return 1;
/*
 * Render the part of the line that the user has typed in so far.
 */
  if(gl_output_string(gl, gl->line, '\0'))
    return 1;
/*
 * Restore the cursor position.
 */
  if(gl_place_cursor(gl, buff_curpos))
    return 1;
/*
 * Flush the redisplayed line to the terminal.
 */
  return gl_flush_output(gl);
}

/*.......................................................................
 * This is an action function which clears the display and redraws the
 * input line from the home position.
 */
static KT_KEY_FN(gl_clear_screen)
{
/*
 * Record the current cursor position.
 */
  int buff_curpos = gl->buff_curpos;
/*
 * Home the cursor and clear from there to the end of the display.
 */
  if(gl_output_control_sequence(gl, gl->nline, gl->home) ||
     gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
/*
 * Redisplay the line.
 */
  gl->term_curpos = 0;
  gl->buff_curpos = 0;
  if(gl_redisplay(gl,1))
    return 1;
/*
 * Restore the cursor position.
 */
  return gl_place_cursor(gl, buff_curpos);
}

/*.......................................................................
 * This is an action function which swaps the character under the cursor
 * with the character to the left of the cursor.
 */
static KT_KEY_FN(gl_transpose_chars)
{
  char from[3];     /* The original string of 2 characters */
  char swap[3];     /* The swapped string of two characters */
/*
 * If we are at the beginning or end of the line, there aren't two
 * characters to swap.
 */
  if(gl->buff_curpos < 1 || gl->buff_curpos >= gl->ntotal)
    return 0;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Get the original and swapped strings of the two characters.
 */
  from[0] = gl->line[gl->buff_curpos - 1];
  from[1] = gl->line[gl->buff_curpos];
  from[2] = '\0';
  swap[0] = gl->line[gl->buff_curpos];
  swap[1] = gl->line[gl->buff_curpos - 1];
  swap[2] = '\0';
/*
 * Move the cursor to the start of the two characters.
 */
  if(gl_place_cursor(gl, gl->buff_curpos-1))
    return 1;
/*
 * Swap the two characters in the buffer.
 */
  gl->line[gl->buff_curpos] = swap[0];
  gl->line[gl->buff_curpos+1] = swap[1];
/*
 * If the sum of the displayed width of the two characters
 * in their current and final positions is the same, swapping can
 * be done by just overwriting with the two swapped characters.
 */
  if(gl_displayed_string_width(gl, from, -1, gl->term_curpos) ==
     gl_displayed_string_width(gl, swap, -1, gl->term_curpos)) {
    int insert = gl->insert;
    gl->insert = 0;
    if(gl_output_char(gl, swap[0], swap[1]) ||
       gl_output_char(gl, swap[1], gl->line[gl->buff_curpos+2]))
      return 1;
    gl->insert = insert;
/*
 * If the swapped substring has a different displayed size, we need to
 * redraw everything after the first of the characters.
 */
  } else {
    if(gl_output_string(gl, gl->line + gl->buff_curpos, '\0') ||
       gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
      return 1;
  };
/*
 * Advance the cursor to the character after the swapped pair.
 */
  return gl_place_cursor(gl, gl->buff_curpos + 2);
}

/*.......................................................................
 * This is an action function which sets a mark at the current cursor
 * location.
 */
static KT_KEY_FN(gl_set_mark)
{
  gl->buff_mark = gl->buff_curpos;
  return 0;
}

/*.......................................................................
 * This is an action function which swaps the mark location for the
 * cursor location.
 */
static KT_KEY_FN(gl_exchange_point_and_mark)
{
/*
 * Get the old mark position, and limit to the extent of the input
 * line.
 */
  int old_mark = gl->buff_mark <= gl->ntotal ? gl->buff_mark : gl->ntotal;
/*
 * Make the current cursor position the new mark.
 */
  gl->buff_mark = gl->buff_curpos;
/*
 * Move the cursor to the old mark position.
 */
  return gl_place_cursor(gl, old_mark);
}

/*.......................................................................
 * This is an action function which deletes the characters between the
 * mark and the cursor, recording them in gl->cutbuf for later pasting.
 */
static KT_KEY_FN(gl_kill_region)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Limit the mark to be within the line.
 */
  if(gl->buff_mark > gl->ntotal)
    gl->buff_mark = gl->ntotal;
/*
 * If there are no characters between the cursor and the mark, simply clear
 * the cut buffer.
 */
  if(gl->buff_mark == gl->buff_curpos) {
    gl->cutbuf[0] = '\0';
    return 0;
  };
/*
 * If the mark is before the cursor, swap the cursor and the mark.
 */
  if(gl->buff_mark < gl->buff_curpos && gl_exchange_point_and_mark(gl,1))
    return 1;
/*
 * Delete the characters.
 */
  if(gl_delete_chars(gl, gl->buff_mark - gl->buff_curpos, 1))
    return 1;
/*
 * Make the mark the same as the cursor position.
 */
  gl->buff_mark = gl->buff_curpos;
  return 0;
}

/*.......................................................................
 * This is an action function which records the characters between the
 * mark and the cursor, in gl->cutbuf for later pasting.
 */
static KT_KEY_FN(gl_copy_region_as_kill)
{
  int ca, cb;  /* The indexes of the first and last characters in the region */
  int mark;    /* The position of the mark */
/*
 * Get the position of the mark, limiting it to lie within the line.
 */
  mark = gl->buff_mark > gl->ntotal ? gl->ntotal : gl->buff_mark;
/*
 * If there are no characters between the cursor and the mark, clear
 * the cut buffer.
 */
  if(mark == gl->buff_curpos) {
    gl->cutbuf[0] = '\0';
    return 0;
  };
/*
 * Get the line indexes of the first and last characters in the region.
 */
  if(mark < gl->buff_curpos) {
    ca = mark;
    cb = gl->buff_curpos - 1;
  } else {
    ca = gl->buff_curpos;
    cb = mark - 1;
  };
/*
 * Copy the region to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line + ca, cb + 1 - ca);
  gl->cutbuf[cb + 1 - ca] = '\0';
  return 0;
}

/*.......................................................................
 * This is an action function which inserts the contents of the cut
 * buffer at the current cursor location.
 */
static KT_KEY_FN(gl_yank)
{
  int i;
/*
 * Set the mark at the current location.
 */
  gl->buff_mark = gl->buff_curpos;
/*
 * Do nothing else if the cut buffer is empty.
 */
  if(gl->cutbuf[0] == '\0')
    return gl_ring_bell(gl, 1);
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Insert the string count times.
 */
  for(i=0; i<count; i++) {
    if(gl_add_string_to_line(gl, gl->cutbuf))
      return 1;
  };
/*
 * gl_add_string_to_line() leaves the cursor after the last character that
 * was pasted, whereas vi leaves the cursor over the last character pasted.
 */
  if(gl->editor == GL_VI_MODE && gl_cursor_left(gl, 1))
    return 1;
  return 0;
}

/*.......................................................................
 * This is an action function which inserts the contents of the cut
 * buffer one character beyond the current cursor location.
 */
static KT_KEY_FN(gl_append_yank)
{
  int was_command = gl->vi.command;
  int i;
/*
 * If the cut buffer is empty, ring the terminal bell.
 */
  if(gl->cutbuf[0] == '\0')
    return gl_ring_bell(gl, 1);
/*
 * Set the mark at the current location + 1.
 */
  gl->buff_mark = gl->buff_curpos + 1;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Arrange to paste the text in insert mode after the current character.
 */
  if(gl_vi_append(gl, 0))
    return 1;
/*
 * Insert the string count times.
 */
  for(i=0; i<count; i++) {
    if(gl_add_string_to_line(gl, gl->cutbuf))
      return 1;
  };
/*
 * Switch back to command mode if necessary.
 */
  if(was_command)
    gl_vi_command_mode(gl);
  return 0;
}

#ifdef USE_SIGWINCH
/*.......................................................................
 * Respond to the receipt of a window change signal.
 *
 * Input:
 *  gl     GetLine *  The resource object of this library.
 *  redisplay  int    If true redisplay the current line after
 *                    getting the new window size.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int gl_resize_terminal(GetLine *gl, int redisplay)
{
  int lines_used;       /* The number of lines currently in use */
  struct winsize size;  /* The new size information */
  int i;
/*
 * Record the fact that the sigwinch signal has been noted.
 */
  if(gl_pending_signal == SIGWINCH)
    gl_pending_signal = -1;
/*
 * Query the new terminal window size. Ignore invalid responses.
 */
  if(ioctl(gl->output_fd, TIOCGWINSZ, &size) == 0 &&
     size.ws_row > 0 && size.ws_col > 0) {
/*
 * Redisplay the input line?
 */
    if(redisplay) {
/*
 * How many lines are currently displayed.
 */
      lines_used = (gl_displayed_string_width(gl,gl->line,-1,gl->prompt_len) +
		    gl->prompt_len + gl->ncolumn - 1) / gl->ncolumn;
/*
 * Move to the cursor to the start of the line.
 */
      for(i=1; i<lines_used; i++) {
	if(gl_output_control_sequence(gl, 1, gl->up))
	  return 1;
      };
      if(gl_output_control_sequence(gl, 1, gl->bol))
	return 1;
/*
 * Clear to the end of the terminal.
 */
      if(gl_output_control_sequence(gl, size.ws_row, gl->clear_eod))
	return 1;
/*
 * Record the fact that the cursor is now at the beginning of the line.
 */
      gl->term_curpos = 0;
    };
/*
 * Update the recorded window size.
 */
    gl->nline = size.ws_row;
    gl->ncolumn = size.ws_col;
  };
/*
 * Redisplay the line?
 */
  return redisplay ? gl_redisplay(gl,1) : 0;
}
#endif

/*.......................................................................
 * This is the action function that recalls the previous line in the
 * history buffer.
 */
static KT_KEY_FN(gl_up_history)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * Forget any previous recall session.
 */
  gl->preload_id = 0;
/*
 * We don't want a search prefix for this function.
 */
  if(_glh_search_prefix(gl->glh, gl->line, 0))
    return 1;
/*
 * Recall the count'th next older line in the history list. If the first one
 * fails we can return since nothing has changed otherwise we must continue
 * and update the line state.
 */
  if(_glh_find_backwards(gl->glh, gl->line, gl->linelen) == NULL)
    return 0;
  while(--count && _glh_find_backwards(gl->glh, gl->line, gl->linelen))
    ;
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange to have the cursor placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * This is the action function that recalls the next line in the
 * history buffer.
 */
static KT_KEY_FN(gl_down_history)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * If no search is currently in progress continue a previous recall
 * session from a previous entered line if possible.
 */
  if(_glh_line_id(gl->glh, 0) == 0 && gl->preload_id) {
    _glh_recall_line(gl->glh, gl->preload_id, gl->line, gl->linelen);
    gl->preload_id = 0;
  } else {
/*
 * We don't want a search prefix for this function.
 */
    if(_glh_search_prefix(gl->glh, gl->line, 0))
      return 1;
/*
 * Recall the count'th next newer line in the history list. If the first one
 * fails we can return since nothing has changed otherwise we must continue
 * and update the line state.
 */
    if(_glh_find_forwards(gl->glh, gl->line, gl->linelen) == NULL)
      return 0;
    while(--count && _glh_find_forwards(gl->glh, gl->line, gl->linelen))
      ;
  };
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange to have the cursor placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * This is the action function that recalls the previous line in the
 * history buffer whos prefix matches the characters that currently
 * precede the cursor. By setting count=-1, this can be used internally
 * to force searching for the prefix used in the last search.
 */
static KT_KEY_FN(gl_history_search_backward)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * Forget any previous recall session.
 */
  gl->preload_id = 0;
/*
 * If the previous thing that the user did wasn't to execute a history
 * search function, set the search prefix equal to the string that
 * precedes the cursor. In vi command mode include the character that
 * is under the cursor in the string. If count<0 force a repeat search
 * even if the last command wasn't a history command.
 */
  if(gl->last_search != gl->keyseq_count - 1 && count>=0 &&
     _glh_search_prefix(gl->glh, gl->line, gl->buff_curpos +
			(gl->editor==GL_VI_MODE && gl->ntotal>0)))
    return 1;
/*
 * Record the key sequence number in which this search function is
 * being executed, so that the next call to this function or
 * gl_history_search_forward() knows if any other operations
 * were performed in between.
 */
  gl->last_search = gl->keyseq_count;
/*
 * Search backwards for a match to the part of the line which precedes the
 * cursor.
 */
  if(_glh_find_backwards(gl->glh, gl->line, gl->linelen) == NULL)
    return 0;
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange to have the cursor placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * This is the action function that recalls the previous line in the
 * history buffer who's prefix matches that specified in an earlier call
 * to gl_history_search_backward() or gl_history_search_forward().
 */
static KT_KEY_FN(gl_history_re_search_backward)
{
  return gl_history_search_backward(gl, -1);
}

/*.......................................................................
 * This is the action function that recalls the next line in the
 * history buffer who's prefix matches that specified in the earlier call
 * to gl_history_search_backward) which started the history search.
 * By setting count=-1, this can be used internally to force searching
 * for the prefix used in the last search.
 */
static KT_KEY_FN(gl_history_search_forward)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * If the previous thing that the user did wasn't to execute a history
 * search function, set the search prefix equal to the string that
 * precedes the cursor. In vi command mode include the character that
 * is under the cursor in the string. If count<0 force a repeat search
 * even if the last command wasn't a history command.
 */
  if(gl->last_search != gl->keyseq_count - 1 && count>=0 &&
     _glh_search_prefix(gl->glh, gl->line, gl->buff_curpos +
			(gl->editor==GL_VI_MODE && gl->ntotal>0)))
    return 1;
/*
 * Record the key sequence number in which this search function is
 * being executed, so that the next call to this function or
 * gl_history_search_backward() knows if any other operations
 * were performed in between.
 */
  gl->last_search = gl->keyseq_count;
/*
 * Search forwards for the next matching line.
 */
  if(_glh_find_forwards(gl->glh, gl->line, gl->linelen) == NULL)
    return 0;
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange for the cursor to be placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * This is the action function that recalls the next line in the
 * history buffer who's prefix matches that specified in an earlier call
 * to gl_history_search_backward() or gl_history_search_forward().
 */
static KT_KEY_FN(gl_history_re_search_forward)
{
  return gl_history_search_forward(gl, -1);
}

/*.......................................................................
 * This is the tab completion function that completes the filename that
 * precedes the cursor position.
 */
static KT_KEY_FN(gl_complete_word)
{
  CplMatches *matches;    /* The possible completions */
  int redisplay=0;        /* True if the whole line needs to be redrawn */
  int suffix_len;         /* The length of the completion extension */
  int cont_len;           /* The length of any continuation suffix */
  int nextra;             /* The number of characters being added to the */
                          /*  total length of the line. */
  int buff_pos;           /* The buffer index at which the completion is */
                          /*  to be inserted. */
/*
 * In vi command mode, switch to append mode so that the character below
 * the character is included in the completion (otherwise people can't
 * complete at the end of the line).
 */
  if(gl->vi.command && gl_vi_append(gl, 0))
    return 1;
/*
 * Get the cursor position at which the completion is to be inserted.
 */
  buff_pos = gl->buff_curpos;
/*
 * Perform the completion.
 */
  matches = cpl_complete_word(gl->cpl, gl->line, gl->buff_curpos, gl->cpl_data,
			      gl->cpl_fn);
  if(!matches) {
    if(gl->echo &&
       fprintf(gl->output_fp, "\r\n%s\n", cpl_last_error(gl->cpl)) < 0)
      return 1;
    gl->term_curpos = 0;
    redisplay = 1;
/*
 * Are there any completions?
 */
  } else if(matches->nmatch >= 1) {
/*
 * If there any ambiguous matches, report them, starting on a new line.
 */
    if(matches->nmatch > 1 && gl->echo) {
      if(fprintf(gl->output_fp, "\r\n") < 0)
	return 1;
      cpl_list_completions(matches, gl->output_fp, gl->ncolumn);
      redisplay = 1;
    };
/*
 * If the callback called gl_change_prompt(), we will need to redisplay
 * the whole line.
 */
    if(gl->prompt_changed)
      redisplay = 1;
/*
 * Get the length of the suffix and any continuation suffix to add to it.
 */
    suffix_len = strlen(matches->suffix);
    cont_len = strlen(matches->cont_suffix);
/*
 * If there is an unambiguous match, and the continuation suffix ends in
 * a newline, strip that newline and arrange to have getline return
 * after this action function returns.
 */
    if(matches->nmatch==1 && cont_len > 0 &&
       matches->cont_suffix[cont_len - 1] == '\n') {
      cont_len--;
      if(gl_newline(gl, 1))
	return 1;
    };
/*
 * Work out the number of characters that are to be added.
 */
    nextra = suffix_len + cont_len;
/*
 * Is there anything to be added?
 */
    if(nextra) {
/*
 * Will there be space for the expansion in the line buffer?
 */
      if(gl->ntotal + nextra < gl->linelen) {
/*
 * Make room to insert the filename extension.
 */
	memmove(gl->line + gl->buff_curpos + nextra, gl->line + gl->buff_curpos,
		gl->ntotal - gl->buff_curpos);
/*
 * Insert the filename extension.
 */
	memcpy(gl->line + gl->buff_curpos, matches->suffix, suffix_len);
/*
 * Add the terminating characters.
 */
	memcpy(gl->line + gl->buff_curpos + suffix_len, matches->cont_suffix,
	       cont_len);
/*
 * Record the increased length of the line.
 */
	gl->ntotal += nextra;
/*
 * Place the cursor position at the end of the completion.
 */
	gl->buff_curpos += nextra;
/*
 * Terminate the extended line.
 */
	gl->line[gl->ntotal] = '\0';
/*
 * If we don't have to redisplay the whole line, redisplay the part
 * of the line which follows the original cursor position, and place
 * the cursor at the end of the completion.
 */
	if(!redisplay) {
	  if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod) ||
	     gl_output_string(gl, gl->line + buff_pos, '\0') ||
	     gl_place_cursor(gl, gl->buff_curpos))
	    return 1;
	};
      } else {
	fprintf(stderr,
		"\r\nInsufficient room in line for file completion.\r\n");
	redisplay = 1;
      };
    };
  };
/*
 * Redisplay the whole line?
 */
  if(redisplay) {
    gl->term_curpos = 0;
    if(gl_redisplay(gl,1))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * This is the function that expands the filename that precedes the
 * cursor position. It expands ~user/ expressions, $envvar expressions,
 * and wildcards.
 */
static KT_KEY_FN(gl_expand_filename)
{
  char *start_path;      /* The pointer to the start of the pathname in */
                         /*  gl->line[]. */
  FileExpansion *result; /* The results of the filename expansion */
  int pathlen;           /* The length of the pathname being expanded */
  int redisplay=0;       /* True if the whole line needs to be redrawn */
  int length;            /* The number of characters needed to display the */
                         /*  expanded files. */
  int nextra;            /* The number of characters to be added */
  int i,j;
/*
 * In vi command mode, switch to append mode so that the character below
 * the character is included in the completion (otherwise people can't
 * complete at the end of the line).
 */
  if(gl->vi.command && gl_vi_append(gl, 0))
    return 1;
/*
 * Locate the start of the filename that precedes the cursor position.
 */
  start_path = _pu_start_of_path(gl->line,
				 gl->buff_curpos > 0 ? gl->buff_curpos : 0);
  if(!start_path)
    return 1;
/*
 * Get the length of the string that is to be expanded.
 */
  pathlen = gl->buff_curpos - (start_path - gl->line);
/*
 * Attempt to expand it.
 */
  result = ef_expand_file(gl->ef, start_path, pathlen);
/*
 * If there was an error, report the error on a new line, then redraw
 * the original line.
 */
  if(!result) {
    if(gl->echo &&
       fprintf(gl->output_fp, "\r\n%s\n", ef_last_error(gl->ef)) < 0)
      return 1;
    gl->term_curpos = 0;
    return gl_redisplay(gl,1);
  };
/*
 * If no files matched, report this as well.
 */
  if(result->nfile == 0 || !result->exists) {
    if(gl->echo && fprintf(gl->output_fp, "\r\nNo files match.\n") < 0)
      return 1;
    gl->term_curpos = 0;
    return gl_redisplay(gl,1);
  };
/*
 * If in vi command mode, preserve the current line for potential use by
 * vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Work out how much space we will need to display all of the matching
 * filenames, taking account of the space that we need to place between
 * them, and the number of additional '\' characters needed to escape
 * spaces, tabs and backslash characters in the individual filenames.
 */
  length = 0;
  for(i=0; i<result->nfile; i++) {
    char *file = result->files[i];
    while(*file) {
      int c = *file++;
      switch(c) {
      case ' ': case '\t': case '\\': case '*': case '?': case '[':
	length++;  /* Count extra backslash characters */
      };
      length++;    /* Count the character itself */
    };
    length++;      /* Count the space that follows each filename */
  };
/*
 * Work out the number of characters that are to be added.
 */
  nextra = length - pathlen;
/*
 * Will there be space for the expansion in the line buffer?
 */
  if(gl->ntotal + nextra >= gl->linelen) {
    fprintf(stderr, "\r\nInsufficient room in line for file expansion.\r\n");
    redisplay = 1;
  } else {
/*
 * Do we need to move the part of the line that followed the unexpanded
 * filename?
 */
    if(nextra != 0) {
      memmove(gl->line + gl->buff_curpos + nextra, gl->line + gl->buff_curpos,
	      gl->ntotal - gl->buff_curpos);
    };
/*
 * Insert the filenames, separated by spaces, and with internal spaces,
 * tabs and backslashes escaped with backslashes.
 */
    for(i=0,j=start_path - gl->line; i<result->nfile; i++) {
      char *file = result->files[i];
      while(*file) {
	int c = *file++;
	switch(c) {
	case ' ': case '\t': case '\\': case '*': case '?': case '[':
	  gl->line[j++] = '\\';
	};
	gl->line[j++] = c;
      };
      gl->line[j++] = ' ';
    };
/*
 * Record the increased length of the line.
 */
    gl->ntotal += nextra;
/*
 * Place the cursor position at the end of the expansion.
 */
    gl->buff_curpos += nextra;
/*
 * Terminate the extended line.
 */
    gl->line[gl->ntotal] = '\0';
  };
/*
 * Display the whole line on a new line?
 */
  if(redisplay) {
    gl->term_curpos = 0;
    return gl_redisplay(gl,1);
  };
/*
 * Otherwise redisplay the part of the line which follows the start of
 * the original filename.
 */
  if(gl_set_term_curpos(gl, gl_buff_curpos_to_term_curpos(gl, start_path - gl->line)) ||
     gl_output_control_sequence(gl, gl->nline, gl->clear_eod) ||
     gl_output_string(gl, start_path, gl->line[gl->buff_curpos]))
    return 1;
/*
 * Restore the cursor position to the end of the expansion.
 */
  return gl_place_cursor(gl, gl->buff_curpos);
}

/*.......................................................................
 * This is the action function that lists glob expansions of the
 * filename that precedes the cursor position. It expands ~user/
 * expressions, $envvar expressions, and wildcards.
 */
static KT_KEY_FN(gl_list_glob)
{
  char *start_path;      /* The pointer to the start of the pathname in */
                         /*  gl->line[]. */
  FileExpansion *result; /* The results of the filename expansion */
  int pathlen;           /* The length of the pathname being expanded */
/*
 * Locate the start of the filename that precedes the cursor position.
 */
  start_path = _pu_start_of_path(gl->line,
				 gl->buff_curpos > 0 ? gl->buff_curpos : 0);
  if(!start_path)
    return 1;
/*
 * Get the length of the string that is to be expanded.
 */
  pathlen = gl->buff_curpos - (start_path - gl->line);
/*
 * Attempt to expand it.
 */
  result = ef_expand_file(gl->ef, start_path, pathlen);
/*
 * If there was an error, report the error.
 */
  if(!result) {
    if(gl->echo &&
       fprintf(gl->output_fp, "\r\n%s\n", ef_last_error(gl->ef)) < 0)
      return 1;
/*
 * If no files matched, report this as well.
 */
  } else if(result->nfile == 0 || !result->exists) {
    if(gl->echo && fprintf(gl->output_fp, "\r\nNo files match.\n") < 0)
      return 1;
/*
 * List the matching expansions.
 */
  } else if(gl->echo) {
    if(fprintf(gl->output_fp, "\r\n") < 0)
      return 1;
    ef_list_expansions(result, gl->output_fp, gl->ncolumn);
  };
/*
 * Redisplay the line being edited.
 */
  gl->term_curpos = 0;
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * Return non-zero if a character should be considered a part of a word.
 *
 * Input:
 *  c       int  The character to be tested.
 * Output:
 *  return  int  True if the character should be considered part of a word.
 */
static int gl_is_word_char(int c)
{
  return isalnum((int)(unsigned char)c) || strchr(GL_WORD_CHARS, c) != NULL;
}

/*.......................................................................
 * Override the builtin file-completion callback that is bound to the
 * "complete_word" action function.
 *
 * Input:
 *  gl            GetLine *  The resource object of the command-line input
 *                           module.
 *  data             void *  This is passed to match_fn() whenever it is
 *                           called. It could, for example, point to a
 *                           symbol table where match_fn() could look
 *                           for possible completions.
 *  match_fn   CplMatchFn *  The function that will identify the prefix
 *                           to be completed from the input line, and
 *                           report matching symbols.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
int gl_customize_completion(GetLine *gl, void *data, CplMatchFn *match_fn)
{
/*
 * Check the arguments.
 */
  if(!gl || !match_fn) {
    fprintf(stderr, "gl_customize_completion: NULL argument(s).\n");
    return 1;
  };
/*
 * Record the new completion function and its callback data.
 */
  gl->cpl_fn = match_fn;
  gl->cpl_data = data;
  return 0;
}

/*.......................................................................
 * Change the terminal (or stream) that getline interacts with.
 *
 * Input:
 *  gl            GetLine *  The resource object of the command-line input
 *                           module.
 *  input_fp         FILE *  The stdio stream to read from.
 *  output_fp        FILE *  The stdio stream to write to.
 *  term             char *  The terminal type. This can be NULL if
 *                           either or both of input_fp and output_fp don't
 *                           refer to a terminal. Otherwise it should refer
 *                           to an entry in the terminal information database.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
int gl_change_terminal(GetLine *gl, FILE *input_fp, FILE *output_fp,
		       const char *term)
{
  int is_term = 0;   /* True if both input_fd and output_fd are associated */
                     /*  with a terminal. */
/*
 * Require that input_fp and output_fp both be valid.
 */
  if(!input_fp || !output_fp) {
    fprintf(stderr, "\r\ngl_change_terminal: Bad input/output stream(s).\n");
    return 1;
  };
/*
 * If we are displacing a previous terminal, remove it from the list
 * of fds being watched.
 */
#ifdef HAVE_SELECT
  if(gl->input_fd >= 0)
    FD_CLR(gl->input_fd, &gl->rfds);
#endif
/*
 * Record the file descriptors and streams.
 */
  gl->input_fp = input_fp;
  gl->input_fd = fileno(input_fp);
  gl->output_fp = output_fp;
  gl->output_fd = fileno(output_fp);
/*
 * Make sure that the file descriptor will be visible in the set to
 * be watched.
 */
#ifdef HAVE_SELECT
  FD_SET(gl->input_fd, &gl->rfds);
  if(gl->input_fd > gl->max_fd)
    gl->max_fd = gl->input_fd;
#endif
/*
 * Disable terminal interaction until we have enough info to interact
 * with the terminal.
 */
  gl->is_term = 0;
/*
 * For terminal editing, we need both output_fd and input_fd to refer to
 * a terminal. While we can't verify that they both point to the same
 * terminal, we can verify that they point to terminals.
 */
  if (! gl->is_net)
    is_term = isatty(gl->input_fd) && isatty(gl->output_fd);
/*
 * If we are interacting with a terminal and no terminal type has been
 * specified, treat it as a generic ANSI terminal.
 */
  if(is_term && !term)
    term = "ansi";
/*
 * Make a copy of the terminal type string.
 */
  if(term != gl->term) {
/*
 * Delete any old terminal type string.
 */
    if(gl->term) {
      free(gl->term);
      gl->term = NULL;
    };
/*
 * Make a copy of the new terminal-type string, if any.
 */
    if(term) {
      gl->term = (char *) malloc(strlen(term)+1);
      if(gl->term)
	strcpy(gl->term, term);
    };
  };
/*
 * Clear any terminal-specific key bindings that were taken from the
 * settings of the last terminal.
 */
  _kt_clear_bindings(gl->bindings, KTB_TERM);
/*
 * If we have a terminal install new bindings for it.
 */
  if(is_term) {
/*
 * Get the current settings of the terminal.
 */
    if(tcgetattr(gl->input_fd, &gl->oldattr)) {
      fprintf(stderr, "\r\ngl_change_terminal: tcgetattr error: %s\n",
	      strerror(errno));
      return 1;
    };
/*
 * Lookup the terminal control string and size information.
 */
    if(gl_control_strings(gl, term))
      return 1;
/*
 * We now have enough info to interact with the terminal.
 */
    gl->is_term = 1;
/*
 * Bind terminal-specific keys.
 */
    if(gl_bind_terminal_keys(gl))
      return 1;
  };

  if (gl->is_net) {
/*
 * Lookup the terminal control string and size information.
 */
    if(gl_control_strings(gl, term))
      return 1;
/*
 * Bind terminal-specific keys.
 */
    if(gl_bind_terminal_keys(gl))
      return 1;
  };

  return 0;
}

/*.......................................................................
 * Set up terminal-specific key bindings.
 *
 * Input:
 *  gl            GetLine *  The resource object of the command-line input
 *                           module.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Error.
 */
static int gl_bind_terminal_keys(GetLine *gl)
{
/*
 * Install key-bindings for the special terminal characters.
 */
  if(gl_bind_control_char(gl, KTB_TERM, gl->oldattr.c_cc[VINTR],
			  "user-interrupt") ||
     gl_bind_control_char(gl, KTB_TERM, gl->oldattr.c_cc[VQUIT], "abort") ||
     gl_bind_control_char(gl, KTB_TERM, gl->oldattr.c_cc[VSUSP], "suspend"))
    return 1;
/*
 * In vi-mode, arrange for the above characters to be seen in command
 * mode.
 */
  if(gl->editor == GL_VI_MODE) {
    if(gl_bind_control_char(gl, KTB_TERM, MAKE_META(gl->oldattr.c_cc[VINTR]),
			    "user-interrupt") ||
       gl_bind_control_char(gl, KTB_TERM, MAKE_META(gl->oldattr.c_cc[VQUIT]),
			    "abort") ||
       gl_bind_control_char(gl, KTB_TERM, MAKE_META(gl->oldattr.c_cc[VSUSP]),
			    "suspend"))
      return 1;
  };
/*
 * Non-universal special keys.
 */
#ifdef VLNEXT
  if(gl_bind_control_char(gl, KTB_TERM, gl->oldattr.c_cc[VLNEXT],
			  "literal-next"))
    return 1;
#else
  if(_kt_set_keybinding(gl->bindings, KTB_TERM, "^V", "literal-next"))
    return 1;
#endif
/*
 * Bind action functions to the terminal-specific arrow keys
 * looked up by gl_control_strings().
 */
  if(_gl_bind_arrow_keys(gl))
    return 1;
  return 0;
}

/*.......................................................................
 * This function is normally bound to control-D. When it is invoked within
 * a line it deletes the character which follows the cursor. When invoked
 * at the end of the line it lists possible file completions, and when
 * invoked on an empty line it causes gl_get_line() to return EOF. This
 * function emulates the one that is normally bound to control-D by tcsh.
 */
static KT_KEY_FN(gl_del_char_or_list_or_eof)
{
/*
 * If we have an empty line arrange to return EOF.
 */
  if(gl->ntotal < 1) {
    return 1;
/*
 * If we are at the end of the line list possible completions.
 */
  } else if(gl->buff_curpos >= gl->ntotal) {
/*
 * Get the list of possible completions.
 */
    CplMatches *matches = cpl_complete_word(gl->cpl, gl->line, gl->buff_curpos,
					    gl->cpl_data, gl->cpl_fn);
    if(!matches) {
      if(gl->echo &&
	 fprintf(gl->output_fp, "\r\n%s\n", cpl_last_error(gl->cpl)) < 0)
	return 1;
      gl->term_curpos = 0;
/*
 * List the matches.
 */
    } else if(matches->nmatch > 0 && gl->echo) {
      if(fprintf(gl->output_fp, "\r\n") < 0)
	return 1;
      cpl_list_completions(matches, gl->output_fp, gl->ncolumn);
    };
/*
 * Redisplay the line unchanged.
 */
    return gl_redisplay(gl,1);
/*
 * Within the line delete the character that follows the cursor.
 */
  } else {
/*
 * If in vi command mode, first preserve the current line for potential use
 * by vi-undo.
 */
    gl_save_for_undo(gl);
/*
 * Delete 'count' characters.
 */
    return gl_forward_delete_char(gl, count);
  };
}

/*.......................................................................
 * This function is normally bound to control-D in vi mode. When it is
 * invoked within a line it lists possible file completions, and when
 * invoked on an empty line it causes gl_get_line() to return EOF. This
 * function emulates the one that is normally bound to control-D by tcsh.
 */
static KT_KEY_FN(gl_list_or_eof)
{
/*
 * If we have an empty line arrange to return EOF.
 */
  if(gl->ntotal < 1) {
    return 1;
/*
 * Otherwise list possible completions.
 */
  } else {
/*
 * Get the list of possible completions.
 */
    CplMatches *matches = cpl_complete_word(gl->cpl, gl->line, gl->buff_curpos,
					    gl->cpl_data, gl->cpl_fn);
    if(!matches) {
      if(gl->echo &&
	 fprintf(gl->output_fp, "\r\n%s\n", cpl_last_error(gl->cpl)) < 0)
	return 1;
      gl->term_curpos = 0;
/*
 * List the matches.
 */
    } else if(matches->nmatch > 0 && gl->echo) {
      if(fprintf(gl->output_fp, "\r\n") < 0)
	return 1;
      cpl_list_completions(matches, gl->output_fp, gl->ncolumn);
    };
/*
 * Redisplay the line unchanged.
 */
    return gl_redisplay(gl,1);
  };
}

/*.......................................................................
 * Where the user has used the symbolic arrow-key names to specify
 * arrow key bindings, bind the specified action functions to the default
 * and terminal specific arrow key sequences.
 *
 * Input:
 *  gl     GetLine *   The getline resource object.
 * Output:
 *  return     int     0 - OK.
 *                     1 - Error.
 */
static int _gl_bind_arrow_keys(GetLine *gl)
{
/*
 * Process each of the arrow keys.
 */
  if(_gl_rebind_arrow_key(gl->bindings, "up", gl->u_arrow, "^[[A", "^[OA") ||
     _gl_rebind_arrow_key(gl->bindings, "down", gl->d_arrow, "^[[B", "^[OB") ||
     _gl_rebind_arrow_key(gl->bindings, "left", gl->l_arrow, "^[[D", "^[OD") ||
     _gl_rebind_arrow_key(gl->bindings, "right", gl->r_arrow, "^[[C", "^[OC"))
    return 1;
  return 0;
}

/*.......................................................................
 * Lookup the action function of a symbolic arrow-key binding, and bind
 * it to the terminal-specific and default arrow-key sequences. Note that
 * we don't trust the terminal-specified key sequences to be correct.
 * The main reason for this is that on some machines the xterm terminfo
 * entry is for hardware X-terminals, rather than xterm terminal emulators
 * and the two terminal types emit different character sequences when the
 * their cursor keys are pressed. As a result we also supply a couple
 * of default key sequences.
 *
 * Input:
 *  bindings     KeyTab *   The table of key bindings.
 *  name           char *   The symbolic name of the arrow key.
 *  term_seq       char *   The terminal-specific arrow-key sequence.
 *  def_seq1       char *   The first default arrow-key sequence.
 *  def_seq2       char *   The second arrow-key sequence.
 * Output:
 *  return          int     0 - OK.
 *                          1 - Error.
 */
static int _gl_rebind_arrow_key(KeyTab *bindings, const char *name,
				const char *term_seq, const char *def_seq1,
				const char *def_seq2)
{
  int first,last;  /* The indexes of the first and last matching entries */
/*
 * Lookup the key binding for the symbolic name of the arrow key. This
 * will either be the default action, or a user provided one.
 */
  if(_kt_lookup_keybinding(bindings, name, strlen(name), &first, &last)
     == KT_EXACT_MATCH) {
/*
 * Get the action function.
 */
    KtKeyFn *key_fn = bindings->table[first].keyfn;
/*
 * Bind this to each of the specified key sequences.
 */
    if((term_seq && _kt_set_keyfn(bindings, KTB_TERM, term_seq, key_fn)) ||
       (def_seq1 && _kt_set_keyfn(bindings, KTB_NORM, def_seq1, key_fn)) ||
       (def_seq2 && _kt_set_keyfn(bindings, KTB_NORM, def_seq2, key_fn)))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Read getline configuration information from a given file.
 *
 * Input:
 *  gl           GetLine *  The getline resource object.
 *  filename  const char *  The name of the file to read configuration
 *                          information from. The contents of this file
 *                          are as described in the gl_get_line(3) man
 *                          page for the default ~/.teclarc configuration
 *                          file.
 *  who         KtBinder    Who bindings are to be installed for.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Irrecoverable error.
 */
static int _gl_read_config_file(GetLine *gl, const char *filename, KtBinder who)
{
  FileExpansion *expansion; /* The expansion of the filename */
  FILE *fp;                 /* The opened file */
  int waserr = 0;           /* True if an error occurred while reading */
  int lineno = 1;           /* The line number being processed */
/*
 * Check the arguments.
 */
  if(!gl || !filename) {
    fprintf(stderr, "_gl_read_config_file: Invalid arguments.\n");
    return 1;
  };
/*
 * Expand the filename.
 */
  expansion = ef_expand_file(gl->ef, filename, -1);
  if(!expansion) {
    fprintf(stderr, "Unable to expand %s (%s).\n", filename,
            ef_last_error(gl->ef));
    return 1;
  };
/*
 * Attempt to open the file.
 */
  fp = fopen(expansion->files[0], "r");
/*
 * It isn't an error for there to be no configuration file.
 */
  if(!fp)
    return 0;
/*
 * Parse the contents of the file.
 */
  while(!waserr && !feof(fp))
    waserr = _gl_parse_config_line(gl, fp, glc_file_getc, filename, who,
				   &lineno);
/*
 * Bind action functions to the terminal-specific arrow keys.
 */
  if(_gl_bind_arrow_keys(gl))
    return 1;
/*
 * Clean up.
 */
  (void) fclose(fp);
  return waserr;
}

/*.......................................................................
 * Read GetLine configuration information from a string. The contents of
 * the string are the same as those described in the gl_get_line(3)
 * man page for the contents of the ~/.teclarc configuration file.
 */
static int _gl_read_config_string(GetLine *gl, const char *buffer, KtBinder who)
{
  const char *bptr;         /* A pointer into buffer[] */
  int waserr = 0;           /* True if an error occurred while reading */
  int lineno = 1;           /* The line number being processed */
/*
 * Check the arguments.
 */
  if(!gl || !buffer) {
    fprintf(stderr, "_gl_read_config_string: Invalid arguments.\n");
    return 1;
  };
/*
 * Get a pointer to the start of the buffer.
 */
  bptr = buffer;
/*
 * Parse the contents of the buffer.
 */
  while(!waserr && *bptr)
    waserr = _gl_parse_config_line(gl, &bptr, glc_buff_getc, "", who, &lineno);
/*
 * Bind action functions to the terminal-specific arrow keys.
 */
  if(_gl_bind_arrow_keys(gl))
    return 1;
  return waserr;
}

/*.......................................................................
 * Parse the next line of a getline configuration file.
 *
 * Input:
 *  gl         GetLine *  The getline resource object.
 *  stream        void *  The pointer representing the stream to be read
 *                        by getc_fn().
 *  getc_fn  GlcGetcFn *  A callback function which when called with
 *                       'stream' as its argument, returns the next
 *                        unread character from the stream.
 *  origin  const char *  The name of the entity being read (eg. a
 *                        file name).
 *  who       KtBinder    Who bindings are to be installed for.
 * Input/Output:
 *  lineno         int *  The line number being processed is to be
 *                        maintained in *lineno.
 * Output:
 *  return         int    0 - OK.
 *                        1 - Irrecoverable error.
 */
static int _gl_parse_config_line(GetLine *gl, void *stream, GlcGetcFn *getc_fn,
				 const char *origin, KtBinder who, int *lineno)
{
  char buffer[GL_CONF_BUFLEN+1];  /* The input line buffer */
  char *argv[GL_CONF_MAXARG];     /* The argument list */
  int argc = 0;                   /* The number of arguments in argv[] */
  int c;                          /* A character from the file */
  int escaped = 0;                /* True if the next character is escaped */
  int i;
/*
 * Skip spaces and tabs.
 */
  do c = getc_fn(stream); while(c==' ' || c=='\t');
/*
 * Comments extend to the end of the line.
 */
  if(c=='#')
    do c = getc_fn(stream); while(c != '\n' && c != EOF);
/*
 * Ignore empty lines.
 */
  if(c=='\n' || c==EOF) {
    (*lineno)++;
    return 0;
  };
/*
 * Record the buffer location of the start of the first argument.
 */
  argv[argc] = buffer;
/*
 * Read the rest of the line, stopping early if a comment is seen, or
 * the buffer overflows, and replacing sequences of spaces with a
 * '\0', and recording the thus terminated string as an argument.
 */
  i = 0;
  while(i<GL_CONF_BUFLEN) {
/*
 * Did we hit the end of the latest argument?
 */
    if(c==EOF || (!escaped && (c==' ' || c=='\n' || c=='\t' || c=='#'))) {
/*
 * Terminate the argument.
 */
      buffer[i++] = '\0';
      argc++;
/*
 * Skip spaces and tabs.
 */
      while(c==' ' || c=='\t')
	c = getc_fn(stream);
/*
 * If we hit the end of the line, or the start of a comment, exit the loop.
 */
      if(c==EOF || c=='\n' || c=='#')
	break;
/*
 * Start recording the next argument.
 */
      if(argc >= GL_CONF_MAXARG) {
	fprintf(stderr, "%s:%d: Too many arguments.\n", origin, *lineno);
	do c = getc_fn(stream); while(c != '\n' && c != EOF); /* Skip past eol */
	return 0;
      };
      argv[argc] = buffer + i;
/*
 * The next character was preceded by spaces, so it isn't escaped.
 */
      escaped = 0;
    } else {
/*
 * If we hit an unescaped backslash, this means that we should arrange
 * to treat the next character like a simple alphabetical character.
 */
      if(c=='\\' && !escaped) {
	escaped = 1;
/*
 * Splice lines where the newline is escaped.
 */
      } else if(c=='\n' && escaped) {
	(*lineno)++;
/*
 * Record a normal character, preserving any preceding backslash.
 */
      } else {
	if(escaped)
	  buffer[i++] = '\\';
	if(i>=GL_CONF_BUFLEN)
	  break;
	escaped = 0;
	buffer[i++] = c;
      };
/*
 * Get the next character.
 */
      c = getc_fn(stream);
    };
  };
/*
 * Did the buffer overflow?
 */
  if(i>=GL_CONF_BUFLEN) {
    fprintf(stderr, "%s:%d: Line too long.\n", origin, *lineno);
    return 0;
  };
/*
 * The first argument should be a command name.
 */
  if(strcmp(argv[0], "bind") == 0) {
    const char *action = NULL; /* A NULL action removes a keybinding */
    const char *keyseq = NULL;
    switch(argc) {
    case 3:
      action = argv[2];
    case 2:              /* Note the intentional fallthrough */
      keyseq = argv[1];
/*
 * Attempt to record the new keybinding.
 */
      if(_kt_set_keybinding(gl->bindings, who, keyseq, action)) {
	fprintf(stderr, "The error occurred at line %d of %s.\n", *lineno,
		origin);
      };
      break;
    default:
      fprintf(stderr, "%s:%d: Wrong number of arguments.\n", origin, *lineno);
    };
  } else if(strcmp(argv[0], "edit-mode") == 0) {
    if(argc == 2 && strcmp(argv[1], "emacs") == 0) {
      gl_change_editor(gl, GL_EMACS_MODE);
    } else if(argc == 2 && strcmp(argv[1], "vi") == 0) {
      gl_change_editor(gl, GL_VI_MODE);
    } else if(argc == 2 && strcmp(argv[1], "none") == 0) {
      gl_change_editor(gl, GL_NO_EDITOR);
    } else {
      fprintf(stderr, "%s:%d: The argument of editor should be vi or emacs.\n",
	      origin, *lineno);
    };
  } else if(strcmp(argv[0], "nobeep") == 0) {
    gl->silence_bell = 1;
  } else {
    fprintf(stderr, "%s:%d: Unknown command name '%s'.\n", origin, *lineno,
	    argv[0]);
  };
/*
 * Skip any trailing comment.
 */
  while(c != '\n' && c != EOF)
    c = getc_fn(stream);
  (*lineno)++;
  return 0;
}

/*.......................................................................
 * This is the _gl_parse_config_line() callback function which reads the
 * next character from a configuration file.
 */
static GLC_GETC_FN(glc_file_getc)
{
  return fgetc((FILE *) stream);
}

/*.......................................................................
 * This is the _gl_parse_config_line() callback function which reads the
 * next character from a buffer. Its stream argument is a pointer to a
 * variable which is, in turn, a pointer into the buffer being read from.
 */
static GLC_GETC_FN(glc_buff_getc)
{
  const char **lptr = (char const **) stream;
  return **lptr ? *(*lptr)++ : EOF;
}

/*.......................................................................
 * When this action is triggered, it arranges to temporarily read command
 * lines from the regular file whos name precedes the cursor.
 * The current line is first discarded.
 */
static KT_KEY_FN(gl_read_from_file)
{
  char *start_path;       /* The pointer to the start of the pathname in */
                          /*  gl->line[]. */
  FileExpansion *result;  /* The results of the filename expansion */
  int pathlen;            /* The length of the pathname being expanded */
  int error_reported = 0; /* True after an error has been reported */
/*
 * Locate the start of the filename that precedes the cursor position.
 */
  start_path = _pu_start_of_path(gl->line,
				 gl->buff_curpos > 0 ? gl->buff_curpos : 0);
  if(!start_path)
    return 1;
/*
 * Get the length of the pathname string.
 */
  pathlen = gl->buff_curpos - (start_path - gl->line);
/*
 * Attempt to expand the pathname.
 */
  result = ef_expand_file(gl->ef, start_path, pathlen);
/*
 * If there was an error, report the error on a new line, then redraw
 * the original line.
 */
  if(!result) {
    if(gl->echo &&
       fprintf(gl->output_fp, "\r\n%s\n", ef_last_error(gl->ef)) < 0)
      return 1;
    error_reported = 1;
/*
 * If no files matched, report this as well.
 */
  } else if(result->nfile == 0 || !result->exists) {
    if(gl->echo && fprintf(gl->output_fp, "\r\nNo files match.\n") < 0)
      return 1;
    error_reported = 1;
/*
 * Complain if more than one file matches.
 */
  } else if(result->nfile > 1) {
    if(gl->echo &&
       fprintf(gl->output_fp, "\r\nMore than one file matches.\n") < 0)
      return 1;
    error_reported = 1;
/*
 * Disallow input from anything but normal files. In principle we could
 * also support input from named pipes. Terminal files would be a problem
 * since we wouldn't know the terminal type, and other types of files
 * might cause the library to lock up.
 */
  } else if(!_pu_path_is_file(result->files[0])) {
    if(gl->echo && fprintf(gl->output_fp, "\r\nNot a normal file.\n") < 0)
      return 1;
    error_reported = 1;
  } else {
/*
 * Attempt to open and install the specified file for reading.
 */
    gl->file_fp = fopen(result->files[0], "r");
    if(!gl->file_fp) {
      if(gl->echo && fprintf(gl->output_fp, "\r\nUnable to open: %s\n",
		 result->files[0]) < 0)
	return 1;
      error_reported = 1;
    };
/*
 * Inform the user what is happening.
 */
    if(gl->echo && fprintf(gl->output_fp, "\r\n<Taking input from %s>\n",
			   result->files[0]) < 0)
      return 1;
  };
/*
 * If an error was reported, redisplay the current line.
 */
  if(error_reported) {
    gl->term_curpos = 0;
    return gl_redisplay(gl,1);
  };
  return 0;
}

/*.......................................................................
 * Close any temporary file that is being used for input.
 *
 * Input:
 *  gl     GetLine *  The getline resource object.
 */
static void gl_revert_input(GetLine *gl)
{
  if(gl->file_fp)
    fclose(gl->file_fp);
  gl->file_fp = NULL;
}

/*.......................................................................
 * This is the action function that recalls the oldest line in the
 * history buffer.
 */
static KT_KEY_FN(gl_beginning_of_history)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * Forget any previous recall session.
 */
  gl->preload_id = 0;
/*
 * Recall the next oldest line in the history list.
 */
  if(_glh_oldest_line(gl->glh, gl->line, gl->linelen) == NULL)
    return 0;
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange to have the cursor placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * If a history session is currently in progress, this action function
 * recalls the line that was being edited when the session started. If
 * no history session is in progress, it does nothing.
 */
static KT_KEY_FN(gl_end_of_history)
{
/*
 * In vi mode, switch to command mode, since the user is very
 * likely to want to move around newly recalled lines.
 */
  gl_vi_command_mode(gl);
/*
 * Forget any previous recall session.
 */
  gl->preload_id = 0;
/*
 * Recall the next oldest line in the history list.
 */
  if(_glh_current_line(gl->glh, gl->line, gl->linelen) == NULL)
    return 0;
/*
 * Record the number of characters in the new string.
 */
  gl->ntotal = strlen(gl->line);
/*
 * Arrange to have the cursor placed at the end of the new line.
 */
  gl->buff_curpos = strlen(gl->line);
/*
 * Erase and display the new line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * This action function is treated specially, in that its count argument
 * is set to the end keystroke of the keysequence that activated it.
 * It accumulates a numeric argument, adding one digit on each call in
 * which the last keystroke was a numeric digit.
 */
static KT_KEY_FN(gl_digit_argument)
{
/*
 * Was the last keystroke a digit?
 */
  int is_digit = isdigit((int)(unsigned char) count);
/*
 * In vi command mode, a lone '0' means goto-start-of-line.
 */
  if(gl->vi.command && gl->number < 0 && count == '0')
    return gl_beginning_of_line(gl, count);
/*
 * Are we starting to accumulate a new number?
 */
  if(gl->number < 0 || !is_digit)
    gl->number = 0;
/*
 * Was the last keystroke a digit?
 */
  if(is_digit) {
/*
 * Read the numeric value of the digit, without assuming ASCII.
 */
    int n;
    char s[2]; s[0] = count; s[1] = '\0';
    n = atoi(s);
/*
 * Append the new digit.
 */
    gl->number = gl->number * 10 + n;
  };
  return 0;
}

/*.......................................................................
 * The newline action function sets gl->endline to tell
 * gl_get_input_line() that the line is now complete.
 */
static KT_KEY_FN(gl_newline)
{
  GlhLineID id;    /* The last history line recalled while entering this line */
/*
 * Flag the line as ended.
 */
  gl->endline = 1;
/*
 * Record the next position in the history buffer, for potential
 * recall by an action function on the next call to gl_get_line().
 */
  id = _glh_line_id(gl->glh, 1);
  if(id)
    gl->preload_id = id;
  return 0;
}

/*.......................................................................
 * The 'repeat' action function sets gl->endline to tell
 * gl_get_input_line() that the line is now complete, and records the
 * ID of the next history line in gl->preload_id so that the next call
 * to gl_get_input_line() will preload the line with that history line.
 */
static KT_KEY_FN(gl_repeat_history)
{
  gl->endline = 1;
  gl->preload_id = _glh_line_id(gl->glh, 1);
  gl->preload_history = 1;
  return 0;
}

/*.......................................................................
 * Flush unwritten characters to the terminal.
 *
 * Input:
 *  gl     GetLine *  The getline resource object.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
static int gl_flush_output(GetLine *gl)
{
/*
 * Attempt to flush output to the terminal, restarting the output
 * if a signal is caught.
 */
  while(fflush(gl->output_fp) != 0) {
    if(errno!=EINTR)
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Change the style of editing to emulate a given editor.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  editor  GlEditor    The type of editor to emulate.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
static int gl_change_editor(GetLine *gl, GlEditor editor)
{
/*
 * Install the default key-bindings of the requested editor.
 */
  switch(editor) {
  case GL_EMACS_MODE:
    _kt_clear_bindings(gl->bindings, KTB_NORM);
    _kt_clear_bindings(gl->bindings, KTB_TERM);
    (void) _kt_add_bindings(gl->bindings, KTB_NORM, gl_emacs_bindings,
		   sizeof(gl_emacs_bindings)/sizeof(gl_emacs_bindings[0]));
    break;
  case GL_VI_MODE:
    _kt_clear_bindings(gl->bindings, KTB_NORM);
    _kt_clear_bindings(gl->bindings, KTB_TERM);
    (void) _kt_add_bindings(gl->bindings, KTB_NORM, gl_vi_bindings,
			    sizeof(gl_vi_bindings)/sizeof(gl_vi_bindings[0]));
    break;
  case GL_NO_EDITOR:
    break;
  default:
    fprintf(stderr, "gl_change_editor: Unknown editor.\n");
    return 1;
  };
/*
 * Record the new editing mode.
 */
  gl->editor = editor;
  gl->vi.command = 0;     /* Start in input mode */
  gl->insert_curpos = 0;
/*
 * Reinstate terminal-specific bindings.
 */
  if(gl->editor != GL_NO_EDITOR && gl->input_fp)
    (void) gl_bind_terminal_keys(gl);
  return 0;
}

/*.......................................................................
 * This is an action function that switches to editing using emacs bindings
 */
static KT_KEY_FN(gl_emacs_editing_mode)
{
  return gl_change_editor(gl, GL_EMACS_MODE);
}

/*.......................................................................
 * This is an action function that switches to editing using vi bindings
 */
static KT_KEY_FN(gl_vi_editing_mode)
{
  return gl_change_editor(gl, GL_VI_MODE);
}

/*.......................................................................
 * This is the action function that switches to insert mode.
 */
static KT_KEY_FN(gl_vi_insert)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Switch to vi insert mode.
 */
  gl->insert = 1;
  gl->vi.command = 0;
  gl->insert_curpos = gl->buff_curpos;
  return 0;
}

/*.......................................................................
 * This is an action function that switches to overwrite mode.
 */
static KT_KEY_FN(gl_vi_overwrite)
{
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * Switch to vi overwrite mode.
 */
  gl->insert = 0;
  gl->vi.command = 0;
  gl->insert_curpos = gl->buff_curpos;
  return 0;
}

/*.......................................................................
 * This action function toggles the case of the character under the
 * cursor.
 */
static KT_KEY_FN(gl_change_case)
{
  int i;
/*
 * Keep a record of the current insert mode and the cursor position.
 */
  int insert = gl->insert;
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
  gl_save_for_undo(gl);
/*
 * We want to overwrite the modified word.
 */
  gl->insert = 0;
/*
 * Toggle the case of 'count' characters.
 */
  for(i=0; i<count && gl->buff_curpos < gl->ntotal; i++) {
    char *cptr = gl->line + gl->buff_curpos++;
/*
 * Convert the character to upper case?
 */
    if(islower((int)(unsigned char) *cptr))
      *cptr = toupper((int) *cptr);
    else if(isupper((int)(unsigned char) *cptr))
      *cptr = tolower((int) *cptr);
/*
 * Write the possibly modified character back. Note that for non-modified
 * characters we want to do this as well, so as to advance the cursor.
 */
      if(gl_output_char(gl, *cptr, cptr[1]))
	return 1;
  };
/*
 * Restore the insertion mode.
 */
  gl->insert = insert;
  return gl_place_cursor(gl, gl->buff_curpos);	/* bounds check */
}

/*.......................................................................
 * This is the action function which implements the vi-style action which
 * moves the cursor to the start of the line, then switches to insert mode.
 */
static KT_KEY_FN(gl_vi_insert_at_bol)
{
  gl_save_for_undo(gl);
  return gl_beginning_of_line(gl, 0) ||
         gl_vi_insert(gl, 0);
         
}

/*.......................................................................
 * This is the action function which implements the vi-style action which
 * moves the cursor to the end of the line, then switches to insert mode
 * to allow text to be appended to the line.
 */
static KT_KEY_FN(gl_vi_append_at_eol)
{
  gl_save_for_undo(gl);
  gl->vi.command = 0;	/* Allow cursor at EOL */
  return gl_end_of_line(gl, 0) ||
         gl_vi_insert(gl, 0);
}

/*.......................................................................
 * This is the action function which implements the vi-style action which
 * moves the cursor to right one then switches to insert mode, thus
 * allowing text to be appended after the next character.
 */
static KT_KEY_FN(gl_vi_append)
{
  gl_save_for_undo(gl);
  gl->vi.command = 0;	/* Allow cursor at EOL */
  return gl_cursor_right(gl, 1) ||
         gl_vi_insert(gl, 0);
}

/*.......................................................................
 * This action function moves the cursor to the column specified by the
 * numeric argument. Column indexes start at 1.
 */
static KT_KEY_FN(gl_goto_column)
{
  return gl_place_cursor(gl, count - 1);
}

/*.......................................................................
 * Starting with the character under the cursor, replace 'count'
 * characters with the next character that the user types.
 */
static KT_KEY_FN(gl_vi_replace_char)
{
  char c;  /* The replacement character */
  int i;
/*
 * Keep a record of the current insert mode.
 */
  int insert = gl->insert;
/*
 * Get the replacement character.
 */
  if(gl->vi.repeat.active) {
    c = gl->vi.repeat.input_char;
  } else {
    if(gl_read_character(gl, &c, -1))
      return 1;
    gl->vi.repeat.input_char = c;
  };
/*
 * Are there 'count' characters to be replaced?
 */
  if(gl->ntotal - gl->buff_curpos >= count) {
/*
 * If in vi command mode, preserve the current line for potential
 * use by vi-undo.
 */
    gl_save_for_undo(gl);
/*
 * Temporarily switch to overwrite mode.
 */
    gl->insert = 0;
/*
 * Overwrite the current character plus count-1 subsequent characters
 * with the replacement character.
 */
    for(i=0; i<count; i++)
      gl_add_char_to_line(gl, c);
/*
 * Restore the original insert/overwrite mode.
 */
    gl->insert = insert;
  };
  return gl_place_cursor(gl, gl->buff_curpos);	/* bounds check */
}

/*.......................................................................
 * This is an action function which changes all characters between the
 * current cursor position and the end of the line.
 */
static KT_KEY_FN(gl_vi_change_rest_of_line)
{
  gl_save_for_undo(gl);
  gl->vi.command = 0;	/* Allow cursor at EOL */
  return gl_kill_line(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * This is an action function which changes all characters between the
 * start of the line and the current cursor position.
 */
static KT_KEY_FN(gl_vi_change_to_bol)
{
  return gl_backward_kill_line(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * This is an action function which deletes the entire contents of the
 * current line and switches to insert mode.
 */
static KT_KEY_FN(gl_vi_change_line)
{
  return gl_delete_line(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * Starting from the cursor position and looking towards the end of the
 * line, copy 'count' characters to the cut buffer.
 */
static KT_KEY_FN(gl_forward_copy_char)
{
/*
 * Limit the count to the number of characters available.
 */
  if(gl->buff_curpos + count >= gl->ntotal)
    count = gl->ntotal - gl->buff_curpos;
  if(count < 0)
    count = 0;
/*
 * Copy the characters to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line + gl->buff_curpos, count);
  gl->cutbuf[count] = '\0';
  return 0;
}

/*.......................................................................
 * Starting from the character before the cursor position and looking
 * backwards towards the start of the line, copy 'count' characters to
 * the cut buffer.
 */
static KT_KEY_FN(gl_backward_copy_char)
{
/*
 * Limit the count to the number of characters available.
 */
  if(count > gl->buff_curpos)
    count = gl->buff_curpos;
  if(count < 0)
    count = 0;
  gl_place_cursor(gl, gl->buff_curpos - count);
/*
 * Copy the characters to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line + gl->buff_curpos, count);
  gl->cutbuf[count] = '\0';
  return 0;
}

/*.......................................................................
 * Starting from the cursor position copy to the specified column into the
 * cut buffer.
 */
static KT_KEY_FN(gl_copy_to_column)
{
  if (--count >= gl->buff_curpos)
    return gl_forward_copy_char(gl, count - gl->buff_curpos);
  else
    return gl_backward_copy_char(gl, gl->buff_curpos - count);
}

/*.......................................................................
 * Starting from the cursor position copy characters up to a matching
 * parenthesis into the cut buffer.
 */
static KT_KEY_FN(gl_copy_to_parenthesis)
{
  int curpos = gl_index_of_matching_paren(gl);
  if(curpos >= 0) {
    gl_save_for_undo(gl);
    if(curpos >= gl->buff_curpos)
      return gl_forward_copy_char(gl, curpos - gl->buff_curpos + 1);
    else
      return gl_backward_copy_char(gl, ++gl->buff_curpos - curpos + 1);
  };
  return 0;
}

/*.......................................................................
 * Starting from the cursor position copy the rest of the line into the
 * cut buffer.
 */
static KT_KEY_FN(gl_copy_rest_of_line)
{
/*
 * Copy the characters to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line + gl->buff_curpos, gl->ntotal - gl->buff_curpos);
  gl->cutbuf[gl->ntotal - gl->buff_curpos] = '\0';
  return 0;
}

/*.......................................................................
 * Copy from the beginning of the line to the cursor position into the
 * cut buffer.
 */
static KT_KEY_FN(gl_copy_to_bol)
{
/*
 * Copy the characters to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line, gl->buff_curpos);
  gl->cutbuf[gl->buff_curpos] = '\0';
  gl_place_cursor(gl, 0);
  return 0;
}

/*.......................................................................
 * Copy the entire line into the cut buffer.
 */
static KT_KEY_FN(gl_copy_line)
{
/*
 * Copy the characters to the cut buffer.
 */
  memcpy(gl->cutbuf, gl->line, gl->ntotal);
  gl->cutbuf[gl->ntotal] = '\0';
  return 0;
}

/*.......................................................................
 * Search forwards for the next character that the user enters.
 */
static KT_KEY_FN(gl_forward_find_char)
{
  int pos = gl_find_char(gl, count, 1, 1, '\0');
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Search backwards for the next character that the user enters.
 */
static KT_KEY_FN(gl_backward_find_char)
{
  int pos = gl_find_char(gl, count, 0, 1, '\0');
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Search forwards for the next character that the user enters. Move up to,
 * but not onto, the found character.
 */
static KT_KEY_FN(gl_forward_to_char)
{
  int pos = gl_find_char(gl, count, 1, 0, '\0');
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Search backwards for the next character that the user enters. Move back to,
 * but not onto, the found character.
 */
static KT_KEY_FN(gl_backward_to_char)
{
  int pos = gl_find_char(gl, count, 0, 0, '\0');
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Searching in a given direction, return the index of a given (or
 * read) character in the input line, or the character that precedes
 * it in the specified search direction. Return -1 if not found.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  count        int    The number of times to search.
 *  forward      int    True if searching forward.
 *  onto         int    True if the search should end on top of the
 *                      character, false if the search should stop
 *                      one character before the character in the
 *                      specified search direction.
 *  c           char    The character to be sought, or '\0' if the
 *                      character should be read from the user.
 * Output:
 *  return       int    The index of the character in gl->line[], or
 *                      -1 if not found.
 */
static int gl_find_char(GetLine *gl, int count, int forward, int onto, char c)
{
  int pos;     /* The index reached in searching the input line */
  int i;
/*
 * Get a character from the user?
 */
  if(!c) {
/*
 * If we are in the process of repeating a previous change command, substitute
 * the last find character.
 */
    if(gl->vi.repeat.active) {
      c = gl->vi.find_char;
    } else {
      if(gl_read_character(gl, &c, -1))
	return -1;
/*
 * Record the details of the new search, for use by repeat finds.
 */
      gl->vi.find_forward = forward;
      gl->vi.find_onto = onto;
      gl->vi.find_char = c;
    };
  };
/*
 * Which direction should we search?
 */
  if(forward) {
/*
 * Search forwards 'count' times for the character, starting with the
 * character that follows the cursor.
 */
    for(i=0, pos=gl->buff_curpos; i<count && pos < gl->ntotal; i++) {
/*
 * Advance past the last match (or past the current cursor position
 * on the first search).
 */
      pos++;
/*
 * Search for the next instance of c.
 */
      for( ; pos<gl->ntotal && c!=gl->line[pos]; pos++)
	;
    };
/*
 * If the character was found and we have been requested to return the
 * position of the character that precedes the desired character, then
 * we have gone one character too far.
 */
    if(!onto && pos<gl->ntotal)
      pos--;
  } else {
/*
 * Search backwards 'count' times for the character, starting with the
 * character that precedes the cursor.
 */
    for(i=0, pos=gl->buff_curpos; i<count && pos >= gl->insert_curpos; i++) {
/*
 * Step back one from the last match (or from the current cursor
 * position on the first search).
 */
      pos--;
/*
 * Search for the next instance of c.
 */
      for( ; pos>=gl->insert_curpos && c!=gl->line[pos]; pos--)
	;
    };
/*
 * If the character was found and we have been requested to return the
 * position of the character that precedes the desired character, then
 * we have gone one character too far.
 */
    if(!onto && pos>=gl->insert_curpos)
      pos++;
  };
/*
 * If found, return the cursor position of the count'th match.
 * Otherwise ring the terminal bell.
 */
  if(pos >= gl->insert_curpos && pos < gl->ntotal) {
    return pos;
  } else {
    (void) gl_ring_bell(gl, 1);
    return -1;
  }
}

/*.......................................................................
 * Repeat the last character search in the same direction as the last
 * search.
 */
static KT_KEY_FN(gl_repeat_find_char)
{
  int pos = gl->vi.find_char ?
    gl_find_char(gl, count, gl->vi.find_forward, gl->vi.find_onto,
		 gl->vi.find_char) : -1;
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Repeat the last character search in the opposite direction as the last
 * search.
 */
static KT_KEY_FN(gl_invert_refind_char)
{
  int pos = gl->vi.find_char ?
    gl_find_char(gl, count, !gl->vi.find_forward, gl->vi.find_onto,
		 gl->vi.find_char) : -1;
  return pos >= 0 && gl_place_cursor(gl, pos);
}

/*.......................................................................
 * Search forward from the current position of the cursor for 'count'
 * word endings, returning the index of the last one found, or the end of
 * the line if there were less than 'count' words.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  n            int    The number of word boundaries to search for.
 * Output:
 *  return       int    The buffer index of the located position.
 */
static int gl_nth_word_end_forward(GetLine *gl, int n)
{
  int bufpos;   /* The buffer index being checked. */
  int i;
/*
 * In order to guarantee forward motion to the next word ending,
 * we need to start from one position to the right of the cursor
 * position, since this may already be at the end of a word.
 */
  bufpos = gl->buff_curpos + 1;
/*
 * If we are at the end of the line, return the index of the last
 * real character on the line. Note that this will be -1 if the line
 * is empty.
 */
  if(bufpos >= gl->ntotal)
    return gl->ntotal - 1;
/*
 * Search 'n' times, unless the end of the input line is reached first.
 */
  for(i=0; i<n && bufpos<gl->ntotal; i++) {
/*
 * If we are not already within a word, skip to the start of the next word.
 */
    for( ; bufpos<gl->ntotal && !gl_is_word_char((int)gl->line[bufpos]);
	bufpos++)
      ;
/*
 * Find the end of the next word.
 */
    for( ; bufpos<gl->ntotal && gl_is_word_char((int)gl->line[bufpos]);
	bufpos++)
      ;
  };
/*
 * We will have overshot.
 */
  return bufpos > 0 ? bufpos-1 : bufpos;
}

/*.......................................................................
 * Search forward from the current position of the cursor for 'count'
 * word starts, returning the index of the last one found, or the end of
 * the line if there were less than 'count' words.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  n            int    The number of word boundaries to search for.
 * Output:
 *  return       int    The buffer index of the located position.
 */
static int gl_nth_word_start_forward(GetLine *gl, int n)
{
  int bufpos;   /* The buffer index being checked. */
  int i;
/*
 * Get the current cursor position.
 */
  bufpos = gl->buff_curpos;
/*
 * Search 'n' times, unless the end of the input line is reached first.
 */
  for(i=0; i<n && bufpos<gl->ntotal; i++) {
/*
 * Find the end of the current word.
 */
    for( ; bufpos<gl->ntotal && gl_is_word_char((int)gl->line[bufpos]);
	bufpos++)
      ;
/*
 * Skip to the start of the next word.
 */
    for( ; bufpos<gl->ntotal && !gl_is_word_char((int)gl->line[bufpos]);
	bufpos++)
      ;
  };
  return bufpos;
}

/*.......................................................................
 * Search backward from the current position of the cursor for 'count'
 * word starts, returning the index of the last one found, or the start
 * of the line if there were less than 'count' words.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  n            int    The number of word boundaries to search for.
 * Output:
 *  return       int    The buffer index of the located position.
 */
static int gl_nth_word_start_backward(GetLine *gl, int n)
{
  int bufpos;   /* The buffer index being checked. */
  int i;
/*
 * Get the current cursor position.
 */
  bufpos = gl->buff_curpos;
/*
 * Search 'n' times, unless the beginning of the input line (or vi insertion
 * point) is reached first.
 */
  for(i=0; i<n && bufpos > gl->insert_curpos; i++) {
/*
 * Starting one character back from the last search, so as not to keep
 * settling on the same word-start, search backwards until finding a
 * word character.
 */
    while(--bufpos >= gl->insert_curpos &&
          !gl_is_word_char((int)gl->line[bufpos]))
      ;
/*
 * Find the start of the word.
 */
    while(--bufpos >= gl->insert_curpos &&
          gl_is_word_char((int)gl->line[bufpos]))
      ;
/*
 * We will have gone one character too far.
 */
    bufpos++;
  };
  return bufpos >= gl->insert_curpos ? bufpos : gl->insert_curpos;
}

/*.......................................................................
 * Copy one or more words into the cut buffer without moving the cursor
 * or deleting text.
 */
static KT_KEY_FN(gl_forward_copy_word)
{
/*
 * Find the location of the count'th start or end of a word
 * after the cursor, depending on whether in emacs or vi mode.
 */
  int next = gl->editor == GL_EMACS_MODE ?
    gl_nth_word_end_forward(gl, count) :
    gl_nth_word_start_forward(gl, count);
/*
 * How many characters are to be copied into the cut buffer?
 */
  int n = next - gl->buff_curpos;
/*
 * Copy the specified segment and terminate the string.
 */
  memcpy(gl->cutbuf, gl->line + gl->buff_curpos, n);
  gl->cutbuf[n] = '\0';
  return 0;
}

/*.......................................................................
 * Copy one or more words preceding the cursor into the cut buffer,
 * without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_backward_copy_word)
{
/*
 * Find the location of the count'th start of word before the cursor.
 */
  int next = gl_nth_word_start_backward(gl, count);
/*
 * How many characters are to be copied into the cut buffer?
 */
  int n = gl->buff_curpos - next;
  gl_place_cursor(gl, next);
/*
 * Copy the specified segment and terminate the string.
 */
  memcpy(gl->cutbuf, gl->line + next, n);
  gl->cutbuf[n] = '\0';
  return 0;
}

/*.......................................................................
 * Copy the characters between the cursor and the count'th instance of
 * a specified character in the input line, into the cut buffer.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  count        int    The number of times to search.
 *  c           char    The character to be searched for, or '\0' if
 *                      the character should be read from the user.
 *  forward      int    True if searching forward.
 *  onto         int    True if the search should end on top of the
 *                      character, false if the search should stop
 *                      one character before the character in the
 *                      specified search direction.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 *
 */
static int gl_copy_find(GetLine *gl, int count, char c, int forward, int onto)
{
  int n;  /* The number of characters in the cut buffer */
/*
 * Search for the character, and abort the operation if not found.
 */
  int pos = gl_find_char(gl, count, forward, onto, c);
  if(pos < 0)
    return 0;
/*
 * Copy the specified segment.
 */
  if(forward) {
    n = pos + 1 - gl->buff_curpos;
    memcpy(gl->cutbuf, gl->line + gl->buff_curpos, n);
  } else {
    n = gl->buff_curpos - pos;
    memcpy(gl->cutbuf, gl->line + pos, n);
    if(gl->editor == GL_VI_MODE)
      gl_place_cursor(gl, pos);
  }
/*
 * Terminate the copy.
 */
  gl->cutbuf[n] = '\0';
  return 0;
}

/*.......................................................................
 * Copy a section up to and including a specified character into the cut
 * buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_forward_copy_find)
{
  return gl_copy_find(gl, count, '\0', 1, 1);
}

/*.......................................................................
 * Copy a section back to and including a specified character into the cut
 * buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_backward_copy_find)
{
  return gl_copy_find(gl, count, '\0', 0, 1);
}

/*.......................................................................
 * Copy a section up to and not including a specified character into the cut
 * buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_forward_copy_to)
{
  return gl_copy_find(gl, count, '\0', 1, 0);
}

/*.......................................................................
 * Copy a section back to and not including a specified character into the cut
 * buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_backward_copy_to)
{
  return gl_copy_find(gl, count, '\0', 0, 0);
}

/*.......................................................................
 * Copy to a character specified in a previous search into the cut
 * buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_copy_refind)
{
  return gl_copy_find(gl, count, gl->vi.find_char, gl->vi.find_forward,
		      gl->vi.find_onto);
}

/*.......................................................................
 * Copy to a character specified in a previous search, but in the opposite
 * direction, into the cut buffer without moving the cursor or deleting text.
 */
static KT_KEY_FN(gl_copy_invert_refind)
{
  return gl_copy_find(gl, count, gl->vi.find_char, !gl->vi.find_forward,
		      gl->vi.find_onto);
}

/*.......................................................................
 * Set the position of the cursor in the line input buffer and the
 * terminal.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 *  buff_curpos  int    The new buffer cursor position.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
int gl_place_cursor(GetLine *gl, int buff_curpos)
{
/*
 * Don't allow the cursor position to go out of the bounds of the input
 * line.
 */
  if(buff_curpos >= gl->ntotal)
    buff_curpos = gl->vi.command ? gl->ntotal-1 : gl->ntotal;
  if(buff_curpos < 0)
    buff_curpos = 0;
/*
 * Record the new buffer position.
 */
  gl->buff_curpos = buff_curpos;
/*
 * Move the terminal cursor to the corresponding character.
 */
  return gl_set_term_curpos(gl, gl_buff_curpos_to_term_curpos(gl, buff_curpos));
}

/*.......................................................................
 * In vi command mode, this function saves the current line to the
 * historical buffer needed by the undo command. In emacs mode it does
 * nothing. In order to allow action functions to call other action
 * functions, gl_interpret_char() sets gl->vi.undo.saved to 0 before
 * invoking an action, and thereafter once any call to this function
 * has set it to 1, further calls are ignored.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 */
static void gl_save_for_undo(GetLine *gl)
{
  if(gl->vi.command && !gl->vi.undo.saved) {
    strcpy(gl->vi.undo.line, gl->line);
    gl->vi.undo.buff_curpos = gl->buff_curpos;
    gl->vi.undo.ntotal = gl->ntotal;
    gl->vi.undo.saved = 1;
  };
  if(gl->vi.command && !gl->vi.repeat.saved &&
     gl->current_fn != gl_vi_repeat_change) {
    gl->vi.repeat.fn = gl->current_fn;
    gl->vi.repeat.count = gl->current_count;
    gl->vi.repeat.saved = 1;
  };
  return;
}

/*.......................................................................
 * In vi mode, restore the line to the way it was before the last command
 * mode operation, storing the current line in the buffer so that the
 * undo operation itself can subsequently be undone.
 */
static KT_KEY_FN(gl_vi_undo)
{
/*
 * Get pointers into the two lines.
 */
  char *undo_ptr = gl->vi.undo.line;
  char *line_ptr = gl->line;
/*
 * Swap the characters of the two buffers up to the length of the shortest
 * line.
 */
  while(*undo_ptr && *line_ptr) {
    char c = *undo_ptr;
    *undo_ptr++ = *line_ptr;
    *line_ptr++ = c;
  };
/*
 * Copy the rest directly.
 */
  if(gl->ntotal > gl->vi.undo.ntotal) {
    strcpy(undo_ptr, line_ptr);
    *line_ptr = '\0';
  } else {
    strcpy(line_ptr, undo_ptr);
    *undo_ptr = '\0';
  };
/*
 * Swap the length information.
 */
  {
    int ntotal = gl->ntotal;
    gl->ntotal = gl->vi.undo.ntotal;
    gl->vi.undo.ntotal = ntotal;
  };
/*
 * Set both cursor positions to the leftmost of the saved and current
 * cursor positions to emulate what vi does.
 */
  if(gl->buff_curpos < gl->vi.undo.buff_curpos)
    gl->vi.undo.buff_curpos = gl->buff_curpos;
  else
    gl->buff_curpos = gl->vi.undo.buff_curpos;
/*
 * Since we have bipassed calling gl_save_for_undo(), record repeat
 * information inline.
 */
  gl->vi.repeat.fn = gl_vi_undo;
  gl->vi.repeat.count = 1;
/*
 * Display the restored line.
 */
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * Delete the following word and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_forward_change_word)
{
  gl_save_for_undo(gl);
  gl->vi.command = 0;	/* Allow cursor at EOL */
  return gl_forward_delete_word(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * Delete the preceding word and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_backward_change_word)
{
  return gl_backward_delete_word(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * Delete the following section and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_forward_change_find)
{
  return gl_delete_find(gl, count, '\0', 1, 1, 1);
}

/*.......................................................................
 * Delete the preceding section and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_backward_change_find)
{
  return gl_delete_find(gl, count, '\0', 0, 1, 1);
}

/*.......................................................................
 * Delete the following section and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_forward_change_to)
{
  return gl_delete_find(gl, count, '\0', 1, 0, 1);
}

/*.......................................................................
 * Delete the preceding section and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_backward_change_to)
{
  return gl_delete_find(gl, count, '\0', 0, 0, 1);
}

/*.......................................................................
 * Delete to a character specified by a previous search and leave the user
 * in vi insert mode.
 */
static KT_KEY_FN(gl_vi_change_refind)
{
  return gl_delete_find(gl, count, gl->vi.find_char, gl->vi.find_forward,
			gl->vi.find_onto, 1);
}

/*.......................................................................
 * Delete to a character specified by a previous search, but in the opposite
 * direction, and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_change_invert_refind)
{
  return gl_delete_find(gl, count, gl->vi.find_char, !gl->vi.find_forward,
			gl->vi.find_onto, 1);
}

/*.......................................................................
 * Delete the following character and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_forward_change_char)
{
  gl_save_for_undo(gl);
  gl->vi.command = 0;	/* Allow cursor at EOL */
  return gl_delete_chars(gl, count, 1) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * Delete the preceding character and leave the user in vi insert mode.
 */
static KT_KEY_FN(gl_vi_backward_change_char)
{
  return gl_backward_delete_char(gl, count) || gl_vi_insert(gl, 0);
}

/*.......................................................................
 * Starting from the cursor position change characters to the specified column.
 */
static KT_KEY_FN(gl_vi_change_to_column)
{
  if (--count >= gl->buff_curpos)
    return gl_vi_forward_change_char(gl, count - gl->buff_curpos);
  else
    return gl_vi_backward_change_char(gl, gl->buff_curpos - count);
}

/*.......................................................................
 * Starting from the cursor position change characters to a matching
 * parenthesis.
 */
static KT_KEY_FN(gl_vi_change_to_parenthesis)
{
  int curpos = gl_index_of_matching_paren(gl);
  if(curpos >= 0) {
    gl_save_for_undo(gl);
    if(curpos >= gl->buff_curpos)
      return gl_vi_forward_change_char(gl, curpos - gl->buff_curpos + 1);
    else
      return gl_vi_backward_change_char(gl, ++gl->buff_curpos - curpos + 1);
  };
  return 0;
}

/*.......................................................................
 * If in vi mode, switch to vi command mode.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 */
static void gl_vi_command_mode(GetLine *gl)
{
  if(gl->editor == GL_VI_MODE && !gl->vi.command) {
    gl->insert = 1;
    gl->vi.command = 1;
    gl->vi.repeat.input_curpos = gl->insert_curpos;
    gl->vi.repeat.command_curpos = gl->buff_curpos;
    gl->insert_curpos = 0;	/* unrestrict left motion boundary */
    gl_cursor_left(gl, 1); /* Vi moves left one on entering command mode */
  };
}

/*.......................................................................
 * This is an action function which rings the terminal bell.
 */
static KT_KEY_FN(gl_ring_bell)
{
  return gl->silence_bell ? 0 :
    gl_output_control_sequence(gl, 1, gl->sound_bell);
}

/*.......................................................................
 * This is the action function which implements the vi-repeat-change
 * action.
 */
static KT_KEY_FN(gl_vi_repeat_change)
{
  int status;   /* The return status of the repeated action function */
  int i;
/*
 * Nothing to repeat?
 */
  if(!gl->vi.repeat.fn)
    return gl_ring_bell(gl, 1);
/*
 * Provide a way for action functions to know whether they are being
 * called by us.
 */
  gl->vi.repeat.active = 1;
/*
 * Re-run the recorded function.
 */
  status = gl->vi.repeat.fn(gl, gl->vi.repeat.count);
/*
 * Mark the repeat as completed.
 */
  gl->vi.repeat.active = 0;
/*
 * Is we are repeating a function that has just switched to input
 * mode to allow the user to type, re-enter the text that the user
 * previously entered.
 */
  if(status==0 && !gl->vi.command) {
/*
 * Make sure that the current line has been saved.
 */
    gl_save_for_undo(gl);
/*
 * Repeat a previous insertion or overwrite?
 */
    if(gl->vi.repeat.input_curpos >= 0 &&
       gl->vi.repeat.input_curpos <= gl->vi.repeat.command_curpos &&
       gl->vi.repeat.command_curpos <= gl->vi.undo.ntotal) {
/*
 * Using the current line which is saved in the undo buffer, plus
 * the range of characters therein, as recorded by gl_vi_command_mode(),
 * add the characters that the user previously entered, to the input
 * line.
 */
      for(i=gl->vi.repeat.input_curpos; i<gl->vi.repeat.command_curpos; i++) {
	if(gl_add_char_to_line(gl, gl->vi.undo.line[i]))
	  return 1;
      };
    };
/*
 * Switch back to command mode, now that the insertion has been repeated.
 */
    gl_vi_command_mode(gl);
  };
  return status;
}

/*.......................................................................
 * If the cursor is currently over a parenthesis character, return the
 * index of its matching parenthesis. If not currently over a parenthesis
 * character, return the next close parenthesis character to the right of
 * the cursor. If the respective parenthesis character isn't found,
 * ring the terminal bell and return -1.
 *
 * Input:
 *  gl       GetLine *  The getline resource object.
 * Output:
 *  return       int    Either the index of the matching parenthesis,
 *                      or -1 if not found.
 */
static int gl_index_of_matching_paren(GetLine *gl)
{
  int i;
/*
 * List the recognized parentheses, and their matches.
 */
  const char *o_paren = "([{";
  const char *c_paren = ")]}";
  const char *cptr;
/*
 * Get the character that is currently under the cursor.
 */
  char c = gl->line[gl->buff_curpos];
/*
 * If the character under the cursor is an open parenthesis, look forward
 * for the matching close parenthesis.
 */
  if((cptr=strchr(o_paren, c))) {
    char match = c_paren[cptr - o_paren];
    int matches_needed = 1;
    for(i=gl->buff_curpos+1; i<gl->ntotal; i++) {
      if(gl->line[i] == c)
	matches_needed++;
      else if(gl->line[i] == match && --matches_needed==0)
	return i;
    };
/*
 * If the character under the cursor is an close parenthesis, look forward
 * for the matching open parenthesis.
 */
  } else if((cptr=strchr(c_paren, c))) {
    char match = o_paren[cptr - c_paren];
    int matches_needed = 1;
    for(i=gl->buff_curpos-1; i>=0; i--) {
      if(gl->line[i] == c)
	matches_needed++;
      else if(gl->line[i] == match && --matches_needed==0)
	return i;
    };
/*
 * If not currently over a parenthesis character, search forwards for
 * the first close parenthesis (this is what the vi % binding does).
 */
  } else {
    for(i=gl->buff_curpos+1; i<gl->ntotal; i++)
      if(strchr(c_paren, gl->line[i]) != NULL)
	return i;
  };
/*
 * Not found.
 */
  (void) gl_ring_bell(gl, 1);
  return -1;
}

/*.......................................................................
 * If the cursor is currently over a parenthesis character, this action
 * function moves the cursor to its matching parenthesis.
 */
static KT_KEY_FN(gl_find_parenthesis)
{
  int curpos = gl_index_of_matching_paren(gl);
  if(curpos >= 0)
    return gl_place_cursor(gl, curpos);
  return 0;
}

/*.......................................................................
 * Handle the receipt of the potential start of a new key-sequence from
 * the user.
 *
 * Input:
 *  gl      GetLine *   The resource object of this library.
 *  first_char char     The first character of the sequence.
 * Output:
 *  return      int     0 - OK.
 *                      1 - Error.
 */
static int gl_interpret_char(GetLine *gl, char first_char)
{
  KtKeyFn *keyfn;            /* An action function */
  int count;                 /* The repeat count of an action function */
  int ret = 0;               /* The return value of an action function */
  int i;
/*
 * Get the first character.
 */
  char c = first_char;
/*
 * If editting is disabled, just add newly entered characters to the
 * input line buffer, and watch for the end of the line.
 */
  if(gl->editor == GL_NO_EDITOR) {
    if(gl->ntotal >= gl->linelen) {
      ret = 0;
      goto ret_label;
    }
    if(c == '\n' || c == '\r') {
      ret = gl_newline(gl, 1);
      if (ret)
        ret = 1;
      goto ret_label;
    }
    gl->line[gl->ntotal++] = c;
    ret = 0;
    goto ret_label;
  };
/*
 * If the user is in the process of specifying a repeat count and the
 * new character is a digit, increment the repeat count accordingly.
 */
  if(gl->number >= 0 && isdigit((int)(unsigned char) c)) {
    ret = gl_digit_argument(gl, c);
    if (ret)
      ret = 1;
    goto ret_label;
  }
/*
 * In vi command mode, all key-sequences entered need to be
 * either implicitly or explicitly prefixed with an escape character.
 */
  else if(gl->vi.command && c != GL_ESC_CHAR) {
    if (gl->nkey == 0)
      gl->keyseq[gl->nkey++] = GL_ESC_CHAR;
  }
/*
 * If the first character of the sequence is a printable character,
 * then to avoid confusion with the special "up", "down", "left"
 * or "right" cursor key bindings, we need to prefix the
 * printable character with a backslash escape before looking it up.
 */
  else if(!IS_META_CHAR(c) && !IS_CTRL_CHAR(c)) {
    if (gl->nkey == 0)
      gl->keyseq[gl->nkey++] = '\\';
  }
/*
 * Compose a potentially multiple key-sequence in gl->keyseq.
 */
  while(gl->nkey < GL_KEY_MAX) {
    int first, last;   /* The matching entries in gl->keys */
/*
 * If the character is an unprintable meta character, split it
 * into two characters, an escape character and the character
 * that was modified by the meta key.
 */
    if(IS_META_CHAR(c)) {
      if (gl->nkey == 0)
        gl->keyseq[gl->nkey++] = GL_ESC_CHAR;
      c = META_TO_CHAR(c);
      continue;
    };
/*
 * Append the latest character to the key sequence.
 */
    gl->keyseq[gl->nkey++] = c;
/*
 * When doing vi-style editing, an escape at the beginning of any binding
 * switches to command mode.
 */
    if(gl->keyseq[0] == GL_ESC_CHAR && !gl->vi.command)
      gl_vi_command_mode(gl);
/*
 * Lookup the key sequence.
 */
    switch(_kt_lookup_keybinding(gl->bindings, gl->keyseq, gl->nkey, &first, &last)) {
    case KT_EXACT_MATCH:
/*
 * Get the matching action function.
 */
      keyfn = gl->bindings->table[first].keyfn;
/*
 * Get the repeat count, passing the last keystroke if executing the
 * digit-argument action.
 */
      if(keyfn == gl_digit_argument) {
	count = c;
      } else {
	count = gl->number >= 0 ? gl->number : 1;
      };
/*
 * Record the function that is being invoked.
 */
      gl->current_fn = keyfn;
      gl->current_count = count;
/*
 * Mark the current line as not yet preserved for use by the vi undo command.
 */
      gl->vi.undo.saved = 0;
      gl->vi.repeat.saved = 0;
/*
 * Execute the action function. Note the action function can tell
 * whether the provided repeat count was defaulted or specified
 * explicitly by looking at whether gl->number is -1 or not. If
 * it is negative, then no repeat count was specified by the user.
 */
      ret = keyfn(gl, count);
/*
 * Reset the repeat count after running action functions (other
 * than digit-argument).
 */
      if(keyfn != gl_digit_argument)
	gl->number = -1;
      if(ret)
	ret = 1;
      goto ret_label;
      break;
    case KT_AMBIG_MATCH:             /* Ambiguous match - so look ahead */
      if(gl_read_character(gl, &c, -1)) {/* Get the next character */
        ret = 1;
        goto ret_label;
      }
      break;
    case KT_NO_MATCH:
/*
 * If the first character looked like it might be a prefix of a key-sequence
 * but it turned out not to be, ring the bell to tell the user that it
 * wasn't recognised.
 */
      if(gl->keyseq[0] != '\\') {
	gl_ring_bell(gl, 0);
      } else {
/*
 * The user typed a single printable character that doesn't match
 * the start of any keysequence, so add it to the line in accordance
 * with the current repeat count.
 */
	count = gl->number >= 0 ? gl->number : 1;
	for(i=0; i<count; i++)
	  gl_add_char_to_line(gl, first_char);
	gl->number = -1;
      };
      ret = 0;
      goto ret_label;
      break;
    case KT_BAD_MATCH:
      ret = 1;
      goto ret_label;
      break;
    };
  };
/*
 * Key sequence too long to match.
 */
  ret = 0;

 ret_label:
  if (ret && (gl->net_may_block || (gl->net_read_attempt > 1))) {
    return 0;	/* Input incomplete */
  } else {
    gl->nkey = 0;
  }
  return ret;
}

/*.......................................................................
 * Configure the application and/or user-specific behavior of
 * gl_get_line().
 *
 * Note that calling this function between calling new_GetLine() and
 * the first call to new_GetLine(), disables the otherwise automatic
 * reading of ~/.teclarc on the first call to gl_get_line().
 *
 * Input:
 *  gl             GetLine *  The resource object of this library.
 *  app_string  const char *  Either NULL, or a string containing one
 *                            or more .teclarc command lines, separated
 *                            by newline characters. This can be used to
 *                            establish an application-specific
 *                            configuration, without the need for an external
 *                            file. This is particularly useful in embedded
 *                            environments where there is no filesystem.
 *  app_file    const char *  Either NULL, or the pathname of an
 *                            application-specific .teclarc file. The
 *                            contents of this file, if provided, are
 *                            read after the contents of app_string[].
 *  user_file   const char *  Either NULL, or the pathname of a
 *                            user-specific .teclarc file. Except in
 *                            embedded applications, this should
 *                            usually be "~/.teclarc".
 * Output:
 *  return             int    0 - OK.
 *                            1 - Bad argument(s).
 */
int gl_configure_getline(GetLine *gl, const char *app_string,
			 const char *app_file, const char *user_file)
{
/*
 * Check the arguments.
 */
  if(!gl) {
    fprintf(stderr, "gl_configure_getline: NULL gl argument.\n");
    return 1;
  };
/*
 * Mark getline as having been explicitly configured.
 */
  gl->configured = 1;
/*
 * Start by parsing the configuration string, if provided.
 */
  if(app_string)
    (void) _gl_read_config_string(gl, app_string, KTB_NORM);
/*
 * Now parse the application-specific configuration file, if provided.
 */
  if(app_file)
    (void) _gl_read_config_file(gl, app_file, KTB_NORM);
/*
 * Finally, parse the user-specific configuration file, if provided.
 */
  if(user_file)
    (void) _gl_read_config_file(gl, user_file, KTB_USER);
/*
 * Record the names of the configuration files to allow them to
 * be re-read if requested at a later time.
 */
  if(gl_record_string(&gl->app_file, app_file) ||
     gl_record_string(&gl->user_file, user_file)) {
    fprintf(stderr,
	    "Insufficient memory to record tecla configuration file names.\n");
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Replace a malloc'd string (or NULL), with another malloc'd copy of
 * a string (or NULL).
 *
 * Input:
 *  sptr          char **  On input if *sptr!=NULL, *sptr will be
 *                         free'd and *sptr will be set to NULL. Then,
 *                         on output, if string!=NULL a malloc'd copy
 *                         of this string will be assigned to *sptr.
 *  string  const char *   The string to be copied, or NULL to simply
 *                         discard any existing string.
 * Output:
 *  return         int     0 - OK.
 *                         1 - Malloc failure (no error message is generated).
 */
static int gl_record_string(char **sptr, const char *string)
{
/*
 * If the original string is the same string, don't do anything.
 */
  if(*sptr == string || (*sptr && string && strcmp(*sptr, string)==0))
    return 0;
/*
 * Discard any existing cached string.
 */
  if(*sptr) {
    free(*sptr);
    *sptr = NULL;
  };
/*
 * Allocate memory for a copy of the specified string.
 */
  if(string) {
    *sptr = (char *) malloc(strlen(string) + 1);
    if(!*sptr)
      return 1;
/*
 * Copy the string.
 */
    strcpy(*sptr, string);
  };
  return 0;
}

/*.......................................................................
 * Re-read any application-specific and user-specific files previously
 * specified via the gl_configure_getline() function.
 */
static KT_KEY_FN(gl_read_init_files)
{
  return gl_configure_getline(gl, NULL, gl->app_file, gl->user_file);
}

/*.......................................................................
 * Save the contents of the history buffer to a given new file.
 *
 * Input:
 *  gl             GetLine *  The resource object of this library.
 *  filename    const char *  The name of the new file to write to.
 *  comment     const char *  Extra information such as timestamps will
 *                            be recorded on a line started with this
 *                            string, the idea being that the file can
 *                            double as a command file. Specify "" if
 *                            you don't care.
 *  max_lines          int    The maximum number of lines to save, or -1
 *                            to save all of the lines in the history
 *                            list.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
int gl_save_history(GetLine *gl, const char *filename, const char *comment,
		    int max_lines)
{
  FileExpansion *expansion; /* The expansion of the filename */
/*
 * Check the arguments.
 */
  if(!gl || !filename || !comment) {
    fprintf(stderr, "gl_save_history: NULL argument(s).\n");
    return 1;
  };
/*
 * Expand the filename.
 */
  expansion = ef_expand_file(gl->ef, filename, -1);
  if(!expansion) {
    fprintf(stderr, "Unable to expand %s (%s).\n", filename,
            ef_last_error(gl->ef));
    return 1;
  };
/*
 * Attempt to save to the specified file.
 */
  return _glh_save_history(gl->glh, expansion->files[0], comment, max_lines);
}

/*.......................................................................
 * Restore the contents of the history buffer from a given new file.
 *
 * Input:
 *  gl             GetLine *  The resource object of this library.
 *  filename    const char *  The name of the new file to write to.
 *  comment     const char *  This must be the same string that was
 *                            passed to gl_save_history() when the file
 *                            was written.
 * Output:
 *  return             int     0 - OK.
 *                             1 - Error.
 */
int gl_load_history(GetLine *gl, const char *filename, const char *comment)
{
  FileExpansion *expansion; /* The expansion of the filename */
/*
 * Check the arguments.
 */
  if(!gl || !filename || !comment) {
    fprintf(stderr, "gl_load_history: NULL argument(s).\n");
    return 1;
  };
/*
 * Expand the filename.
 */
  expansion = ef_expand_file(gl->ef, filename, -1);
  if(!expansion) {
    fprintf(stderr, "Unable to expand %s (%s).\n", filename,
            ef_last_error(gl->ef));
    return 1;
  };
/*
 * Attempt to load from the specified file.
 */
  if(_glh_load_history(gl->glh, expansion->files[0], comment,
		       gl->cutbuf, gl->linelen)) {
    gl->cutbuf[0] = '\0';
    return 1;
  };
  gl->cutbuf[0] = '\0';
  return 0;
}

/*.......................................................................
 * Where possible, register a function and associated data to be called
 * whenever a specified event is seen on a file descriptor.
 *
 * Input:
 *  gl            GetLine *  The resource object of the command-line input
 *                           module.
 *  fd                int    The file descriptor to watch.
 *  event       GlFdEvent    The type of activity to watch for.
 *  callback  GlFdEventFn *  The function to call when the specified
 *                           event occurs. Setting this to 0 removes
 *                           any existing callback.
 *  data             void *  A pointer to arbitrary data to pass to the
 *                           callback function.
 * Output:
 *  return            int    0 - OK.
 *                           1 - Either gl==NULL, or this facility isn't
 *                               available on the the host system
 *                               (ie. select() isn't available). No
 *                               error message is generated in the latter
 *                               case.
 */
int gl_watch_fd(GetLine *gl, int fd, GlFdEvent event,
		GlFdEventFn *callback, void *data)
#if !defined(HAVE_SELECT)
{return 1;}               /* The facility isn't supported on this system */
#else
{
  GlFdNode *prev;  /* The node that precedes 'node' in gl->fd_nodes */
  GlFdNode *node;  /* The file-descriptor node being checked */
/*
 * Check the arguments.
 */
  if(!gl) {
    fprintf(stderr, "gl_watch_fd: NULL gl argument.\n");
    return 1;
  };
  if(fd < 0) {
    fprintf(stderr, "gl_watch_fd: Error fd < 0.\n");
    return 1;
  };
/*
 * Search the list of already registered fd activity nodes for the specified
 * file descriptor.
 */
  for(prev=NULL,node=gl->fd_nodes; node && node->fd != fd;
      prev=node, node=node->next)
    ;
/*
 * Hasn't a node been allocated for this fd yet?
 */
  if(!node) {
/*
 * If there is no callback to record, just ignore the call.
 */
    if(!callback)
      return 0;
/*
 * Allocate the new node.
 */
    node = (GlFdNode *) _new_FreeListNode(gl->fd_node_mem);
    if(!node) {
      fprintf(stderr, "gl_watch_fd: Insufficient memory.\n");
      return 1;
    };
/*
 * Prepend the node to the list.
 */
    node->next = gl->fd_nodes;
    gl->fd_nodes = node;
/*
 * Initialize the node.
 */
    node->fd = fd;
    node->rd.fn = 0;
    node->rd.data = NULL;
    node->ur = node->wr = node->rd;
  };
/*
 * Record the new callback.
 */
  switch(event) {
  case GLFD_READ:
    node->rd.fn = callback;
    node->rd.data = data;
    if(callback)
      FD_SET(fd, &gl->rfds);
    else
      FD_CLR(fd, &gl->rfds);
    break;
  case GLFD_WRITE:
    node->wr.fn = callback;
    node->wr.data = data;
    if(callback)
      FD_SET(fd, &gl->wfds);
    else
      FD_CLR(fd, &gl->wfds);
    break;
  case GLFD_URGENT:
    node->ur.fn = callback;
    node->ur.data = data;
    if(callback)
      FD_SET(fd, &gl->ufds);
    else
      FD_CLR(fd, &gl->ufds);
    break;
  };
/*
 * Keep a record of the largest file descriptor being watched.
 */
  if(fd > gl->max_fd)
    gl->max_fd = fd;
/*
 * If we are deleting an existing callback, also delete the parent
 * activity node if no callbacks are registered to the fd anymore.
 */
  if(!callback) {
    if(!node->rd.fn && !node->wr.fn && !node->ur.fn) {
      if(prev)
	prev->next = node->next;
      else
	gl->fd_nodes = node->next;
      node = (GlFdNode *) _del_FreeListNode(gl->fd_node_mem, node);
    };
  };
  return 0;
}

/*.......................................................................
 * When select() is available, this function is called by
 * gl_read_character() to respond to file-descriptor events registered by
 * the caller.
 *
 * Input:
 *  gl    GetLine *  The resource object of this module.
 * Output:
 *  return    int    0 - A character is waiting to be read from the
 *                       terminal.
 *                   1 - An error occurred.
 */
static int gl_event_handler(GetLine *gl)
{
/*
 * If at any time no external callbacks remain, quit the loop return,
 * so that we can simply wait in read(). This is designed as an
 * optimization for when no callbacks have been registered on entry to
 * this function, but since callbacks can delete themselves, it can
 * also help later.
 */
  while(gl->fd_nodes) {
/*
 * Get the set of descriptors to be watched.
 */
    fd_set rfds = gl->rfds;
    fd_set wfds = gl->wfds;
    fd_set ufds = gl->ufds;
/*
 * Wait for activity on any of the file descriptors.
 */
    int nready = select(gl->max_fd+1, &rfds, &wfds, &ufds, NULL);
/*
 * If select() returns but none of the file descriptors are reported
 * to have activity, then select() timed out.
 */
    if(nready == 0) {
      fprintf(stdout, "\r\nUnexpected select() timeout\r\n");
      return 1;
/*
 * If nready < 0, this means an error occurred.
 */
    } else if(nready < 0) {
      if(errno != EINTR) {
#ifdef EAGAIN
	if(!errno)          /* This can happen with SysV O_NDELAY */
	  errno = EAGAIN;
#endif
	return 1;
      };
/*
 * If the terminal input file descriptor has data available, return.
 */
    } else if(FD_ISSET(gl->input_fd, &rfds)) {
      return 0;
/*
 * Check for activity on any of the file descriptors registered by the
 * calling application, and call the associated callback functions.
 */
    } else {
      GlFdNode *node;   /* The fd event node being checked */
/*
 * Search the list for the file descriptor that caused select() to return.
 */
      for(node=gl->fd_nodes; node; node=node->next) {
/*
 * Is there urgent out of band data waiting to be read on fd?
 */
	if(node->ur.fn && FD_ISSET(node->fd, &ufds)) {
	  if(gl_call_fd_handler(gl, &node->ur, node->fd, GLFD_URGENT))
	    return 1;
	  break;  /* The callback may have changed the list of nodes */
/*
 * Is the fd readable?
 */
	} else if(node->rd.fn && FD_ISSET(node->fd, &rfds)) {
	  if(gl_call_fd_handler(gl, &node->rd, node->fd, GLFD_READ))
	    return 1;
	  break;  /* The callback may have changed the list of nodes */
/*
 * Is the fd writable?
 */
	} else if(node->wr.fn && FD_ISSET(node->fd, &rfds)) {
	  if(gl_call_fd_handler(gl, &node->wr, node->fd, GLFD_WRITE))
	    return 1;
	  break;  /* The callback may have changed the list of nodes */
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * This is a private function of gl_event_handler(), used to call a
 * file-descriptor callback.
 *
 * Input:
 *  gl       GetLine *  The resource object of gl_get_line().
 *  gfh  GlFdHandler *  The I/O handler.
 *  fd           int    The file-descriptor being reported.
 *  event  GlFdEvent    The I/O event being reported.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
static int gl_call_fd_handler(GetLine *gl, GlFdHandler *gfh, int fd,
			      GlFdEvent event)
{
  Termios attr;       /* The terminal attributes */
  int redisplay = 0;  /* True to have the input line redisplayed */
  int waserr = 0;     /* True after any error */
/*
 * We don't want to do a longjmp in the middle of a callback that
 * might be modifying global or heap data, so block all the signals
 * that we are trapping.
 */
  if(sigprocmask(SIG_BLOCK, &gl->new_signal_set, NULL) == -1) {
    fprintf(stderr, "getline(): sigprocmask error: %s\n", strerror(errno));
    return 1;
  };
/*
 * Re-enable conversion of newline characters to carriage-return/linefeed,
 * so that the callback can write to the terminal without having to do
 * anything special.
 */
  if(tcgetattr(gl->input_fd, &attr)) {
    fprintf(stderr, "\r\ngetline(): tcgetattr error: %s\r\n", strerror(errno));
    return 1;
  };
  attr.c_oflag |= OPOST;
  while(tcsetattr(gl->input_fd, TCSADRAIN, &attr)) {
    if (errno != EINTR) {
      fprintf(stderr, "\r\ngetline(): tcsetattr error: %s\r\n",
	      strerror(errno));
      return 1;
    };
  };
/*
 * Invoke the application's callback function.
 */
  switch(gfh->fn(gl, gfh->data, fd, event)) {
  default:
  case GLFD_ABORT:
    waserr = 1;
    break;
  case GLFD_REFRESH:
    redisplay = 1;
    break;
  case GLFD_CONTINUE:
    redisplay = gl->prompt_changed;
    break;
  };
/*
 * Disable conversion of newline characters to carriage-return/linefeed.
 */
  attr.c_oflag &= ~(OPOST);
  while(tcsetattr(gl->input_fd, TCSADRAIN, &attr)) {
    if(errno != EINTR) {
      fprintf(stderr, "\ngetline(): tcsetattr error: %s\n", strerror(errno));
      return 1;
    };
  };
/*
 * If requested, redisplay the input line.
 */
  if(redisplay && gl_redisplay(gl, 1))
    return 1;
/*
 * Unblock the signals that we were trapping before this function
 * was called.
 */
  if(sigprocmask(SIG_UNBLOCK, &gl->new_signal_set, NULL) == -1) {
    fprintf(stderr, "getline(): sigprocmask error: %s\n", strerror(errno));
    return 1;
  };
  return waserr;
}
#endif  /* HAVE_SELECT */

/*.......................................................................
 * Switch history groups. History groups represent separate history
 * lists recorded within a single history buffer. Different groups
 * are distinguished by integer identifiers chosen by the calling
 * appplicaton. Initially new_GetLine() sets the group identifier to
 * 0. Whenever a new line is appended to the history list, the current
 * group identifier is recorded with it, and history lookups only
 * consider lines marked with the current group identifier.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  id     unsigned    The new history group identifier.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
int gl_group_history(GetLine *gl, unsigned id)
{
/*
 * Check the arguments.
 */
  if(!gl) {
    fprintf(stderr, "gl_group_history: NULL argument(s).\n");
    return 1;
  };
/*
 * If the group isn't being changed, do nothing.
 */
  if(_glh_get_group(gl->glh) == id)
    return 0;
/*
 * Establish the new group.
 */
  if(_glh_set_group(gl->glh, id))
    return 1;
/*
 * Prevent history information from the previous group being
 * inappropriately used by the next call to gl_get_line().
 */
  gl->preload_history = 0;
  gl->last_search = -1;
  return 0;
}

/*.......................................................................
 * Display the contents of the history list.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  fp         FILE *  The stdio output stream to write to.
 *  fmt  const char *  A format string. This containing characters to be
 *                     written verbatim, plus any of the following
 *                     format directives:
 *                       %D  -  The date, formatted like 2001-11-20
 *                       %T  -  The time of day, formatted like 23:59:59
 *                       %N  -  The sequential entry number of the
 *                              line in the history buffer.
 *                       %G  -  The number of the history group that
 *                              the line belongs to.
 *                       %%  -  A literal % character.
 *                       %H  -  The history line itself.
 *                     Note that a '\n' newline character is not
 *                     appended by default.
 *  all_groups  int    If true, display history lines from all
 *                     history groups. Otherwise only display
 *                     those of the current history group.
 *  max_lines   int    If max_lines is < 0, all available lines
 *                     are displayed. Otherwise only the most
 *                     recent max_lines lines will be displayed.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Error.
 */
int gl_show_history(GetLine *gl, FILE *fp, const char *fmt, int all_groups,
		    int max_lines)
{
/*
 * Check the arguments.
 */
  if(!gl || !fp || !fmt) {
    fprintf(stderr, "gl_show_history: NULL argument(s).\n");
    return 1;
  };
  return _glh_show_history(gl->glh, fp, fmt, all_groups, max_lines);
}

/*.......................................................................
 * Update if necessary, and return the current size of the terminal.
 *
 * Input:
 *  gl            GetLine *  The resource object of gl_get_line().
 *  def_ncolumn       int    If the number of columns in the terminal
 *                           can't be determined, substitute this number.
 *  def_nline         int    If the number of lines in the terminal can't
 *                           be determined, substitute this number.
 * Output:
 *  return GlTerminalSize    The current terminal size.
 */
GlTerminalSize gl_terminal_size(GetLine *gl, int def_ncolumn, int def_nline)
{
  GlTerminalSize size;  /* The return value */
  const char *env;      /* The value of an environment variable */
  int n;                /* A number read from env[] */
/*
 * Set the number of lines and columns to non-sensical values so that
 * we know later if they have been set.
 */
  gl->nline = 0;
  gl->ncolumn = 0;
/*
 * Are we reading from a terminal?
 */
  if(gl->is_term) {
/*
 * Ask the terminal directly if possible.
 */
#ifdef USE_SIGWINCH
    (void) gl_resize_terminal(gl, 0);
#endif
/*
 * If gl_resize_terminal() couldn't be used, or it returned non-sensical
 * values for the number of lines, see if the LINES environment variable
 * exists and specifies a believable number. If this doesn't work,
 * look up the default size in the terminal information database,
 * where available.
 */
    if(gl->nline < 1) {
      if((env = getenv("LINES")) && (n=atoi(env)) > 0)
	gl->nline = n;
#ifdef USE_TERMINFO
      else
	gl->nline = tigetnum((char *)"lines");
#elif defined(USE_TERMCAP)
      else
        gl->nline = tgetnum("li");
#endif
    };
/*
 * If gl_resize_terminal() couldn't be used, or it returned non-sensical
 * values for the number of columns, see if the COLUMNS environment variable
 * exists and specifies a believable number. If this doesn't work, fall
 * lookup the default size in the terminal information database,
 * where available.
 */
    if(gl->ncolumn < 1) {
      if((env = getenv("COLUMNS")) && (n=atoi(env)) > 0)
	gl->ncolumn = n;
#ifdef USE_TERMINFO
      else
	gl->ncolumn = tigetnum((char *)"cols");
#elif defined(USE_TERMCAP)
      else
	gl->ncolumn = tgetnum("co");
#endif
    };
  };
/*
 * If we still haven't been able to acquire reasonable values, substitute
 * the default values specified by the caller.
 */
  if(gl->nline <= 0)
    gl->nline = def_nline;
  if(gl->ncolumn <= 0)
    gl->ncolumn = def_ncolumn;
/*
 * Copy the new size into the return value.
 */
  size.nline = gl->nline;
  size.ncolumn = gl->ncolumn;
  return size;
}

/*.......................................................................
 * Resize or delete the history buffer.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  bufsize  size_t    The number of bytes in the history buffer, or 0
 *                     to delete the buffer completely.
 * Output:
 *  return      int    0 - OK.
 *                     1 - Insufficient memory (the previous buffer
 *                         will have been retained). No error message
 *                         will be displayed.
 */
int gl_resize_history(GetLine *gl, size_t bufsize)
{
  return gl ? _glh_resize_history(gl->glh, bufsize) : 1;
}

/*.......................................................................
 * Set an upper limit to the number of lines that can be recorded in the
 * history list, or remove a previously specified limit.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  max_lines   int    The maximum number of lines to allow, or -1 to
 *                     cancel a previous limit and allow as many lines
 *                     as will fit in the current history buffer size.
 */
void gl_limit_history(GetLine *gl, int max_lines)
{
  if(gl)
    _glh_limit_history(gl->glh, max_lines);
}

/*.......................................................................
 * Discard either all historical lines, or just those associated with the
 * current history group.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  all_groups  int    If true, clear all of the history. If false,
 *                     clear only the stored lines associated with the
 *                     currently selected history group.
 */
void gl_clear_history(GetLine *gl, int all_groups)
{
  if(gl)
    _glh_clear_history(gl->glh, all_groups);
}

/*.......................................................................
 * Temporarily enable or disable the gl_get_line() history mechanism.
 *
 * Input:
 *  gl      GetLine *  The resource object of gl_get_line().
 *  enable      int    If true, turn on the history mechanism. If
 *                     false, disable it.
 */
void gl_toggle_history(GetLine *gl, int enable)
{
  if(gl)
    _glh_toggle_history(gl->glh, enable);
}

/*.......................................................................
 * Lookup a history line by its sequential number of entry in the
 * history buffer.
 *
 * Input:
 *  gl            GetLine *  The resource object of gl_get_line().
 *  id      unsigned long    The identification number of the line to
 *                           be returned, where 0 denotes the first line
 *                           that was entered in the history list, and
 *                           each subsequently added line has a number
 *                           one greater than the previous one. For
 *                           the range of lines currently in the list,
 *                           see the gl_range_of_history() function.
 * Input/Output:
 *  line    GlHistoryLine *  A pointer to the variable in which to
 *                           return the details of the line.
 * Output:
 *  return            int    0 - The line is no longer in the history
 *                               list, and *line has not been changed.
 *                           1 - The requested line can be found in
 *                               *line. Note that line->line is part
 *                               of the history buffer, so a
 *                               private copy should be made if you
 *                               wish to use it after subsequent calls
 *                               to any functions that take *gl as an
 *                               argument.
 */
int gl_lookup_history(GetLine *gl, unsigned long id, GlHistoryLine *line)
{
  return gl ? _glh_lookup_history(gl->glh, (GlhLineID) id, &line->line,
				  &line->group, &line->timestamp) : 0;
}

/*.......................................................................
 * Query the state of the history list. Note that any of the input/output
 * pointers can be specified as NULL.
 *
 * Input:
 *  gl            GetLine *  The resource object of gl_get_line().
 * Input/Output:
 *  state  GlHistoryState *  A pointer to the variable in which to record
 *                           the return values.
 */
void gl_state_of_history(GetLine *gl, GlHistoryState *state)
{
  if(gl && state)
    _glh_state_of_history(gl->glh, &state->enabled, &state->group,
			  &state->max_lines);
}

/*.......................................................................
 * Query the number and range of lines in the history buffer.
 *
 * Input:
 *  gl            GetLine *  The resource object of gl_get_line().
 *  range  GlHistoryRange *  A pointer to the variable in which to record
 *                           the return values. If range->nline=0, the
 *                           range of lines will be given as 0-0.
 */
void gl_range_of_history(GetLine *gl, GlHistoryRange *range)
{
  if(gl && range)
    _glh_range_of_history(gl->glh, &range->oldest, &range->newest,
			  &range->nlines);
}

/*.......................................................................
 * Return the size of the history buffer and the amount of the
 * buffer that is currently in use.
 *
 * Input:
 *  gl         GetLine *  The gl_get_line() resource object.
 * Input/Output:
 *  GlHistorySize size *  A pointer to the variable in which to return
 *                        the results.
 */
void gl_size_of_history(GetLine *gl, GlHistorySize *size)
{
  if(gl && size)
    _glh_size_of_history(gl->glh, &size->size, &size->used);
}

/*.......................................................................
 * This is the action function that lists the contents of the history
 * list.
 */
static KT_KEY_FN(gl_list_history)
{
/*
 * Start a new line.
 */
  if(fprintf(gl->output_fp, "\r\n") < 0)
    return 1;
/*
 * List history lines that belong to the current group.
 */
  _glh_show_history(gl->glh, gl->output_fp, "%N  %T   %H\r\n", 0,
		    count<=1 ? -1 : count);
/*
 * Redisplay the line being edited.
 */
  gl->term_curpos = 0;
  return gl_redisplay(gl,1);
}

/*.......................................................................
 * Specify whether text that users type should be displayed or hidden.
 * In the latter case, only the prompt is displayed, and the final
 * input line is not archived in the history list.
 *
 * Input:
 *  gl         GetLine *  The gl_get_line() resource object.
 *  enable         int     0 - Disable echoing.
 *                         1 - Enable echoing.
 *                        -1 - Just query the mode without changing it.
 * Output:
 *  return         int    The echoing disposition that was in effect
 *                        before this function was called:
 *                         0 - Echoing was disabled.
 *                         1 - Echoing was enabled.
 */
int gl_echo_mode(GetLine *gl, int enable)
{
  if(gl) {
    int was_echoing = gl->echo;
    if(enable >= 0)
      gl->echo = enable;
    return was_echoing;
  };
  return 1;
}

/*.......................................................................
 * Display the prompt.
 *
 * Input:
 *  gl         GetLine *  The resource object of gl_get_line().
 * Output:
 *  return         int    0 - OK.
 *                        1 - Error.
 */
static int gl_display_prompt(GetLine *gl)
{
  const char *pptr;       /* A pointer into gl->prompt[] */
  unsigned old_attr=0;    /* The current text display attributes */
  unsigned new_attr=0;    /* The requested text display attributes */
/*
 * Temporarily switch to echoing output characters.
 */
  int kept_echo = gl->echo;
  gl->echo = 1;
/*
 * In case the screen got messed up, send a carriage return to
 * put the cursor at the beginning of the current terminal line.
 */
  if(gl_output_control_sequence(gl, 1, gl->bol))
    return 1;
/*
 * Write the prompt, using the currently selected prompt style.
 */
  switch(gl->prompt_style) {
  case GL_LITERAL_PROMPT:
    if(gl_output_string(gl, gl->prompt, '\0'))
      return 1;
    break;
  case GL_FORMAT_PROMPT:
    for(pptr=gl->prompt; *pptr; pptr++) {
/*
 * Does the latest character appear to be the start of a directive?
 */
      if(*pptr == '%') {
/*
 * Check for and act on attribute changing directives.
 */
	switch(pptr[1]) {
/*
 * Add or remove a text attribute from the new set of attributes.
 */ 
	case 'B': case 'U': case 'S': case 'P': case 'F': case 'V':
	case 'b': case 'u': case 's': case 'p': case 'f': case 'v':
	  switch(*++pptr) {
	  case 'B':           /* Switch to a bold font */
	    new_attr |= GL_TXT_BOLD;
	    break;
	  case 'b':           /* Switch to a non-bold font */
	    new_attr &= ~GL_TXT_BOLD;
	    break;
	  case 'U':           /* Start underlining */
	    new_attr |= GL_TXT_UNDERLINE;
	    break;
	  case 'u':           /* Stop underlining */
	    new_attr &= ~GL_TXT_UNDERLINE;
	    break;
	  case 'S':           /* Start highlighting */
	    new_attr |= GL_TXT_STANDOUT;
	    break;
	  case 's':           /* Stop highlighting */
	    new_attr &= ~GL_TXT_STANDOUT;
	    break;
	  case 'P':           /* Switch to a pale font */
	    new_attr |= GL_TXT_DIM;
	    break;
	  case 'p':           /* Switch to a non-pale font */
	    new_attr &= ~GL_TXT_DIM;
	    break;
	  case 'F':           /* Switch to a flashing font */
	    new_attr |= GL_TXT_BLINK;
	    break;
	  case 'f':           /* Switch to a steady font */
	    new_attr &= ~GL_TXT_BLINK;
	    break;
	  case 'V':           /* Switch to reverse video */
	    new_attr |= GL_TXT_REVERSE;
	    break;
	  case 'v':           /* Switch out of reverse video */
	    new_attr &= ~GL_TXT_REVERSE;
	    break;
	  };
	  continue;
/*
 * A literal % is represented by %%. Skip the leading %.
 */
	case '%':
	  pptr++;
	  break;
	};
      };
/*
 * Many terminals, when asked to turn off a single text attribute, turn
 * them all off, so the portable way to turn one off individually is to
 * explicitly turn them all off, then specify those that we want from
 * scratch.
 */
      if(old_attr & ~new_attr) {
	if(gl_output_control_sequence(gl, 1, gl->text_attr_off))
	  return 1;
	old_attr = 0;
      };
/*
 * Install new text attributes?
 */
      if(new_attr != old_attr) {
	if(new_attr & GL_TXT_BOLD && !(old_attr & GL_TXT_BOLD) &&
	   gl_output_control_sequence(gl, 1, gl->bold))
	  return 1;
	if(new_attr & GL_TXT_UNDERLINE && !(old_attr & GL_TXT_UNDERLINE) &&
	   gl_output_control_sequence(gl, 1, gl->underline))
	  return 1;
	if(new_attr & GL_TXT_STANDOUT && !(old_attr & GL_TXT_STANDOUT) &&
	   gl_output_control_sequence(gl, 1, gl->standout))
	  return 1;
	if(new_attr & GL_TXT_DIM && !(old_attr & GL_TXT_DIM) &&
	   gl_output_control_sequence(gl, 1, gl->dim))
	  return 1;
	if(new_attr & GL_TXT_REVERSE && !(old_attr & GL_TXT_REVERSE) &&
	   gl_output_control_sequence(gl, 1, gl->reverse))
	  return 1;
	if(new_attr & GL_TXT_BLINK && !(old_attr & GL_TXT_BLINK) &&
	   gl_output_control_sequence(gl, 1, gl->blink))
	  return 1;
	old_attr = new_attr;
      };
/*
 * Display the latest character.
 */
      if(gl_output_char(gl, *pptr, pptr[1]))
	return 1;
    };
/*
 * Turn off all text attributes now that we have finished drawing
 * the prompt.
 */
    if(gl_output_control_sequence(gl, 1, gl->text_attr_off))
      return 1;
    break;
  };
/*
 * Restore the original echo mode.
 */
  gl->echo = kept_echo;
/*
 * The prompt has now been displayed at least once.
 */
  gl->prompt_changed = 0;
  return 0;
}

/*.......................................................................
 * This function can be called from gl_get_line() callbacks to have
 * the prompt changed when they return. It has no effect if gl_get_line()
 * is not currently being invoked.
 *
 * Input:
 *  gl         GetLine *  The resource object of gl_get_line().
 *  prompt  const char *  The new prompt.
 */
void gl_replace_prompt(GetLine *gl, const char *prompt)
{
  if(gl) {
    gl->prompt = prompt ? prompt : "";
    gl->prompt_len = gl_displayed_prompt_width(gl);
    gl->prompt_changed = 1;
  };
}

/*.......................................................................
 * Work out the length of the current prompt on the terminal, according
 * to the current prompt formatting style.
 *
 * Input:
 *  gl       GetLine *  The resource object of this library.
 * Output:
 *  return       int    The number of displayed characters.
 */
static int gl_displayed_prompt_width(GetLine *gl)
{
  int slen=0;         /* The displayed number of characters */
  const char *pptr;   /* A pointer into prompt[] */
/*
 * The length differs according to the prompt display style.
 */
  switch(gl->prompt_style) {
  case GL_LITERAL_PROMPT:
    return gl_displayed_string_width(gl, gl->prompt, -1, 0);
    break;
  case GL_FORMAT_PROMPT:
/*
 * Add up the length of the displayed string, while filtering out
 * attribute directives.
 */
    for(pptr=gl->prompt; *pptr; pptr++) {
/*
 * Does the latest character appear to be the start of a directive?
 */
      if(*pptr == '%') {
/*
 * Check for and skip attribute changing directives.
 */
	switch(pptr[1]) {
	case 'B': case 'b': case 'U': case 'u': case 'S': case 's':
	  pptr++;
	  continue;
/*
 * A literal % is represented by %%. Skip the leading %.
 */
	case '%':
	  pptr++;
	  break;
	};
      };
      slen += gl_displayed_char_width(gl, *pptr, slen);
    };
    break;
  };
  return slen;
}

/*.......................................................................
 * Specify whether to heed text attribute directives within prompt
 * strings.
 *
 * Input:
 *  gl           GetLine *  The resource object of gl_get_line().
 *  style  GlPromptStyle    The style of prompt (see the definition of
 *                          GlPromptStyle in libtecla.h for details).
 */
void gl_prompt_style(GetLine *gl, GlPromptStyle style)
{
  if(gl) {
    if(style != gl->prompt_style) {
      gl->prompt_style = style;
      gl->prompt_len = gl_displayed_prompt_width(gl);
      gl->prompt_changed = 1;
    };
  };
}

/*.......................................................................
 * Tell gl_get_line() how to respond to a given signal. This can be used
 * both to override the default responses to signals that gl_get_line()
 * normally catches and to add new signals to the list that are to be
 * caught.
 *
 * Input:
 *  gl           GetLine *  The resource object of gl_get_line().
 *  signo            int    The number of the signal to be caught.
 *  flags       unsigned    A bitwise union of GlSignalFlags enumerators.
 *  after  GlAfterSignal    What to do after the application's signal
 *                          handler has been called.
 *  errno_value      int    The value to set errno to.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int gl_trap_signal(GetLine *gl, int signo, unsigned flags,
		   GlAfterSignal after, int errno_value)
{
  GlSignalNode *sig;
/*
 * Check the arguments.
 */
  if(!gl) {
    fprintf(stderr, "gl_trap_signal: NULL argument(s).\n");
    return 1;
  };
/*
 * See if the signal has already been registered.
 */
  for(sig=gl->sigs; sig && sig->signo != signo; sig = sig->next)
    ;
/*
 * If the signal hasn't already been registered, allocate a node for
 * it.
 */
  if(!sig) {
    sig = (GlSignalNode *) _new_FreeListNode(gl->sig_mem);
    if(!sig)
      return 1;
/*
 * Add the new node to the head of the list.
 */
    sig->next = gl->sigs;
    gl->sigs = sig;
/*
 * Record the signal number.
 */
    sig->signo = signo;
/*
 * Create a signal set that includes just this signal.
 */
    sigemptyset(&sig->proc_mask);
    if(sigaddset(&sig->proc_mask, signo) == -1) {
      fprintf(stderr, "gl_trap_signal: sigaddset error: %s\n",
	      strerror(errno));
      sig = (GlSignalNode *) _del_FreeListNode(gl->sig_mem, sig);
      return 1;
    };
  };
/*
 * Record the new signal attributes.
 */
  sig->flags = flags;
  sig->after = after;
  sig->errno_value = errno_value;
  return 0;
}

/*.......................................................................
 * Remove a signal from the list of signals that gl_get_line() traps.
 *
 * Input:
 *  gl           GetLine *  The resource object of gl_get_line().
 *  signo            int    The number of the signal to be ignored.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int gl_ignore_signal(GetLine *gl, int signo)
{
  GlSignalNode *sig;  /* The gl->sigs list node of the specified signal */
  GlSignalNode *prev; /* The node that precedes sig in the list */
/*
 * Check the arguments.
 */
  if(!gl) {
    fprintf(stderr, "gl_ignore_signal: NULL argument(s).\n");
    return 1;
  };
/*
 * Find the node of the gl->sigs list which records the disposition
 * of the specified signal.
 */
  for(prev=NULL,sig=gl->sigs; sig && sig->signo != signo;
      prev=sig,sig=sig->next)
    ;
  if(sig) {
/*
 * Remove the node from the list.
 */
    if(prev)
      prev->next = sig->next;
    else
      gl->sigs = sig->next;
/*
 * Return the node to the freelist.
 */
    sig = (GlSignalNode *) _del_FreeListNode(gl->sig_mem, sig);
  };
  return 0;
}

/*.......................................................................
 * This function is called when an input line has been completed. It
 * appends the specified newline character, terminates the line,
 * records the line in the history buffer if appropriate, and positions
 * the terminal cursor at the start of the next line.
 *
 * Input:
 *  gl           GetLine *  The resource object of gl_get_line().
 *  newline_char     int    The newline character to add to the end
 *                          of the line.
 *  archive          int    True to have the line archived in the
 *                          history buffer.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
static int gl_line_ended(GetLine *gl, int newline_char, int archive)
{
/*
 * If the newline character is printable, display it.
 */
  if(isprint((int)(unsigned char) newline_char)) {
    if(gl_end_of_line(gl, 1) || gl_add_char_to_line(gl, newline_char))
      return 1;
  } else {
/*
 * Otherwise just append it to the input line buffer.
 */
    gl->line[gl->ntotal++] = newline_char;
    gl->line[gl->ntotal] = '\0';
  };
/*
 * Add the line to the history buffer if it was entered with a
 * newline or carriage return character.
 */
  if(archive)
    (void) _glh_add_history(gl->glh, gl->line, 0);
/*
 * Unless depending on the system-provided line editing, start a new
 * line after the end of the line that has just been entered.
 */
  if(gl->editor != GL_NO_EDITOR) {
    if(gl_end_of_line(gl, 1) ||
       gl_output_raw_string(gl, "\r\n"))
      return 1;
  };
  return 0;
}

/*.......................................................................
 * Return the last signal that was caught by the most recent call to
 * gl_get_line(), or -1 if no signals were caught. This is useful if
 * gl_get_line() returns errno=EINTR and you need to find out what signal
 * caused it to abort.
 *
 * Input:
 *  gl           GetLine *  The resource object of gl_get_line().
 * Output:
 *  return           int    The last signal caught by the most recent
 *                          call to gl_get_line(), or -1 if no signals
 *                          were caught.
 */
int gl_last_signal(const GetLine *gl)
{
  return gl ? gl->last_signal : -1;
}

/*.......................................................................
 * Read a line from the user via a network connection (e.g., telnet).
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 *  prompt      char *  The prompt to prefix the line with.
 *  start_line  char *  The initial contents of the input line, or NULL
 *                      if it should start out empty.
 *  start_pos    int    If start_line isn't NULL, this specifies the
 *                      index of the character over which the cursor
 *                      should initially be positioned within the line.
 *                      If you just want it to follow the last character
 *                      of the line, send -1.
 *  val          int    If non-negative, the single-character value already
 *                      read from the network.
 * Output:
 *  return      char *  An internal buffer containing the input line, or
 *                      NULL at the end of input. If the line fitted in
 *                      the buffer there will be a '\n' newline character
 *                      before the terminating '\0'. If it was truncated
 *                      there will be no newline character, and the remains
 *                      of the line should be retrieved via further calls
 *                      to this function.
 */
char *gl_get_line_net(GetLine *gl, const char *prompt,
		      const char *start_line, int start_pos, int val)
{
  int waserr = 0;    /* True if an error occurs */
  gl->is_net = 1;    /* TODO: assume it is indeed a network connection */
  gl->net_may_block = 0; /* reset every time we call gl_get_line_net() */
  gl->net_read_attempt = 0;
  gl->user_event_value = 0;
/*
 * Check the arguments.
 */
  if(!gl || !prompt) {
    fprintf(stderr, "gl_get_line: NULL argument(s).\n");
    return NULL;
  };
/*
 * If this is the first call to this function since new_GetLine(),
 * complete any postponed configuration.
 */
  if(!gl->configured) {
    (void) gl_configure_getline(gl, NULL, NULL, TECLA_CONFIG_FILE);
    gl->configured = 1;
  };
/*
 * If input is temporarily being taken from a file, return lines
 * from the file until the file is exhausted, then revert to
 * the normal input stream.
 */
  if(gl->file_fp) {
    if(fgets(gl->line, gl->linelen, gl->file_fp))
      return gl->line;
    gl_revert_input(gl);
  };
/*
 * Is input coming from a non-interactive source?
 */
#if 0		/* XXX: the input is suppose to come from the network */
  if(!gl->is_term)
    return fgets(gl->line, gl->linelen, gl->input_fp);
#endif
/*
 * Record the new prompt and its displayed width.
 */
  gl_replace_prompt(gl, prompt);
/*
 * Before installing our signal handler functions, record the fact
 * that there are no pending signals.
 */
  gl_pending_signal = -1;
/*
 * Temporarily override the signal handlers of the calling program,
 * so that we can intercept signals that would leave the terminal
 * in a bad state.
 */
  waserr = gl_override_signal_handlers(gl);
/*
 * After recording the current terminal settings, switch the terminal
 * into raw input mode.
 */
#if 0
  waserr = waserr || gl_raw_terminal_mode(gl);
#endif
/*
 * Attempt to read the line.
 */
  waserr = waserr || gl_get_input_line(gl, start_line, start_pos, val);
/*
 * Restore terminal settings.
 */
#if 0
  gl_restore_terminal_attributes(gl);
#endif
/*
 * Restore the signal handlers.
 */
  gl_restore_signal_handlers(gl);
/*
 * Having restored the program terminal and signal environment,
 * re-submit any signals that were received.
 */
  if(gl_pending_signal != -1) {
    raise(gl_pending_signal);
    waserr = 1;
  };
/*
 * If gl_get_input_line() aborted input due to the user asking to
 * temporarily read lines from a file, read the first line from
 * this file.
 */
  if(!waserr && gl->file_fp)
    return gl_get_line(gl, prompt, NULL, 0);
/*
 * Return the new input line.
 */
  return waserr ? NULL : gl->line;
}

/*.......................................................................
 * Get whether the type of the GetLine object is network.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 * Output:
 *  return   int        If non-zero, the GetLine object type is network.
 *                      buffer.
 */
int gl_is_net(GetLine *gl)
{
    return gl->is_net;
}

/*.......................................................................
 * Set whether the type of the GetLine object is network.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 *  is_net   int        If non-zero, the GetLine object type is network.
 */
void gl_set_is_net(GetLine *gl, int is_net)
{
    gl->is_net = is_net;
}

/*.......................................................................
 * Get the current position of the cursor within the internal buffer.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 * Output:
 *  return   int        The current position of the cursor within the internal
 *                      buffer.
 */
int gl_get_buff_curpos(const GetLine *gl)
{
    return gl->buff_curpos;
}

/*.......................................................................
 * Redraw the current line.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
int
gl_redisplay_line(GetLine *gl)
{
    if (gl != NULL)
	gl->term_curpos = 0;
    return (gl_redisplay(gl, 1));
}

/*.......................................................................
 * Reset the current line.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 * Output:
 *  return   int    0 - OK.
 *                  1 - Error.
 */
int
gl_reset_line(GetLine *gl)
{
/*
 * Clear the buffer.
 */
  gl->ntotal = 0;
  gl->line[0] = '\0';
#if 0	/* TODO: do we need those? */
  gl->number = -1;
  gl->endline = 0;
  gl->vi.command = 0;
  gl->vi.undo.line[0] = '\0';
  gl->vi.undo.ntotal = 0;
  gl->vi.undo.buff_curpos = 0;
  gl->vi.repeat.fn = 0;
  gl->last_signal = -1;
#endif /* 0 */

/*
 * Forget any previous recall session.
 */
  gl->preload_id = 0;
/*
 * Recall the next oldest line in the history list.
 */
  _glh_current_line(gl->glh, gl->line, gl->linelen);
/*
 * Move the terminal cursor to just after the prompt.
 */
  if(gl_place_cursor(gl, 0))
    return 1;
/*
 * Clear from the end of the prompt to the end of the terminal.
 */
  if(gl_output_control_sequence(gl, gl->nline, gl->clear_eod))
    return 1;
  return 0;
}

/*.......................................................................
 * This is the user-defined action function 1.
 */
static KT_KEY_FN(gl_user_event1)
{
  gl->user_event_value = 1;
  return 0;
}

/*.......................................................................
 * This is the user-defined action function 2.
 */
static KT_KEY_FN(gl_user_event2)
{
  gl->user_event_value = 2;
  return 0;
}

/*.......................................................................
 * This is the user-defined action function 3.
 */
static KT_KEY_FN(gl_user_event3)
{
  gl->user_event_value = 3;
  return 0;
}

/*.......................................................................
 * This is the user-defined action function 4.
 */
static KT_KEY_FN(gl_user_event4)
{
  gl->user_event_value = 4;
  return 0;
}

/*.......................................................................
 * Get the current "user-event" value.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 * Output:
 *  return   int        The current "user-event" value.
 */
int
gl_get_user_event(GetLine *gl)
{
  return gl->user_event_value;
}

/*.......................................................................
 * Reset the "user-event" value.
 *
 * Input:
 *  gl       GetLine *  A resource object returned by new_GetLine().
 */
void
gl_reset_user_event(GetLine *gl)
{
  gl->user_event_value = 0;
}

/*.......................................................................
 * Get the action name of the function a key (sequence) has been binded to.
 *
 * Input:
 *  gl       GetLine  * A resource object returned by new_GetLine().
 *  keyseq     char   * The key-sequence to find the action name of the
 *                      binded function of.
 * Output:
 *  return const char * The name of the binded function, or NULL if no binding.
 */
const char *
gl_get_key_binding_action_name(GetLine *gl, const char *keyseq)
{
  const char *kptr;  /* A pointer into keyseq[] */
  char *binary;      /* The binary version of keyseq[] */
  int nc;            /* The number of characters in binary[] */
  int first,last;    /* The first and last entries in the table which */
                     /*  minimally match. */
  int size;          /* The size to allocate for the binary string */
  KeyTab *kt;        /* The table of key-bindings */
  const char *action_name = NULL; /* The action name to return */
  Symbol *sym;   /* The symbol table entry of the action */
/*
 * Check arguments.
 */
  if(gl==NULL || (kt = gl->bindings)==NULL || !keyseq) {
    return NULL;
  };
/*
 * Work out a pessimistic estimate of how much space will be needed
 * for the binary copy of the string, noting that binary meta characters
 * embedded in the input string get split into two characters.
 */
  for(size=0,kptr = keyseq; *kptr; kptr++)
    size += IS_META_CHAR(*kptr) ? 2 : 1;
/*
 * Allocate a string that has the length of keyseq[].
 */
  binary = _new_StringMemString(kt->smem, size + 1);
  if(!binary) {
    fprintf(stderr,
	    "gl_get_key_binding_action_name: Insufficient memory to record key sequence.\n");
    return NULL;
  };
/*
 * Convert control and octal character specifications to binary characters.
 */
  if(_kt_parse_keybinding_string(keyseq, binary, &nc)) {
    binary = _del_StringMemString(kt->smem, binary);
    return NULL;
  };
/*
 * Lookup the position in the table at which there may be binding already.
 * If an exact match for the key-sequence is already in the table,
 * lookup its name and return it.
 */
  if (_kt_lookup_keybinding(kt, binary, nc, &first, &last) == KT_EXACT_MATCH) {
    int i;
    for (i = 0; i < sizeof(gl_actions)/sizeof(gl_actions[0]); i++) {
	if (gl_actions[i].fn == kt->table[first].keyfn) {
	    action_name = gl_actions[i].name;
	    break;
	}
    }
  };
  binary = _del_StringMemString(kt->smem, binary);
  
  return action_name;
}
