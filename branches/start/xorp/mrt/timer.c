/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
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

#ident "$XORP: xorp/mrt/timer.c,v 1.8 2002/12/09 11:49:08 pavlin Exp $"


/*
 * Timing wheels implementation.
 */


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "timer.h"
/* #include <assert.h> */


/*
 * XXX: Limitations:
 *   - The max. timer value is 2^32 seconds, which means
 *     that used in combination with gettimeofday(2) it will overflow
 *     in year 2037 (or 2038, don't remember exactly). The solution
 *     is to add t_array4[] and modify the code to use it, but of course
 *     this will work only if the number of seconds returned by
 *     gettimeofday(2) is represented by more than 32 bits.
 *   - The virtual time granularity is 1 microsecond.
 *   - Only the seconds are managed by timing wheels. The microseconds
 *     are a linked lists.
 */

/*
 * Exported variables
 */

/*
 * Local constants definitions
 */

/*
 * Local structures, typedefs and macros
 */
/* A macro to avoid compilation warnings about unused functions arguments */
#ifndef UNUSED
#if 1
#define UNUSED(arg)	((arg) = (arg))
#else
#define UNUSED(arg)
#endif /* 1/0 */
#endif /* UNUSED */

#if defined(MASC_SIM)			/* XXX */
#define ENABLE_SIM
#endif

#undef DEBUG_VALIDATE_TIMER		/* define for timer validation */

/*
 * The absolute time as seen by the timers. Outside of this file it
 * should be accessed only by using `NOW()', `NOW_SEC()', and `NOW_USEC()' .
 */
struct timeval current_time_;
#define MTIMER_SEC(mtimer)	(TIMEVAL_SEC(&(mtimer)->abstime))
#define MTIMER_USEC(mtimer)	(TIMEVAL_USEC(&(mtimer)->abstime))

/* The timing wheels element */
typedef struct htimer_ {
    mtimer_t		*head;		/* First of the timers in the list */
    mtimer_t		*tail;		/* Last of the timers in the list  */
} htimer_t;

/*
 * Memory management stuff. Instead of calling malloc() every time a
 * new timer is created, we keep a pool of elements of right size
 * by chaining them in a single-linked list.
 */
/*
 * If the pool of free elements runs out of gas, the pool will be
 * refilled to have L_BUFF_MIN emements. If the number of free elements
 * is more than L_BUFF_MAX, the number of elements will be reduced
 * to L_BUFF_MIN by returning the excess to the system (by free()-ing them).
 */
#define L_BUFF_MIN 200
#define L_BUFF_MAX (2*L_BUFF_MIN)
/* The convenient single-linked list structure */
typedef struct lcl_list_ {
    struct lcl_list_ *next;
} lcl_list_t;
/*
 * Variables to keep the head of the list with the free elements
 * and the number of the elements in the list.
 */
#define DECLARE_MEM_POOL(type)						\
static lcl_list_t	*_ ## type ## _head	= NULL;			\
static unsigned int	_  ## type ## _number	= 0
#define INIT_MEM_POOL(type)						\
do {									\
	_ ## type ## _head	= NULL;					\
	_ ## type ## _number	= 0;					\
} while (false)

DECLARE_MEM_POOL(mtimer_t);

/*
 * The handy macros to manipulate the pool of free elements.
 * MALLOC() and FREE() are user-friendly versions that make
 * the interface simpler. If you can read C-preprocessor commands,
 * you may notice that the variable names with the head of the linked
 * list and the number of the elements in the list have a strict syntax:
 *		_ELEMENTTYPE_head and _ELEMENTTYPE_number
 */
#if defined(WITH_DMALLOC) && (WITH_DMALLOC)
#define MALLOC(name, type)						\
do {									\
	(name) = (type *)malloc(sizeof(type));				\
} while (false)
#define FREE(name, type)						\
do {									\
	free((name));							\
} while (false)

