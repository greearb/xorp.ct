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

/*
 * $XORP: xorp/mrt/timer.h,v 1.22 2002/12/09 11:49:08 pavlin Exp $
 */


#ifndef __MRT_TIMER_H__
#define __MRT_TIMER_H__


/*
 * Timer implementation header file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>

#include "random.h"

/*
 * Constants definitions
 */
#if (!defined(TRUE)) || (!defined(FALSE))
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define FALSE (0)
#define TRUE (!FALSE)
#ifndef __cplusplus
typedef enum { true = TRUE, false = FALSE } bool;
#endif
typedef bool bool_t;
#endif /* TRUE, FALSE */

#define FOREVER		0xffffffffU
#define NO_DELAY	0
#define SECOND		1
#define MINUTE		(60*SECOND)
#define HOUR		(60*MINUTE)
#define DAY		(24*HOUR)
#define WEEK		(7*DAY)
#define MONTH		(30*DAY)
#define YEAR		(365*DAY)

/*
 * Structures, typedefs and macros
 */
#define NOW()		(&current_time_)
#define NOW_SEC()	TIMEVAL_SEC(NOW())
#define NOW_USEC()	TIMEVAL_USEC(NOW())

/*
 * The prototype of the timer function to call
 */
typedef void (*cfunc_t)(void *data_pointer);

/*
 * The timer definition. Usually, the user code shoudn't access any
 * of its fields directly, but should use the appropriate
 * macros or functions instead.
 */
typedef struct mtimer_ {
    struct mtimer_	*next;		/* next event in the hashed list     */
    struct mtimer_	*prev;		/* prev event in the hashed list     */
    struct mtimer_	**myself;	/* pointer to user code's ptr to me  */
    struct htimer_	*curr_queue;	/* pointer to current queue	     */
    struct timeval	abstime;	/* absolute expiration time	     */
    cfunc_t		func;  	        /* function to call		     */
    void		*data_pointer;	/* func's data pointer (its argument)*/
} mtimer_t;


/*
 * The user code should use only the macros and functions listed below
 * to deal with mtimers:
 *
 *	timers_init(void)
 *	timer_next_timeval(struct timeval *timeval)
 *
 *	TIMER_NULL((mtimer_t *)mtimer_p)
 *	TIMER_ISSET((mtimer_t *)mtimer_p)
 *	TIMER_START((mtimer_t *)mtimer_p, uint32_t delay_sec,
 *			uint32_t delay_usec, cfunc_t func,
 *			(void *)data_pointer)
 *	XXX: the scheduled randomized time is @delay +- @delay*@random_factor
 *	TIMER_START_RANDOM((mtimer_t *)mtimer_p, uint32_t delay_sec,
 *				uint32_t delay_usec, cfunc_t func,
 *				(void *)data_pointer,
 *				(double)random_factor)
 *	TIMER_START_SEMAPHORE((mtimer_t *mtimer_p, uint32_t delay_sec,
 *			uint32_t delay_usec)
 *	TIMER_CANCEL((mtimer_t *)mtimer_p)
 *	TIMER_LEFT_SEC((mtimer_t *)mtimer_p)
 *	TIMER_LEFT_TIMEVAL((mtimer_t *)mtimer_p, struct timeval *timeval_diff)
 *	TIMER_PROCESS()
 *	TIMER_PROCESS_UNTIL(uint32_t abs_sec, uint32_t abs_usec)
 *	TIMER_PROCESS_FOREVER()
 */

#define TIMER_NULL(mtimer_p)						\
do {									\
	(mtimer_p) = NULL;						\
} while (false)

#define TIMER_ISSET(mtimer_p) ((mtimer_p) != NULL)

#define TIMER_START(mtimer_p, delay_sec, delay_usec, func,		\
			data_pointer)					\
do {									\
	if (TIMER_ISSET((mtimer_p)))					\
		timer_cancel((mtimer_p));				\
	(mtimer_p) = timer_start((delay_sec), (delay_usec),		\
				(func),					\
				(data_pointer));			\
	(mtimer_p)->myself = &(mtimer_p);				\
} while (false)

#define TIMER_START_RANDOM(timer, delay_sec, delay_usec, func,		\
				data_pointer, random_factor)		\
do {									\
	struct timeval timer_start_random_timeval_;			\
									\
	TIMEVAL_SET(&timer_start_random_timeval_, (delay_sec), (delay_usec)); \
	TIMEVAL_RANDOM(&timer_start_random_timeval_, (random_factor));	\
	TIMER_START(timer, TIMEVAL_SEC(&timer_start_random_timeval_),	\
			TIMEVAL_USEC(&timer_start_random_timeval_), (func), \
			(data_pointer));				\
} while (false)

