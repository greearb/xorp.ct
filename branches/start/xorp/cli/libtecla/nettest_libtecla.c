/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001,2002 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * Portions of this software have one or more of the following copyrights, and
 * are subject to the license below. The relevant source files are clearly
 * marked; they refer to this file using the phrase ``the Xorp LICENSE file''.
 *
 * (c) 2001,2002 International Computer Science Institute
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * The names and trademarks of copyright holders may not be used in
 * advertising or publicity pertaining to the software without specific
 * prior permission. Title to copyright in this software and any associated
 * documentation will at all times remain with the copyright holders.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/*
 * Sample CLI (Command-Line Interface) implementation.
 *
 * Author: Pavlin Radoslavov (pavlin@icsi.berkeley.edu)
 * Date: Wed Dec 12 00:11:34 PST 2001
 *
 * XXX: note that this example is very simplified:
 * it doesn't handle more than one connection, the error checks are
 * not adequate, etc.
 */

/*
 * To complile, you need a modified version of libtecla that supports
 * network connection. If the directory to that library is ``./libtecla/'',
 * then:
 * gcc -g nettest_libtecla.c -L./libtecla -ltecla -ltermcap -o nettest_libtecla
 *
 * Usage:
 *  1. Start this program: ./nettest_libtecla
 *  2. From a different window you can telnet to this process on port 12000:
 *     telnet localhost 12000
 */



#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <netinet/in.h>

#include "libtecla/libtecla.h"


/*
 * Exported variables
 */

/*
 * Local constants definitions
 */
#define CLIENT_TERMINAL 1
#define CLI_WELCOME "Welcome!"
#define CLI_PROMPT  "CLI> "

/*
 * Local structures, typedefs and macros
 */


/*
 * Local variables
 */

static int _serv_socket = -1;
static unsigned short _serv_port = 0;	/* XXX: not defined yet */

static FILE *_fd_file_read = NULL;
static FILE *_fd_file_write = NULL;
static int _client_type = CLIENT_TERMINAL;
static GetLine *_gl = NULL;
static int _telnet_iac = 0;
static int _telnet_sb = 0;
static int _window_width = 0;
static int _window_height = 0;
static int _telnet_sb_buffer_size = 0;
static char _telnet_sb_buffer[1024];
static int _telnet_dont = 0;
static int _telnet_do = 0;
static int _telnet_wont = 0;
static int _telnet_will = 0;
static char _command_buffer[1024];
static int _command_buffer_size = 0;
static int _buff_curpos = 0;

/*
 * Local functions prototypes
 */
void cli_setup();
void start_connection(int fd);
void client_read(int fd);
int command_line_character(const char *line, int word_end, uint8_t val);


int
main(int argc, char *argv[])
{
    struct sockaddr_in sin_addr;
    struct sockaddr_in cli_addr;
    int addrlen;
    fd_set read_fds, write_fds;
    int sock = -1;
    int max_sock;
    int reuse = 1;
    
    _serv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(_serv_socket, SOL_SOCKET, SO_REUSEADDR,
		   (void *)&reuse, sizeof(reuse)) < 0) {
	perror("setsockopt(SO_REUSEADDR)");
	exit (1);
    }
    /* TODO: comment-out sin_len setup if the OS doesn't have it */
    memset(&sin_addr, 0, sizeof(sin_addr));
    sin_addr.sin_len = sizeof(sin_addr);
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_port = htons(12000);		/* XXX: fixed port 12000 */
    sin_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(_serv_socket, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
	perror("Error binding");
	exit (1);
    }
    if (listen(_serv_socket, 5) < 0) {
	perror("Error listen");
	exit (1);
    }
    cli_setup();
    
    for ( ;; ) {
	struct timeval my_timeval;
	
	max_sock = _serv_socket;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_SET(_serv_socket, &read_fds);
	/* FD_SET(_serv_socket, &write_fds); */
	if (sock >= 0) {
	    FD_SET(sock, &read_fds);
	    /* FD_SET(sock, &write_fds); */
	    if (max_sock < sock)
		max_sock = sock;
	}
	max_sock++;
	
	my_timeval.tv_sec = 1;
	my_timeval.tv_usec = 0;
	select(max_sock, &read_fds, &write_fds, NULL, &my_timeval);
	
	if (FD_ISSET(_serv_socket, &read_fds)) {
	    memset(&cli_addr, 0, sizeof(cli_addr));
	    cli_addr.sin_len = sizeof(cli_addr);
	    cli_addr.sin_family = AF_INET;
	    sock = accept(_serv_socket, (struct sockaddr *)&cli_addr,
			  &addrlen);
	    if (sock < 0) {
		perror("Error accepting connection");
		exit (1);
	    }
	    start_connection(sock);
	    continue;
	}
	if ((sock >= 0) && FD_ISSET(sock, &read_fds)) {
	    client_read(sock);
	}
	printf("Tick...\n");
    }
}

