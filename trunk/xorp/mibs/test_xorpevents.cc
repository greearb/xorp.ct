

#include "config.h"

#include <set>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "xorp_if_module.h"
#include "xorpevents.hh"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/timer.hh"
#include "libxorp/xlog.h"

#include "libxipc/xrl_router.hh"
#include "libxipc/xrl_pf_sudp.hh"
#include "libxipc/xrl_args.hh"
#include "libxipc/finder_server.hh"

#include "libxipc/finder.hh"
#include "libxipc/finder_tcp_messenger.hh"
#include "libxipc/finder_xrl_target.hh"
#include "libxipc/permits.hh"
#include "libxipc/sockutil.hh"
 
enum TestResult {
    ALL_PASS = 0,
    TEST_FAIL = 1,
    INTERNAL_FAIL = 2
};

typedef std::set<int>  FakeSnmpdFdSet;

// Local variables
static int cb1_counter, cb2_counter, cb3_counter;
static FakeSnmpdFdSet exported_readfds, exported_writefds, exported_exceptfds;


// declared here so it can be called by the test program.  Not public
extern void run_timer_callbacks (u_int, void *);

// --------------------------------------------------------------------------- 
// These stubs replace functions provided by the Net-SNMP agent library.  
//

int
register_readfd(int fd, void (*) (int, void *), void *)
{
    fprintf(stderr,"Read file descriptor %d exported\n", fd);
    if (exported_readfds.end() != exported_readfds.find(fd)) 
	{
	XLOG_ERROR("fd %d was already exported!!", fd);
	exit(TEST_FAIL);
	}
    exported_readfds.insert(fd);
    return FD_REGISTERED_OK;
}

int
register_writefd(int fd, void (*) (int, void *), void *)
{
    fprintf(stderr,"Write file descriptor %d exported\n", fd);
    if (exported_writefds.end() != exported_writefds.find(fd)) 
	{
	XLOG_ERROR("fd %d was already exported!!", fd);
	exit(TEST_FAIL);
	}
    exported_writefds.insert(fd);
    return FD_REGISTERED_OK;
}

int
register_exceptfd(int fd, void (*) (int, void *), void *)
{
    fprintf(stderr,"Exception file descriptor %d exported\n", fd);
    if (exported_exceptfds.end() != exported_exceptfds.find(fd)) 
	{
	XLOG_ERROR("fd %d was already exported!!", fd);
	exit(TEST_FAIL);
	}
    exported_exceptfds.insert(fd);
    return FD_REGISTERED_OK;
}

int
unregister_readfd(int fd)
{
    fprintf(stderr,"Read file descriptor %d unregistered\n", fd);
    exported_readfds.erase(fd);
    return FD_UNREGISTERED_OK;
}

int
unregister_writefd(int fd)
{
    fprintf(stderr,"Write file descriptor %d unregistered\n", fd);
    exported_writefds.erase(fd);
    return FD_UNREGISTERED_OK;
}

int
unregister_exceptfd(int fd)
{
    fprintf(stderr,"Exception file descriptor %d unregistered\n", fd);
    exported_exceptfds.erase(fd);
    return FD_UNREGISTERED_OK;
}

unsigned int
snmp_alarm_register_hr(struct timeval t, unsigned int flags,
                       SNMPAlarmCallback *, void *)
{
    static int  alarm_id = 0; 
    long ms = t.tv_sec*1000 + t.tv_usec/1000;
    if (flags)
	fprintf(stderr,"Periodic alarm registered:  %ld ms\n", ms);
    else
	fprintf(stderr,"One time alarm registered:  %ld ms\n", ms);
    return ++alarm_id;
}

void
snmp_alarm_unregister (unsigned int) {};

void  snmp_set_do_debugging(int) {};
int   snmp_get_do_debugging(void) {return 1;};
void  debugmsg(const char *, const char *, ...) {}
void  debugmsgtoken(const char *token, const char * format, ...) 
{
    if (strcmp("SnmpEventLoop", token)) return; 
    printf("snmp debug:");
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    return;
}

