// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/master_conf_tree.cc,v 1.12 2003/05/28 19:02:30 mjh Exp $"

#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>

#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "master_conf_tree.hh"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "module_command.hh"
#include "template_tree.hh"
#include "split.hh"
#include "main_rtrmgr.hh"

extern string booterrormsg(const char *s);

/*************************************************************************
 * Master Config Tree class 
 *************************************************************************/

MasterConfigTree::MasterConfigTree(const string& conffile, TemplateTree *tt, 
				   TaskManager &task_manager) 
    : ConfigTree(tt), _task_manager(task_manager)
{
    string configuration;
    string errmsg;
    if (!read_file(configuration, conffile, errmsg)) {
	XLOG_ERROR(errmsg.c_str());
	xorp_throw0(InitError);
    }
    if (!parse(configuration, conffile)) {
	printf("throwing InitError\n");
	xorp_throw0(InitError);
    }

    //Go through the config tree, and create nodes for any defaults
    //specified in the template tree that aren't already configured
    add_default_children();

    //If we got this far, it looks like we have a good bootfile.
    //The bootfile is now stored in the ConfigTree.
    //Now do the second pass, starting processes and configuring things.
    execute();
}

bool
MasterConfigTree::read_file(string& configuration, const string& conffile,
			    string& errmsg) {
    size_t len=0;
    FILE *file;
    file = fopen(conffile.c_str(), "r");
    if (file == NULL) {
	errmsg = c_format("Failed to open config file: %s\n", 
			  conffile.c_str());
	return false;
    }
#define MCT_READBUF 8192
    char buf[MCT_READBUF+1];
    while (feof(file)==0) {
	size_t bytes;
	bytes = fread(buf, 1, MCT_READBUF, file);
	len+=bytes;
	//null terminate it.
	buf[bytes]=0;
	if (bytes > 0) {
	    configuration += buf;
	}
    }
    fclose(file);
    return true;
}

bool MasterConfigTree::parse(const string& configuration, 
			    const string& conffile) {
    try {
	((ConfigTree*)this)->parse(configuration, conffile);
    } catch (ParseError &pe) {
	printf("caught ParseError: %s\n", pe.err.c_str());
	booterrormsg(pe.err.c_str());
	printf("caught ParseError 2\n");
	return false;
    }
    return true;
}

void MasterConfigTree::execute() {
    printf("##############################################################\n");
    printf("MasterConfigTree::execute\n");
    list <string> changed_modules = find_changed_modules();
    list <string>::const_iterator i;
    printf("Changed Modules:\n");
    for (i=changed_modules.begin(); i!=changed_modules.end(); i++) {
	printf("%s ", (*i).c_str());
    }
    printf("\n");
    _commit_cb = callback(this, &MasterConfigTree::config_done);
    commit_changes_pass2();
}

void MasterConfigTree::config_done(bool success, string errmsg) {
    printf("MasterConfigTree::config_done: ");
    if (success)
	printf("success\n");
    else
	printf("fail: %s\n", errmsg.c_str());

    if (!success) {
	XLOG_ERROR(("Startup failed (" + errmsg + ")\n").c_str());
	fprintf(stderr, ("Startup failed (" + errmsg + ")\n").c_str());
	xorp_throw0(InitError);
    }
    string errmsg2;
    if (check_commit_status(errmsg2) == false) {
	XLOG_ERROR(errmsg2.c_str());
	fprintf(stderr, errmsg2.c_str());
	xorp_throw0(InitError);
    }
    printf("MasterConfigTree::config_done returning\n");
}

list <string>
MasterConfigTree::find_changed_modules() const {
    printf("Find changed modules\n");
    set <string> changed_modules;
    _root_node.find_changed_modules(changed_modules);
    list <string> ordered_modules;
    order_module_list(changed_modules, ordered_modules);
    return ordered_modules;
}

list <string>
MasterConfigTree::find_active_modules() const {
    set <string> active_modules;
    _root_node.find_active_modules(active_modules);
    list <string> ordered_modules;
    order_module_list(active_modules, ordered_modules);
    return ordered_modules;
}

