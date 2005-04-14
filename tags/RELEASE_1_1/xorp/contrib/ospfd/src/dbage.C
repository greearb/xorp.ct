/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Aging of the link state database.
 */

#include "ospfinc.h"
#include "nbrfsm.h"
#include "system.h"

// Declarations of statics
LSA *LSA::AgeBins[MaxAge+1];	// Aging Bins
int LSA::Bin0;			// Current age 0 bin
int32 LSA::RefreshBins[MaxAgeDiff];// Refresh bins
int LSA::RefreshBin0;		// Current refresh bin


/* Start aging an LSA. Remove it from an existing bin, if necessary.
 * MaxAge LSAs are not installed in any age bin; however, "lsa_agebin"
 * is set so that "since_received()" returns the correct answer.
 * MaxAge LSAs are instead installed on the "MaxAge_list", so that
 * they will be removed from the database as soon as they are
 * acknowledged.
 *
 * DoNotAge LSAs are always installed in the age 0 bin. Otherwise,
 * install in the bin currently corresponding to the LSA's
 * received age.
 */

void LSA::start_aging()

{
    uns16 bin;

    if (in_agebin)
	stop_aging();
    if (lsa_rcvage == MaxAge) {
	lsa_agebin = Age2Bin((age_t) 0);
	ospf->MaxAge_list.addEntry(this);
	return;
    }
    else if ((lsa_rcvage & DoNotAge) != 0)
	bin = Age2Bin((age_t) 0);
    else
	bin = Age2Bin(lsa_rcvage);

    in_agebin = true;
    lsa_agebin = bin;
    lsa_agefwd = AgeBins[bin];
    lsa_agerv = 0;
    if (AgeBins[bin])
	AgeBins[bin]->lsa_agerv = this;
    AgeBins[bin] = this;
}

/* Stop aging an LSA. Simply remove it from its current bin, and
 * reset the "aging" bit.
 */

void LSA::stop_aging()

{
    if (!in_agebin)
	return;

    if (lsa_agerv)
	lsa_agerv->lsa_agefwd = lsa_agefwd;
    else
	AgeBins[lsa_agebin] = lsa_agefwd;
    if (lsa_agefwd)
	lsa_agefwd->lsa_agerv = lsa_agerv;

    in_agebin = false;
    lsa_agefwd = 0;
    lsa_agerv = 0;
}


/* Database aging timer. Called once a second. Besides
 * perfoming database functions, also perfoms other
 * functions that need to be done periodically, like
 * rerunning the routing table calculation.
 */

void DBageTimer::action()

{
    LSA *lsap;
    LsaListIterator iter(&ospf->replied_list);

    // Age the link-state database
    ospf->dbage();
    // Exit helper mode?
    if (ospf->topology_change)
	ospf->htl_topology_change();
    // Delete down neighbors
    ospf->delete_down_neighbors();
    // Establish more adjacencies?
    while (ospf->n_lcl_inits < ospf->max_dds) {
	SpfNbr *np;
	if (!(np = GetNextAdj()))
	    break;
	np->nbr_fsm(NBE_EVAL);
    }
    // Clear reply flags in any LSAs
    while ((lsap = iter.get_next())) {
	lsap->sent_reply = false;
	iter.remove_current();
    }
    // Run routing calculations
    if (ospf->full_sched)
	ospf->full_calculation();
    if (ospf->ase_sched)
	ospf->do_all_ases();
    if (ospf->clear_mospf == true)
        ospf->mospf_clear_cache();
    // Process any pending LSA activity (flooding, origination)
    // Synchronize with kernel
    ospf->krt_sync();

    // Upload remnants of routing table installed by previous instances
    if (ospf->need_remnants) {
        ospf->need_remnants = false;
	sys->upload_remnants();
    }

    // Perform hitless restart processing
    if (ospf->in_hitless_restart()) {
        if (ospf->check_htl_termination)
	    ospf->htl_exit_criteria();
        if (ospf->start_htl_exit)
	    ospf->exit_hitless_restart(ospf->htl_exit_reason);
    }
}

