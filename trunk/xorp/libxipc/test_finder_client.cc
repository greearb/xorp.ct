#include <stdio.h>
#include <unistd.h>

#include "finder_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "finder_client.hh"
#include "finder_server.hh"

const char* okay_str(bool okay) {
    return okay ? "okay" : "fail";
}

void 
finder_addition(FinderClientError e,
		const char*	  /* name */,
		const char*       /* value */,
		void*             thunked_success) {
    static int iter = 0;

    if (e != FC_OKAY) {
	fprintf(stderr, "Add failed (iteration = %d)\n", iter);
	exit(-1);
    }
    iter ++;

    bool* p_success = reinterpret_cast<bool*>(thunked_success);
    *p_success = true;
}

void 
finder_removal(FinderClientError e,
	       const char*	 /* name */,
	       const char*       /* value */,
	       void*		 thunked_success) {
    static int iter = 0;
    if (e != FC_OKAY) {
	fprintf(stderr, "Remove failed (iteration = %d)\n", iter);
	exit(-1);
    }
    iter ++;

    bool* p_success = reinterpret_cast<bool*>(thunked_success);
    *p_success = true;
}

void timed_out() 
{
    fprintf(stderr, "Test timed out.");
    exit(-1);
}


static void
run_test()
{
    EventLoop event_loop;

    FinderServer server(event_loop);
    FinderClient client(event_loop);
    bool success = false;

    XorpTimer timeout = event_loop.new_oneoff_after_ms(5000,
						       callback(&timed_out));

    while (server.connection_count() == 0) {
	event_loop.run();
    }

    for (int i = 0; i < 100; i++) {
	client.add("hello", "world", finder_addition, 
		   reinterpret_cast<void*>(&success));
	for (success = false; false == success; event_loop.run());

	client.remove("hello", finder_removal, 
		      reinterpret_cast<void*>(&success));
	for (success = false; false == success; event_loop.run());
    }
    return;
}

int main(int /* argc */, char *argv[]) {

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    run_test();
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return 0;
}