list <string>
MasterConfigTree::find_inactive_modules() const {
    set <string> all_modules;
    list <string> ordered_all_modules;
    _root_node.find_all_modules(all_modules);
    order_module_list(all_modules, ordered_all_modules);

    set <string> active_modules;
    list <string> ordered_active_modules;
    _root_node.find_active_modules(active_modules);
    order_module_list(active_modules, ordered_active_modules);

    //remove things that are common to both lists
    list <string>::iterator i;
    while (!ordered_active_modules.empty()) {
	for (i = ordered_all_modules.begin(); 
	     i != ordered_all_modules.end(); 
	     i++) {
	    if (*i == ordered_active_modules.front()) {
		ordered_all_modules.erase(i);
		ordered_active_modules.pop_front();
		break;
	    }
	}
	XLOG_ASSERT(i != ordered_all_modules.end());
    }
    return ordered_all_modules;
}


void
MasterConfigTree::order_module_list(const set <string>& module_set,
				    list <string>& ordered_modules) const {
    //we've found the list of modules that have changed that need to
    //be applied.  Now we need to sort them so that the modules to be
    //performed first are at the beginning of the list.

    //handle the degenerate cases simply
    if (module_set.empty())
	return;
	
    multimap <string, string> depends; //first depends on second
    set <string> no_info; //modules we found no info about
    set <string> satisfied; //modules we found no info about
    set <string> additional_modules; /*modules that we depend on that
				       aren't in module_set */
    set <string>::const_iterator i;
    for (i = module_set.begin(); i!=module_set.end(); i++) {
	ModuleCommand* mc = _template_tree->find_module(*i);

	if (mc == NULL) {
	    no_info.insert(*i);
	    printf("%s has no module info\n", (*i).c_str());
	    continue;
	}
	if (mc->depends().empty()) {
	    //this module doesn't depend on anything else, so it can
	    //be started early in the sequence
	    ordered_modules.push_back(*i);
	    satisfied.insert(*i);
	    printf("%s has no dependencies\n", (*i).c_str());
	    continue;
	}
	list <string>::const_iterator di;
	for (di = mc->depends().begin(); di != mc->depends().end(); di++) {
	    printf("%s depends on %s\n", i->c_str(), di->c_str());
	    depends.insert(pair<string,string>(*i, *di));
	    //check that the dependency is already in our list of modules.
	    if (module_set.find(*di)==module_set.end()) {
		//if not, add it.
		additional_modules.insert(*di);
	    }
	}
    }

    printf("doing additional modules\n");
    //figure out the dependencies for all the additional modules
    set <string> additional_done;
    while (!additional_modules.empty()) {
	i = additional_modules.begin();
	ModuleCommand* mc = _template_tree->find_module(*i);
	if (mc == NULL) {
	    printf("%s has no info\n", (*i).c_str());
	    additional_done.insert(*i);
	    additional_modules.erase(i);
	    ordered_modules.push_back(*i);
	    satisfied.insert(*i);
	    continue;
	}
	if (mc->depends().empty()) {
	    printf("%s has no dependencies\n", (*i).c_str());
	    additional_done.insert(*i);
	    additional_modules.erase(i);
	    ordered_modules.push_back(*i);
	    satisfied.insert(*i);
	    continue;
	}
	list <string>::const_iterator di;
	for (di = mc->depends().begin(); di != mc->depends().end(); di++) {
	    depends.insert(pair<string,string>(*i, *di));
	    printf("%s depends on %s\n", i->c_str(), di->c_str());
	    //check that the dependency is already in our list of modules.
	    if (module_set.find(*di)==module_set.end()
		&& additional_modules.find(*di)==additional_modules.end()
		&& additional_done.find(*di)==additional_done.end()) {
		//if not, add it.
		additional_modules.insert(*di);
	    }
	}
	additional_done.insert(*i);
	additional_modules.erase(i);
    }
    printf("done additional modules\n");

    multimap<string,string>::iterator cur, next;
    while (!depends.empty()) {
	bool progress_made = false;
	cur = depends.begin(); 
	while (cur!= depends.end()) {
	    next = cur;
	    next++;
	    printf("searching for dependency for %s on %s\n",
		   cur->first.c_str(), cur->second.c_str());
	    if (satisfied.find(cur->second) != satisfied.end()) {
		//rule is now satisfied.
		string module = cur->first;
		depends.erase(cur);
		progress_made = true;
		printf("dependency of %s on %s satisfied\n",
		       module.c_str(), cur->second.c_str());
		if (depends.find(module) == depends.end()) {
		    //this was the last dependency
		    satisfied.insert(module);
		    ordered_modules.push_back(module);
		    printf("dependencies for %s now satisfied\n",
			   module.c_str());
		}
	    }
	    cur = next;
	}
	if (progress_made == false) {
	    XLOG_FATAL("Module dependencies cannot be satisfied\n");
	}
    }
    
    //finally add the modules for which we have no information
    for (i = no_info.begin(); i!=no_info.end(); i++) {
	ordered_modules.push_back(*i);
    }
    return;
}