#else /* !WITH_DMALLOC */
#define MALLOC(name, type)						\
	MALLOC_LONG((name), type, _ ## type ## _head, _ ## type ## _number)
#define FREE(name, type)						\
	FREE_LONG((name), type, _ ## type ## _head, _ ## type ## _number)
#endif /* !WITH_DMALLOC */

#define MALLOC_LONG(new_element, type, head, number)			\
do {									\
	if ((head) == NULL) {						\
		while ((number) < L_BUFF_MIN) {				\
			(new_element) = (type *)malloc(sizeof(type));	\
			((lcl_list_t *)(new_element))->next		\
				= (lcl_list_t *)(head);			\
			(head) = (lcl_list_t *)(new_element);		\
			(number)++;					\
		}							\
	}								\
	(new_element) = (type *)(head);					\
	(head) = ((lcl_list_t *)(head))->next;				\
	(number)--;							\
} while (false)
#define FREE_LONG(del_element, type, head, number)			\
do {									\
	((lcl_list_t *)(del_element))->next = (lcl_list_t *)(head);	\
	(head) = (lcl_list_t *)(del_element);				\
	if (++(number) > L_BUFF_MAX) {					\
		while ((number) > L_BUFF_MIN) {				\
			(del_element) = (type *)(head);			\
			(head) = ((lcl_list_t *)(head))->next;		\
			free((void *)(del_element));			\
			(number)--;					\
		}							\
	}								\
} while (false)

#define MALLOC_MTIMER(mtimer)						\
do {									\
	MALLOC((mtimer), mtimer_t);					\
} while (false)

#define UNLINK_MTIMER(mtimer)						\
do {									\
	if ((mtimer)->myself != NULL) {					\
		assert(*((mtimer)->myself) == (mtimer));		\
		*((mtimer)->myself) = NULL;				\
		(mtimer)->myself = NULL;				\
	}								\
} while (false)

#define FREE_MTIMER(mtimer)						\
do {									\
	/* XXX: For speed reason we don't call UNLINK_MTIMER() again */ \
	/* UNLINK_MTIMER(mtimer); */					\
	FREE((mtimer), mtimer_t);					\
} while (false)

/* Handy macros to reduce the size of the code */
#define MTIMER_APPEND(t_head, mtimer)					\
do {									\
	(mtimer)->prev = (t_head).tail;					\
	(mtimer)->next = NULL;						\
	if ((t_head).tail != NULL)					\
	    (t_head).tail->next = (mtimer);				\
	if ((t_head).head == NULL)					\
	    (t_head).head = (mtimer);					\
	(t_head).tail = (mtimer);					\
	(mtimer)->curr_queue = &(t_head);				\
} while (false)

/*
 * XXX: will work only for the 'seconds' (t_array0).
 * To make it working for other arrays, replace MTIMER_USEC() with
 * TIMEVAL_CMP()
 */
#define MTIMER_APPEND_ORDER(t_head, mtimer)				\
do {									\
	mtimer_t *r;							\
									\
	r = (t_head).tail;						\
	/* 'r' should point to the entry to add 'mtimer' after */	\
	while (r != NULL) {						\
		if (MTIMER_USEC(r) > MTIMER_USEC(mtimer)) {		\
			r = r->prev;					\
			continue;					\
		}							\
		break;							\
	}								\
	if (r == NULL) {						\
		(mtimer)->next = (t_head).head;				\
		(mtimer)->prev = NULL;					\
		(t_head).head = (mtimer);				\
		if ((mtimer)->next != NULL)				\
			(mtimer)->next->prev = (mtimer);		\
		else							\
			(t_head).tail = (mtimer);			\
	} else {							\
		(mtimer)->next = r->next;				\
		(mtimer)->prev = r;					\
		r->next = (mtimer);					\
		if ((t_head).tail == r)					\
			(t_head).tail = (mtimer);			\
		else							\
			(mtimer)->next->prev = (mtimer);		\
	}								\
	(mtimer)->curr_queue = &(t_head);				\
} while (false)

/*
 * The entries will not be ordered by their timeout value
 * l - array index we are removing, indexed by index##l
 * s - array index we are populating by above entries
 * shift - shift seconds to obtain index in array##s
 * SIDE EFFECT: changes index##s
 */
#define TIMEOUT_ARRAY(l, s, shift)					\
do {									\
	q = t_array##l[index##l].head;					\
	t_array##l[index##l].head = NULL;				\
	t_array##l[index##l].tail = NULL;				\
	while (q != NULL) {						\
		p = q;							\
		q = q->next;						\
		index##s = (uint16_t)((MTIMER_SEC(p) >> (shift)) & 0xff); \
		MTIMER_APPEND(t_array##s[index##s], p);			\
	}								\
} while (false)

/*
 * The entries will be ordered by their timeout value
 * l - array index we are removing, indexed by index##l
 * s - array index we are populating by above entries
 * shift - shift seconds to obtain index in array##s
 * SIDE EFFECT: changes index##s
 */
#define TIMEOUT_ARRAY_ORDER(l, s, shift)				\
do {									\
	q = t_array##l[index##l].head;					\
	t_array##l[index##l].head = NULL;				\
	t_array##l[index##l].tail = NULL;				\
	while (q != NULL) {						\
		p = q;							\
		q = q->next;						\
		index##s = (uint16_t)((MTIMER_SEC(p) >> (shift)) & 0xff); \
		MTIMER_APPEND_ORDER(t_array##s[index##s], p);		\
	}								\
} while (false)

#define TIMEOUT_ARRAY1()	TIMEOUT_ARRAY_ORDER(1, 0, 0)
#define TIMEOUT_ARRAY2()	TIMEOUT_ARRAY(2, 1, 8)
#define TIMEOUT_ARRAY3()	TIMEOUT_ARRAY(3, 2, 16)

/*
 * Local variables
 */
/* The timing wheels */
/* XXX: timing wheel size is hardcoded!! */
static htimer_t t_array3[256];
static htimer_t t_array2[256];
static htimer_t t_array1[256];
static htimer_t t_array0[256];

/*
 * Local functions prototypes
 */


/**
 * timers_init:
 * @void: 
 * 
 * Init stuff. Need to be called only once (during startup).
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
timers_init(void)
{
    int i;
    static bool_t init_flag = false;
    
    if (init_flag)
	return (-1);

    INIT_MEM_POOL(mtimer_t);
    
#ifdef ENABLE_SIM
    TIMEVAL_CLEAR(&current_time_);
#else
    gettimeofday(&current_time_, NULL);
#endif /* ENABLE_SIM */
    
    for (i = 0; i < 256; i++) {
	t_array0[i].head = NULL;
	t_array0[i].tail = NULL;
	t_array1[i].head = NULL;
	t_array1[i].tail = NULL;
	t_array2[i].head = NULL;
	t_array2[i].tail = NULL;
	t_array3[i].head = NULL;
	t_array3[i].tail = NULL;
    }
    
    init_flag = true;
    
    return (0);
}

/**
 * timers_exit:
 * @void: 
 * 
 * Exit stuff. Need to be called only once (before exiting gracefully).
 * 
 * Return value: 0 on success, otherwise -1.
 **/
int
timers_exit(void)
{
    static bool_t exit_flag = false;
    int i;
    
    if (exit_flag)
	return (-1);
    
    /* Clean-up all mtimers */
#define MTIMER_FREE_ALL_(id)						\
do {									\
	mtimer_t *p, *q;						\
									\
	q = t_array##id[i].head;					\
	while (q != NULL) {						\
		p = q; q = q->next;					\
		UNLINK_MTIMER(p);					\
		FREE_MTIMER(p);						\
	}								\
	t_array##id[i].head = NULL;					\
	t_array##id[i].tail = NULL;					\
} while (false)
    
    for (i = 0; i < 256; i++) {
	MTIMER_FREE_ALL_(0);
	MTIMER_FREE_ALL_(1);
	MTIMER_FREE_ALL_(2);
	MTIMER_FREE_ALL_(3);
    }
#undef MTIMER_FREE_ALL_
    
    exit_flag = true;
    return (0);
}

/**
 * timer_process_event_queue:
 * @elapsed_time: The number of units that have passed.
 * 
 * Process all the events that should happen within a specified window
 * of time.
 * 
 * Return value: %true if events were processed, otherwise %false.
 **/
bool_t
timer_process_event_queue(struct timeval *elapsed_time)
{
    struct timeval end_time;
    
    TIMEVAL_ADD(elapsed_time, NOW(), &end_time);
    return (timer_process_abs_event_queue(&end_time));
}

/**
 * timer_process_abs_event_queue:
 * @end_time: The absolute time.
 * 
 * Process all the events that should happen until given time (included).
 * 
 * Return value: %true if events were processed, otherwise %false.
 **/
bool_t
timer_process_abs_event_queue(struct timeval *end_time)
{
    uint16_t index0, index1, index2, index3;
    mtimer_t *p, *q;
    bool_t process_flag = false;
    uint32_t end_time_sec;
    uint32_t end_time_usec;
#if defined(DEBUG_VALIDATE_TIMER) && defined(ENABLE_SIM)
    struct timeval  prev_timer;
    TIMEVAL_COPY(&current_time_, &prev_timer);
    fprintf(stderr, "debugging timer...\n");
#endif /* DEBUG_VALIDATE_TIMER && ENABLE_SIM */
    end_time_sec = TIMEVAL_SEC(end_time);
    end_time_usec = TIMEVAL_USEC(end_time);
    
    index0 = (uint16_t)(NOW_SEC() & 0xff);
    index1 = (uint16_t)((NOW_SEC() >> 8) & 0xff);
    index2 = (uint16_t)((NOW_SEC() >> 16) & 0xff);
    index3 = (uint16_t)((NOW_SEC() >> 24) & 0xff);

 process_wheel0:
    for (; index0 < 256; ++index0) { 
	while (t_array0[index0].head != NULL) {
	    p = t_array0[index0].head;
	    if (TIMEVAL_CMP(&p->abstime, end_time, >)) {
		TIMEVAL_COPY(end_time, &current_time_);
		return (process_flag);
	    }
	    /* expire p */
	    t_array0[index0].head = p->next;
	    if (p->next == NULL)
		t_array0[index0].tail = NULL;
	    else
		p->next->prev = NULL;
	    UNLINK_MTIMER(p);
	    /* advance current time */
	    assert(TIMEVAL_SEC(&p->abstime)%256 == index0);
	    TIMEVAL_COPY(&p->abstime, &current_time_);
#if defined(DEBUG_VALIDATE_TIMER) && defined(ENABLE_SIM)
	    if (TIMEVAL_CMP(&current_time_, &prev_timer, <)) {
		fprintf(stderr, "time goes backwards\n");
		abort();
	    }
	    TIMEVAL_COPY(&current_time_, &prev_timer);
#endif /* DEBUG_VALIDATE_TIMER && ENABLE_SIM */
	    if (p->func != NULL) {
		p->func(p->data_pointer);
		process_flag = true;
	    }
	    FREE_MTIMER(p);
	}
	if (NOW_SEC() >= end_time_sec) {
	    assert(NOW_SEC() == end_time_sec);
	    /* advance current time to equal end_time */
	    current_time_.tv_usec = TIMEVAL_USEC(end_time);
	    return (process_flag);
	}
	current_time_.tv_sec++;
	assert(NOW_SEC() <= end_time_sec);
	current_time_.tv_usec = 0;
    } /* END: processed whole array 0 */
    
 process_wheel1:
    /* find the first non-empty bin of array 1 starting from index1 */
    for (; index1 < 256; ++index1) {
	if (t_array1[index1].head != NULL)
	    break;
#if 0
	current_time_.tv_sec += 256;
#endif
    }
    if (index1 < 256) { 
	/* found it: move content of the bin index1 into t_array0 */
	TIMEOUT_ARRAY1(); /* XXX changes index0 */
	index0 = 0; /* will start processing array0 from 0 */
	goto process_wheel0;
    }
    /* END: processed whole array 1 */
    
 process_wheel2:
    /* find the first non-empty bin of array 2 starting from index2 */
    for (; index2 < 256; ++index2) {
	if (t_array2[index2].head != NULL)
	    break;
#if 0
	current_time_.tv_sec += 256*256;
#endif
    }
    if (index2 < 256) { 
	/* found it: move content of the bin index2 into t_array1 */
	TIMEOUT_ARRAY2(); /* XXX changes index1 */
	index1 = 0; /* will start processing array1 from 0 */
	goto process_wheel1;
    }
    /* END: processed whole array 2 */
    
    /* find the first non-empty bin of array 3 starting from index3 */
    for (; index3 < 256; ++index3) {
	if (t_array3[index3].head != NULL)
	    break;
#if 0
	current_time_.tv_sec += 256*256*256;
#endif
    }
    if (index3 < 256) { 
	/* found it: move content of the bin index2 into t_array2 */
	TIMEOUT_ARRAY3(); /* XXX changes index2 */
	index2 = 0; /* will start processing array2 from 0 */
	goto process_wheel2;
    }
    /* END: process whole array 3 */
    
    /* no more events scheduled */
    /* XXX TODO: what to set current_time_ to ? */
    TIMEVAL_COPY(end_time, &current_time_);
    assert(NOW_SEC() <= end_time_sec);
    
    return (process_flag);
}

/**
 * timer_start:
 * @delay_sec: Number of seconds for timeout.
 * @delay_usec: Number of microseconds for timeout.
 * @func: The function to be called on timeout.
 * @data_pointer: The data pointer argument to @func (its argument).
 * 
 * Start a timer.
 * 
 * Return value: A pointer to the newly created #mtimer_t timer,
 * or %NULL if error.
 **/
mtimer_t *
timer_start(uint32_t delay_sec, uint32_t delay_usec, cfunc_t func,
	    void *data_pointer)
{
    mtimer_t *mtimer;
    struct timeval delay_time, end_time;
    uint32_t end_sec;
    uint16_t my_index;
    
    MALLOC_MTIMER(mtimer);
    if (mtimer == NULL)
	return (NULL);
    
    TIMEVAL_SET(&delay_time, delay_sec, delay_usec);
    TIMEVAL_ADD(&delay_time, NOW(), &end_time);
    end_sec = TIMEVAL_SEC(&end_time);
    mtimer->func = func; 
    mtimer->data_pointer = data_pointer;
    TIMEVAL_COPY(&end_time, &mtimer->abstime);
    mtimer->myself = NULL;
    
    my_index = (uint16_t)((end_sec >> 24) & 0xff);
    if (my_index > ((NOW_SEC() >> 24) & 0xff)) {
	MTIMER_APPEND(t_array3[my_index], mtimer);
	return (mtimer);
    }
    my_index = (uint16_t)((end_sec >> 16) & 0xff);
    if (my_index > ((NOW_SEC() >> 16) & 0xff)) {
	MTIMER_APPEND(t_array2[my_index], mtimer);
	return (mtimer);
    }
    my_index = (uint16_t)((end_sec >> 8) & 0xff);
    if (my_index > ((NOW_SEC() >> 8) & 0xff)) {
	MTIMER_APPEND(t_array1[my_index], mtimer);
	return (mtimer);
    }
    my_index = (uint16_t)(end_sec & 0xff);
    MTIMER_APPEND_ORDER(t_array0[my_index], mtimer);
    
    return (mtimer);
}

/**
 * timer_cancel:
 * @mtimer: The timer to cancel.
 * 
 * Cancel a timer.
 * 
 * Return value: %true on success, otherwise %false.
 **/
bool_t
timer_cancel(mtimer_t *mtimer)
{
    if (mtimer == NULL)
	return (false);
    
    if (mtimer->next == NULL) {
	mtimer->curr_queue->tail = mtimer->prev;
    } else {
	mtimer->next->prev = mtimer->prev;
    }
    if (mtimer->prev == NULL) {
	mtimer->curr_queue->head = mtimer->next;
    } else {
	mtimer->prev->next = mtimer->next;
    }
    UNLINK_MTIMER(mtimer);
    FREE_MTIMER(mtimer);
    
    return (true);
}

/**
 * timer_next_timeval:
 * @timeval_diff: The timeval structure to return the time difference.
 * 
 * Get how much time to the next pending event.
 * 
 * Return value: The number of seconds to the next event, or %FOREVER
 * if no event pending.
 **/
uint32_t
timer_next_timeval(struct timeval *timeval_diff)
{
    uint16_t index0, index1, index2, index3;
    mtimer_t *p;
    struct timeval *next_abstime;
    int i;
    
    index0 = (uint16_t)(NOW_SEC() & 0xff);
    for (i = index0; i < 256; i++) {
	if (t_array0[i].head != NULL) {
	    TIMEVAL_SUB(&t_array0[i].head->abstime, NOW(), timeval_diff);
	    return (TIMEVAL_SEC(timeval_diff));
	}
    }
    
#define MTIMER_DELTA_(id)						\
do {									\
	for (i = index##id; i < 256; i++) {				\
		if (t_array##id[i].head != NULL) {			\
			p = t_array##id[i].head;			\
			next_abstime = &p->abstime;			\
			while ((p = p->next) != NULL) {			\
				if (TIMEVAL_CMP(&p->abstime, next_abstime, <))\
					next_abstime = &p->abstime;	\
			}						\
			TIMEVAL_SUB(next_abstime, NOW(), timeval_diff);	\
			return (TIMEVAL_SEC(timeval_diff));		\
		}							\
	}								\
} while (false)
    
    index1 = (uint16_t)((NOW_SEC() >> 8) & 0xff);
    MTIMER_DELTA_(1);
    index2 = (uint16_t)((NOW_SEC() >> 16) & 0xff);
    MTIMER_DELTA_(2);
    index3 = (uint16_t)((NOW_SEC() >> 24) & 0xff);
    MTIMER_DELTA_(3);
    
    TIMEVAL_SET(timeval_diff, FOREVER, FOREVER);
    return (FOREVER);
#undef MTIMER_DELTA_
}

/**
 * timer_left_sec:
 * @mtimer: The timer whose number of seconds to expire we need.
 * 
 * Get the number of seconds until a timer expires.
 * 
 * Return value: The number of seconds until the timer expire.
 * XXX: if @mtimer is %NULL, return 0 (zero seconds).
 **/
uint32_t
timer_left_sec(mtimer_t *mtimer)
{
    struct timeval timeval_diff;
    
    if (!TIMER_ISSET(mtimer))
	return (0);
    TIMEVAL_SUB(&mtimer->abstime, NOW(), &timeval_diff);
    
    return (TIMEVAL_SEC(&timeval_diff));
}

/**
 * timer_left_timeval:
 * @mtimer: The timer whose time to expire we need.
 * @timeval_diff: The time difference result.
 * 
 * Get the time difference until a timer expire.
 * 
 * Return value: The number of seconds until the timer expire.
 * XXX: if @mtimer is %NULL, return 0 (zero seconds).
 **/
uint32_t
timer_left_timeval(mtimer_t *mtimer, struct timeval *timeval_diff)
{
    TIMEVAL_CLEAR(timeval_diff);
    if (!TIMER_ISSET(mtimer))
	return (0);
    TIMEVAL_SUB(&mtimer->abstime, NOW(), timeval_diff);
    
    return (TIMEVAL_SEC(timeval_diff));
}

/**
 * timer_timeout_semaphore:
 * @data_pointer: A pointer to the #(mtimer_t *) memory that stored
 * the pointer to the running timer.
 *
 * A function to be called after a semaphore timer expires.
 * A 'semaphore timer' is a timer that only records the fact that it
 * is running, but after its expiration, no particular actions are taken.
 **/
void
timer_timeout_semaphore(void *data_pointer)
{
    mtimer_t **mtimer_p;
    
    mtimer_p = (mtimer_t **)data_pointer;
    TIMER_NULL(*mtimer_p);
}