/* Main aging routine called once a second. Increment the current
 * "Bin0", which effectively ages all bins by one second. Then,
 * perform processing on the bins whose age values have
 * special meaning in the OSPF specification:
 *
 *	Check for deferred originations (5 second bin)
 *	Refresh self-originated LSAs (30 minute bin)
 *	CheckAge processing (15,30,45 minute bins)
 *	Flush (by reflooding) MaxAge advertisements (60 minute bin)
 *
 * After this is done, go through the list of the LSAs that are being
 * flushed to see which ones can be removed from the database
 * (i.e., when no neighbors in Database Exchange, and LSA has been
 * acked by all adjacent neighbors). As a special case, those
 * LSAs that indicate "LSA::rollover" will be refreshed with the
 * initial sequence number, instead of being removed.
 */

void OSPF::dbage()

{
    // Increment age by one second
    LSA::Bin0++;
    if (LSA::Bin0 > MaxAge)
	LSA::Bin0 = 0;

    // Process LSAs of certain ages
    deferred_lsas();
    checkages();
    refresh_lsas();
    maxage_lsas();
    refresh_donotages();
    do_refreshes();

    // Finish any flooding that was caused by age routines
    send_updates();
    // Check to see whether any MaxAge can be deleted
    free_maxage_lsas();
    // Check to see whether we need to flush DoNotAge LSAs
    donotage_changes();
    // If shutting down, see if we can go to next phase
    if (shutting_down() &&
	(--countdown <= 0 || MaxAge_list.is_empty()))
	shutdown_continue();
    
}

/* Go through the LSAs of age MinLSInterval, reoriginating
 * those that have been deferred.
 * We firest put them on a list, so that the act of reoriginating
 * doesn't corrupt the singly linked list within t he age bins (in
 * some cases, reoriginating one LSA causes others to be reoriginated,
 * so it is not enough to simply remember the next LSA in the
 * age bin).
 */

void OSPF::deferred_lsas()

{
    uns16 bin;
    LSA	*lsap;
    LsaList defer_list;

    bin = Age2Bin(MinLSInterval);

    for (lsap = LSA::AgeBins[bin]; lsap; lsap = lsap->lsa_agefwd) {
	if (!lsap->deferring)
	    continue;
	if (lsap->adv_rtr() == myid) {
	    lsap->deferring = false;
	    defer_list.addEntry(lsap);
	}
    }

    LsaListIterator *iter;
    iter = new LsaListIterator(&defer_list);
    while((lsap = iter->get_next())) {
        if (lsap->valid() && lsap->lsa_agebin == bin)
	    lsap->reoriginate(false);
	iter->remove_current();
    }
    delete iter;
}

/* Verify LSA checksums every 15 minutes, on the average.
 * Even out the checksum calculations so that we're doing
 * the same number every second.
 */

void OSPF::checkages()

{
    age_t age;
    int limit;
    LSA *lsap;

    for (age = CheckAge; age < MaxAge; age += CheckAge) {
	uns16 bin;
	bin = Age2Bin(age);
	for (lsap = LSA::AgeBins[bin]; lsap; lsap = lsap->lsa_agefwd) {
	    if (!lsap->checkage) {
	        lsap->checkage = true;
		dbcheck_list.addEntry(lsap);
	    }
	}
    }

    LsaListIterator iter(&dbcheck_list);
    limit = total_lsas/CheckAge + 1;

    for (int i = 0; (lsap = iter.get_next()) && i < limit; i++) {
	if (lsap->valid()) {	  
	    LShdr *hdr;
	    int xlen;
	    hdr = BuildLSA(lsap);
	    xlen = ntoh16(hdr->ls_length) - sizeof(uns16);
	    if (!hdr->verify_cksum())
		sys->halt(HALT_DBCORRUPT, "Corrupted LS database");
	}
	lsap->checkage = false;
	iter.remove_current();
    }

    // Get rid of any invlide entries
    dbcheck_list.garbage_collect();
}

