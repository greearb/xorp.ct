// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/ifconfig_reporter.hh,v 1.6 2008/01/03 22:59:34 pavlin Exp $

#ifndef __FEA_IFCONFIG_REPORTER_HH__
#define __FEA_IFCONFIG_REPORTER_HH__


//
// Mechanism for reporting network interface related information to
// interested parties.
//

#include <list>

class IfConfigUpdateReplicator;
class IfTree;
class IPv4;
class IPv6;


/**
 * @short Base class for propagating update information on from IfConfig.
 *
 * When the platform @ref IfConfig updates interfaces it can report
 * updates to an IfConfigUpdateReporter.
 */
class IfConfigUpdateReporterBase {
public:
    enum Update { CREATED, DELETED, CHANGED };

    /**
     * Constructor for a given replicator.
     *
     * @param update_replicator the corresponding replicator
     * (@ref IfConfigUpdateReplicator).
     */
    IfConfigUpdateReporterBase(IfConfigUpdateReplicator& update_replicator);

    /**
     * Constructor for a given replicator and observed tree.
     *
     * @param update_replicator the corresponding replicator
     * (@ref IfConfigUpdateReplicator).
     * @param observed_iftree the corresponding interface tree (@ref IfTree).
     */
    IfConfigUpdateReporterBase(IfConfigUpdateReplicator& update_replicator,
			       const IfTree& observed_iftree);

    /**
     * Destructor.
     */
    virtual ~IfConfigUpdateReporterBase() {}

    /**
     * Get a reference to the observed interface tree.
     *
     * @return a reference to the observed interface tree (see @IfTree).
     */
    const IfTree& observed_iftree() const { return (_observed_iftree); }

    /**
     * Add itself to the replicator (see @IfConfigUpdateReplicator).
     */
    void add_to_replicator();

    /**
     * Remove itself from the replicator (see @IfConfigUpdateReplicator).
     */
    void remove_from_replicator();

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

private:
    IfConfigUpdateReplicator& _update_replicator;
    const IfTree&	_observed_iftree;
};

/**
 * @short A class to replicate update notifications to multiple reporters.
 */
class IfConfigUpdateReplicator : public IfConfigUpdateReporterBase {
public:
    typedef IfConfigUpdateReporterBase::Update Update;

public:
    IfConfigUpdateReplicator(const IfTree& observed_iftree)
	: IfConfigUpdateReporterBase(*this, observed_iftree)
    {}
    virtual ~IfConfigUpdateReplicator();

    /**
     * Add a reporter instance to update notification list.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_reporter(IfConfigUpdateReporterBase* rp);

    /**
     * Remove a reporter instance from update notification list.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_reporter(IfConfigUpdateReporterBase* rp);

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
    const string& first_error() const		{ return _first_error; }

    /**
     * @return error message of last error encountered.
     */
    const string& last_error() const		{ return _last_error; }

    /**
     * @return number of errors reported.
     */
    uint32_t error_count() const		{ return _error_cnt; }

    /**
     * Reset error count and error message.
     */
    void reset() {
	_first_error.erase();
	_last_error.erase();
	_error_cnt = 0;
    }

    void log_error(const string& s) {
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
