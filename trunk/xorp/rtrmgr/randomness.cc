// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/rtrmgr/randomness.cc,v 1.1.1.1 2002/12/11 23:56:16 hodson Exp $"

#include <fcntl.h>
#include <string>
#include "config.h"
#include "libxorp/eventloop.hh"
#include "randomness.hh"
#include "md5.h"



RandomGen::RandomGen() {
    _eventloop = NULL;
    _random_data = NULL;
    _last_loc = 0;
    _counter = 0;

    u_int i;
    FILE *file;
    file = fopen("/dev/random", "r");
    if (file == NULL)
	_random_exists = false;
    else {
	_random_exists = true;
	fclose(file);
    }

    file = fopen("/dev/urandom", "r");
    if (file == NULL) 
	_urandom_exists = false;
    else {
	_urandom_exists = true;
	fclose(file);
	return;
    }

    bool not_enough_randomness = false;
    if (_random_exists) {
	file = fopen("/dev/random", "r");
	//we need non-blocking reads
	fcntl(fileno(file), F_SETFD, O_NONBLOCK);

	//use /dev/random to initialized the random pool
	_random_data = new uint8_t[RAND_POOL_SIZE];
	int bytes = fread(_random_data, 1, RAND_POOL_SIZE, file);
	if (bytes < 16) {
	    //we didn't get enough randomness to be useful
	    not_enough_randomness = true;
	}
	fclose(file);
    }

    if ((!_urandom_exists && !_random_exists) || not_enough_randomness) {
	//We need to generate our own randomness.  This is hard.
	//General strategy: 1. read a bunch of stuff that the attacker
	//can't read (this assumes we're running as root).  2. read a
	//bunch of stuff that the attacker can read, but that will be
	//different each time the system starts.  3. build a big
	//buffer of all this data, XORed together.  4. Use MD5 to
	//extract data from the buffer.  5. Add timing-based
	//randomness every opportunity we can.
	int count = 0;
	int scount = 0;
	if (_random_data == NULL)
	    _random_data = new uint8_t[RAND_POOL_SIZE];

	//current time - this is mostly predictable, but a few less
	//significant bits are hard to predict
	struct timeval tv;
        gettimeofday(&tv, NULL);
	for(i=0; i< sizeof(tv); i++) {
	    _random_data[i]^=*(((char*)(&tv))+i);
	}
	    
	
	//Read host-based secrets - good source of unpredictable data,
	//but we need to be very careful not to reveal the secrets.
	//Note: these so won't vary between iterations, so they're
	//definitely not good enough by themselves.
	if (read_file("/etc/ssh_host_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_host_dsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_dsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_host_rsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh/ssh_host_rsa_key")) {
	    scount++; count++;
	}
	if (read_file("/etc/master.passwd")) {
	    scount++; count++;
	}
	if (read_file("/etc/shadow")) {
	    scount++; count++;
	}
	if (read_file("/etc/ssh_random_seed")) {
	    scount++; count++;
	}
	if (read_file("/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}
	if (read_file("/usr/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}
	if (read_file("/var/tmp/rtrmgr-seed")) {
	    scount++; count++;
	}
	
	//read packet counts - another source of hopefully
	//non-repeating but predictable data
	file = popen("/usr/bin/netstat -i", "r");
	if (file!=NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    fprintf(stderr, "popen of \"netstat -i\" failed\n");
	}

	//read last log - another source of hopefully
	//non-repeating but predictable data
	file = popen("/usr/bin/last", "r");
	if (file!=NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    fprintf(stderr, "popen of \"last\" failed\n");
	}

	//read current processes - another source of hopefully
	//non-repeating but predictable data.
	file = popen("/usr/bin/ps -elf", "r");
	if (file!=NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    fprintf(stderr, "popen of \"ps -elf\" failed\n");
	}
	
	//read current processes, attempt 2 - another source of hopefully
	//non-repeating but predictable data
	file = popen("/usr/bin/ps -auxw", "r");
	if (file!=NULL) {
	    count++;
	    read_fd(file);
	    pclose(file);
	} else {
	    fprintf(stderr, "popen of \"ps -auxw\" failed\n");
	}
	printf("random data pool initialized: count=%d scount=%d\n",
	       count, scount);
	printf("\n");
	for(i=0; i< 80; i++)
	    printf("%x", _random_data[i]);
	printf("\n");

	file = fopen("/tmp/rtrmgr-seed", "w");
	if (file == NULL)
	    file = fopen("/usr/tmp/rtrmgr-seed", "w");
	if (file == NULL)
	    file = fopen("/var/tmp/rtrmgr-seed", "w");
	if (file != NULL) {
	    uint8_t outbuf[16];
	    MD5_CTX md5_context;
	    MD5Init(&md5_context);
	    MD5Update(&md5_context, _random_data, RAND_POOL_SIZE);
	    MD5Final(outbuf, &md5_context);
	    fwrite(outbuf, 1, 16, file);
	    fwrite(&tv, 1, sizeof(tv), file);
	    fclose(file);
	}
    }
}

void RandomGen::add_event_loop(EventLoop *eventloop) {
    _eventloop = eventloop;
}


bool RandomGen::read_file(const string& filename) {
    FILE *file = fopen(filename.c_str(), "r");
    printf("RNG: reading %s\n", filename.c_str());
    bool result = false;
    if (file != NULL) {
	result = read_fd(file);
	fclose(file);
    } else {
	printf("failed to read %s\n", filename.c_str());
    }
    return result;
}
	
bool RandomGen::read_fd(FILE *file) {
    size_t bytes = 0;
    u_char tbuf[RAND_POOL_SIZE];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (file != NULL) {
	bytes = fread(tbuf, 1, RAND_POOL_SIZE, file);
	printf("RNG: read %u bytes\n", (uint32_t)bytes);
	char ixb[4];
	uint32_t ix;
	if (bytes > 0) {
	    u_int i;
#ifdef NOTDEF
	    printf("orig tbuf:\n");
	    for(i=0; i< (uint)min(80, (int)bytes); i++)
		printf("%2x", tbuf[i]);
	    printf("\n");
#endif
	    //Use MD5 to self-encrypt the data.  We don't want to risk
	    //sensitive data being able to be read from a memory image.
	    MD5_CTX md5_context;
	    u_char chain[16];
	    MD5Init(&md5_context);
	    MD5Update(&md5_context, tbuf, bytes);
	    MD5Final(chain, &md5_context);
	    for (i=0; i<RAND_POOL_SIZE/16; i++) {
		MD5Init(&md5_context);
		MD5Update(&md5_context, chain, 16);
		MD5Update(&md5_context, tbuf+(i*16), 16);
		MD5Final(chain, &md5_context);
		//overwrite in situe
		memcpy(tbuf+(i*16), chain, 16);
		if (i*16 > bytes) {
		    printf("i=%d\n", i);
		    break;
		}
	    }
	    
#ifdef NOTDEF
	    printf("tbuf:\n");
	    for(i=0; i< (uint)min(80, (int)bytes); i++)
		printf("%2x", tbuf[i]);
	    printf("\n");
#endif

	    for (i=0; i<bytes; i++) {
		//XOR the two together
		_random_data[i] ^= tbuf[i];
		ixb[i%4] ^= _random_data[i];
	    }
	    //zero the temporary buffer, just to be safe
	    memset(tbuf, 0, bytes);

	    //add more time bits - don't get a lot from this but it's cheap
	    struct timeval tv;
	    gettimeofday(&tv, NULL);
	    memcpy(&ix, ixb, 4);
	    ix ^= tv.tv_usec;
	    ix = ix % (RAND_POOL_SIZE - sizeof(tv));
	    //add them at a "random" location - not truely random
	    //because we may not have enough entropy in the pool yet
	    for(i=0; i< sizeof(tv); i++) {
		_random_data[i]^=*(((char*)(&tv))+i+ix);
	    }
	}
    }
    if (bytes > 0) return true;
    return false;
}

void RandomGen::add_buf_to_randomness(uint8_t *buf, int len) {
    if (_last_loc + len > RAND_POOL_SIZE)
	_last_loc = 0;
    for (int i=0; i<min(len, RAND_POOL_SIZE); i++) {
	_random_data[i] ^= buf[i];
    }
}
	
//get_random_bytes is guaranteed to never block
void RandomGen::get_random_bytes(size_t len, uint8_t *buf) {
    //don't allow stupid usage
    assert(len < RAND_POOL_SIZE/100);
    if (_urandom_exists) {
	FILE *file = fopen("/dev/urandom", "r");    
	if (file == NULL) {
	    abort();
	} else {
	    size_t bytes_read = fread(buf, 1, len, file);
	    if (bytes_read < len) {
		fprintf(stderr, 
			"Failed read on randomness source; read %u words\n",
			(uint32_t)bytes_read);
	    }
	}
	fclose(file);
	return;
    }
    if (_random_exists) {
	FILE *file = fopen("/dev/random", "r");    
	if (file == NULL) {
	    abort();
	} else {
	    fcntl(fileno(file), F_SETFD, O_NONBLOCK);
	    size_t bytes_read = fread(buf, 1, len, file);
	    if (len > 0)
		add_buf_to_randomness(buf, len);
	    if (bytes_read == len) {
		fclose(file);
		return;
	    }
	    //else fall through
	}
    }

    //generate the random data using repeated MD5 hashing of the
    //random pool
    uint8_t outbuf[16];
    MD5_CTX md5_context;
    size_t bytes_written = 0;
    while (bytes_written < len) {
	MD5Init(&md5_context);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	add_buf_to_randomness((uint8_t*)(&_counter), 4);
	_counter++;
	add_buf_to_randomness((uint8_t*)(&tv), sizeof(tv));
	MD5Update(&md5_context, _random_data, RAND_POOL_SIZE);
	MD5Final(outbuf, &md5_context);
	memcpy(buf+bytes_written, outbuf, min(16, (int)(len-bytes_written)));
	bytes_written += min(16, (int)(len-bytes_written));
    }
}
