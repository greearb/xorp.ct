#include <stdlib.h>

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "finder_server.hh"
#include "xrl_router.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_args.hh"

#ifndef UNUSED
#define UNUSED(x) (x) = (x)
#endif

// ----------------------------------------------------------------------------
// Xrl request callbacks

static const XrlCmdError 
hello_world(const Xrl&	/* request */,
	    XrlArgs*	/* response */)
{
    return XrlCmdError::OKAY();
}

static const XrlCmdError
passback_integer(const Xrl&	/* request*/,
		 XrlArgs*	response)
{
    response->add_int32("the_number", 5);
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Xrl dispatch handlers and related

static void
exit_on_xrlerror(const XrlError& e, const char* file, int line)
{

    fprintf(stderr, "From %s: %d XrlError: %s\n", file, line, e.str().c_str());
    exit(-1);
}

static void
hello_world_complete(const XrlError&	e,
		     XrlRouter&		/* router */,
		     const Xrl&		/* request */,
		     XrlArgs*		/* response */,
		     bool*		done) {
    if (e != XrlError::OKAY())
	exit_on_xrlerror(e, __FILE__, __LINE__);

    *done = true;
}

static void
got_integer(const XrlError&	e,
	    XrlRouter&		/* router */,
	    const Xrl&		/* request */,
	    XrlArgs*		response,
	    bool*		done) {
    if (e != XrlError::OKAY())
	exit_on_xrlerror(e, __FILE__, __LINE__);

    try {
	int32_t the_int = response->get_int32("the_number");
	if (the_int != 5) {
	    fprintf(stderr, "Corrupt integer argument.");
	    exit(-1);
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

int main(int /* argc */, char *argv[]) {
    EventLoop		event_loop;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    // Normally the Finder runs as a separate process, but for this
    // test we create a Finder in process as we can't guarantee Finder
    // is already running.  Most XORP processes do not have to do this.
    FinderServer*	finder = 0;
    try {
	finder = new FinderServer(event_loop);
    } catch (const FinderTCPServerIPCFactory::FactoryError& e) {
	printf("Could not instantiate Finder.  Assuming this is because it's already running.\n");
    }

    // Create and configure "party A"
    XrlRouter		party_a(event_loop, "party A");
    XrlPFSUDPListener	listener_a(event_loop);
    party_a.add_listener(&listener_a);

    // Add command that party A knows about
    party_a.add_handler("hello_world", callback(hello_world));
    party_a.add_handler("passback_integer", callback(passback_integer));

    // Create and configure "party B"
    XrlRouter		party_b(event_loop, "party B");
    XrlPFSUDPListener	listener_b(event_loop);
    party_b.add_listener(&listener_b);

    // "Party B" send "hello world" to "Party A"
    bool step1_done = false;
    Xrl x("party A", "hello_world");
    party_b.send(x, callback(hello_world_complete, &step1_done));

    bool step2_done = false;
    Xrl y("party A", "passback_integer");
    party_b.send(y, callback(got_integer, &step2_done));

    // Just run...
    bool finito = false;
    XorpTimer t = event_loop.set_flag_after_ms(5000, &finito);
    while (step1_done == false || step2_done == false) {
	event_loop.run();
	if (finito) {
	    fprintf(stderr, "Test timed out (%d %d)\n", 
		    step1_done, step2_done);
	    exit(-1);
	}
    }

    if (finder)
	delete finder;

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    printf("Test ran okay.\n");
    return 0;
}
