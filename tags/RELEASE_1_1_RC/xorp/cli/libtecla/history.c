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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "history.h"
#include "freelist.h"

/*
 * GlLineNode's record the location and length of historical lines in
 * a buffer array.
 */
typedef struct GlLineNode GlLineNode;
struct GlLineNode {
  long id;             /* The unique identifier of this history line */
  time_t timestamp;    /* The time at which the line was archived */
  unsigned group;      /* The identifier of the history group to which the */
                       /*  the line belongs. */
  GlLineNode *next;    /* The next youngest line in the list */
  GlLineNode *prev;    /* The next oldest line in the list */
  int start;           /* The start index of the line in the buffer */
  int nchar;           /* The total length of the line, including the '\0' */
};

/*
 * The number of GlLineNode elements per freelist block.
 */
#define LINE_NODE_BLK 100

/*
 * Lines are organised in the buffer from oldest to newest. The
 * positions of the lines are recorded in a doubly linked list
 * of GlLineNode objects.
 */
typedef struct {
  FreeList *node_mem;    /* A freelist of GlLineNode objects */ 
  GlLineNode *head;      /* The head of the list of lines */
  GlLineNode *tail;      /* The tail of the list of lines */
} GlLineList;

/*
 * All elements of the history mechanism are recorded in an object of
 * the following type.
 */
struct GlHistory {
  char *buffer;       /* A circular buffer used to record historical input */
                      /*  lines. */
  size_t buflen;      /* The length of the buffer array */
  GlLineList list;    /* A list of the start of lines in buffer[] */
  GlLineNode *recall; /* The last line recalled, or NULL if no recall */
                      /*  session is currently active. */
  GlLineNode *id_node;/* The node at which the last ID search terminated */
  const char *prefix; /* A pointer to the line containing the prefix that */
                      /*  is being searched for. */
  int prefix_len;     /* The length of the prefix */
  unsigned long seq;  /* The next ID to assign to a line node */
  unsigned group;     /* The identifier of the current history group */
  int nline;          /* The number of lines currently in the history list */
  int max_lines;      /* Either -1 or a ceiling on the number of lines */
  int enable;         /* If false, ignore history additions and lookups */
};

static char *_glh_restore_line(GlHistory *glh, char *line, size_t dim);
static int _glh_cant_load_history(GlHistory *glh, const char *filename,
				  int lineno, const char *message, FILE *fp);
static int _glh_write_timestamp(FILE *fp, time_t timestamp);
static int _glh_decode_timestamp(char *string, char **endp, time_t *t);
static void _glh_discard_node(GlHistory *glh, GlLineNode *node);
static GlLineNode *_glh_find_id(GlHistory *glh, GlhLineID id);

/*.......................................................................
 * Create a line history maintenance object.
 *
 * Input:
 *  buflen     size_t    The number of bytes to allocate to the circular
 *                       buffer that is used to record all of the
 *                       most recent lines of user input that will fit.
 *                       If buflen==0, no buffer will be allocated.
 * Output:
 *  return  GlHistory *  The new object, or NULL on error.
 */
GlHistory *_new_GlHistory(size_t buflen)
{
  GlHistory *glh;  /* The object to be returned */
/*
 * Allocate the container.
 */
  glh = (GlHistory *) malloc(sizeof(GlHistory));
  if(!glh) {
    fprintf(stderr, "_new_GlHistory: Insufficient memory.\n");
    return NULL;
  };
/*
 * Before attempting any operation that might fail, initialize the
 * container at least up to the point at which it can safely be passed
 * to _del_GlHistory().
 */
  glh->buffer = NULL;
  glh->buflen = buflen;
  glh->list.node_mem = NULL;
  glh->list.head = NULL;
  glh->list.tail = NULL;
  glh->recall = NULL;
  glh->id_node = NULL;
  glh->prefix = NULL;
  glh->prefix_len = 0;
  glh->seq = 0;
  glh->group = 0;
  glh->nline = 0;
  glh->max_lines = -1;
  glh->enable = 1;
/*
 * Allocate the buffer, if required.
 */
  if(buflen > 0) {
    glh->buffer = (char *) malloc(sizeof(char) * buflen);
    if(!glh->buffer) {
      fprintf(stderr, "_new_GlHistory: Insufficient memory.\n");
      return _del_GlHistory(glh);
    };
  };
/*
 * Allocate the GlLineNode freelist.
 */
  glh->list.node_mem = _new_FreeList("_new_GlHistory", sizeof(GlLineNode),
				     LINE_NODE_BLK);
  if(!glh->list.node_mem)
    return _del_GlHistory(glh);
  return glh;
}

/*.......................................................................
 * Delete a GlHistory object.
 *
 * Input:
 *  glh    GlHistory *  The object to be deleted.
 * Output:
 *  return GlHistory *  The deleted object (always NULL).
 */
GlHistory *_del_GlHistory(GlHistory *glh)
{
  if(glh) {
/*
 * Delete the buffer.
 */
    if(glh->buffer) {
      free(glh->buffer);
      glh->buffer = NULL;
    };
/*
 * Delete the freelist of GlLineNode's.
 */
    glh->list.node_mem = _del_FreeList("_del_GlHistory", glh->list.node_mem, 1);
/*
 * The contents of the list were deleted by deleting the freelist.
 */
    glh->list.head = NULL;
    glh->list.tail = NULL;
/*
 * Delete the container.
 */
    free(glh);
  };
  return NULL;
}

/*.......................................................................
 * Add a new line to the end of the history buffer, wrapping round to the
 * start of the buffer if needed.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The line to be archived.
 *  force      int    Unless this flag is non-zero, empty lines and
 *                    lines which match the previous line in the history
 *                    buffer, aren't archived. This flag requests that
 *                    the line be archived regardless.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
int _glh_add_history(GlHistory *glh, const char *line, int force)
{
  GlLineList *list; /* The line location list */
  int nchar;        /* The number of characters needed to record the line */
  GlLineNode *node; /* The new line location list node */
  int empty;        /* True if the string is empty */
  const char *nlptr;/* A pointer to a newline character in line[] */
  int i;
/*
 * Check the arguments.
 */
  if(!glh || !line)
    return 1;
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return 0;
/*
 * Get the line location list.
 */
  list = &glh->list;
/*
 * Cancel any ongoing search.
 */
  if(_glh_cancel_search(glh))
    return 1;
/*
 * See how much buffer space will be needed to record the line?
 *
 * If the string contains a terminating newline character, arrange to
 * have the archived line NUL terminated at this point.
 */
  nlptr = strchr(line, '\n');
  if(nlptr)
    nchar = (nlptr - line) + 1;
  else
    nchar = strlen(line) + 1;
