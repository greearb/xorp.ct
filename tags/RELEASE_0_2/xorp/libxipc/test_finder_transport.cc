#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

//#define DEBUG_LOGGING

#include "finder_module.h"
#include "libxorp/xorp.h"
#include "libxorp/callback.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "finder_transport.hh"

XorpTimer g_timeout;

class TimeOut {};
static void time_out(void) throw (TimeOut)
{
    cerr << "Time Out" << endl;
    // Using an exception here is just a cheap way of unwinding the stack.
    // Not pleasant, badly thought out test.
    throw TimeOut();
}

/*
 * In this test a server listens for a connection from a client.  When
 * the client connects it sends back to back hello messages to the
 * server.  The server sends periodic messages back to the server.
 */

class FTT {
public:
    FTT(EventLoop& e)
	: _sfactory(e, FINDER_TCP_DEFAULT_PORT, callback(this, &FTT::connect)),
	_cfactory(e), _server_msgs_recv(0), _client_msgs_recv(0),
	_client_msgs_sent(0), _e(e)
    {
	_cfactory.run(callback(this, &FTT::connected));
	g_timeout = _e.new_oneoff_after_ms(5000, callback(&time_out));
    }

    ~FTT() { cout << endl; }

    /* Server Related */

    void server_arrival_event(const FinderTransport&, const string& /* msg */)
    {
	//	cout << "Server received: " << msg << endl;
	_server_msgs_recv++;
    }
    void server_departure_event(const FinderTransport&,
				const FinderMessage::RefPtr& m)
    {
	//	cout << "Server sent message "<< m->seqno() << endl;
	_server_msgs_sent++;
	assert(_server_send_queue.size() == 1);
	assert(_server_send_queue.front()->name() == "bye");
	departure_event(_server_send_queue, m);
	schedule_server_send();
    }

    void server_failure_event(const FinderTransport&)
    {
	cout << "Server failed." << endl;
	abort();
    }

    void connect(FinderTransport::RefPtr& rp)
    {
	cout << "Server received connection" << endl;
	_server = rp;
	rp->set_arrival_callback(callback(this, &FTT::server_arrival_event));
	rp->set_departure_callback(callback(this, &FTT::server_departure_event));
	rp->set_failure_callback(callback(this, &FTT::server_failure_event));
	schedule_server_send();
    }

    void schedule_server_send()
    {
	int wait_ms[] = { 1000, 237, 33, 194, 44, 456, 71, 9, 1, 2, 3, 1, 200 };
	int n_waits = sizeof(wait_ms) / sizeof(wait_ms[0]);
	int this_wait = wait_ms[_server_msgs_sent % n_waits];
	_server_send_timer = _e.new_oneoff_after_ms(this_wait, callback(this, &FTT::server_send));
    }

    void server_send()
    {
	uint32_t src = random();
	FinderMessage* fm = new FinderBye(src, 0);
	_server_send_queue.push_back(FinderMessage::RefPtr(fm));
	_server->write(_server_send_queue.back());
    }

    /* Client Related */

    void client_arrival_event(const FinderTransport&, const string& /* msg */)
    {
	//	cout << "Client received: " << msg << endl;
	_client_msgs_recv++;
	g_timeout = _e.new_oneoff_after_ms(5000, callback(&time_out));
    }

    void client_departure_event(const FinderTransport&,
				const FinderMessage::RefPtr& m)
    {
	//	cout << "Client sent message "<< m->seqno() << endl;
	_client_msgs_sent++;
	assert(_client_send_queue.size() == 1);
	assert(_client_send_queue.front()->name() == "hello");
	departure_event(_client_send_queue, m);

	client_send_hello();
    }

    void client_failure_event(const FinderTransport&)
    {
	cout << "Client failed." << endl;
	abort();
    }

    void client_send_hello()
    {
	uint32_t src = random();
	FinderMessage* fm = new FinderHello(src, 0);
	_client_send_queue.push_back(FinderMessage::RefPtr(fm));
	_client->write(_client_send_queue.back());
    }

    void connected(FinderTransport::RefPtr& ft) {
	cout << "Client connected to server" << endl;
	_client = ft;

	debug_msg("Wiring up client (%p) callbacks \n", _client.get());

	_client->set_arrival_callback(callback(this, &FTT::client_arrival_event));
	_client->set_departure_callback(callback(this, &FTT::client_departure_event));
	_client->set_failure_callback(callback(this, &FTT::client_failure_event));

	client_send_hello();
    }

    bool done() const { return _client_msgs_sent > 500 && _server_msgs_recv > 50000; }

    void departure_event(list<FinderMessage::RefPtr>& msgs,
			 const FinderMessage::RefPtr& m) {
	list<FinderMessage::RefPtr>::iterator i;

	for (i = msgs.begin(); i != msgs.end(); ++i) {
	    if (m.get() == i->get()) {
		msgs.erase(i);
		return;
	    }
	}
	cerr << endl << msgs.size() << endl;
	cerr << msgs.front()->render() << endl << m->render() << endl;
	assert (i != msgs.end());
    }

private:
    FinderTransport::RefPtr		_client;
    FinderTransport::RefPtr		_server;

    FinderTcpServerFactory		_sfactory;
    FinderTcpClientFactory		_cfactory;

    XorpTimer				_connect_timer;

    list<FinderMessage::RefPtr>		_client_send_queue;
    list<FinderMessage::RefPtr>		_server_send_queue;

    int					_server_msgs_recv;
    int					_server_msgs_sent;
    int					_client_msgs_recv;
    int					_client_msgs_sent;

    XorpTimer				_server_send_timer;
    EventLoop& 				_e;
};

static bool
print_dot()
{
    cout << "." << flush;
    return true;
}

static int
run_test()
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    EventLoop e;
    FTT tester(e);

    XorpTimer dot_printer = e.new_periodic(500, callback(print_dot));

    try {
	while (tester.done() == false) {
	    e.run();
	    flush(cout);
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
	return -1; // Test failed exception thrown somewhere
    }

    if (g_timeout.scheduled())
	g_timeout.unschedule();

    return 0;
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

    int r = run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return r;
}
