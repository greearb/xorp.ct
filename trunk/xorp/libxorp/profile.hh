// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/profile.hh,v 1.1 2004/09/21 17:59:47 atanu Exp $

#ifndef __LIBXORP_PROFILE_HH__
#define __LIBXORP_PROFILE_HH__

#include <list>
#include <map>

#include "timeval.hh"
#include "exceptions.hh"
#include "ref_ptr.hh"

/**
 * Container keyed by profile variable holding log entries.
 *
 * A helper class used by XORP processes to support profiling.
 */

class PVariableUnknown : public XorpReasonedException {
public:
    PVariableUnknown(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("PVariableUnknown", file, line, init_why)
    {}
};

class PVariableExists : public XorpReasonedException {
public:
    PVariableExists(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("PVariableExists", file, line, init_why)
    {}
};

class PVariableNotEnabled : public XorpReasonedException {
public:
    PVariableNotEnabled(const char* file, size_t line,
			const string init_why = "")
 	: XorpReasonedException("PVariableNotEnabled", file, line, init_why)
    {}
};

class PVariableLocked : public XorpReasonedException {
public:
    PVariableLocked(const char* file, size_t line,
			const string init_why = "")
 	: XorpReasonedException("PVariableLocked", file, line, init_why)
    {}
};

class PVariableNotLocked : public XorpReasonedException {
public:
    PVariableNotLocked(const char* file, size_t line,
			const string init_why = "")
 	: XorpReasonedException("PVariableNotLocked", file, line, init_why)
    {}
};

class ProfileLogEntry {
 public:
    ProfileLogEntry() {}
    ProfileLogEntry(TimeVal time, string loginfo)
	: _time(time), _loginfo(loginfo)
    {}
    TimeVal time() {return _time;}
    string& loginfo() {return _loginfo;}
 private:
    TimeVal _time;	// Time the profile was recorded.
    string _loginfo;	// The profile data.
};

/**
 * Support for profiling XORP. Save the time that an event occured for
 * later retrieval.
 */
class Profile {
 public:
    typedef list<ProfileLogEntry> logentries;	// Profiling info

    class ProfileState {
    public:
	ProfileState() {}
	ProfileState(const string& comment, bool enabled, bool locked,
		     logentries *log) 
	    : _comment(comment), _enabled(enabled), _locked(locked), _log(log)
	{}
	inline void set_enabled(bool v) {_enabled = v;}
	inline bool enabled() const {return _enabled;}
	inline void set_locked(bool v) {_locked = v;}
	inline bool locked() const {return _locked;}
	inline logentries *logptr() const {return _log;}
	inline void zap() const {delete _log;}
	inline void set_iterator(logentries::iterator i) {_i = i;}
	inline void get_iterator(logentries::iterator& i) {i = _i;}
	inline int size() const {return _log->size();}
	const string& comment() const {return _comment;}

    private:
	const string _comment;	// Textual description of this variable.
	bool _enabled;		// True, if profiling is enabled.
	bool _locked;		// True, if we are currently reading the log.
	logentries::iterator _i;// pointer into the log
	logentries *_log;
    };

    typedef map<string, ref_ptr<ProfileState> > profiles;

    Profile();

    ~Profile();

    /**
     * Create a new profile variable.
     */
    void create(const string& pname, const string& comment = "")
	throw(PVariableExists);

    /**
     * Test for this profile variable being enabled.
     *
     * @return true if this profile is enabled.
     */
    bool enabled(const string& pname) throw(PVariableUnknown);

    /**
     * Add an entry to the profile log.
     */
    void log(const string& pname, string comment)
	throw(PVariableUnknown,PVariableNotEnabled);

    /**
     * Enable tracing.
     *
     * @parameter profile variable.
     */
    void enable(const string& pname)
	throw(PVariableUnknown,PVariableLocked);
    
    /**
     * Disable tracing.
     * @parameter profile variable.
     */
    void disable(const string& pname) throw(PVariableUnknown);

    /**
     * Lock the log in preparation for reading log entries.
     */
    void lock_log(const string& pname)
	throw(PVariableUnknown,PVariableLocked);

    /**
     * Read the next log entry;
     * @param pname
     * @param entry log entry
     * @return True a entry has been returned.
     */
    bool read_log(const string& pname, ProfileLogEntry& entry)
	throw(PVariableUnknown,PVariableNotLocked);

    /**
     * Release the log.
     */
    void release_log(const string& pname)
	throw(PVariableUnknown,PVariableNotLocked);

    /**
     * Clear the profiledata.
     */
    void clear(const string& pname) throw(PVariableUnknown,PVariableLocked);

    /**
     * @return A newline separated list of profiling variables along
     * with the associated comments.
     */
    string list() const;

 private:
    int _profile_cnt;		// Number of variables that are enabled.
    profiles _profiles;
};

#endif // __LIBXORP_TRACE_HH__