/*
 * If the line is too big to fit in the buffer, truncate it.
 */
  if(nchar > glh->buflen)
    nchar = glh->buflen;
/*
 * Is the line empty?
 */
  empty = 1;
  for(i=0; i<nchar-1 && empty; i++)
    empty = isspace((int)(unsigned char) line[i]);
/*
 * If the line is empty, don't add it to the buffer unless explicitly
 * told to.
 */
  if(empty && !force)
    return 0;
/*
 * If the new line is the same as the most recently added line,
 * don't add it again, unless explicitly told to.
 */
  if(!force &&
     list->tail && strlen(glh->buffer + list->tail->start) == nchar-1 &&
     strncmp(line, glh->buffer + list->tail->start, nchar-1)==0)
    return 0;
/*
 * Allocate the list node that will record the line location.
 */
  node = (GlLineNode *) _new_FreeListNode(list->node_mem);
  if(!node)
    return 1;
/*
 * Is the buffer empty?
 */
  if(!list->head) {
/*
 * Place the line at the beginning of the buffer.
 */
    strncpy(glh->buffer, line, nchar);
    glh->buffer[nchar-1] = '\0';
/*
 * Record the location of the line.
 */
    node->start = 0;
/*
 * The buffer has one or more lines in it.
 */
  } else {
/*
 * Place the start of the new line just after the most recently
 * added line.
 */
    int start = list->tail->start + list->tail->nchar;
/*
 * If there is insufficient room between the end of the most
 * recently added line and the end of the buffer, we place the
 * line at the beginning of the buffer. To make as much space
 * as possible for this line, we first delete any old lines
 * at the end of the buffer, then shift the remaining contents
 * of the buffer to the end of the buffer.
 */
    if(start + nchar >= glh->buflen) {
      GlLineNode *last; /* The last line in the buffer */
      GlLineNode *ln;   /* A member of the list of line locations */
      int shift;        /* The shift needed to move the contents of the */
                        /*  buffer to its end. */
/*
 * Delete any old lines between the most recent line and the end of the
 * buffer.
 */
      while(list->head && list->head->start > list->tail->start)
	_glh_discard_node(glh, list->head);
/*
 * Find the line that is nearest the end of the buffer.
 */
      last = NULL;
      for(ln=list->head; ln; ln=ln->next) {
	if(!last || ln->start > last->start)
	  last = ln;
      };
/*
 * How big a shift is needed to move the existing contents of the
 * buffer to the end of the buffer?
 */
      shift = last ? (glh->buflen - (last->start + last->nchar)) : 0;
/*
 * Is any shift needed?
 */
      if(shift > 0) {
/*
 * Move the buffer contents to the end of the buffer.
 */
	memmove(glh->buffer + shift, glh->buffer, glh->buflen - shift);
/*
 * Update the listed locations to reflect the shift.
 */
	for(ln=list->head; ln; ln=ln->next)
	  ln->start += shift;
      };
/*
 * The new line should now be located at the start of the buffer.
 */
      start = 0;
    };
/*
 * Make space for the new line at the beginning of the buffer by
 * deleting the oldest lines. This just involves removing them
 * from the list of used locations. Also enforce the current
 * maximum number of lines.
 */
    while(list->head &&
	  ((list->head->start >= start && list->head->start - start < nchar) ||
	   (glh->max_lines >= 0 && glh->nline>=glh->max_lines))) {
      _glh_discard_node(glh, list->head);
    };
/*
 * Copy the new line into the buffer.
 */
    memcpy(glh->buffer + start, line, nchar);
    glh->buffer[start + nchar - 1] = '\0';
/*
 * Record its location.
 */
    node->start = start;
  };
/*
 * Append the line location node to the end of the list.
 */
  node->id = glh->seq++;
  node->timestamp = time(NULL);
  node->group = glh->group;
  node->nchar = nchar;
  node->next = NULL;
  node->prev = list->tail;
  if(list->tail)
    list->tail->next = node;
  else
    list->head = node;
  list->tail = node;
  glh->nline++;
  return 0;
}

/*.......................................................................
 * Recall the next oldest line that has the search prefix last recorded
 * by _glh_search_prefix().
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, if anything
 *                    was found, its contents will have been replaced
 *                    with the matching line.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  A pointer to line[0], or NULL if not found.
 */