int snmp_log(int, const char * format, ...) 
{
    printf("snmp log:");
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    return 0;
}

// end of Net-SNMP stubs
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Xrl request callbacks

static const XrlCmdError
hello_world(const XrlArgs& /* inputs */,
	    XrlArgs*	   /* outputs */)
{
    return XrlCmdError::OKAY();
}

static const XrlCmdError
passback_integer(const XrlArgs&	/* inputs */,
		 XrlArgs*	outputs)
{
    outputs->add_int32("the_number", 5);
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Xrl dispatch handlers and related

static void
exit_on_xrlerror(const XrlError& e, const char* file, int line)
{

    fprintf(stderr, "From %s: %d XrlError: %s\n", file, line, e.str().c_str());
    exit(TEST_FAIL);
}

static void
hello_world_complete(const XrlError&	e,
		     XrlArgs*		/* response */,
		     bool*		done)
{
    if (e != XrlError::OKAY())
	exit_on_xrlerror(e, __FILE__, __LINE__);

    *done = true;
}

static void
got_integer(const XrlError&	e,
	    XrlArgs*		response,
	    bool*		done)
{
    if (e != XrlError::OKAY())
	exit_on_xrlerror(e, __FILE__, __LINE__);

    try {
	int32_t the_int = response->get_int32("the_number");
	if (the_int != 5) {
	    fprintf(stderr, "Corrupt integer argument.");
	    exit(TEST_FAIL);
	}
    } catch (const XrlArgs::XrlAtomNotFound& e) {
	printf("Failed to get expected integer\n");
    }

    // Eg we can iterate through the argument list
    for (size_t i = 0; i < response->size(); i++) {
	/* const XrlAtom &arg = */ response->item(i);
	/*	printf("%s = %d\n", arg.name().c_str(), arg.int32()); */
    }
    *done = true;
}


// ----------------------------------------------------------------------------- 
// Periodic timer callbacks

static bool timer_callback1() {
    cb1_counter++;
    fprintf(stderr,"TCB1 counting...%d\n", cb1_counter);
    return true;
}

static bool timer_callback2() {
    cb2_counter++;
    fprintf(stderr,"TCB2 counting...%d\n", cb2_counter);
    return true;
}

static bool timer_callback3() {
    cb3_counter++;
    fprintf(stderr,"TCB3 counting...%d\n", cb3_counter);
    return true;
}
// end of timer callbacks
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// fake snmp select loop  


void 
fake_snmpd()
{
    TimerList & tl = SnmpEventLoop::the_instance().timer_list();
    /* simulate snmpd select */
    TimeVal t;
    timeval tv;
    fd_set readfds, writefds, exceptfds;
    tl.get_next_delay(t);
    t.copy_out(tv);
    FD_ZERO(&readfds); FD_ZERO(&writefds); FD_ZERO(&exceptfds);
    FakeSnmpdFdSet::iterator p;
    for (p = exported_readfds.begin(); p != exported_readfds.end(); p++) {
	FD_SET (*p, &readfds);
	fprintf(stderr,"fake snmp added fd %d to read fd mask\n", *p);
    }
    for (p = exported_writefds.begin(); p != exported_writefds.end(); p++) {
	FD_SET (*p, &writefds);
	fprintf(stderr,"fake snmp added fd %d to write fd mask\n", *p);
    }
    for (p = exported_exceptfds.begin(); p != exported_exceptfds.end(); p++) {
	FD_SET (*p, &exceptfds);
	fprintf(stderr,"fake snmp added fd %d to exception fd mask\n", *p);
    }
    if (select (100, &readfds, &writefds, &exceptfds, &tv)) {
	fprintf(stderr,"fake_select detected activity on "
	    "xorp file descriptors\n");
	run_fd_callbacks(0,0);
    } else {
	fprintf(stderr,"fake_select timed out after %ld ms\n", tv.tv_sec*1000 +
	    tv.tv_usec/1000);
	run_timer_callbacks(0,0);
    }
}

// ----------------------------------------------------------------------------
// test of periodic timers.  The test simulates three MIB modules each using a
// reference to SnmpEventLoop to create periodic events.  For the test to pass,
// all three callback functions must be called at the appropriate rate.
//

void
run_test_1()
{
    XorpTimer xt1, xt2, xt3;
    PeriodicTimerCallback ptcb1, ptcb2, ptcb3;
    SnmpEventLoop& e1 = SnmpEventLoop::the_instance();
    SnmpEventLoop& e2 = SnmpEventLoop::the_instance();
    SnmpEventLoop& e3 = SnmpEventLoop::the_instance();
    int period_ms = 1200;
    fprintf(stderr,"Creating periodic events...\n");
    ptcb1 = callback(timer_callback1);
    ptcb2 = callback(timer_callback2);
    ptcb3 = callback(timer_callback3);
    xt1 = e1.new_periodic (period_ms, ptcb1);
    xt2 = e2.new_periodic (period_ms/2, ptcb2);
    xt3 = e3.new_periodic (period_ms/3, ptcb3);
    bool finito = false;
    const int test1_loops = 10;
    cb1_counter = cb2_counter = cb3_counter = 0;
    XorpTimer t = e1.set_flag_after_ms((test1_loops) * period_ms, &finito);
    while (finito == false) {
	fake_snmpd();
    }
    if ((cb1_counter != cb2_counter/2) || (cb1_counter != cb3_counter/3)) 
	exit(TEST_FAIL); 
    return;
}

// -----------------------------------------------------------------------------
// test xrl communication.  This test is based on the one found in 
// test_xrl_router.cc
//





void
run_test_2()
{
    SnmpEventLoop&  e = SnmpEventLoop::the_instance();

    // Normally the Finder runs as a separate process, but for this
    // test we create a Finder in process as we can't guarantee Finder
    // is already running.  Most XORP processes do not have to do this.
    
    FinderServer* finder = new FinderServer(e);
    
    // Create and configure "party_A"
    XrlRouter		party_a(e, "party_A", finder->addr(), finder->port());
    XrlPFSUDPListener	listener_a(e);
    party_a.add_listener(&listener_a);

    // Add command that party_A knows about
    party_a.add_handler("hello_world", callback(hello_world));
    party_a.add_handler("passback_integer", callback(passback_integer));

    party_a.finalize();
    
    // Create and configure "party_B"
    XrlRouter		party_b(e, "party_B", finder->addr(), finder->port());
    XrlPFSUDPListener	listener_b(e);
    party_b.add_listener(&listener_b);
    party_b.finalize();

    //
    // Pause for a second to allow parties to register Xrls and enable them
    // This is a bit slack but XrlRouter interface doesn't have enabled check
    // for time being and finder does.
    //
    bool finito = false;
    XorpTimer t = e.set_flag_after_ms(1000, &finito);
    while (finito == false) {
	fake_snmpd();
    }
    
    // "Party_B" send "hello world" to "Party_A"
    bool step1_done = false;
    Xrl x("party_A", "hello_world");
    party_b.send(x, callback(hello_world_complete, &step1_done));

    bool step2_done = false;
    Xrl y("party_A", "passback_integer");
    party_b.send(y, callback(got_integer, &step2_done));

    // Just run...
    finito = false;
    t = e.set_flag_after_ms(5000, &finito);
    while (step1_done == false || step2_done == false) {
	fake_snmpd();
	if (finito) {
	    fprintf(stderr, "Test timed out (%d %d)\n",
		    step1_done, step2_done);
	    exit(TEST_FAIL);
	}
    }
    if (finder)
	delete finder;
}

int
main(int /* argc */, char *argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    // xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);

    xlog_add_default_output();
    xlog_start();
    TimeVal now;
    TimerList::system_gettimeofday(&now);
    // Some of test generates warnings - under normal circumstances the
    // end user wants to know, but here not.
    xlog_disable(XLOG_LEVEL_WARNING);
    
    try {
    run_test_1();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    try {
    run_test_2();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return ALL_PASS;
}