void
cli_setup()
{
    _fd_file_read = NULL;
    _fd_file_write = NULL;
    _client_type = CLIENT_TERMINAL;	/* XXX: default is terminal */
    
    _gl = NULL;
    
    _telnet_iac = 0;
    _telnet_sb = 0;
    /* TODO: those should be read in the beginning */
    _window_width = 80;
    _window_height = 25;
}

const char *
newline()
{
    if (_client_type == CLIENT_TERMINAL)
	return ("\r\n");
    
    return ("\n");
}

void
start_connection(int fd)
{
    char cli_welcome[] = CLI_WELCOME;
    char cli_prompt[] = CLI_PROMPT;
    char buf[128];
    
    /* Setup the telnet options */
    {
	char will_echo_cmd[] = { IAC, WILL, TELOPT_ECHO, '\0' };
	char will_sga_cmd[]  = { IAC, WILL, TELOPT_SGA, '\0'  };
	char dont_linemode_cmd[] = { IAC, DONT, TELOPT_LINEMODE, '\0' };
	char do_window_size_cmd[] = { IAC, DO, TELOPT_NAWS, '\0' };
	char do_transmit_binary_cmd[] = { IAC, DO, TELOPT_BINARY, '\0' };
	
	write(fd, will_echo_cmd, sizeof(will_echo_cmd));
	write(fd, will_sga_cmd, sizeof(will_sga_cmd));
	write(fd, dont_linemode_cmd, sizeof(dont_linemode_cmd));
	write(fd, do_window_size_cmd, sizeof(do_window_size_cmd));
	write(fd, do_transmit_binary_cmd, sizeof(do_transmit_binary_cmd));
    }
    
    snprintf(buf, sizeof(buf), "%s%s", cli_welcome, newline());
    write(fd, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "%s", cli_prompt);
    write(fd, buf, strlen(buf));
    
    
    _fd_file_read = fdopen(fd, "r");
    _fd_file_write = fdopen(fd, "w");
    _gl = new_GetLine(1024, 2048);
    if (_gl == NULL)
	exit (1);
    
    /* Change the input and output streams for libtecla */
    if (gl_change_terminal(_gl, _fd_file_read, _fd_file_write, NULL) != 0) {
	_gl = del_GetLine(_gl);
	exit (1);
    }
}