#define TIMER_START_SEMAPHORE(mtimer_p, delay_sec, delay_usec)		\
do {									\
	if (TIMER_ISSET((mtimer_p)))					\
		timer_cancel((mtimer_p));				\
	(mtimer_p) = timer_start((delay_sec), (delay_usec),		\
				timer_timeout_semaphore,		\
				&(mtimer_p));				\
	(mtimer_p)->myself = &(mtimer_p);				\
} while (false)

#define TIMER_CANCEL(mtimer_p)						\
do {									\
	timer_cancel((mtimer_p));					\
	TIMER_NULL((mtimer_p));						\
} while (false)

#define TIMER_LEFT_SEC(mtimer_p)					\
		(timer_left_sec((mtimer_p)))

#define TIMER_LEFT_TIMEVAL(mtimer_p, timeval)				\
		(timer_left_timeval((mtimer_p), (timeval)))

#define TIMER_PROCESS()							\
do {									\
	struct timeval timeout_;					\
									\
	do {								\
		gettimeofday(&timeout_, NULL);				\
	} while (timer_process_abs_event_queue(&timeout_));		\
} while (false)

#define TIMER_PROCESS_UNTIL(abs_sec, abs_usec)				\
do {									\
	struct timeval timeout_;					\
									\
	TIMEVAL_SET(&timeout_, (abs_sec), (abs_usec));			\
	timer_process_abs_event_queue(&timeout_);			\
} while (false)

#define TIMER_PROCESS_FOREVER()						\
do {									\
	TIMER_PROCESS_UNTIL(FOREVER, 0);				\
} while (false)

/*
 * Convenient macros for operations on timevals.
 * Taken and adapted from <sys/time.h>
 *
 * XXX: NOTE: The Linux and Solaris-5.5.1 code in <sys/time.h> have a comment
 * that timercmp() does not work for >= or <= , but FreeBSD does
 * not have this comment. However, it turned out that the Solaris
 * definition of timercmp() indeed doesn't work for those two comparison.
 * The FreeBSD and Linux definitions seem to be OK, and I don't see
 * any reason why 'TIMEVAL_CMP()' below won't work either.
 */
#define TIMEVAL_SEC(tvp)	((uint32_t)(tvp)->tv_sec)
#define TIMEVAL_USEC(tvp)	((uint32_t)(tvp)->tv_usec)
#define	TIMEVAL_CLEAR(tvp)	(tvp)->tv_sec = (tvp)->tv_usec = 0
#define	TIMEVAL_ISSET(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)

#define TIMEVAL_SET(tvp, sec, usec)					\
do {									\
	(tvp)->tv_sec = (sec);						\
	(tvp)->tv_usec = (usec);					\
} while (false)

#define TIMEVAL_COPY(s, d)						\
do {									\
	(d)->tv_sec = (s)->tv_sec;					\
	(d)->tv_usec = (s)->tv_usec;					\
} while (false)

#define	TIMEVAL_CMP(a, b, CMP)						\
	(((a)->tv_sec == (b)->tv_sec) ?					\
	    ((uint32_t)(a)->tv_usec CMP (uint32_t)(b)->tv_usec) :	\
	    ((uint32_t)(a)->tv_sec  CMP (uint32_t)(b)->tv_sec))

#define TIMEVAL_ADD(a, b, result)					\
do {									\
	(result)->tv_sec = (uint32_t)(a)->tv_sec + (uint32_t)(b)->tv_sec;    \
	(result)->tv_usec = (uint32_t)(a)->tv_usec + (uint32_t)(b)->tv_usec; \
	if ((uint32_t)(result)->tv_usec >= 1000000) {			\
		++(result)->tv_sec;					\
		(result)->tv_usec -= 1000000;				\
	}								\
} while (false)

#define TIMEVAL_INCR(result, a) TIMEVAL_ADD(a, result, result)

#define TIMEVAL_SUB(a, b, result)					\
do {									\
	(result)->tv_sec = (uint32_t)(a)->tv_sec - (uint32_t)(b)->tv_sec;    \
	(result)->tv_usec = (uint32_t)(a)->tv_usec - (uint32_t)(b)->tv_usec; \
	if ((signed)(result)->tv_usec < 0) {				\
		--(result)->tv_sec;					\
		(result)->tv_usec += 1000000;				\
	}								\
} while (false)

#define TIMEVAL_DECR(result, b) TIMEVAL_SUB(result, b, result)

#define TIMEVAL_DIV(a, d, result)                                       \
do {                                                                    \
        uint32_t rem_;                                                  \
                                                                        \
        rem_ = (uint32_t)(a)->tv_sec % (d);			        \
        (result)->tv_sec  = (uint32_t)(a)->tv_sec / (d);                \
        (result)->tv_usec = ((uint32_t)(a)->tv_usec + 1000000*rem_)/(d);\
} while (false)