char *_glh_find_backwards(GlHistory *glh, char *line, size_t dim)
{
  GlLineNode *node; /* The line location node being checked */
  int first;        /* True if this is the start of a new search */
/*
 * Check the arguments.
 */
  if(!glh || !line) {
    fprintf(stderr, "_glh_find_backwards: NULL argument(s).\n");
    return NULL;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return NULL;
/*
 * Check the line dimensions.
 */
  if(dim < strlen(line) + 1) {
    fprintf(stderr,
       "_glh_find_backwards: 'dim' inconsistent with strlen(line) contents.\n");
    return NULL;
  };
/*
 * Is this the start of a new search?
 */
  first = glh->recall==NULL;
/*
 * If this is the first search backwards, save the current line
 * for potential recall later, and mark it as the last line
 * recalled.
 */
  if(first) {
    if(_glh_add_history(glh, line, 1))
      return NULL;
    glh->recall = glh->list.tail;
  };
/*
 * If there is no search prefix, the prefix last set by glh_search_prefix()
 * doesn't exist in the history buffer.
 */
  if(!glh->prefix)
    return NULL;
/*
 * From where should we start the search?
 */
  if(glh->recall)
    node = glh->recall->prev;
  else
    node = glh->list.tail;
/*
 * Search backwards through the list for the first match with the
 * prefix string.
 */
  for( ; node &&
      (node->group != glh->group ||
       strncmp(glh->buffer + node->start, glh->prefix, glh->prefix_len) != 0);
      node = node->prev)
    ;
/*
 * Was a matching line found?
 */
  if(node) {
/*
 * Recall the found node as the starting point for subsequent
 * searches.
 */
    glh->recall = node;
/*
 * Copy the matching line into the provided line buffer.
 */
    strncpy(line, glh->buffer + node->start, dim);
    line[dim-1] = '\0';
    return line;
  };
/*
 * No match was found.
 */
  return NULL;
}

/*.......................................................................
 * Recall the next newest line that has the search prefix last recorded
 * by _glh_search_prefix().
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, if anything
 *                    was found, its contents will have been replaced
 *                    with the matching line.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  The line requested, or NULL if no matching line
 *                    was found.
 */
char *_glh_find_forwards(GlHistory *glh, char *line, size_t dim)
{
  GlLineNode *node; /* The line location node being checked */
/*
 * Check the arguments.
 */
  if(!glh || !line) {
    fprintf(stderr, "_glh_find_forwards: NULL argument(s).\n");
    return NULL;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return NULL;
/*
 * Check the line dimensions.
 */
  if(dim < strlen(line) + 1) {
    fprintf(stderr,
       "_glh_find_forwards: 'dim' inconsistent with strlen(line) contents.\n");
    return NULL;
  };
/*
 * From where should we start the search?
 */
  if(glh->recall)
    node = glh->recall->next;
  else
    return NULL;
/*
 * If there is no search prefix, the prefix last set by glh_search_prefix()
 * doesn't exist in the history buffer.
 */
  if(!glh->prefix)
    return NULL;
/*
 * Search forwards through the list for the first match with the
 * prefix string.
 */
  for( ; node &&
      (node->group != glh->group ||
       strncmp(glh->buffer + node->start, glh->prefix, glh->prefix_len) != 0);
      node = node->next)
    ;
/*
 * Was a matching line found?
 */
  if(node) {
/*
 * Did we hit the line that was originally being edited when the
 * current history traversal started?
 */
    if(node == glh->list.tail)
      return _glh_restore_line(glh, line, dim);
/*
 * Copy the matching line into the provided line buffer.
 */
    strncpy(line, glh->buffer + node->start, dim);
    line[dim-1] = '\0';
/*
 * Record the starting point of the next search.
 */
    glh->recall = node;
/*
 * Return the matching line to the user.
 */
    return line;
  };
/*
 * No match was found.
 */
  return NULL;
}

/*.......................................................................
 * If a search is in progress, cancel it.
 *
 * This involves discarding the line that was temporarily saved by
 * _glh_find_backwards() when the search was originally started,
 * and reseting the search iteration pointer to NULL.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
int _glh_cancel_search(GlHistory *glh)
{
/*
 * Check the arguments.
 */
  if(!glh) {
    fprintf(stderr, "_glh_cancel_search: NULL argument(s).\n");
    return 1;
  };
/*
 * If there wasn't a search in progress, do nothing.
 */
  if(!glh->recall)
    return 0;
/*
 * Delete the node of the preserved line.
 */
  _glh_discard_node(glh, glh->list.tail);
/*
 * Reset the search pointers.
 */
  glh->recall = NULL;
  glh->prefix = "";
  glh->prefix_len = 0;
  return 0;
}

/*.......................................................................
 * Set the prefix of subsequent history searches.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The command line who's prefix is to be used.
 *  prefix_len int    The length of the prefix.
 * Output:
 *  return     int    0 - OK.
 *                    1 - Error.
 */
int _glh_search_prefix(GlHistory *glh, const char *line, int prefix_len)
{
  GlLineNode *node; /* The line location node being checked */
/*
 * Check the arguments.
 */
  if(!glh) {
    fprintf(stderr, "_glh_search_prefix: NULL argument(s).\n");
    return 1;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return 0;
/*
 * Record a zero length search prefix?
 */
  if(prefix_len <= 0) {
    glh->prefix_len = 0;
    glh->prefix = "";
    return 0;
  };
/*
 * Record the length of the new search prefix.
 */
  glh->prefix_len = prefix_len;
/*
 * If any history line starts with the specified prefix, record a
 * pointer to it for comparison in subsequent searches. If the prefix
 * doesn't match any of the lines, then simply record NULL to indicate
 * that there is no point in searching. Note that _glh_add_history()
 * clears this pointer by calling _glh_cancel_search(), so there is
 * no danger of it being used after the buffer has been modified.
 */
  for(node = glh->list.tail ; node &&
      (node->group != glh->group ||
       strncmp(glh->buffer + node->start, line, prefix_len) != 0);
      node = node->prev)
    ;
/*
 * If a matching line was found record it for use as the search
 * prefix.
 */
  glh->prefix = node ? glh->buffer + node->start : NULL;
  return 0;
}

/*.......................................................................
 * Recall the oldest recorded line.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, its contents
 *                    will have been replaced with the oldest line.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  A pointer to line[0], or NULL if not found.
 */
char *_glh_oldest_line(GlHistory *glh, char *line, size_t dim)
{
  GlLineNode *node; /* The line location node being checked */
  int first;        /* True if this is the start of a new search */
/*
 * Check the arguments.
 */
  if(!glh || !line) {
    fprintf(stderr, "_glh_oldest_line: NULL argument(s).\n");
    return NULL;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return NULL;
/*
 * Check the line dimensions.
 */
  if(dim < strlen(line) + 1) {
    fprintf(stderr,
       "_glh_oldest_line: 'dim' inconsistent with strlen(line) contents.\n");
    return NULL;
  };
/*
 * Is this the start of a new search?
 */
  first = glh->recall==NULL;
/*
 * If this is the first search backwards, save the current line
 * for potential recall later, and mark it as the last line
 * recalled.
 */
  if(first) {
    if(_glh_add_history(glh, line, 1))
      return NULL;
    glh->recall = glh->list.tail;
  };
/*
 * Locate the oldest line that belongs to the current group.
 */
  for(node=glh->list.head; node && node->group != glh->group; 
      node = node->next)
    ;
/*
 * No line found?
 */
  if(!node)
    return NULL;
/*
 * Record the above node as the starting point for subsequent
 * searches.
 */
  glh->recall = node;
/*
 * Copy the recalled line into the provided line buffer.
 */
  strncpy(line, glh->buffer + node->start, dim);
  line[dim-1] = '\0';
  return line;
}

/*.......................................................................
 * Recall the line that was being entered when the search started.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, its contents
 *                    will have been replaced with the line that was
 *                    being entered when the search was started.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  A pointer to line[0], or NULL if not found.
 */
char *_glh_current_line(GlHistory *glh, char *line, size_t dim)
{
/*
 * Check the arguments.
 */
  if(!glh || !line) {
    fprintf(stderr, "_glh_current_line: NULL argument(s).\n");
    return NULL;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return NULL;
/*
 * Check the line dimensions.
 */
  if(dim < strlen(line) + 1) {
    fprintf(stderr,
       "_glh_current_line: 'dim' inconsistent with strlen(line) contents.\n");
    return NULL;
  };
/*
 * Restore the original line.
 */
  return _glh_restore_line(glh, line, dim);
}

/*.......................................................................
 * Remove the line that was originally being edited when the history
 * traversal was started, from its saved position in the history list,
 * and place it in the provided line buffer.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, its contents
 *                    will have been replaced with the saved line.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  A pointer to line[0], or NULL if not found.
 */
static char *_glh_restore_line(GlHistory *glh, char *line, size_t dim)
{
  GlLineNode *tail;   /* The tail node to be discarded */
/*
 * If there wasn't a search in progress, do nothing.
 */
  if(!glh->recall)
    return NULL;
/*
 * Get the list node that is to be removed.
 */
  tail = glh->list.tail;
/*
 * If a pointer to the saved line is being used to record the
 * current search prefix, reestablish the search prefix, to
 * have it recorded by another history line if possible.
 */
  if(glh->prefix == glh->buffer + tail->start)
    (void) _glh_search_prefix(glh, glh->buffer + tail->start, glh->prefix_len);
/*
 * Copy the recalled line into the input-line buffer.
 */
  strncpy(line, glh->buffer + tail->start, dim);
  line[dim-1] = '\0';
/*
 * Discard the line-location node.
 */
  _glh_discard_node(glh, tail);
/*
 * Mark the search as ended.
 */
  glh->recall = NULL;
  return line;
}

/*.......................................................................
 * Query the id of a history line offset by a given number of lines from
 * the one that is currently being recalled. If a recall session isn't
 * in progress, or the offset points outside the history list, 0 is
 * returned.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  offset       int    The line offset (0 for the current line, < 0
 *                      for an older line, > 0 for a newer line.
 * Output:
 *  return GlhLineID    The identifier of the line that is currently
 *                      being recalled, or 0 if no recall session is
 *                      currently in progress.
 */
GlhLineID _glh_line_id(GlHistory *glh, int offset)
{
  GlLineNode *node; /* The line location node being checked */
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return 0;
/*
 * Search forward 'offset' lines to find the required line.
 */
  if(offset >= 0) {
    for(node=glh->recall; node && offset != 0; node=node->next) {
      if(node->group == glh->group)
	offset--;
    };
  } else {
    for(node=glh->recall; node && offset != 0; node=node->prev) {
      if(node->group == glh->group)
	offset++;
    };
  };
  return node ? node->id : 0;
}

/*.......................................................................
 * Recall a line by its history buffer ID. If the line is no longer
 * in the buffer, or the id is zero, NULL is returned.
 *
 * Input:
 *  glh  GlHistory *  The input-line history maintenance object.
 *  id   GlhLineID    The ID of the line to be returned.
 *  line      char *  The input line buffer. On input this should contain
 *                    the current input line, and on output, its contents
 *                    will have been replaced with the saved line.
 *  dim     size_t    The allocated dimensions of the line buffer.
 * Output:
 *  return    char *  A pointer to line[0], or NULL if not found.
 */
char *_glh_recall_line(GlHistory *glh, GlhLineID id, char *line, size_t dim)
{
  GlLineNode *node; /* The line location node being checked */
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->buffer || glh->max_lines == 0)
    return NULL;
/*
 * If we are starting a new recall session, save the current line
 * for potential recall later.
 */
  if(!glh->recall && _glh_add_history(glh, line, 1))
    return NULL;
/*
 * Search for the specified line.
 */
  node = _glh_find_id(glh, id);
/*
 * Not found?
 */
  if(!node || node->group != glh->group)
    return NULL;
/*
 * Record the node of the matching line as the starting point
 * for subsequent searches.
 */
  glh->recall = node;
/*
 * Copy the recalled line into the provided line buffer.
 */
  strncpy(line, glh->buffer + node->start, dim);
  line[dim-1] = '\0';
  return line;
}

/*.......................................................................
 * Save the current history in a specified file.
 *
 * Input:
 *  glh        GlHistory *  The input-line history maintenance object.
 *  filename  const char *  The name of the new file to record the
 *                          history in.
 *  comment   const char *  Extra information such as timestamps will
 *                          be recorded on a line started with this
 *                          string, the idea being that the file can
 *                          double as a command file. Specify "" if
 *                          you don't care.
 *  max_lines        int    The maximum number of lines to save, or -1
 *                          to save all of the lines in the history
 *                          list.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int _glh_save_history(GlHistory *glh, const char *filename, const char *comment,
		      int max_lines)
{
  FILE *fp;         /* The output file */
  GlLineNode *node; /* The line being saved */
  GlLineNode *head; /* The head of the list of lines to be saved */
/*
 * Check the arguments.
 */
  if(!glh || !filename || !comment) {
    fprintf(stderr, "_glh_save_history: NULL argument(s).\n");
    return 1;
  };
/*
 * Attempt to open the specified file.
 */
  fp = fopen(filename, "w");
  if(!fp) {
    fprintf(stderr, "_glh_save_history: Can't open %s (%s).\n",
	    filename, strerror(errno));
    return 1;
  };
/*
 * If a ceiling on the number of lines to save was specified, count
 * that number of lines backwards, to find the first line to be saved.
 */
  head = NULL;
  if(max_lines >= 0) {
    for(head=glh->list.tail; head && --max_lines > 0; head=head->prev)
      ;
  };
  if(!head)
    head = glh->list.head;
/*
 * Write the contents of the history buffer to the history file, writing
 * associated data such as timestamps, to a line starting with the
 * specified comment string.
 */
  for(node=head; node; node=node->next) {
/*
 * Write peripheral information associated with the line, as a comment.
 */
    if(fprintf(fp, "%s ", comment) < 0 ||
       _glh_write_timestamp(fp, node->timestamp) ||
       fprintf(fp, " %u\n", node->group) < 0) {
      fprintf(stderr, "Error writing %s (%s).\n", filename, strerror(errno));
      (void) fclose(fp);
      return 1;
    };
/*
 * Write the history line.
 */
    if(fprintf(fp, "%s\n", glh->buffer + node->start) < 0) {
      fprintf(stderr, "Error writing %s (%s).\n", filename, strerror(errno));
      (void) fclose(fp);
      return 1;
    };
  };
/*
 * Close the history file.
 */
  if(fclose(fp) == EOF) {
    fprintf(stderr, "Error writing %s (%s).\n", filename, strerror(errno));
    return 1;
  };
  return 0;
}

/*.......................................................................
 * Restore previous history lines from a given file.
 *
 * Input:
 *  glh        GlHistory *  The input-line history maintenance object.
 *  filename  const char *  The name of the file to read from.
 *  comment   const char *  The same comment string that was passed to
 *                          _glh_save_history() when this file was
 *                          written.
 *  line            char *  A buffer into which lines can be read.
 *  dim            size_t   The allocated dimension of line[].
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int _glh_load_history(GlHistory *glh, const char *filename, const char *comment,
		      char *line, size_t dim)
{
  FILE *fp;            /* The output file */
  size_t comment_len;  /* The length of the comment string */
  time_t timestamp;    /* The timestamp of the history line */
  unsigned group;      /* The identifier of the history group to which */
                       /*  the line belongs. */
  int lineno;          /* The line number being read */
/*
 * Check the arguments.
 */
  if(!glh || !filename || !comment || !line) {
    fprintf(stderr, "_glh_load_history: NULL argument(s).\n");
    return 1;
  };
/*
 * Measure the length of the comment string.
 */
  comment_len = strlen(comment);
/*
 * Clear the history list.
 */
  _glh_clear_history(glh, 1);
/*
 * Attempt to open the specified file. Don't treat it as an error
 * if the file doesn't exist.
 */
  fp = fopen(filename, "r");
  if(!fp)
    return 0;
/*
 * Attempt to read each line and preceding peripheral info, and add these
 * to the history list.
 */
  for(lineno=1; fgets(line, dim, fp) != NULL; lineno++) {
    char *lptr;          /* A pointer into the input line */
/*
 * Check that the line starts with the comment string.
 */
    if(strncmp(line, comment, comment_len) != 0) {
      return _glh_cant_load_history(glh, filename, lineno,
				    "Corrupt history parameter line", fp);
    };
/*
 * Skip spaces and tabs after the comment.
 */
    for(lptr=line+comment_len; *lptr && (*lptr==' ' || *lptr=='\t'); lptr++)
      ;
/*
 * The next word must be a timestamp.
 */
    if(_glh_decode_timestamp(lptr, &lptr, &timestamp)) {
      return _glh_cant_load_history(glh, filename, lineno,
				    "Corrupt timestamp", fp);
    };
/*
 * Skip spaces and tabs.
 */
    while(*lptr==' ' || *lptr=='\t')
      lptr++;
/*
 * The next word must be an unsigned integer group number.
 */
    group = (int) strtoul(lptr, &lptr, 10);
    if(*lptr != ' ' && *lptr != '\n') {
      return _glh_cant_load_history(glh, filename, lineno,
				    "Corrupt group id", fp);
    };
/*
 * Skip spaces and tabs.
 */
    while(*lptr==' ' || *lptr=='\t')
      lptr++;
/*
 * There shouldn't be anything left on the line.
 */
    if(*lptr != '\n') {
      return _glh_cant_load_history(glh, filename, lineno,
				    "Corrupt parameter line", fp);
    };
/*
 * Now read the history line itself.
 */
    lineno++;
    if(fgets(line, dim, fp) == NULL)
      return _glh_cant_load_history(glh, filename, lineno, "Read error", fp);
/*
 * Append the line to the history buffer.
 */
    if(_glh_add_history(glh, line, 1)) {
      return _glh_cant_load_history(glh, filename, lineno,
				    "Insufficient memory to record line", fp);
    };
/*
 * Record the group and timestamp information along with the line.
 */
    if(glh->list.tail) {
      glh->list.tail->timestamp = timestamp;
      glh->list.tail->group = group;
    };
  };
/*
 * Close the file.
 */
  (void) fclose(fp);
  return 0;
}

/*.......................................................................
 * This is a private error return function of _glh_load_history().
 */
static int _glh_cant_load_history(GlHistory *glh, const char *filename,
				  int lineno, const char *message, FILE *fp)
{
  fprintf(stderr, "%s:%d: %s.\n", filename, lineno, message);
  (void) fclose(fp);
  return 1;
}

/*.......................................................................
 * Switch history groups.
 *
 * Input:
 *  glh        GlHistory *  The input-line history maintenance object.
 *  group      unsigned    The new group identifier. This will be recorded
 *                          with subsequent history lines, and subsequent
 *                          history searches will only return lines with
 *                          this group identifier. This allows multiple
 *                          separate history lists to exist within
 *                          a single GlHistory object. Note that the
 *                          default group identifier is 0.
 * Output:
 *  return           int    0 - OK.
 *                          1 - Error.
 */
int _glh_set_group(GlHistory *glh, unsigned group)
{
/*
 * Check the arguments.
 */
  if(!glh) {
    fprintf(stderr, "_glh_set_group: NULL argument(s).\n");
    return 1;
  };
/*
 * Is the group being changed?
 */
  if(group != glh->group) {
/*
 * Cancel any ongoing search.
 */
    if(_glh_cancel_search(glh))
      return 1;
/*
 * Record the new group.
 */
    glh->group = group;
  };
  return 0;
}

/*.......................................................................
 * Query the current history group.
 *
 * Input:
 *  glh        GlHistory *  The input-line history maintenance object.
 * Output:
 *  return      unsigned    The group identifier.    
 */
int _glh_get_group(GlHistory *glh)
{
  return glh ? glh->group : 0;
}

/*.......................................................................
 * Write a timestamp to a given stdio stream, in the format
 * yyyymmddhhmmss
 *
 * Input:
 *  fp             FILE *  The stream to write to.
 *  timestamp    time_t    The timestamp to be written.
 * Output:
 *  return          int    0 - OK.
 *                         1 - Error.
 */
static int _glh_write_timestamp(FILE *fp, time_t timestamp)
{
  struct tm *t;  /* THe broken-down calendar time */
/*
 * Get the calendar components corresponding to the given timestamp.
 */
  if(timestamp < 0 || (t = localtime(&timestamp)) == NULL) {
    if(fprintf(fp, "?") < 0)
      return 1;
    return 0;
  };
/*
 * Write the calendar time as yyyymmddhhmmss.
 */
  if(fprintf(fp, "%04d%02d%02d%02d%02d%02d", t->tm_year + 1900, t->tm_mon + 1,
	     t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec) < 0)
    return 1;
  return 0;
}

/*.......................................................................
 * Read a timestamp from a string.
 *
 * Input:
 *  string    char *  The string to read from.
 * Input/Output:
 *  endp        char ** On output *endp will point to the next unprocessed
 *                      character in string[].
 *  timestamp time_t *  The timestamp will be assigned to *t.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
static int _glh_decode_timestamp(char *string, char **endp, time_t *timestamp)
{
  unsigned year,month,day,hour,min,sec;  /* Calendar time components */
  struct tm t;
/*
 * There are 14 characters in the date format yyyymmddhhmmss.
 */
  enum {TSLEN=14};                    
  char timestr[TSLEN+1];   /* The timestamp part of the string */
/*
 * If the time wasn't available at the time that the line was recorded
 * it will have been written as "?". Check for this before trying
 * to read the timestamp.
 */
  if(string[0] == '\?') {
    *endp = string+1;
    *timestamp = -1;
    return 0;
  };
/*
 * The timestamp is expected to be written in the form yyyymmddhhmmss.
 */
  if(strlen(string) < TSLEN) {
    *endp = string;
    return 1;
  };
/*
 * Copy the timestamp out of the string.
 */
  strncpy(timestr, string, TSLEN);
  timestr[TSLEN] = '\0';
/*
 * Decode the timestamp.
 */
  if(sscanf(timestr, "%4u%2u%2u%2u%2u%2u", &year, &month, &day, &hour, &min,
	    &sec) != 6) {
    *endp = string;
    return 1;
  };
/*
 * Advance the string pointer over the successfully read timestamp.
 */
  *endp = string + TSLEN;
/*
 * Copy the read values into a struct tm.
 */
  t.tm_sec = sec;
  t.tm_min = min;
  t.tm_hour = hour;
  t.tm_mday = day;
  t.tm_wday = 0;
  t.tm_yday = 0;
  t.tm_mon = month - 1;
  t.tm_year = year - 1900;
  t.tm_isdst = -1;
/*
 * Convert the contents of the struct tm to a time_t.
 */
  *timestamp = mktime(&t);
  return 0;
}

