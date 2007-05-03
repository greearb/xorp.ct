// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_reporter.hh,v 1.1 2007/04/18 06:20:57 pavlin Exp $

#ifndef __FEA_IFCONFIG_REPORTER_HH__
#define __FEA_IFCONFIG_REPORTER_HH__


//
// Mechanism for reporting network interface related information to
// interested parties.
//

#include <list>

class IPv4;
class IPv6;


/**
 * @short Base class for propagating update information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * updates to an IfConfigUpdateReporter.  The @ref IfConfig instance
 * takes a pointer to the IfConfigUpdateReporter object it should use.
 */
class IfConfigUpdateReporterBase {
public:
    enum Update { CREATED, DELETED, CHANGED };

    virtual ~IfConfigUpdateReporterBase() {}

    virtual void interface_update(const string& ifname,
				  const Update& u) = 0;

    virtual void vif_update(const string& ifname,
			    const string& vifname,
			    const Update& u) = 0;

    virtual void vifaddr4_update(const string& ifname,
				 const string& vifname,
				 const IPv4&   addr,
				 const Update& u) = 0;

    virtual void vifaddr6_update(const string& ifname,
				 const string& vifname,
				 const IPv6&   addr,
				 const Update& u) = 0;

    virtual void updates_completed() = 0;
};

/**
 * @short A class to replicate update notifications to multiple reporters.
 */
class IfConfigUpdateReplicator : public IfConfigUpdateReporterBase {
public:
    typedef IfConfigUpdateReporterBase::Update Update;

public:
    virtual ~IfConfigUpdateReplicator();

    /**
     * Add a reporter instance to update notification list.
     *
     * @return true on success, false if object already receiving updates or
     * could not be added.
     */
    bool add_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Remove a reporter instance from update notification list.
     *
     * @return true on success, false if instance is not on the
     * receiving updates list.
     */
    bool remove_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Forward interface update notification to reporter instances on
     * update notification list.
     */
    void interface_update(const string& ifname,
			  const Update& u);

    /**
     * Forward virtual interface update notification to reporter
     * instances on update notification list.
     */
    void vif_update(const string& ifname,
		    const string& vifname,
		    const Update& u);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr4_update(const string& ifname,
			 const string& vifname,
			 const IPv4&   addr,
			 const Update& u);

    /**
     * Forward virtual interface address update notification to
     * reporter instances on update notification list.
     */
    void vifaddr6_update(const string& ifname,
			 const string& vifname,
			 const IPv6&   addr,
			 const Update& u);

    /**
     * Forward notification that updates were completed to
     * reporter instances on update notification list.
     */
    void updates_completed();

protected:
    list<IfConfigUpdateReporterBase*> _reporters;
};

/**
 * @short Base class for propagating error information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * errors to an IfConfigErrorReporterBase.  The @ref IfConfig instance
 * takes a pointer to the IfConfigErrorReporterBase object it should use.
 */
class IfConfigErrorReporterBase {
public:
    IfConfigErrorReporterBase() : _error_cnt(0) {}
    virtual ~IfConfigErrorReporterBase() {}

    virtual void config_error(const string& error_msg) = 0;

    virtual void interface_error(const string& ifname,
				 const string& error_msg) = 0;

    virtual void vif_error(const string& ifname,
			   const string& vifname,
			   const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv4&   addr,
			       const string& error_msg) = 0;

    virtual void vifaddr_error(const string& ifname,
			       const string& vifname,
			       const IPv6&   addr,
			       const string& error_msg) = 0;

    /**
     * @return error message of first error encountered.
     */
    inline const string& first_error() const	{ return _first_error; }

    /**
     * @return error message of last error encountered.
     */
    inline const string& last_error() const	{ return _last_error; }

    /**
     * @return number of errors reported.
     */
    inline uint32_t error_count() const		{ return _error_cnt; }

    /**
     * Reset error count and error message.
     */
    inline void reset() {
	_first_error.erase();
	_last_error.erase();
	_error_cnt = 0;
    }

    inline void	log_error(const string& s) {
	if (0 == _error_cnt)
	    _first_error = s;
	_last_error = s;
	_error_cnt++;
    }

private:
    string	_last_error;	// last error reported
    string	_first_error;	// first error reported

    uint32_t	_error_cnt;	// number of errors reported
};

/**
 * @short Class for propagating error information during IfConfig operations.
 *
 */
class IfConfigErrorReporter : public IfConfigErrorReporterBase {
public:
    IfConfigErrorReporter();

    void config_error(const string& error_msg);

    void interface_error(const string& ifname,
			 const string& error_msg);

    void vif_error(const string& ifname,
		   const string& vifname,
		   const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv4&   addr,
		       const string& error_msg);

    void vifaddr_error(const string& ifname,
		       const string& vifname,
		       const IPv6&   addr,
		       const string& error_msg);

private:
};

#endif // __FEA_IFCONFIG_REPORTER_HH__