void
MasterConfigTree::commit_changes_pass1(CallBack cb) {
    printf("##############################################################\n");
    printf("MasterConfigTree::commit_changes_pass1\n");

    list <string> changed_modules = find_changed_modules();
    list <string> inactive_modules = find_inactive_modules();
    list <string>::const_iterator i;
    printf("Changed Modules:\n");
    for (i=changed_modules.begin(); i!=changed_modules.end(); i++) {
	printf("%s ", (*i).c_str());
    }
    printf("\n");

    /* Two passes: the first checks for errors.  If no errors are
       found, attempt the actual commit */

    /*******************************************************************/
    /* Pass 1: check for errors without actually doing anything        */
    /*******************************************************************/

    _task_manager.reset();
    _task_manager.set_do_exec(false);
    _commit_cb = cb;
    string result;

    _root_node.initialize_commit();
    //sort the changes in order of module dependencies
    for (i=changed_modules.begin(); i!=changed_modules.end(); i++) {
	if (!module_config_start(*i, result)) {
	    cb->dispatch(false, result);
	    return;
	}
    }

    if (_root_node.commit_changes(_task_manager,
				  /*do_commit = */false, 
				  0, 0, 
				  result) == false) {
	//something went wrong - return the error message.
	cb->dispatch(false, result);
	return;
    }

    for (i=inactive_modules.begin(); i!=inactive_modules.end(); i++) {
	_task_manager.shutdown_module(*i);
    }
    
    try {
	_task_manager.run(callback(this, 
				   &MasterConfigTree::commit_pass1_done));
    } catch (UnexpandedVariable& uvar) {
	//ignore this?
	//XXX
    }
}

void 
MasterConfigTree::commit_pass1_done(bool success, string result) {
    printf("##############################################################\n");
    printf("## commit_pass1_done\n");
    if (success)
	commit_changes_pass2();
    else {
	XLOG_WARNING(("Commit pass 1 failed: " + result).c_str());
	_commit_cb->dispatch(false, result);
    }
}

void
MasterConfigTree::commit_changes_pass2() {
    printf("##############################################################\n");
    printf("## commit_changes_pass2\n");
    /*******************************************************************/
    /* Pass 2: implement the changes                                   */
    /*******************************************************************/

    list <string> changed_modules = find_changed_modules();
    list <string> inactive_modules = find_inactive_modules();
    list <string>::const_iterator i;

    _task_manager.reset();
    _task_manager.set_do_exec(true);

    _root_node.initialize_commit();
    string result;
    //sort the changes in order of module dependencies
    for (i=changed_modules.begin(); i!=changed_modules.end(); i++) {
	if (!module_config_start(*i, result)) {
	    XLOG_ERROR("Commit failed in deciding startups\n");
	    _commit_cb->dispatch(false, result);
	    return;
	}
    }
	

    if (!_root_node.commit_changes(_task_manager,
				   /*do_commit = */true, 
				   0, 0, result)) {
	//abort the commit
	XLOG_ERROR("Commit failed in config tree\n");
	_commit_cb->dispatch(false, result);
	return;
    }

    for (i=inactive_modules.begin(); i!=inactive_modules.end(); i++) {
	_task_manager.shutdown_module(*i);
    }

    try {
	_task_manager.run(callback(this, 
				   &MasterConfigTree::commit_pass2_done));
    } catch (UnexpandedVariable& uvar) {
	result = uvar.str();
	XLOG_ERROR("Commit failed due to unexpanded variable\n");
	_commit_cb->dispatch(false, result);
	return;
    }
    return;
}