/*.......................................................................
 * Display the contents of the history list.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  fp          FILE *  The stdio stream to write to.
 *  fmt   const char *  A format string. This can contain arbitrary
 *                      characters, which are written verbatim, plus
 *                      any of the following format directives:
 *                        %D  -  The date, like 2001-11-20
 *                        %T  -  The time of day, like 23:59:59
 *                        %N  -  The sequential entry number of the
 *                               line in the history buffer.
 *                        %G  -  The history group number of the line.
 *                        %%  -  A literal % character.
 *                        %H  -  The history line.
 *  all_groups   int    If true, display history lines from all
 *                      history groups. Otherwise only display
 *                      those of the current history group.
 *  max_lines    int    If max_lines is < 0, all available lines
 *                      are displayed. Otherwise only the most
 *                      recent max_lines lines will be displayed.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Error.
 */
int _glh_show_history(GlHistory *glh, FILE *fp, const char *fmt,
		      int all_groups, int max_lines)
{
  GlLineNode *node;      /* The line being displayed */
  GlLineNode *oldest;    /* The oldest line to display */
  enum {TSMAX=32};       /* The maximum length of the date and time string */
  char buffer[TSMAX+1];  /* The buffer in which to write the date and time */
  int idlen;             /* The length of displayed ID strings */
  unsigned grpmax;       /* The maximum group number in the buffer */
  int grplen;            /* The number of characters needed to print grpmax */
/*
 * Check the arguments.
 */
  if(!glh || !fp || !fmt) {
    fprintf(stderr, "_glh_show_history: NULL argument(s).\n");
    return 1;
  };
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->list.head)
    return 0;