#define TIMEVAL_MUL(a, m, result)					\
do {									\
	(result)->tv_sec  = (uint32_t)((uint32_t)(a)->tv_sec * (m));	\
	(result)->tv_usec = (uint32_t)((uint32_t)(a)->tv_usec * (m));	\
	(result)->tv_sec  += (uint32_t)((uint32_t)(result)->tv_usec / 1000000);	\
	(result)->tv_usec = (uint32_t)((uint32_t)(result)->tv_usec % 1000000);	\
} while (false)

#define TIMEVAL2FLOAT(t)						\
	((double)TIMEVAL_SEC(t) + ((double)TIMEVAL_USEC(t))/1000000.0)

#define FLOAT2TIMEVAL(f, tv) 						\
do {									\
	(tv)->tv_sec = (uint32_t)(f);					\
	(tv)->tv_usec = (uint32_t)					\
		(((double)(f) - (uint32_t)(tv)->tv_sec)			\
		  * 1000000.0 + .0000001);				\
} while(false)

/*
 * Randomize 'tvp' (a pointer to 'struct timeval') with 'random_factor'
 * in either direction. E.g., if 'random_factor' is 0.1, add/substract
 * up to 10% to/from 'tvp'.
 */
#define TIMEVAL_RANDOM(tvp, random_factor)				\
do {									\
	uint32_t random_sec_, random_usec_;				\
	struct timeval timeval_random_timeval_;				\
	double xsec_;							\
									\
	if (random_factor == 0.0)					\
		break;							\
	/* Randomize by up to 'random_factor' either way */		\
	random_sec_ = TIMEVAL_SEC(tvp);					\
	random_usec_ = TIMEVAL_USEC(tvp);				\
	/* Randomize */							\
	xsec_ = (random_factor)*(random_sec_);				\
	random_sec_ = (uint32_t)(xsec_ + 0.5);				\
	random_usec_ = (uint32_t)((random_factor)*(random_usec_) + 0.5) \
	    		+ (uint32_t)((xsec_ - random_sec_)*1000000);	\
	if ((uint32_t)random_sec_ > 0) {				\
		/* To make sure that random_usec didn't get negative */	\
		/*    and it may only happen if xsec_ was rounded up */	\
		random_sec_--;						\
		random_usec_ += 1000000;				\
	}								\
	/* losing precision here! */ 					\
	if (random_sec_ != 0)						\
		random_sec_ = RANDOM(random_sec_);			\
	if (random_usec_ != 0)						\
		random_usec_ = RANDOM(random_usec_);			\
	if ((uint32_t)random_usec_ >= 1000000) {			\
		random_sec_ += random_usec_ / 1000000;			\
		random_usec_ %= 1000000;				\
	}								\
	/* Assign to 'timeval_random_timeval_', then add or substract */  \
	TIMEVAL_SET(&timeval_random_timeval_, random_sec_, random_usec_); \
	if (RANDOM(2)) {						\
		/* Add */						\
		TIMEVAL_ADD(&timeval_random_timeval_, (tvp), (tvp));	\
	} else {							\
		/* Substract */						\
		if (TIMEVAL_CMP((tvp), &timeval_random_timeval_, >))	\
			TIMEVAL_SUB((tvp), &timeval_random_timeval_, (tvp)); \
		else							\
			TIMEVAL_CLEAR(tvp);				\
	}								\
} while (false)

/*
 * MIN/MAX handy macros
 */
#ifndef MAX
#define MAX(a, b) (((a) >= (b))? (a) : (b))
#define MIN(a, b) (((a) <= (b))? (a) : (b))
#endif /* MAX & MIN */

#ifndef MAX_VAL_SET
#define MAX_VAL_SET(a, val)						\
do {									\
	if ((a) < (val))						\
		(a) = (val);						\
} while (false)
#define MIN_VAL_SET(a, val)						\
do {									\
	if ((a) > (val))						\
		(a) = (val);						\
} while (false)
#endif /* MAX_VAL_SET & MIN_VAL_SET */

/*
 * Global variables
 */
extern struct timeval	current_time_;

/*
 * Global functions prototypes
 */
__BEGIN_DECLS
extern int timers_init(void);
extern int timers_exit(void);
extern bool_t timer_process_event_queue(struct timeval *elapsed_time);
extern bool_t timer_process_abs_event_queue(struct timeval *end_time);
extern mtimer_t *timer_start(uint32_t delay_sec,
			     uint32_t delay_usec,
			     cfunc_t func,
			     void *data_pointer);
extern bool_t	timer_cancel(mtimer_t *mtimer);
extern uint32_t timer_next_timeval(struct timeval *timeval);
extern uint32_t timer_left_sec(mtimer_t *mtimer);
extern uint32_t timer_left_timeval(mtimer_t *mtimer,
				   struct timeval *timeval_diff);
extern void timer_timeout_semaphore(void *data_pointer);
__END_DECLS

#endif /* __MRT_TIMER_H__ */