void 
MasterConfigTree::commit_pass2_done(bool success, string result) {
    printf("##############################################################\n");
    printf("## commit_pass2_done\n");
    if (success) 
	printf("## commit seems successful\n");
    else
	printf("## commit failed: %s\n", result.c_str());
    _commit_cb->dispatch(success, result);
}

bool MasterConfigTree::check_commit_status(string &result) {
    debug_msg("check_commit_status\n");
    bool success = _root_node.check_commit_status(result);
    if (success) {
	//if the commit was successful, clear all the temporary state.
	printf("commit was successful, finalizing...\n");
	_root_node.finalize_commit();
	printf("finalizing done\n");
    }
    return success;
}


string
MasterConfigTree::discard_changes() 
{
    printf("##############################################################\n");
    printf("MasterConfigTree::discard_changes\n");
    string result =
	_root_node.discard_changes(0, 0);
    printf("##############################################################\n");
    return result;
}

string
MasterConfigTree::mark_subtree_for_deletion(const list <string>& pathsegs,
					    uid_t user_id) 
{
    ConfigTreeNode *found = find_node(pathsegs);
    if (found == NULL)
	return "ERROR";

    if (found->parent() != NULL
	&& found->parent()->is_tag()
	&& found->parent()->children().size()==1) {
	found = found->parent();
    }
    
    found->mark_subtree_for_deletion(user_id);
    return string("OK");
}

void 
MasterConfigTree::delete_entire_config()
{
    _root_node.mark_subtree_for_deletion(0);
    _root_node.undelete();

    //we don't need a verification pass - this isn't allowed to fail.
    commit_changes_pass2();
}

bool 
MasterConfigTree::lock_node(const string& /*node*/, uid_t /*user_id*/, 
			    uint32_t /*timeout*/, 
			    uint32_t& /*holder*/) 
{
    //XXXX
    return true;
}

bool 
MasterConfigTree::unlock_node(const string& /*node*/, uid_t /*user_id*/) 
{
    //XXXX
    return true;
}