/*
 * Work out the length to display ID numbers, choosing the length of
 * the biggest number in the buffer. Smaller numbers will be padded 
 * with leading zeroes if needed.
 */
  sprintf(buffer, "%lu", (unsigned long) glh->list.tail->id);
  idlen = strlen(buffer);
/*
 * Find the largest group number.
 */
  grpmax = 0;
  for(node=glh->list.head; node; node=node->next) {
    if(node->group > grpmax)
      grpmax = node->group;
  };
/*
 * Find out how many characters are needed to display the group number.
 */
  sprintf(buffer, "%u", (unsigned) grpmax);
  grplen = strlen(buffer);
/*
 * Find the node that follows the oldest line to be displayed.
 */
  if(max_lines < 0) {
    oldest = glh->list.head;
  } else if(max_lines==0) {
    return 0;
  } else {
    for(oldest=glh->list.tail; oldest; oldest=oldest->prev) {
      if((all_groups || oldest->group == glh->group) && --max_lines <= 0)
	break;
    };
/*
 * If the number of lines in the buffer doesn't exceed the specified
 * maximum, start from the oldest line in the buffer.
 */
    if(!oldest)
      oldest = glh->list.head;
  };
/*
 * List the history lines in increasing time order.
 */
  for(node=oldest; node; node=node->next) {
/*
 * Only display lines from the current history group, unless
 * told otherwise.
 */
    if(all_groups || node->group == glh->group) {
      const char *fptr;      /* A pointer into the format string */
      struct tm *t = NULL;   /* The broken time version of the timestamp */
/*
 * Work out the calendar representation of the node timestamp.
 */
      if(node->timestamp != (time_t) -1)
	t = localtime(&node->timestamp);
/*
 * Parse the format string.
 */
      fptr = fmt;
      while(*fptr) {
/*
 * Search for the start of the next format directive or the end of the string.
 */
	const char *start = fptr;
	while(*fptr && *fptr != '%')
	  fptr++;
/*
 * Display any literal characters that precede the located directive.
 */
	if(fptr > start && fprintf(fp, "%.*s", (int) (fptr - start), start) < 0)
	  return 1;
/*
 * Did we hit a new directive before the end of the line?
 */
	if(*fptr) {
/*
 * Obey the directive. Ignore unknown directives.
 */
	  switch(*++fptr) {
	  case 'D':          /* Display the date */
	    if(t && strftime(buffer, TSMAX, "%Y-%m-%d", t) != 0 &&
	       fprintf(fp, "%s", buffer) < 0)
	      return 1;
	    break;
	  case 'T':          /* Display the time of day */
	    if(t && strftime(buffer, TSMAX, "%H:%M:%S", t) != 0 &&
	       fprintf(fp, "%s", buffer) < 0)
	      return 1;
	    break;
	  case 'N':          /* Display the sequential entry number */
	    if(fprintf(fp, "%*lu", idlen, (unsigned long) node->id) < 0)
	      return 1;
	    break;
	  case 'G':
	    if(fprintf(fp, "%*u", grplen, (unsigned) node->group) < 0)
	      return 1;
	    break;
	  case 'H':          /* Display the history line */
	    if(fprintf(fp, "%s", glh->buffer + node->start) < 0)
	      return 1;
	    break;
	  case '%':          /* A literal % symbol */
	    if(fputc('%', fp) == EOF)
	      return 1;
	    break;
	  };
/*
 * Skip the directive.
 */
	  if(*fptr)
	    fptr++;
	};
      };
    };
  };
  return 0;
}