void
client_read(int fd)
{
    char *line = NULL;
    char buf[1024];
    int i, n, ret_value;

    /*
     * XXX: Peek at the data, and later it will be read
     * by the command-line processing library
     */
    n = recv(fd, buf, sizeof(buf), MSG_PEEK);
    for (i = 0; i < n; i++)
	printf("VAL = %d\n", buf[i]);
    
    printf("client_read %d octets\n", n);
    if (n <= 0) {
	exit (0);
    }
    
    /* Scan the input data and filter-out the Telnet commands */
    for (i = 0; i < n; i++) {
	uint8_t val = buf[i];
	
	if (val == IAC) {
	    /* Probably a telnet command */
	    if (! _telnet_iac) {
		_telnet_iac = 1;
		goto post_process_telnet_label;
	    }
	    _telnet_iac = 0;
	}
	if (_telnet_iac) {
	    switch (val) {
	    case SB:
		/* Begin subnegotiation of the indicated option. */
		_telnet_sb_buffer_size = 0;
		_telnet_sb = 1;
		break;
	    case SE:
		/* End subnegotiation of the indicated option. */
		if (! _telnet_sb)
		    break;
		switch(_telnet_sb_buffer[0]) {
		case TELOPT_NAWS:
		    /* Telnet Window Size Option */
		    if (_telnet_sb_buffer_size < 5)
			break;
		    {
			_window_width
			    = 256 * _telnet_sb_buffer[1];
			_window_width
			    += _telnet_sb_buffer[2];
			_window_height
			    = 256 * _telnet_sb_buffer[3];
			_window_height
			    += _telnet_sb_buffer[4];
			printf("Client window size changed to width = %d "
			       "height = %d\n",
			       _window_width, _window_height);
		    }
		    break;
		default:
		    break;
		}
		_telnet_sb_buffer_size = 0;
		_telnet_sb = 0;
		break;
	    case DONT:
		/* you are not to use option */
		_telnet_dont = 1;
		break;
	    case DO:
		/* please, you use option */
		_telnet_do = 1;
		break;
	    case WONT:
		/* I won't use option */
		_telnet_wont = 1;
		break;
	    case WILL:
		/* I will use option */
		_telnet_will = 1;
		break;
	    case GA:
		/* you may reverse the line */
		break;
	    case EL:
		/* erase the current line */
		break;
	    case EC:
		/* erase the current character */
		break;
	    case AYT:
		/* are you there */
		break;
	    case AO:
		/* abort output--but let prog finish */
		break;
	    case IP:
		/* interrupt process--permanently */
		break;
	    case BREAK:
		/* break */
		break;
	    case DM:
		/* data mark--for connect. cleaning */
		break;
	    case NOP:
		/* nop */
		break;
	    case EOR:
		/* end of record (transparent mode) */
		break;
	    case ABORT:
		/* Abort process */
		break;
	    case SUSP:
		/* Suspend process */
		break;
	    case xEOF:
		/* End of file: EOF is already used... */
		break;
	    default:
		break;
	    }
	    _telnet_iac = 0;
	    goto post_process_telnet_label;
	}
	if (_telnet_sb) {
	    /* A negotiated option value */
	    _telnet_sb_buffer[_telnet_sb_buffer_size] = val;
	    if ( ++_telnet_sb_buffer_size >= sizeof(_telnet_sb_buffer)) {
		/* This client is sending too much options. Kick it out! */
		printf("Removing client!\n");
		exit (1);
	    }
	    goto post_process_telnet_label;
	}
	if (_telnet_dont) {
	    /* Telnet DONT option code */
	    _telnet_dont = 0;
	    goto post_process_telnet_label;
	}
	if (_telnet_do) {
	    /* Telnet DO option code */
	    _telnet_do = 0;
	    goto post_process_telnet_label;
	}
	if (_telnet_wont) {
	    /* Telnet WONT option code */
	    _telnet_wont = 0;
	    goto post_process_telnet_label;
	}
	if (_telnet_will) {
	    /* Telnet WILL option code */
	    _telnet_will = 0;
	    goto post_process_telnet_label;
	}
	
	line = gl_get_line_net(_gl,
			       CLI_PROMPT,
			       _command_buffer,
			       _buff_curpos);
	fflush(_fd_file_write);
	

	/* Store the line in the command buffer */
	if (line != NULL) {
	    if ((val == '\n') || (val == '\r')) {
		/* New command */
		fprintf(_fd_file_write, "Your command is: %s%s",
			line, newline());
		fflush(_fd_file_write);
		fprintf(_fd_file_write, "%s", CLI_PROMPT);
		fflush(_fd_file_write);
		_command_buffer_size = 0;
		_command_buffer[0] = '\0';
	    } else {
		/* Same-line character */
		int gl_buff_curpos = gl_get_buff_curpos(_gl);
		if (command_line_character(line, gl_buff_curpos, val)) {
		    int i;
		    /* We need to print this character */
		    _command_buffer_size = 0;
		    _command_buffer[0] = '\0';
		    ret_value = 0;
		    for (i = 0; line[i] != '\0'; i++) {
			_command_buffer[_command_buffer_size] = line[i];
			if (++_command_buffer_size >= sizeof(_command_buffer))
			    ret_value = -1;
			if (ret_value < 0)
			    break;
		    }
		    if (ret_value == 0)
			_command_buffer[_command_buffer_size] = '\0';
		    if (++_command_buffer_size >= sizeof(_command_buffer))
			ret_value = -1;
		    if (ret_value != 0) {
			/* This client is sending too much data. Kick it out!*/
			printf("Removing client!\n");
			exit (1);
		    }
		    _buff_curpos = gl_buff_curpos;
		}
	    }
	}
	fflush(_fd_file_write);
	continue;
	
    post_process_telnet_label:
	recv(fd, buf, 1, 0); /* Read-out this octet */
	continue;
    }
}

/**
 * command_line_character:
 * @line: The current command line.
 * @word_end: The cursor position.
 * @val: The last typed character.
 * 
 * Have a peek in the last type character and perform any special actions
 * if needed.
 * 
 * Return value: 1 to output this character, otherwise 0
 **/
int
command_line_character(const char *line, int word_end, uint8_t val)
{
    if (val == '?') {
	
	fprintf(_fd_file_write, "%sThis is a help!%s", newline(), newline());
	fflush(_fd_file_write);
	fprintf(_fd_file_write, "%s", CLI_PROMPT);
	fflush(_fd_file_write);
	fprintf(_fd_file_write, "%s", _command_buffer);
	fflush(_fd_file_write);
	return (0);
    }
    return (1);
}