bool 
MasterConfigTree::save_to_file(const string& filename, 
			       uid_t user_id, 
			       string& errmsg) {
    errmsg = "";

    //set the effective group to "xorp"
    struct group *grp = getgrnam("xorp");
    if (grp == NULL) {
	errmsg = "ERROR: config files are saved as group \"xorp\".";
	errmsg = "Group \"xorp\" does not exist on this system.\n";
	return false;
    }
    gid_t gid, orig_gid;
    orig_gid = getgid();
    gid = grp->gr_gid;
    if (setegid(gid)<0) {
	errmsg = c_format("Failed to seteuid to group \"xorp\", gid %d\n",
			  gid);
	return false;
    }

    //set effective user ID to the uid of the user that requested the save
    uid_t orig_uid;
    orig_uid = getuid();
    if (seteuid(user_id)<0) {
	errmsg = c_format("Failed to seteuid to uid %d\n", user_id);
	setegid(orig_gid);
	return false;
    }

    //set a umask of 664, to allow sharing of config files between
    //users in group xorp
    mode_t orig_mask = umask(S_IWOTH);
    
    FILE *file;
    struct stat sb;
    if (stat(filename.c_str(), &sb)==0) {
	if (sb.st_mode & S_IFREG == 0) {
	    if (((sb.st_mode & S_IFMT) == S_IFCHR) ||
		((sb.st_mode & S_IFMT) == S_IFBLK)) {
		errmsg = c_format("File %s is a special device.\n", 
				  filename.c_str());
	    } else if ((sb.st_mode & S_IFMT) == S_IFIFO) {
		errmsg = c_format("File %s is a named pipe.\n", 
				  filename.c_str());
	    } else if ((sb.st_mode & S_IFMT) == S_IFDIR) {
		errmsg = c_format("File %s is a directory.\n", 
				  filename.c_str());
	    }
	    seteuid(orig_uid);
	    setegid(orig_gid);
	    umask(orig_mask);
	    return false;
	    }
    
	file = fopen(filename.c_str(), "r");
	if (file != NULL) {
	    //we've been asked to overwrite a file
	    char line[80];
	    if (fgets(line, 79, file)==NULL) {
		errmsg = "File " + filename + " exists, but an error occured when trying to check that it was OK to overwrite it\n";
		errmsg += "File was NOT overwritten\n";
		fclose(file);
		seteuid(orig_uid);
		setegid(orig_gid);
		umask(orig_mask);
		return false;
	    }
	    if (strncmp(line, "/*XORP", 6)!=0) {
		errmsg = "File " + filename + " exists, but it is not an existing XORP config file.\n";
		errmsg += "File was NOT overwritten\n";
		fclose(file);
		seteuid(orig_uid);
		setegid(orig_gid);
		umask(orig_mask);
		return false;
	    }
	    fclose(file);
	}
	//it seems OK to overwrite this file
	if (unlink(filename.c_str())<0) {
	    errmsg = "File " + filename + " exists, and can not be overwritten.\n";
	    errmsg += strerror(errno);
	    fclose(file);
	    seteuid(orig_uid);
	    setegid(orig_gid);
	    umask(orig_mask);
	    return false;
	}
    }
    
    file = fopen(filename.c_str(), "w");
    if (file == NULL) {
	errmsg = "Could not create file \"" + filename + "\"";
	errmsg += strerror(errno);
	seteuid(orig_uid);
	setegid(orig_gid);
	umask(orig_mask);
	return false;
    }

    //write the file header
    string header = "/*XORP Configuration File, v1.0*/\n";
    uint bytes;
    bytes = fwrite(header.c_str(), 1, header.size(), file);
    if (bytes < header.size()) {
	fclose(file);
	errmsg = "Error writing to file \"" + filename + "\"\n";
	//we couldn't even write the header - clean up if we can
	if (unlink(filename.c_str())==0) {
	    errmsg += "Save aborted; truncated file has been removed\n";
	} else {
	    errmsg += "Save aborted; truncated file may exist\n";
	}
	seteuid(orig_uid);
	setegid(orig_gid);
	umask(orig_mask);
	return false;
    }
    
    //write the config to the file
    string config = show_unannotated_tree();
    bytes = fwrite(config.c_str(), 1, config.size(), file);
    if (bytes < config.size()) {
	fclose(file);
	errmsg = "Error writing to file \"" + filename + "\"\n";
	errmsg += strerror(errno);
	errmsg += "\n";
	//we couldn't write the config - clean up if we can
	if (unlink(filename.c_str())==0) {
	    errmsg += "Save aborted; truncated file has been removed\n";
	} else {
	    errmsg += "Save aborted; truncated file may exist\n";
	}
	seteuid(orig_uid);
	setegid(orig_gid);
	umask(orig_mask);
	return false;
    }

    //set file group correctly
    if (fchown(fileno(file), user_id, gid)<0) {
	//this shouldn't be able to happen, but if it does, it
	//shouldn't be fatal
	errmsg = "WARNING: failed to set saved file to be group \"xorp\"\n";
    }

    //close properly and clean up
    if (fclose(file) != 0) {
	errmsg = "Error closing file \"" + filename + "\"\n";
	errmsg += strerror(errno);
	errmsg += "\nFile may not have been written correctly\n";
	seteuid(orig_uid);
	setegid(orig_gid);
	return false;
    }

    errmsg += "Save complete\n";
    seteuid(orig_uid);
    setegid(orig_gid);
    umask(orig_mask);
    return true;
}

