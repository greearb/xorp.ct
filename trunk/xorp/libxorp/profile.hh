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

// $XORP: xorp/libxorp/profile.hh,v 1.2 2004/09/21 21:44:35 atanu Exp $

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
    inline
    bool
    enabled(const string& pname) throw(PVariableUnknown)
    {
	// This is the most frequently called method hence make it
	// inline. As an optimisation if no profiling is enabled don't
	// perform any string maniplulation or lookups.

	// If global profiling has not been enabled get out of here.
	if (0 == _profile_cnt)
	    return false;

	profiles::iterator i = _profiles.find(pname);
	// Catch any mispelt pnames.
	if (i == _profiles.end())
	    xorp_throw(PVariableUnknown, pname.c_str());

	return i->second->enabled();
    }

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

#ifdef	PROFILE_UTILS_REQUIRED
/**
 * Utility methods to be used by programs providing profiling.
 */

class ProfileUtils {
 public:

    static
    void
    transmit_log(const string& pname, XrlStdRouter *xrl_router,
		 const string& instance_name,
		 Profile *profile)
    {
	ProfileLogEntry ple;
	if (profile->read_log(pname, ple)) {
	    TimeVal t = ple.time();
	    XrlProfileClientV0p1Client pc(xrl_router);
	    pc.send_log(instance_name.c_str(),
			pname, t.sec(), t.usec(), ple.loginfo(),
			callback(ProfileUtils::transmit_callback, 
				 pname, xrl_router, instance_name, profile));
	} else {
	    // Unlock the log entry.
	    profile->release_log(pname);
	    ProfileUtils::transmit_finished(pname, xrl_router, instance_name);
	}
    }

    static
    void
    transmit_callback(const XrlError& error, const string pname,
		      XrlStdRouter *xrl_router,
		      const string instance_name,
		      Profile *profile)
    {
	if (XrlError::OKAY() != error) {
	    XLOG_WARNING(error.error_msg());
	    // Unlock the log entry.
	    profile->release_log(pname);
	    return;
	}
	ProfileUtils::transmit_log(pname, xrl_router, instance_name, profile);
    }

    static
    void
    transmit_finished(const string& pname, XrlStdRouter *xrl_router,
		      const string& instance_name)
    {
	debug_msg("pname = %s instance_name = %s\n", pname.c_str(),
		  instance_name.c_str());

	XrlProfileClientV0p1Client pc(xrl_router);
	pc.send_finished(instance_name.c_str(), pname,
			 callback(ProfileUtils::transmit_finished_callback,
				  pname));
    }

    static
    void
    transmit_finished_callback(const XrlError& error,
			       const string /*pname*/)
    {
	if (XrlError::OKAY() != error)
	    XLOG_WARNING(error.error_msg());
    }

 private:    
    ProfileUtils();		// Don't allow instantiation
    ProfileUtils(const ProfileUtils&);
};
#endif // PROFILE_UTILS_REQUIRED

#endif // __LIBXORP_TRACE_HH__