/*.......................................................................
 * Change the size of the history buffer.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  bufsize   size_t    The number of bytes in the history buffer, or 0
 *                      to delete the buffer completely.
 * Output:
 *  return       int    0 - OK.
 *                      1 - Insufficient memory (the previous buffer
 *                          will have been retained). No error message
 *                          will be displayed.
 */
int _glh_resize_history(GlHistory *glh, size_t bufsize)
{
  GlLineNode *node;  /* A line location node in the list of lines */
  GlLineNode *prev;  /* The line location node preceding 'node' */
/*
 * Check the arguments.
 */
  if(!glh)
    return bufsize > 0;
/*
 * If the new size doesn't differ from the existing size, do nothing.
 */
  if(glh->buflen == bufsize)
    return 0;
/*
 * Cancel any ongoing search.
 */
  (void) _glh_cancel_search(glh);
/*
 * Create a wholly new buffer?
 */
  if(glh->buflen == 0) {
    glh->buffer = (char *) malloc(bufsize);
    if(!glh->buffer)
      return 1;
    glh->buflen = bufsize;
/*
 * Delete an existing buffer?
 */
  } else if(bufsize == 0) {
    _glh_clear_history(glh, 1);
    free(glh->buffer);
    glh->buffer = NULL;
    glh->buflen = 0;
/*
 * To get here, we must be shrinking or expanding from one
 * finite size to another.
 */
  } else {
/*
 * If we are shrinking the size of the buffer, then we first need
 * to discard the oldest lines that won't fit in the new buffer.
 */
    if(bufsize < glh->buflen) {
      size_t nbytes = 0;    /* The number of bytes used in the new buffer */
      GlLineNode *oldest;   /* The oldest node to be kept */
/*
 * Searching backwards from the youngest line, find the oldest
 * line for which there will be sufficient room in the new buffer.
 */
      for(oldest = glh->list.tail;
	  oldest && (nbytes += oldest->nchar) <= bufsize;
	  oldest = oldest->prev)
	;
/*
 * We will have gone one node too far, unless we reached the oldest line
 * without exceeding the target length.
 */
      if(oldest) {
	nbytes -= oldest->nchar;
	oldest = oldest->next;
      };
/*
 * Discard the nodes that can't be retained.
 */
      while(glh->list.head && glh->list.head != oldest)
	_glh_discard_node(glh, glh->list.head);
/*
 * If we are increasing the size of the buffer, we need to reallocate
 * the buffer before shifting the lines into their new positions.
 */
    } else {
      char *new_buffer = (char *) realloc(glh->buffer, bufsize);
      if(!new_buffer)
	return 1;
      glh->buffer = new_buffer;
      glh->buflen = bufsize;
    };
/*
 * If there are any lines to be preserved, copy the block of lines
 * that precedes the end of the existing buffer to what will be 
 * the end of the new buffer.
 */
    if(glh->list.head) {
      int shift;  /* The number of bytes to shift lines in the buffer */
/*
 * Get the oldest line to be kept.
 */
      GlLineNode *oldest = glh->list.head;
/*
 * Count the number of characters that are used in the lines that
 * precede the end of the current buffer (ie. not including those
 * lines that have been wrapped to the start of the buffer).
 */
      int n = 0;
      for(node=oldest,prev=oldest->prev; node && node->start >= oldest->start;
	  prev=node, node=node->next)
	n += node->nchar;
/*
 * Move these bytes to the end of the resized buffer.
 */
      memmove(glh->buffer + bufsize - n, glh->buffer + oldest->start, n);
/*
 * Adjust the buffer pointers to reflect the new locations of the moved
 * lines.
 */
      shift = bufsize - n - oldest->start;
      for(node=prev; node && node->start >= oldest->start; node=node->prev)
	node->start += shift;
    };
/*
 * Shrink the buffer?
 */
    if(bufsize < glh->buflen) {
      char *new_buffer = (char *) realloc(glh->buffer, bufsize);
      if(new_buffer)
	glh->buffer = new_buffer;
      glh->buflen = bufsize;  /* Mark it as shrunk, regardless of success */
    };
  };
  return 0;
}

