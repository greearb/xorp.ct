#ifndef keytab_h
#define keytab_h

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

#include "libtecla.h"
#include "hash.h"
#include "strngmem.h"

/*-----------------------------------------------------------------------*
 * This module defines a binary-search symbol table of key-bindings.     *
 *-----------------------------------------------------------------------*/

/*
 * All key-binding functions are defined as follows.
 *
 * Input:
 *  gl    GetLine *  The resource object of this library.
 *  count     int    A positive repeat count specified by the user,
 *                   or 1. Action functions should ignore this if
 *                   repeating the action multiple times isn't
 *                   appropriate.
 * Output:
 *  return    int    0 - OK.
 *                   1 - Error.
 */
#define KT_KEY_FN(fn) int (fn)(GetLine *gl, int count)

typedef KT_KEY_FN(KtKeyFn);

/*
 * Define an entry of a key-binding binary symbol table.
 */
typedef struct {
  char *keyseq;         /* The key sequence that triggers the macro */
  int nc;               /* The number of characters in keyseq[] */
  KtKeyFn *user_fn;     /* A user specified binding (or 0 if none) */
  KtKeyFn *term_fn;     /* A terminal-specific binding (or 0 if none) */
  KtKeyFn *norm_fn;     /* The default binding (or 0 if none) */
  KtKeyFn *keyfn;       /* The function to execute when this key sequence */
                        /*  is seen. This is the function above which has */
                        /*  the highest priority. */
} KeySym;

/*
 * When allocating or reallocating the key-binding table, how
 * many entries should be added?
 */
#define KT_TABLE_INC 100

/*
 * Define the size of the hash table that is used to associate action
 * names with action functions. This should be a prime number.
 */
#define KT_HASH_SIZE 113

/*
 * Define a binary-symbol-table object.
 */
typedef struct {
  int size;           /* The allocated dimension of table[] */
  int nkey;           /* The current number of members in the table */
  KeySym *table;      /* The table of lexically sorted key sequences */
  HashTable *actions; /* The hash table of actions */
  StringMem *smem;    /* Memory for allocating strings */
} KeyTab;

KeyTab *_new_KeyTab(void);
KeyTab *_del_KeyTab(KeyTab *kt);

/*
 * Enumerate the possible sources of key-bindings.
 */
typedef enum {
  KTB_USER,         /* This is a binding being set by the user */
  KTB_TERM,         /* This is a binding taken from the terminal settings */
  KTB_NORM          /* This is the default binding set by the library */
} KtBinder;

int _kt_set_keybinding(KeyTab *kt, KtBinder binder,
		       const char *keyseq, const char *action);
int _kt_set_keyfn(KeyTab *kt, KtBinder binder, const char *keyseq,
		  KtKeyFn *keyfn);

int _kt_set_action(KeyTab *kt, const char *action, KtKeyFn *fn);

typedef enum {
  KT_EXACT_MATCH,   /* An exact match was found */
  KT_AMBIG_MATCH,   /* An ambiguous match was found */
  KT_NO_MATCH,      /* No match was found */
  KT_BAD_MATCH      /* An error occurred while searching */
} KtKeyMatch;

KtKeyMatch _kt_lookup_keybinding(KeyTab *kt, const char *binary_keyseq,
				 int nc, int *first,int *last);

/*
 * Remove all key bindings that came from a specified source.
 */
void _kt_clear_bindings(KeyTab *kt, KtBinder binder);

/*
 * When installing an array of keybings each binding is defined by
 * an element of the following type:
 */
typedef struct {
  const char *keyseq;   /* The sequence of keys that trigger this binding */
  const char *action;   /* The name of the action function that is triggered */
} KtKeyBinding;

/*
 * Merge an array of bindings with existing bindings.
 */
int _kt_add_bindings(KeyTab *kt, KtBinder binder, const KtKeyBinding *bindings,
		     unsigned n);

/*
 * Convert a keybinding string into a uniq binary representation.
 */
int _kt_parse_keybinding_string(const char *keyseq,
				char *binary, int *nc);

#endif