/* Reoriginate all self-originated LSAs of age LSRefreshTime.
 */

void OSPF::refresh_lsas()

{
    uns16 bin;
    LSA	*lsap;
    LSA	*next_lsa;

    bin = Age2Bin(LSRefreshTime);

    for (lsap = LSA::AgeBins[bin]; lsap; lsap = next_lsa) {
	next_lsa = lsap->lsa_agefwd;
	if (lsap->do_not_age())
	    continue;
	if (lsap->adv_rtr() == myid)
	    schedule_refresh(lsap);
    }
}

/* Flush all MaxAge LSAs by reflooding them. Remove from
 * aging bins and keep on separate list which will be
 * periodically scanned to see whether we can garbage collect
 * them.
 */

void OSPF::maxage_lsas()

{
    uns16 bin;
    LSA	*lsap;
    LSA	*next_lsa;

    bin = Age2Bin(MaxAge);

    for (lsap = LSA::AgeBins[bin]; lsap; lsap = next_lsa) {
	next_lsa = lsap->lsa_agefwd;
	if (lsap->do_not_age()) {
	    if (lsap->adv_rtr() == myid) {
	        lsap->lsa_hour++;
		continue;
	    }
	    else if (lsap->source->valid())
	        continue;
	}
	age_prematurely(lsap);
    }
}

/* Reoriginate all self-originated DoNotAge LSAs, according to
 * OSPF::refresh_rate.
 */

void OSPF::refresh_donotages()

{
    uns16 age;
    uns32 hour;
    uns16 bin;
    LSA	*lsap;
    LSA	*next_lsa;

    if (refresh_rate <= LSRefreshTime)
        return;

    hour = refresh_rate/3600;
    age = refresh_rate%3600;
    bin = Age2Bin(age);

    for (lsap = LSA::AgeBins[bin]; lsap; lsap = next_lsa) {
	next_lsa = lsap->lsa_agefwd;
	if (lsap->adv_rtr() != myid)
	    continue;
	if (!lsap->do_not_age())
	    continue;
	if (lsap->lsa_hour >= hour)
	    schedule_refresh(lsap);
    }
}

/* Go through the list of LSAs that are being flushed and see
 * whether they can be returned to the heap.
 */

void OSPF::free_maxage_lsas()

{
    LSA *lsap;
    LsaListIterator iter(&MaxAge_list);

    while ((lsap = iter.get_next())) {
	if (!lsap->valid()) {	  
	    iter.remove_current();
	    continue;
	}
	if (lsap->lsa_rxmt != 0)
	    continue;
	if (!maxage_free(lsap->ls_type()))
	    continue;
	// OK to free. Remove from database
	// List processing will then return
	// to heap when appropriate
	iter.remove_current();
	if (lsap->rollover) {
	    lsap->rollover = false;
	    lsap->refresh(InvalidLSSeq);
	}
	else
	    ospf->DeleteLSA(lsap);
    }
}

/* Process changes in the DoNotAge capability of the network.
 */

void OSPF::donotage_changes()

{
    AreaIterator iter(this);
    SpfArea *a;

    // Reoriginate indication-LSAs?
    // Also flush AS-external-LSAs, if necessary
    if (dna_change) {
        AreaIterator *oiter;
        oiter = new AreaIterator(this);
	dna_flushq.clear();
	while ((a = oiter->get_next())) {
	    ASBRrte *rrte;
	    if (a->is_stub())
	        continue;
	    // (Re)originate indication-LSAs
	    rrte = add_asbr(myid);
	    a->asbr_orig(rrte);
	}
	if (!donotage())
	    flush_donotage();
	delete oiter;
	dna_change = false;
    }

    // Now do areas whose DoNotAge capability has changed
    while ((a = iter.get_next())) {
        if (!a->dna_change)
	    continue;
	a->a_dna_flushq.clear();
	if (!a->donotage())
	    a->a_flush_donotage();
	a->dna_change = false;
    }
}