/*.......................................................................
 * Set an upper limit to the number of lines that can be recorded in the
 * history list, or remove a previously specified limit.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  max_lines    int    The maximum number of lines to allow, or -1 to
 *                      cancel a previous limit and allow as many lines
 *                      as will fit in the current history buffer size.
 */
void _glh_limit_history(GlHistory *glh, int max_lines)
{
  if(!glh)
    return;
/*
 * Apply a new limit?
 */
  if(max_lines >= 0 && max_lines != glh->max_lines) {
/*
 * Count successively older lines until we reach the start of the
 * list, or until we have seen max_lines lines (at which point 'node'
 * will be line number max_lines+1).
 */
    int nline = 0;
    GlLineNode *node;
    for(node=glh->list.tail; node && ++nline <= max_lines; node=node->prev)
      ;
/*
 * Discard any lines that exceed the limit.
 */
    if(node) {
      GlLineNode *oldest = node->next;  /* The oldest line to be kept */
/*
 * Delete nodes from the head of the list until we reach the node that
 * is to be kept.
 */
      while(glh->list.head && glh->list.head != oldest)
	_glh_discard_node(glh, glh->list.head);
    };
  };
/*
 * Record the new limit.
 */
  glh->max_lines = max_lines;
  return;
}

/*.......................................................................
 * Discard either all history, or the history associated with the current
 * history group.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  all_groups   int    If true, clear all of the history. If false,
 *                      clear only the stored lines associated with the
 *                      currently selected history group.
 */
void _glh_clear_history(GlHistory *glh, int all_groups)
{
/*
 * Check the arguments.
 */
  if(!glh)
    return;
/*
 * Cancel any ongoing search.
 */
  (void) _glh_cancel_search(glh);
/*
 * Delete all history lines regardless of group?
 */
  if(all_groups) {
    _rst_FreeList(glh->list.node_mem);
    glh->list.head = glh->list.tail = NULL;
    glh->nline = 0;
    glh->id_node = NULL;
/*
 * Just delete lines of the current group?
 */
  } else {
    GlLineNode *node;  /* The line node being checked */
    GlLineNode *prev;  /* The line node that precedes 'node' */
    GlLineNode *next;  /* The line node that follows 'node' */
/*
 * Search out and delete the line nodes of the current group.
 */
    for(node=glh->list.head; node; node=next) {
/*
 * Keep a record of the following node before we delete the current
 * node.
 */
      next = node->next;
/*
 * Discard this node?
 */
      if(node->group == glh->group)
	_glh_discard_node(glh, node);
    };
/*
 * If there are any lines left, and we deleted any lines, there will
 * be gaps in the buffer. These need to be removed.
 */
    if(glh->list.head) {
      int epos;   /* The index of the last used element in the buffer */
/*
 * Find the line nearest the end of the buffer.
 */
      GlLineNode *enode;
      for(node=glh->list.head, prev=NULL;
	  node && node->start >= glh->list.head->start;
	  prev=node, node = node->next)
	;
      enode = prev;
/*
 * Move the end line to abutt the end of the buffer, and remove gaps
 * between the lines that precede it.
 */
      epos = glh->buflen;
      for(node=enode; node; node=node->prev) {
	int shift = epos - (node->start + node->nchar);
	if(shift) {
	  memmove(glh->buffer + node->start + shift,
		  glh->buffer + node->start, node->nchar);
	  node->start += shift;
	};
	epos = node->start;
      };
/*
 * Move the first line in the buffer to the start of the buffer, and
 * remove gaps between the lines that follow it.
 */
      epos = 0;
      for(node=enode ? enode->next : NULL; node; node=node->next) {
	int shift = epos - node->start;
	if(shift) {
	  memmove(glh->buffer + node->start + shift,
		  glh->buffer + node->start, node->nchar);
	  node->start += shift;
	};
	epos = node->start + node->nchar;
      };
    };
  };
  return;
}