bool 
MasterConfigTree::load_from_file(const string& filename, uid_t user_id,
				 string& errmsg,
				 string& deltas, string& deletions) {
    //We run load_from_file as the UID of the user making the request
    //and as group xorp.  This prevents users using the rtrmgr to
    //attempt to load files they wouldn't normally have had the
    //permission to read.  Otherwise it's possible that they could
    //load configs they shouldn't have access to, or get parts of
    //protected files reported to them in error messages.
    
    //set the effective group to "xorp"
    struct group *grp = getgrnam("xorp");
    if (grp == NULL) {
	errmsg = "ERROR: config files are saved as group \"xorp\".";
	errmsg = "Group \"xorp\" does not exist on this system.\n";
	return false;
    }
    gid_t gid, orig_gid;
    orig_gid = getgid();
    gid = grp->gr_gid;
    if (setegid(gid)<0) {
	errmsg = c_format("Failed to seteuid to group \"xorp\", gid %d\n",
			  gid);
	return false;
    }

    //set effective user ID to the user_id of the user that requested the load
    uid_t orig_uid;
    orig_uid = getuid();
    if (seteuid(user_id)<0) {
	errmsg = c_format("Failed to seteuid to user_id %d\n", user_id);
	setegid(orig_gid);
	return false;
    }

    string configuration;
    if (!read_file(configuration, filename, errmsg)) {
	seteuid(orig_uid);
	setegid(orig_gid);
	return false;
    }
    
    //revert UID and GID now we've done reading the file
    seteuid(orig_uid);
    setegid(orig_gid);

    //Test out parsing the config on a new config tree to detect any
    //parse errors before we reconfigure ourselves with the new config.
    ConfigTree new_tree(_template_tree);
    try {
	new_tree.parse(configuration, filename);
    } catch (ParseError &pe) {
	printf("caught ParseError\n");
	errmsg = booterrormsg(pe.err.c_str());
	return false;
    }

    //Ok, so the new config parses.  Now we need to figure out how it
    //differs from the existing config so we don't need to modify
    //anything that hasn't changed.
    ConfigTree delta_tree(_template_tree);
    ConfigTree deletion_tree(_template_tree);
    diff_configs(new_tree, delta_tree, deletion_tree);

    string response;
    if (!root()->merge_deltas(user_id, delta_tree.const_root(), 
			      /*provisional change*/true, response)) {
	discard_changes();
	errmsg = response;
	return false;
    }
    if (!root()->merge_deletions(user_id, deletion_tree.const_root(), 
				 /*provisional change*/true, response)) {
	discard_changes();
	errmsg = response;
	return false;
    }

    //pass these back out so we can notify other users of the change
    deltas = delta_tree.show_unannotated_tree();
    deletions = deletion_tree.show_unannotated_tree();

    //The config is loaded.  We haven't yet committed it, but that
    //happens elsewhere so we can handle the callbacks correctly.
    return true;
}

void 
MasterConfigTree::diff_configs(const ConfigTree& new_tree, 
			       ConfigTree& delta_tree,
			       ConfigTree& deletion_tree) {
    //clone the existing config tree into the delta tree.
    //clone the new config tree into the delta tree
    deletion_tree = *((ConfigTree*)(this));
    delta_tree = new_tree;

    deletion_tree.retain_different_nodes(new_tree, false);
    delta_tree.retain_different_nodes(*((ConfigTree*)(this)), true);

    printf("=========================================================\n");
    printf("ORIG:\n");
    print();
    printf("=========================================================\n");
    printf("NEW:\n");
    new_tree.print();
    printf("=========================================================\n");
    printf("=========================================================\n");
    printf("DELTAS:\n");
    delta_tree.print();
    printf("=========================================================\n");
    printf("DELETIONS:\n");
    deletion_tree.print();
    printf("=========================================================\n");

}

bool
MasterConfigTree::module_config_start(const string& module_name,
				      string& result)
{
    ModuleCommand *cmd = _template_tree->find_module(module_name);
    if (cmd == NULL) {
	result = "Module " + module_name + " is not registered with the TemplateTree, but is needed to satisfy a dependency\n";
	return false;
    }
    _task_manager.add_module(*cmd);
    return true;
}

#ifdef NOTDEF
bool
MasterConfigTree::module_shutdown(const string& module_name,
				     string& result)
{
    ModuleCommand *cmd = _template_tree->find_module(module_name);
    if (cmd == NULL) {
	result = "Module " + module_name + " is not registered with the TemplateTree, but is needed to satisfy a dependency\n";
	return false;
    }
    _task_manager.shutdown_module(*cmd);
    return true;
}

#endif