/* Go through and flush all DoNotAge AS-external-LSAs.
 * Locally originated ones are simply refreshed without
 * the DoNotAge bit set.
 */

void OSPF::flush_donotage()

{
    AVLsearch iter(&extLSAs);
    ASextLSA *lsap;

    while ((lsap = (ASextLSA *)iter.next())) {
        ExRtData *exdata = 0;
        if (!lsap->do_not_age())
	    continue;
	if (lsap->adv_rtr() == my_id() &&
	    lsap->orig_rte != 0 &&
	    (exdata = lsap->orig_rte->exdata)) {
	    ase_schedule(exdata);
	}
	else {
	    dna_flushq.addEntry(lsap);
	    if (!origtim.is_running())
	        origtim.start(Timer::SECOND/10);
	}
    }
}

/* Flush all the DoNotAge LSAs from an area. Our locally-originated
 * LSAs are simply refreshed.
 */

void SpfArea::a_flush_donotage()

{
    byte lstype;

    for (lstype = 0; lstype <= MAX_LST; lstype++) {
	AVLtree *tree;
	AVLsearch *iter;
	LSA *lsap;
	if (flooding_scope(lstype) != AreaScope)
	    continue;
	if (!(tree = ospf->FindLSdb(0, this, lstype)))
	    continue;
	iter = new AVLsearch(tree);
	while ((lsap = (LSA *)iter->next())) {
	    if (!lsap->do_not_age())
	        continue;
	    if (lsap->adv_rtr() == ospf->my_id())
	        lsap->reoriginate(false);
	    else {
	        a_dna_flushq.addEntry(lsap);
		if (!ospf->origtim.is_running())
	            ospf->origtim.start(Timer::SECOND/10);
	    }
	}
	delete iter;
    }
}

/* The official time to refresh a given LSA has arrived.
 * If configured to randomly refresh, delay the refresh
 * randomly up to MaxAgeDiff seconds. Otherwise (the default)
 * refresh the LSA immediately by forcing a reorigination.
 */

void OSPF::schedule_refresh(LSA *lsap)

{
    int slot;

    /* If random_refresh, 
     * we are going to randomly delay reorigination
     * until some time in the next MaxAgeDiff
     * seconds. Note: we don't care where the current
     * refresh bin is.
     */
    slot = Timer::random_period(MaxAgeDiff);
    if (!random_refresh || slot < 0 || slot >= MaxAgeDiff)
        slot = LSA::RefreshBin0;
    LSA::RefreshBins[slot]++;
    pending_refresh.addEntry(lsap);
}

/* Go through the list of scheduled originations, and refresh
 * the number that have been scheduled for this 
 * timeslot (width one second). LSAs that have already
 * been overwritten or deleted still count against the tally,
 * but are of course not refreshed.
 */

void OSPF::do_refreshes()

{
    int count;
    LSA *lsap;
    LsaListIterator iter(&pending_refresh);

    count = LSA::RefreshBins[LSA::RefreshBin0];
    LSA::RefreshBins[LSA::RefreshBin0] = 0;
    // Refresh up to count LSAs
    for (; count > 0 && (lsap = iter.get_next()); count--) {
        int msgno;
	if (!lsap->valid() || lsap->lsa_age() == MaxAge) {
	    iter.remove_current();
	    continue;
	}
	msgno = lsap->do_not_age() ? LOG_DNAREFR : LOG_LSAREFR;
	if (spflog(msgno, 1))
	    log(lsap);
	iter.remove_current();
        lsap->reoriginate(true);
    }

    LSA::RefreshBin0++;
    if (LSA::RefreshBin0 >= MaxAgeDiff)
        LSA::RefreshBin0 = 0;
}