/*.......................................................................
 * Temporarily enable or disable the history list.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  enable       int    If true, turn on the history mechanism. If
 *                      false, disable it.
 */
void _glh_toggle_history(GlHistory *glh, int enable)
{
  if(glh)
    glh->enable = enable;
}

/*.......................................................................
 * Remove a given line location node from the history list, and return
 * it to the freelist.
 *
 * Input:
 *  glh    GlHistory *  The input-line history maintenance object.
 *  node  GlLineNode *  The node to be removed. This must be currently
 *                      in the list who's head is glh->list.head, or
 *                      be NULL.
 */
static void _glh_discard_node(GlHistory *glh, GlLineNode *node)
{
  if(node) {
/*
 * Make the node that precedes the node being removed point
 * to the one that follows it.
 */
    if(node->prev)
      node->prev->next = node->next;
    else
      glh->list.head = node->next;
/*
 * Make the node that follows the node being removed point
 * to the one that precedes it.
 */
    if(node->next)
      node->next->prev = node->prev;
    else
      glh->list.tail = node->prev;
/*
 * If we are deleting the node that is marked as the start point of the
 * last ID search, remove the cached starting point.
 */
    if(node == glh->id_node)
      glh->id_node = NULL;
/*
 * Return the node to the free list.
 */
    node = (GlLineNode *) _del_FreeListNode(glh->list.node_mem, node);
/*
 * Decrement the count of the number of lines in the buffer.
 */
    glh->nline--;
  };
}

/*.......................................................................
 * Lookup the details of a given history line, given its id.
 *
 * Input:
 *  glh      GlHistory *  The input-line history maintenance object.
 *  id        GlLineID    The sequential number of the line.
 * Input/Output:
 *  line    const char ** A pointer to the history line will be assigned
 *                        to *line.
 *  group     unsigned *  The group membership of the line will be assigned
 *                        to *group.
 *  timestamp   time_t *  The timestamp of the line will be assigned to
 *                        *timestamp.
 * Output:
 *  return         int    0 - The requested line wasn't found.
 *                        1 - The line was found.
 */
int _glh_lookup_history(GlHistory *glh, GlhLineID id, const char **line,
			unsigned *group, time_t *timestamp)
{
  GlLineNode *node; /* The located line location node */
/*
 * Check the arguments.
 */
  if(!glh)
    return 0;
/*
 * Search for the line that has the specified ID.
 */
  node = _glh_find_id(glh, (GlhLineID) id);
/*
 * Not found?
 */
  if(!node)
    return 0;
/*
 * Return the details of the line.
 */
  if(line)
    *line = glh->buffer + node->start;
  if(group)
    *group = node->group;
  if(timestamp)
    *timestamp = node->timestamp;
  return 1;
}

/*.......................................................................
 * Lookup a node in the history list by its ID.
 *
 * Input:
 *  glh      GlHistory *  The input-line history maintenance object.
 *  id       GlhLineID    The ID of the line to be returned.
 * Output:
 *  return  GlLIneNode *  The located node, or NULL if not found.
 */
static GlLineNode *_glh_find_id(GlHistory *glh, GlhLineID id)
{
  GlLineNode *node;  /* The node being checked */
/*
 * Is history enabled?
 */
  if(!glh->enable || !glh->list.head)
    return NULL;
/*
 * If possible, start at the end point of the last ID search.
 * Otherwise start from the head of the list.
 */
  node = glh->id_node;
  if(!node)
    node = glh->list.head;
/*
 * Search forwards from 'node'?
 */
  if(node->id < id) {
    while(node && node->id != id)
      node = node->next;
    glh->id_node = node ? node : glh->list.tail;
/*
 * Search backwards from 'node'?
 */
  } else {
    while(node && node->id != id)
      node = node->prev;
    glh->id_node = node ? node : glh->list.head;
  };
/*
 * Return the located node (this will be NULL if the ID wasn't found).
 */
  return node;
}

/*.......................................................................
 * Query the state of the history list. Note that any of the input/output
 * pointers can be specified as NULL.
 *
 * Input:
 *  glh         GlHistory *  The input-line history maintenance object.
 * Input/Output:
 *  enabled           int *  If history is enabled, *enabled will be
 *                           set to 1. Otherwise it will be assigned 0.
 *  group        unsigned *  The current history group ID will be assigned
 *                           to *group.
 *  max_lines         int *  The currently requested limit on the number
 *                           of history lines in the list, or -1 if
 *                           unlimited.
 */
void _glh_state_of_history(GlHistory *glh, int *enabled, unsigned *group,
			   int *max_lines)
{
  if(glh) {
    if(enabled)
     *enabled = glh->enable;
    if(group)
     *group = glh->group;
    if(max_lines)
     *max_lines = glh->max_lines;
  };
}

/*.......................................................................
 * Get the range of lines in the history buffer.
 *
 * Input:
 *  glh         GlHistory *  The input-line history maintenance object.
 * Input/Output:
 *  oldest  unsigned long *  The sequential entry number of the oldest
 *                           line in the history list will be assigned
 *                           to *oldest, unless there are no lines, in
 *                           which case 0 will be assigned.
 *  newest  unsigned long *  The sequential entry number of the newest
 *                           line in the history list will be assigned
 *                           to *newest, unless there are no lines, in
 *                           which case 0 will be assigned.
 *  nlines            int *  The number of lines currently in the history
 *                           list.
 */
void _glh_range_of_history(GlHistory *glh, unsigned long *oldest,
			   unsigned long *newest, int *nlines)
{
  if(glh) {
    if(oldest)
      *oldest = glh->list.head ? glh->list.head->id : 0;
    if(newest)
      *newest = glh->list.tail ? glh->list.tail->id : 0;
    if(nlines)
      *nlines = glh->nline;
  };
}

/*.......................................................................
 * Return the size of the history buffer and the amount of the
 * buffer that is currently in use.
 *
 * Input:
 *  glh      GlHistory *  The input-line history maintenance object.
 * Input/Output:
 *  buff_size   size_t *  The size of the history buffer (bytes).
 *  buff_used   size_t *  The amount of the history buffer that
 *                        is currently occupied (bytes).
 */
void _glh_size_of_history(GlHistory *glh, size_t *buff_size, size_t *buff_used)
{
  if(glh) {
    if(buff_size)
      *buff_size = glh->buflen;
/*
 * Determine the amount of buffer space that is currently occupied.
 */
    if(buff_used) {
      size_t used = 0;
      GlLineNode *node;
      for(node=glh->list.head; node; node=node->next)
	used += node->nchar;
      *buff_used = used;
    };
  };
}
