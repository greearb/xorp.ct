#include <stdio.h>

#include "finder_module.h"
#include "libxorp/xlog.h"
#include "finder_msg.hh"

class Receiver {
public:
    void hello(const FinderHello& /* h */) const {
    }
    void bye(const FinderBye& /* h */) const {
    }
};

static void
run_test()
{
    FinderParser fp;
    Receiver r;

    HMACMD5 hmac(string("Your leisure is my pleasure"));

    fp.add(hello_notifier(callback(r, &Receiver::hello)));
    fp.add(bye_notifier(callback(r, &Receiver::bye)));

    uint32_t src = 1;

    list<FinderMessage*> messages;
    for (int seq = 0; seq < 2000; seq++) {
	messages.push_back(new FinderHello(src, seq));
	messages.push_back(new FinderBye(src, seq));
    }
    for (list<FinderMessage*>::const_iterator ci = messages.begin();
	 ci != messages.end(); ci++) {
	string s = (*ci)->render();
	string::const_iterator sp = s.begin();
	try {
	    fp.parse(s, sp);
	} catch (const XorpException &xe) {
	    fprintf(stderr, "%s\n", xe.str().c_str());
	    exit(-1);
	}
    }
    for (list<FinderMessage*>::const_iterator ci = messages.begin();
	 ci != messages.end(); ci++) {
	string s = (*ci)->render_signed(hmac);
	string::const_iterator sp = s.begin();
	//	printf(">>%s<<\n", s.c_str());
	try {
	    fp.parse_signed(hmac, s, sp);
	} catch (const XorpException &xe) {
	    fprintf(stderr, "%s\n", xe.str().c_str());
	    exit(-1);
	}
    }
}

int main(int /* argc */, char *argv[])
{
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
